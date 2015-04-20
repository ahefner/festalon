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

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <commctrl.h>
#include <stdarg.h>
#include "winamp.h"
#include "../driver.h"

char lastfn[MAX_PATH];
In_Module mod;
HANDLE thread_handle=INVALID_HANDLE_VALUE;

#define CMD_SEEK	0x8000
#define CMD_STOP	0x4000

static WNDPROC OldWP=0;

static FESTALON *Player = NULL;
static uint64 dpms;
static volatile unsigned long disabled=0;
static volatile uint32 command=0;
static unsigned long current;
static volatile int playing=0;
static int soundvolume=100;
static int quality=1;

static unsigned long drate=48000,rrate;

static LRESULT FAR PASCAL NewProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  if(!playing) return(CallWindowProc(OldWP,hWnd,msg,wParam,lParam));
  switch(msg)
  {   
   case WM_COMMAND:
   case WM_SYSCOMMAND:
      switch (LOWORD(wParam))
      {
       //case WINAMP_JUMP10FWD:
       //case WINAMP_JUMP10BACK:

       case WINAMP_BUTTON1:if(current>1) command=CMD_SEEK|(current-2);
                           else
                            return(CallWindowProc(OldWP,hWnd,msg,wParam,lParam));
                           break;
       case WINAMP_BUTTON5:if(current==Player->TotalSongs)
                            return(CallWindowProc(OldWP,hWnd,msg,wParam,lParam));
                           else
                            command=CMD_SEEK|(current);break;
       default:return(CallWindowProc(OldWP,hWnd,msg,wParam,lParam));
      }
      break;
   default:
          return(CallWindowProc(OldWP,hWnd,msg,wParam,lParam));
  }
  return 0;
}

static void SI(void)
{
 mod.SetInfo(rrate*16/1000,rrate/1000,1,1);
}

static volatile int inpoo=0;
static BOOL CALLBACK ConfigProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 int x;

 switch(uMsg)
 {
   case WM_INITDIALOG:
                CheckRadioButton(hwndDlg,150,152,150+(drate/48000));
                for(x=0;x<(5+16);x++)
                 CheckDlgButton(hwndDlg,200+x,((disabled>>x)&1)?BST_UNCHECKED:BST_CHECKED);

                CheckDlgButton(hwndDlg,300,(quality==1)?BST_UNCHECKED:BST_CHECKED);

                /* Volume Trackbar */
                SendDlgItemMessage(hwndDlg,500,TBM_SETRANGE,1,MAKELONG(0,125));
                SendDlgItemMessage(hwndDlg,500,TBM_SETTICFREQ,5,0);
                SendDlgItemMessage(hwndDlg,500,TBM_SETPOS,1,125-soundvolume);
                return(1);
   case WM_VSCROLL:
                soundvolume=125-SendDlgItemMessage(hwndDlg,500,TBM_GETPOS,0,0);
		if(Player)
                 FESTAI_SetVolume(Player, soundvolume);
                break;
   case WM_DESTROY:inpoo=0;return(1);
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                switch(HIWORD(wParam))
                {
                 case BN_CLICKED:
                         if(LOWORD(wParam)>=200 && LOWORD(wParam)<=250)
                          disabled^=1<<(LOWORD(wParam)-200);
                         else if(LOWORD(wParam)==150)
                          drate=44100;
                         else if(LOWORD(wParam)==151)
                          drate=48000;
                         else if(LOWORD(wParam)==152)
                          drate=96000;
                         else if(LOWORD(wParam)==300)
                          quality^=3;
                         else if(LOWORD(wParam)==1)
                         {
                          gornk:
                          DestroyWindow(hwndDlg);                          
                          return(1);
                         }
                         break;
                }

 
 }

 return(0);
}

static void config(HWND hwndParent)
{
 if(!inpoo)
 {
  inpoo=1;
  CreateDialog(mod.hDllInstance,"CONFIG",/*hwndParent*/0,ConfigProc);
 }
}

