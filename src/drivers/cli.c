/*  Festalon - NSF Player
 *  Copyright (C) 2004 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#include "../driver.h"

/* These two includes are for accessing internal structures.  Naughty naughty. :b */
#include "../nes/nsf.h"
#include "../pce/hes.h"

#include "args.h"
#include "../sexyal/sexyal.h"
#include "wave.h"

typedef struct
{
	uint32 rate;
	int32 volume;
	int32 quality;
	uint32 lowpass;
	uint32 lowpasscorner;
	uint32 lowpassorder;
	uint32 buffering;
	char *ao;
	char *aodevice;
} FestalonConfig;

static char *WRFilename;

static FestalonConfig Config = {48000, 100, 0, 0, 10000, 2, 100, NULL, NULL};

static SexyAL *Interface;
static SexyAL_device *Output;
static SexyAL_format format;
static SexyAL_buffering buffering;
static SexyAL_enumtype *DriverTypes;
static int CurDriverIndex;


static uint64 frames_written;
static unsigned long lastms;

static void poosh(void)
{
 Output->Clear(Output);
}

//#define TICKEROO
#ifdef TICKEROO
static uint64 getticks()
{
 struct timeval tv;
 uint64 ret;

 gettimeofday(&tv,0);

 ret=tv.tv_sec;
 ret *= 1000000;

 ret+=tv.tv_usec;
 return(ret);
}
#endif

static uint32 disabled=0;
static int current;
static FESTALON *Player;

static void DisplaySongName(void)
{
   printf("\x1b[A\x1b[K");
   if(Player->SongNames && Player->SongNames[current])
    printf(" Song:  %3d / %3d - %s\n", current+1,Player->TotalSongs, Player->SongNames[current]);
   else
    printf(" Song:  %3d / %3d\n",current+1,Player->TotalSongs);
}

static void ShowStatus(int songname, int force)
{
  char chans[256];	// Should be enough space for up to 64 channels.
  int x;
  long fw, ms;

  fw = (frames_written - buffering.latency);
  if(fw < 0) fw = 0;

  ms = (int64) 1000 * fw / format.rate;
  if(ms < 0) ms = 0;

  if(!force && (ms / 100 == lastms / 100)) return;

  lastms = ms;

  chans[0]=0;

  for(x=0;x<Player->TotalChannels;x++)
   sprintf(chans+strlen(chans),"%1d ",(disabled>>x)&1);

  printf("\x1b[A");

  if(songname) DisplaySongName();

  printf(" %02d:%02d.%1d | Volume: %4d%% | %s\n",(int)((ms)/60000),(int)((ms/1000)%60),(int)((ms/100)%10),Config.volume,chans);
}

static void ShowInfo(void)
{
 int x;
 puts(" File information:");
 if(Player->GameName)
  printf("  Name:\t\t%s\n",Player->GameName);
 if(Player->Artist)
  printf("  Artist:\t%s\n",Player->Artist);
 if(Player->Copyright)
  printf("  Copyright:\t%s\n",Player->Copyright);
 if(Player->Ripper)
  printf("  Ripper:\t%s\n",Player->Ripper);

 if(Player->nsf)
 if(((FESTALON_NSF*)Player->nsf)->SoundChip)
 {
  static char *tab[6]={"Konami VRCVI","Konami VRCVII","Nintendo FDS","Nintendo MMC5","Namco 106","Sunsoft FME-07"};
  int chips;

  for(x=0, chips=0;x<6;x++)
   if(((FESTALON_NSF*)Player->nsf)->SoundChip&(1<<x))
   {
    if(!chips) printf("  Expansion:\t");
    else printf(", ");
    
    printf("%s", tab[x]);
    chips++;
   }
  if(chips)
   putchar('\n');
 }
 printf("  Channels:\t%d\n",Player->TotalChannels);
 printf("  Region Type:\t%s\n\n",(Player->VideoSystem)?"PAL":"NTSC");
 if(Player->nsf)
  printf("  Load address:  $%04x\n  Init address:  $%04x\n  Play address:  $%04x\n\n",((FESTALON_NSF*)Player->nsf)->LoadAddr,((FESTALON_NSF*)Player->nsf)->InitAddr,((FESTALON_NSF*)Player->nsf)->PlayAddr);
}

