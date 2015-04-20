/*  Festalon - NSF Player
 *  Copyright (C) 2004 Xodnizel
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef INTERFACE_BMP
#include <bmp/plugin.h>
#include <bmp/util.h>
#else
#include <xmms/plugin.h>
#include <xmms/util.h>
#endif

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../driver.h"

static FESTALON *Player = NULL;

#define CMD_SEEK	0x8000
#define CMD_STOP	0x4000

static volatile uint32 command=0;
static uint32 current;
static volatile int playing=0;

extern InputPlugin festa_ip;

InputPlugin *get_iplugin_info(void)
      {
         festa_ip.description = "Festalon NSF Plugin v"FESTALON_VERSION;
         return &festa_ip;
      }

void init(void)
{
}
void about(void)
{

}
void config(void)
{

}

int testfile(char *fn)
{
 char buf[5];
 FILE *fp;

 if(!(fp=fopen(fn,"rb"))) return(0);
 if(fread(buf,1,5,fp)!=5) {fclose(fp);return(0);}
 fclose(fp);
 if(memcmp(buf,"NESM\x1a",5) && memcmp(buf, "NSFE", 4) && memcmp(buf,"HESM",4))
   return 0;

 return(1);
}

static void SI(void)
{
 char *tmp;

 if(Player->SongNames && Player->SongNames[current])
  asprintf(&tmp,"[%d/%d] %s - %s",(int)current+1,Player->TotalSongs,Player->GameName,Player->SongNames[current]);
 else
  asprintf(&tmp,"[%d/%d] %s",(int)current+1,Player->TotalSongs,Player->GameName);
 festa_ip.set_info(tmp,Player->TotalSongs*1000,48000*2*Player->OutChannels*8,48000,Player->OutChannels);
 free(tmp);
}
static pthread_t dethread;
int FESTAD_Update(float *Buffer, int Count)
{
 int16 buf[Count * 2];
 char *tmp=(char *)buf;
 int x;

 for(x=0;x<Count * Player->OutChannels;x++) buf[x]=Buffer[x]*65535 - 32767;

 //printf("%d\n",festa_ip.output->written_time());
 //festa_ip.add_vis_pcm(festa_ip.output->written_time(), FMT_S16_NE, 2, Count, buf);

 while(Count>0)
 {
  int t=festa_ip.output->buffer_free();

  t /= 2;
  t /= Player->OutChannels;

  if(t>Count)
   festa_ip.output->write_audio(tmp,Count * 2 * Player->OutChannels);
  else
  {
   if(t)
    festa_ip.output->write_audio(tmp,t * 2 * Player->OutChannels);
   usleep((Count-t)*125/6); // 1000*1000/48000
  }
  Count-=t;
  tmp+=t*2 * Player->OutChannels;
 }
 if(command&CMD_STOP)
 {
  playing=0;   
  festa_ip.output->close_audio();
  command=0;
 }
 if(command&CMD_SEEK)
 {
  int to = (command & 255) - 1;

  if(to == -1) to = current - 1;
  else if(to == (Player->TotalSongs-1) && (abs(to-current)<5)) to = current + 1;
  else if(abs(to - current) == 5) to = (int)current + (to - (int)current) / 5;
  current=FESTAI_SongControl(Player, to, 1);
  SI();
  festa_ip.output->flush(0);
 }
 command=0;
 return(playing);
}

static void *playloop(void *arg)
{
 int count;
 float *buf;
  
 do 
 {
  buf=FESTAI_Emulate(Player, &count);
 } while(FESTAD_Update(buf,count));
 pthread_exit(0);
}

void play(char *fn)
{
 int size;
 char *buf;

 if(playing)
  return;

 {
  FILE *fp=fopen(fn,"rb");
  fseek(fp,0,SEEK_END);
  size=ftell(fp);
  fseek(fp,0,SEEK_SET);
  buf=malloc(size);
  fread(buf,1,size,fp);
  fclose(fp);
 }

 if(!(Player=FESTAI_Load(buf,size)))
 {
  free(buf);
  return;
 }
 free(buf);

 if(!festa_ip.output->open_audio(FMT_S16_LE, 48000, Player->OutChannels))
 {
  puts("Error opening audio.");
  return;
 }
 FESTAI_SetSound(Player, 48000, 0);
 FESTAI_SetVolume(Player, 100);
 current=Player->StartingSong;
 SI();
 playing=1;
 pthread_create(&dethread,0,playloop,0);
}

void stop(void)
{
 //puts("stop");
 festa_ip.output->pause(0);
 command=CMD_STOP;
 pthread_join(dethread,0);
 FESTAI_Close(Player);
 Player = NULL;
}

void festa_pause(short paused)
{
 festa_ip.output->pause(paused);
}

void seek(int time)
{
 //puts("seek");
 command=CMD_SEEK|time;
// festa_ip.output->flush(0); 
}

int gettime(void)
{
// return festa_ip.output->output_time();
 //puts("gettime");
 return((current+1)*1000);
}

void getsonginfo(char *fn, char **title, int *length)
{
 FESTALON *fe;
 int size;
 char *buf;

 FILE *fp=fopen(fn,"rb");
 fseek(fp,0,SEEK_END);
 size=ftell(fp);
 fseek(fp,0,SEEK_SET);
 buf=malloc(size);
 fread(buf,1,size,fp);
 fclose(fp);

 fe = FESTAI_GetFileInfo(buf, size, FESTAGFI_TAGS);
 free(buf);
 if(fe)
 {
  *length=(fe->TotalSongs)*1000;
  *title = strdup(fe->GameName?fe->GameName:fn);
  FESTAI_FreeFileInfo(fe);
 }
}

InputPlugin festa_ip =
{
 0,0,"Some description",0,0,0,testfile,0,play,stop,festa_pause,
 seek,0,gettime,0,0,0,0,0,0,0,getsonginfo,0,0
};