static void about(HWND hwndParent)
{
  MessageBox(hwndParent,"Festalon NSF Plugin v"FESTALON_VERSION"\nhttp://festalon.sourceforge.net\n\nCopyright 2002,2003,2004 Xodnizel\n\nYM2413 Emulator(emu2413) Copyright 2001,2002 Mitsutaka Okazaki\n","About",MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
}

static char inifool[MAX_PATH];


static void init(void)
{
 /* Quick hack to make sure our window procedure stays loaded in memory until
    Winamp really exits.
 */
  FILE *fp;
  char fn[MAX_PATH];
  GetModuleFileName(mod.hDllInstance,fn,MAX_PATH);
  LoadLibrary(fn);
  strncpy(inifool,fn,strrchr(fn,'\\')-fn);
  strcat(inifool,"\\festalon03x.ini");
  if((fp=fopen(inifool,"rb")))
  {
   fscanf(fp,"%d %ld %ld %d %d",&inpoo,&disabled,&drate,&soundvolume,&quality);
   fclose(fp);
   if(quality!=1 && quality!=2) quality=1;
  }
  /* TODO: Load settings. */
  if(inpoo)
   CreateDialog(mod.hDllInstance,"CONFIG",0,ConfigProc);
}

static void quit(void)
{
  FILE *fp;
  if((fp=fopen(inifool,"wb")))
  {
   fprintf(fp,"%d %ld %ld %d %d\n",inpoo,disabled,drate,soundvolume,quality);
   fclose(fp);
  }

}

void setvolume(int volume) { mod.outMod->SetVolume(volume); }
void setpan(int pan) { mod.outMod->SetPan(pan); }

int infoDlg(char *fn, HWND hwnd)
{
 char tbuf[1024];
 #define SOSN(x) (x)?(x):""
 if(!fn || !*fn)
 {
  snprintf(tbuf, 1024, "Game: %s\nArtist: %s\nCopyright: %s\nRipper: %s\n",SOSN(Player->GameName),SOSN(Player->Artist),SOSN(Player->Copyright), SOSN(Player->Ripper));
 }
 else
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
   sprintf(tbuf, "Game: %s\nArtist: %s\nCopyright: %s\nRipper: %s\n",SOSN(fe->GameName),SOSN(fe->Artist),SOSN(fe->Copyright),SOSN(fe->Ripper));
   FESTAI_FreeFileInfo(fe);
  }
  else
   return(0);
 }
 MessageBox(hwnd,tbuf,"NSF Information",MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
 free(tbuf);
 return 0;
 #undef SOSN
}

void getfileinfo(char *fn, char *title, int *length_in_ms)
{
 *length_in_ms=~0;
 if(!fn || !*fn)
 {
  if(Player)
  {
   snprintf(title,256,"%s [%d/%d]",Player->GameName?Player->GameName:"",(int)current,(int)Player->TotalSongs);

   //if(Player->SongLengths && Player->SongLengths[current-1] >= 0)
   //{
   // *length_in_ms = Player->SongLengths[current - 1];
   // if(Player->SongFades && Player->SongFades[current-1] >= 0)
   //  *length_in_ms += Player->SongFades[current - 1];
   //}
  }
 }
 else
 {
  FESTALON *fe;
  int size;
  char *buf;
  FILE *fp=fopen(fn,"rb");

  if(!fp) return;

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
   snprintf(title, 256, fe->GameName);
   FESTAI_FreeFileInfo(fe);
  }
  else return;
 }
}

void eq_set(int on, char data[10], int preamp) 
{ 

}

int isourfile(char *fn) { return 0; }

int FESTAD_Update(int32 *Buffer, int Count)
{
 static int16 buf[4096*2]; /* Bigger is better. :? */
 char *tmp;
 int x;

 /* Resync thingy. */
 FESTAI_Disable(Player, disabled);


 tmp=(char *)buf;
 for(x=0;x<Count;x++) buf[x]=Buffer[x];

 if(mod.dsp_isactive())
  Count=mod.dsp_dosamples((short int *)tmp,Count,16,1,rrate);

 /* Really should do just 576 samples...maybe later I'll fix it. */ 
 mod.SAAddPCMData(tmp, 1, 16, dpms>>8);
 mod.VSAAddPCMData(tmp, 1, 16, dpms>>8);
 dpms+=(Count*1000*256)/rrate;

 Count*=2;
 while(Count>0)
 {
  int t=mod.outMod->CanWrite();
  if(t>Count)
   mod.outMod->Write(tmp,Count);
  else
  {
   mod.outMod->Write(tmp,t);
   Sleep((Count-t)*1000/rrate/2);
  }
  Count-=t;
  tmp+=t;
  if(command) break;
 }

 if(command&CMD_SEEK)
 {
  current=FESTAI_NSFControl(Player, ((command&255)+1),1);
  SI();
  mod.outMod->Flush(0);
  dpms=0;
 }
 if(command&CMD_STOP)
 {
  command = 0;
  return(0);
 }
 command=0;
 return(1);
}