static volatile int eexit=0;
static int sa;
static struct termios oldtio,newtio;

static void siggo()
{
 eexit=1;
}

static void rmode()
{
 puts("\x1b[?25h");
 fcntl(fileno(stdin), F_SETFL, sa);
 tcsetattr(0,TCSANOW,&oldtio);
}

static void badexit(int num)
{
 rmode();
 printf("Exiting on signal %d\n",num);
 exit(-1);
}

static ARGPSTRUCT TODArgs[]={
	{"-rate", 0, &Config.rate, 0},
	{"-quality", 0, &Config.quality, 0},
	{"-volume", 0, &Config.volume, 0},
	{"-buffering", 0, &Config.buffering, 0},
	{"-lowpass", 0, &Config.lowpass, 0},
	{"-lowpasscorner", 0, &Config.lowpasscorner, 0},
	{"-lowpassorder", 0, &Config.lowpassorder, 0},
	{"-record", 0, &WRFilename, 0x4001},
	{"-ao", 0, &Config.ao, 0x4001},
	{"-aodevice", 0, &Config.aodevice, 0x4001},
	{0,0,0,0}
};

static int FindCurDriver(SexyAL_enumtype *types, char *requested)
{
 int x = 0;

 while(types[x].name)
 {
  if(requested == NULL || !strcmp(requested, types[x].short_name)) return(x); 
  x++;
 }

 printf(" Output driver \"%s\" not available!",requested);
 return(-1);
}

static int paused = 0;

static void TogglePause(void)
{
 paused ^= 1;
 Output->Pause(Output, paused);
}