DWORD WINAPI DecodeThread(void *b)
{
//        MessageBox(0,"moo","FCE Ultra Error",MB_ICONERROR|MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
	int count;
	int32 *buf;

        do
 	{
	 buf=FESTAI_Emulate(Player, &count);
	} while(FESTAD_Update(buf,count));
	return 0;
}


int play(char *fn)
{
 int maxlat;
 DWORD thread_id;
 int size;
 FILE *fp;
 char *nsfbuf;

 if(playing)
  return(0);

 fp=fopen(fn,"rb");
 if(!fp)
  return(0);
 fseek(fp,0,SEEK_END);
 size=ftell(fp);
 fseek(fp,0,SEEK_SET);
 nsfbuf=malloc(size);
 fread(nsfbuf,1,size,fp);
 fclose(fp);

 maxlat=mod.outMod->Open(drate,1,16, -1,-1);
 rrate=drate;
 mod.outMod->SetVolume(-666);
 dpms=0;
 /* Vis init. */
 mod.SAVSAInit(maxlat, rrate);
 mod.VSASetInfo(1, rrate);

 if(!(Player=FESTAI_Load(nsfbuf,size, rrate, quality)))
 {
  free(nsfbuf);
  return(0);
 }
 free(nsfbuf);

 FESTAI_SetVolume(Player, soundvolume);

 current=Player->StartingSong;
 {
  int okie=0;
  char lookie[2048];
  char *sr;
  GetWindowText(mod.hMainWindow,lookie,2048);
  sr=lookie+strlen(lookie);

  while(sr>lookie)      /* Some old book said not to do this.  I BURN OLD BOOKS. */
  {
   if(*sr==']' && !okie) okie++;
   if(*sr=='/' && okie==1) okie++;
   if(*sr=='[')
   {
    if(okie==2)
     sscanf(sr+1,"%ld",&current);
    break;
   }
   sr--;
  }

 }
 current=FESTAI_NSFControl(Player, current,1);
 SI();
 playing=1;
 thread_handle = CreateThread(0,0,DecodeThread,0,0,&thread_id);
 if(!thread_handle) 
 {

 }
 if(!OldWP)
 {
  OldWP=(WNDPROC)GetWindowLong(mod.hMainWindow,GWL_WNDPROC);
  SetWindowLong(mod.hMainWindow,GWL_WNDPROC,(LONG)NewProc);
 }
 return(0);
}

void stop(void)
{
       if (thread_handle != INVALID_HANDLE_VALUE)
       {
                command=CMD_STOP;

                /* Don't wait forever.  Forever is too long. */
                if (WaitForSingleObject(thread_handle,5000) == WAIT_TIMEOUT)
                {
                 TerminateThread(thread_handle,0);
                }
		playing = 0;
		CloseHandle(thread_handle);
		thread_handle = INVALID_HANDLE_VALUE;
	}
        FESTAI_Close(Player);
	Player = NULL;
	mod.outMod->Close();
	mod.SAVSADeInit();
}

void pause() { command=3; mod.outMod->Pause(1); }
void unpause() { command=2; mod.outMod->Pause(0); }
int ispaused() { return 0; }

int getlength() {
        return(59000+99*60*1000); 
}


static int getoutputtime(void) 
{ 
   return ((dpms>>8)+ (mod.outMod->GetOutputTime() - mod.outMod->GetWrittenTime()));
}

static void setoutputtime(int time_in_ms) 
{
   dpms = time_in_ms<<8;
   mod.outMod->Flush(dpms>>8);
}

In_Module mod =
{
	IN_VER,
        "Festalon NSF Player v"VSTRING" "
#ifdef __alpha
	"(AXP)"
#else
	"(x86)"
#endif
	,
	0,	// hMainWindow
	0,  // hDllInstance
        "NSF\0NSF Audio File (*.NSF)\0NSFE\0NSFE Audio File\0"
	,
        0,      // is_seekable (not)
	1, // uses output
	config,
	about,
	init,
	quit,
	getfileinfo,
	infoDlg,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,
	
	getlength,
	getoutputtime,
	setoutputtime,

	setvolume,
	setpan,

	0,0,0,0,0,0,0,0,0, // vis stuff


	0,0, // dsp

	eq_set,

	NULL,		// setinfo

	0 // out_mod

};

__declspec( dllexport ) In_Module * winampGetInModule2()
{
	return &mod;
}

BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
        return(1);
}