int main(int argc, char *argv[])
{
 int size;
 int curarg;
 uint8 *buf;

 Interface = SexyAL_Init(0);

 DriverTypes = Interface->EnumerateTypes(Interface);

 puts("\n*** Festalon v"FESTALON_VERSION);
 if(argc<=1) 
 {
  printf("\tUsage: %s [options] file1.nsf file2.nsf ... fileN.nsf\n\n",argv[0]);
  puts("\tExample Options:");
  puts("\t -rate 44100\t\tSet playback rate to 44100Hz(default: 48000Hz).");
  puts("\t -quality 1\t\tSet quality to 1, the highest(the lowestis 0; default: 0).");
  puts("\t -volume 200\t\tSet volume to 200%(default: 100%).");
  puts("\t -buffering 100\t\tSet desired buffering level to 100 milliseconds(default: 100).");
  puts("\t -lowpass 1\t\tTurn on lowpass filter(default: 0).");
  puts("\t -lowpasscorner 8000\tSet lowpass corner frequency(at which the response is about -3dB) to 8000Hz(default: 10000Hz).");
  puts("\t -lowpassorder 2\tSet lowpass filter order to 2(default: 2).");
  puts("\t -record filename.wav\tRecords audio in MS PCM format.  An existing file won't be overwritten.");
  puts("\t\t\t\tWhen playing multiple files, only the first opened file will have its music recorded.");
  puts("\t -ao x\t\t\tSelect output type/driver.  Valid choices are:");
  {
   int x = 0;
   while(DriverTypes[x].name)
   {
    printf("\t\t\t\t %s - %s",DriverTypes[x].short_name,DriverTypes[x].name);
    if(!x) printf("\t(*Default*)");
    puts("");
    x++;
   }
  }
  puts("\t -aodevice id\t\tSelect output device by id.  The default is \"NULL\", which opens the default/preferred device.");
  puts("\t\t\t\t Try \"-aodevice help\" to see the list of devices.");
  Interface->Destroy(Interface);
  return(-1);
 }

 curarg = ParseArguments(argc, argv, TODArgs);

 if(-1 == (CurDriverIndex = FindCurDriver(DriverTypes, Config.ao)))
  return(-1);

 if(Config.aodevice)
 {
  if(!strcmp(Config.aodevice,"help"))
  {
   SexyAL_enumdevice *devices = Interface->EnumerateDevices(Interface, DriverTypes[CurDriverIndex].type);

   if(!devices)
   {
    printf("\tNo predefined output devices are available with this driver type.\n");
    return(-1);
   }
   printf("\tOutput devices(ID, name) for \"%s\":\n",DriverTypes[CurDriverIndex].name);
   while(devices->next)
   {
    printf("\t %s, %s\n",devices->id,devices->name);
    devices = devices->next;
   }
   return(-1);
  }
 }

 if(Config.quality != 0 && Config.quality != 1)
  Config.quality = 0;

 tcgetattr(0,&oldtio);

 newtio=oldtio;
 newtio.c_lflag&=~(ICANON|ECHO);

 signal(SIGTERM,siggo);
 signal(SIGINT,siggo);

 tcsetattr(0,TCSANOW,&newtio);
 sa = fcntl(fileno(stdin), F_GETFL);
 fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);

 for(;curarg < argc && !eexit; curarg++)
 //while(argc-->1 && !eexit)
 {
  FILE *fp;
  printf("\nLoading %s... ",argv[curarg]);
  if(!(fp=fopen(argv[curarg],"rb"))) {printf("Error opening file: %s\n",strerror(errno));continue;}
  fseek(fp,0,SEEK_END);
  size=ftell(fp);
  fseek(fp,0,SEEK_SET);
  buf=malloc(size);
  fread(buf,1,size,fp); 
  fclose(fp);

  if(!(Player=FESTAI_Load(buf,size) ))
  {
   puts("Error loading file!");
   free(buf);
   continue;
  }
  free(buf);

  puts("");

  memset(&format, 0, sizeof(format));
  memset(&buffering, 0, sizeof(buffering));

  format.sampformat = SEXYAL_FMT_PCMFLOAT;
  format.channels = Player->OutChannels;
  format.rate = Config.rate;

  buffering.fragsizems = 10;     // Granularity.
  buffering.ms = Config.buffering;

  printf(" Using \"%s\" audio driver:",DriverTypes[CurDriverIndex].name);
  if(!(Output=Interface->Open(Interface,Config.aodevice,&format,&buffering, DriverTypes[CurDriverIndex].type)))
  {
   puts("Error opening sound device.");
   continue;
   //return(-1);
  }

  putchar('\n');

  printf("  Playback Rate:\t%dHz\n",format.rate);
  printf("  Output Channels:\t%d\n",format.channels);
  printf("  Output Format:\t");
  if(format.sampformat == SEXYAL_FMT_PCMFLOAT)
   puts("\t32-bit Floating Point");
  else
  {
   if(format.sampformat&0x1) printf("Signed, ");
   else printf("Unsigned, ");

   printf("%d bits\n",(format.sampformat >> 4) << 3);
  }
  printf("  Estimated Latency:\t%d ms\n",buffering.latency * 1000 / format.rate);

  FESTAI_SetSound(Player, format.rate, Config.quality);

  if(WRFilename)
  {
      if(!FCEUI_BeginWaveRecord(format.rate,Player->OutChannels,WRFilename))
      {
	  free(WRFilename);
	  WRFilename = 0;
      }
  }



  //printf("%d:%d\n",buffering.fragsize, buffering.fragcount);

  format.sampformat=SEXYAL_FMT_PCMFLOAT;
  format.channels=Player->OutChannels;
  format.byteorder=0;
 
  Output->SetConvert(Output,&format);

  Config.volume = FESTAI_SetVolume(Player, Config.volume);
  FESTAI_Disable(Player, disabled);

  poosh();
  puts("");
  ShowInfo();
  if(!FESTAI_SetLowpass(Player, Config.lowpass, Config.lowpasscorner, Config.lowpassorder))
  {
   puts(" Error activating lowpass filter!");
  }
  else if(Config.lowpass)
   printf(" Lowpass filter on.  Corner frequency: %dHz, Order: %d\n",Config.lowpasscorner,Config.lowpassorder);

 current=FESTAI_SongControl(Player,0,0);
 lastms = ~0;
 frames_written = 0;
 puts("\n\n");

 ShowStatus(1,1);

 //#define TICKEROO
 #ifdef TICKEROO
 static uint64 last_tick,total_ticks,total;
 total = 0;
 total_ticks = 0;
 #endif

 while(!eexit)
 {
  static char t[3]={0,0,0};
  int len;
  float *buf;

  ShowStatus(0,0);

  if(paused)
  {
   usleep(10);
  }
  else
  {
   #ifdef TICKEROO
   last_tick=getticks();
   #endif

   buf=FESTAI_Emulate(Player, &len);

   #ifdef TICKEROO
   total_ticks += getticks() - last_tick;
   total += len;
   #endif

   if(WRFilename)
       FCEU_WriteWaveData(buf, len);
   frames_written += len;
   Output->Write(Output,buf,len);

   #ifdef TICKEROO
   if(total >= 100000/2)
   {
    printf("%8f\n", (double)total*1000000/total_ticks);
    rmode();
    exit(1);
   }
   #endif
  }
  while(read(fileno(stdin),&t[0],1)>=0)
  {
    static char kcc[20] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
			  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')'};
    int x;

    for(x=0; x<20; x++)
	if(kcc[x] == t[0])
	{
	 disabled^=1<<x;
	 FESTAI_Disable(Player,disabled);
         poosh();
	 ShowStatus(0,1);
	 break;
	}

    if(t[2]==27 && t[1]==91)
     switch(t[0])
     {
      case 65:current=FESTAI_SongControl(Player,10,0);poosh(); lastms = ~0;frames_written=0;ShowStatus(1,1);break;
      case 66:current=FESTAI_SongControl(Player,-10,0);poosh(); lastms = ~0;frames_written=0;ShowStatus(1,1);break;
      case 67:current=FESTAI_SongControl(Player,1,0);poosh(); lastms = ~0;frames_written=0;ShowStatus(1,1);break;
      case 68:current=FESTAI_SongControl(Player,-1,0);poosh(); lastms = ~0;frames_written=0;ShowStatus(1,1);break;
     }
    else
    switch(tolower(t[0]))
    {
     case 10: lastms = ~0;frames_written=0;poosh();current=FESTAI_SongControl(Player, 0,0);ShowStatus(1,1);break;
     case '`': poosh();break;

     case 'a': Config.volume-=25; Config.volume = FESTAI_SetVolume(Player, Config.volume);poosh();ShowStatus(0,1);break;
     case 's': Config.volume+=25; Config.volume = FESTAI_SetVolume(Player, Config.volume);poosh();ShowStatus(0,1);break;
     case '_':
     case '-': Config.volume--; FESTAI_SetVolume(Player,Config.volume);poosh();ShowStatus(0,1);break;
     case '+':
     case '=': Config.volume++; FESTAI_SetVolume(Player,Config.volume);poosh();ShowStatus(0,1);break;
     case 'p': TogglePause();break;
     case 'q': goto exito;

     /* Alternate song selection keys.  Especially needed for DOS port. */
     case '\'':current=FESTAI_SongControl(Player,10,0);poosh(); lastms = ~0;frames_written=0;ShowStatus(1,1);break;
     case ';':current=FESTAI_SongControl(Player,-10,0);poosh(); lastms = ~0;frames_written=0;ShowStatus(1,1);break;
     case '.':current=FESTAI_SongControl(Player,1,0);poosh(); lastms = ~0;frames_written=0;ShowStatus(1,1);break;
     case ',':current=FESTAI_SongControl(Player,-1,0);poosh(); lastms = ~0;frames_written=0;ShowStatus(1,1);break;
    }
    t[2]=t[1];
    t[1]=t[0];
  }

 }

  exito:
  if(WRFilename)
  {
      FCEUI_EndWaveRecord();
      free(WRFilename);
      WRFilename = 0;
  }
  poosh();
  FESTAI_Close(Player);
  if(Output) Output->Close(Output);
  Output = 0;
 }

 rmode();

 free(DriverTypes);
 if(Interface) Interface->Destroy(Interface);
 return(0);
}
