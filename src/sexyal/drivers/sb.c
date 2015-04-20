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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/farptr.h>
#include <pc.h>
#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <ctype.h>
#include <crt0.h>

#include "../driver.h"
#include "sb.h"

int _crt0_startup_flags = _CRT0_FLAG_FILL_SBRK_MEMORY |_CRT0_FLAG_LOCK_MEMORY | _CRT0_FLAG_USE_DOS_SLASHES;

static void DOSMemSet(uint32 A, uint8 V, uint32 count)
{
 uint32 x;

 _farsetsel(_dos_ds);
 for(x=0;x<count;x++)
  _farnspokeb(A+x,V);
}

static void SBIRQHandler(_go32_dpmi_registers *r);

typedef struct
{
	uint32 LMBuffer;         /* Address of low memory DMA playback buffer. */
	int LMSelector;

	uint8 *WaveBuffer;
	unsigned int IVector, SBIRQ, SBDMA, SBDMA16, SBPort;
	int DSPV,hsmode;
	int format;
	int frags, fragsize, fragtotal;
	volatile int WritePtr, ReadPtr;
	volatile int hbusy;
	volatile int whichbuf;

	volatile int ssilence;

	uint8 PICMask;

	/* Protected mode interrupt vector info. */
	_go32_dpmi_seginfo SBIH,SBIHOld;

	/* Real mode interrupt vector info. */
	_go32_dpmi_seginfo SBIHRM,SBIHRMOld;
	_go32_dpmi_registers SBIHRMRegs;

} SBWrap;

static int WriteDSP(SBWrap *SBW, uint8 V)
{
 int x;

 for(x=65536;x;x--)
 {
  if(!(inportb(SBW->SBPort+0xC)&0x80))
  {
   outportb(SBW->SBPort+0xC,V);
   return(1);
  }
 }
 return(0);
}

static int ReadDSP(SBWrap *SBW, uint8 *V)
{
 int x;

 for(x=65536;x;x--)             /* Should be more than enough time... */
 {
  if(inportb(SBW->SBPort+0xE)&0x80)
  {
   *V=inportb(SBW->SBPort+0xA);
   return(1);
  }
 }
 return(0);
}


static int SetVectors(SBWrap *SBW)
{
 SBW->SBIH.pm_offset=SBW->SBIHRM.pm_offset=(int)SBW->SBIRQHandler;
 SBW->SBIH.pm_selector=SBW->SBIHRM.pm_selector=_my_cs();

 /* Get and set real mode interrupt vector.  */
 _go32_dpmi_get_real_mode_interrupt_vector(SBW->IVector,&SBW->SBIHRMOld);
 _go32_dpmi_allocate_real_mode_callback_iret(&SBW->SBIHRM, &SBW->SBIHRMRegs);
 _go32_dpmi_set_real_mode_interrupt_vector(SBW->IVector,&SBW->SBIHRM);  

 /* Get and set protected mode interrupt vector. */
 _go32_dpmi_get_protected_mode_interrupt_vector(SBW->IVector,&SBW->SBIHOld);
 _go32_dpmi_allocate_iret_wrapper(&SBW->SBIH);
 _go32_dpmi_set_protected_mode_interrupt_vector(SBW->IVector,&SBW->SBIH); 

 return(1);
}

static void ResetVectors(SBWrap *SBW)
{
 _go32_dpmi_set_protected_mode_interrupt_vector(SBW->IVector,&SBW->SBIHOld);
 _go32_dpmi_free_iret_wrapper(&SBW->SBIH);
 _go32_dpmi_set_real_mode_interrupt_vector(SBW->IVector,&SBW->SBIHRMOld);
 _go32_dpmi_free_real_mode_callback(&SBW->SBIHRM);
}

int GetBLASTER(SBWrap *SBW)
{
 int check=0;
 char *s;

 if(!(s=getenv("BLASTER")))
 {
  puts(" Error getting BLASTER environment variable.");
  return(0);
 }

 while(*s)
 {
  switch(toupper(*s))
  {
   case 'A': check|=(sscanf(s+1,"%x",&SBW->SBPort)==1)?1:0;break;
   case 'I': check|=(sscanf(s+1,"%d",&SBW->SBIRQ)==1)?2:0;break;
   case 'D': check|=(sscanf(s+1,"%d",&SBW->SBDMA)==1)?4:0;break;
   case 'H': check|=(sscanf(s+1,"%d",&SBW->SBDMA16)==1)?8:0;break;
  }
  s++;
 }
 
 if((check^7)&7 || SBW->SBDMA>=4 || (SBW->SBDMA16<=4 && check&8) || SBW->SBIRQ>15)
 {
  puts(" Invalid or incomplete BLASTER environment variable.");
  return(0);
 }
 if(!(check&8))
  format=0;
 return(1);
}

static int ResetDSP(SBWrap *SBW)
{
 uint8 b;

 outportb(SBW->SBPort+0x6,0x1);
 delay(10);
 outportb(SBW->SBPort+0x6,0x0);
 delay(10);

 if(ReadDSP(SBW, &b))
  if(b==0xAA)
   return(1); 
 return(0);
}

static int GetDSPVersion(SBWrap *SBW)
{
 int ret;
 uint8 t;

 if(!WriteDSP(SBW, 0xE1))
  return(0);
 if(!ReadDSP(SBW, &t))
  return(0);
 ret=t<<8;
 if(!ReadDSP(SBW, &t))
  return(0);
 ret|=t;

 return(ret);
}

static void KillDMABuffer(SBWrap *SBW)
{
 __dpmi_free_dos_memory(SBW->LMSelector);
}

static int MakeDMABuffer(SBWrap *SBW)
{
 uint32 size;
 int32 tmp;

 size=fragsize*2;       /* Two buffers in the DMA buffer. */
 size<<=format;         /* Twice the size for 16-bit than for 8-bit. */

 size<<=1;              /* Double the size in case the first 2 buffers
                           cross a 64KB or 128KB page boundary.
                        */
 size=(size+15)>>4;     /* Convert to paragraphs */

 if((tmp=__dpmi_allocate_dos_memory(size,&LMSelector))<0)
  return(0);

 LMBuffer=tmp<<=4;

 if(format)   /* Check for and fix 128KB page boundary crossing. */
 {
  if((LMBuffer&0x20000) != ((LMBuffer+fragsize*2*2-1)&0x20000))
   LMBuffer+=fragsize*2*2;
 }
 else   /* Check for and fix 64KB page boundary crossing. */
 {
  if((LMBuffer&0x10000) != ((LMBuffer+fragsize*2-1)&0x10000))
   LMBuffer+=fragsize*2;
 }

 DOSMemSet(LMBuffer, format?0:128, (fragsize*2)<<format);

 return(1);
}

static void ProgramDMA(SBWrap *SBW)
{
 static int PPorts[8]={0x87,0x83,0x81,0x82,0,0x8b,0x89,0x8a};
 uint32 tmp;

 if(format)
 {
  outportb(0xd4,(SBW->SBDMA16&0x3)|0x4);
  outportb(0xd8,0x0);
  outportb(0xd6,(SBW->SBDMA16&0x3)|0x58);
  tmp=((SBW->SBDMA16&3)<<2)+0xC2;
 }
 else
 {
  outportb(0xA,SBW->SBDMA|0x4);
  outportb(0xC,0x0);
  outportb(0xB,SBW->SBDMA|0x58);
  tmp=(SBW->SBDMA<<1)+1;
 }

 /* Size of entire buffer. */
 outportb(tmp,(fragsize*2-1));
 outportb(tmp,(fragsize*2-1)>>8);

 /* Page of buffer. */
 outportb(PPorts[format?SBW->SBDMA16:SBW->SBDMA],LMBuffer>>16);

 /* Offset of buffer within page. */
 if(format)
  tmp=((SBW->SBDMA16&3)<<2)+0xc0;
 else
  tmp=SBW->SBDMA<<1;

 outportb(tmp,(SBW->LMBuffer>>format));
 outportb(tmp,(SBW->LMBuffer>>(8+format)));
}

int InitSound(int *Rate)
{
 hsmode=hbusy=0;
 whichbuf=1;
 puts("Initializing Sound Blaster...");

 format=1;
 frags=32;
 fragsize=512;

 fragtotal=frags*fragsize;
 WaveBuffer=malloc(fragtotal<<format);

 if(format)
  memset(WaveBuffer,0,fragtotal*2);
 else
  memset(WaveBuffer,128,fragtotal);

 WritePtr=ReadPtr=0;

 if(!GetBLASTER())
  return(0);
 
 /* Disable IRQ line in PIC0 or PIC1 */
 if(SBIRQ>7)
 {
  PICMask=inportb(0xA1);
  outportb(0xA1,PICMask|(1<<(SBIRQ&7)));
 }
 else
 {
  PICMask=inportb(0x21);
  outportb(0x21,PICMask|(1<<SBIRQ));
 }
 if(!ResetDSP(SBW))
 {
  puts(" Error resetting the DSP.");
  return(0);
 }
 
 if(!(DSPV=GetDSPVersion(SBW)))
 {
  puts(" Error getting the DSP version.");
  return(0);
 }

 if(DSPV<0x201)
 {
  //printf(" DSP version number is too low.\n");
  return(0);
 }

 if(DSPV<0x400)
 {
  format=0;
  //puts(" Old DSP version...sound quality won't be very good.");
 }

 if(!MakeDMABuffer())
 {
  //puts(" Error creating low-memory DMA buffer.");
  return(0);
 }

 if(SBIRQ>7) IVector=SBIRQ+0x68;
 else IVector=SBIRQ+0x8;

 if(!SetVectors())
 {
  puts(" Error setting interrupt vectors.");
  KillDMABuffer();
  return(0);
 }

 /* Reenable IRQ line. */
 if(SBIRQ>7)
  outportb(0xA1,PICMask&(~(1<<(SBIRQ&7))));
 else
  outportb(0x21,PICMask&(~(1<<SBIRQ)));

 ProgramDMA(SBW);

 /* Note that the speaker must always be turned on before the mode transfer
    byte is sent to the DSP if we're going into high-speed mode, since
    a real Sound Blaster(at least my SBPro) won't accept DSP commands(except
    for the reset "command") after it goes into high-speed mode.
 */
 WriteDSP(SBW, 0xD1);                 // Turn on DAC speaker

 if(DSPV>=0x400)
 {
  *Rate=44100;	/* I don't think it supports anything higher than this... */
  WriteDSP(SBW, 0x41);                // Set sampling rate
  WriteDSP(SBW, (*Rate)>>8);             // High byte
  WriteDSP(SBW, (*Rate)&0xFF);           // Low byte
  if(!format)
  {
   WriteDSP(SBW, 0xC6);                // 8-bit output
   WriteDSP(SBW, 0x00);                // 8-bit mono unsigned PCM
  }
  else
  {
   WriteDSP(SBW, 0xB6);                // 16-bit output
   WriteDSP(SBW, 0x10);                // 16-bit mono signed PCM
  }
  WriteDSP(SBW, (fragsize-1)&0xFF);// Low byte of size
  WriteDSP(SBW, (fragsize-1)>>8);  // High byte of size
 }
 else
 {
  int tc,command;
  tc=(65536-(256000000/*Rate))>>8;
  *Rate=256000000/(65536-(tc<<8));
  command=0x90;                  // High-speed auto-initialize DMA mode transfer
  hsmode=1;
  WriteDSP(SBW, 0x40);       // Set DSP time constant
  WriteDSP(SBW, tc);         // time constant
  WriteDSP(SBW, 0x48);       // Set DSP block transfer size
  WriteDSP(SBW, (fragsize-1)&0xFF);
  WriteDSP(SBW, (fragsize-1)>>8);

  WriteDSP(SBW, command);
 }

 /* Enable DMA */
 if(format)
  outportb(0xd4,SBW->SBDMA16&3);
 else
  outportb(0xa,SBW->SBDMA);

 //printf(" %d hz, %d-bit\n",*Rate,8<<format);
 return(1);
}

static void SBIRQHandler(_go32_dpmi_registers *r)
{
        uint32 *src;
	uint32 dest;
	int32 x;


        if(format)
        {
         uint8 status;

         outportb(SBW->SBPort+4,0x82);
         status=inportb(SBW->SBPort+5);
         if(status&2)
          inportb(SBW->SBPort+0x0F);
        }
        else
         inportb(SBW->SBPort+0x0E);

        if(hbusy)
        {
         outportb(0x20,0x20);
         if(SBW->SBIRQ>=8)
          outportb(0xA0,0x20);
         SBW->whichbuf^=1;         
         return;
        }
        hbusy=1;

        {
         /* This code seems to fail on many SB emulators.  Bah.
            SCREW SB EMULATORS. ^_^ */
         uint32 count;
	 uint32 block;
	 uint32 port;
       
         if(format)
          port=((SBDMA16&3)*4)+0xc2;
         else
          port=(SBDMA*2)+1;

         count=inportb(port);
         count|=inportb(port)<<8;

         if(count>=fragsize)
          block=1;
         else
          block=0;
         dest=LMBuffer+((block*fragsize)<<format);
        }

        _farsetsel(_dos_ds);

        src=(uint32 *)(WaveBuffer+(ReadPtr<<format));

	if(ssilence)
	{
	 uint32 sby;
	 if(format) sby=0;	/* 16-bit silence.  */
	 else sby=0x80808080;	/* 8-bit silence.   */

         for(x=(fragsize<<format)>>2;x;x--,dest+=4)
         {
          _farnspokel(dest,sby);
         }
	}
	else
	{
         for(x=(fragsize<<format)>>2;x;x--,dest+=4,src++)
         {
          _farnspokel(dest,*src);
         }
         ReadPtr=(ReadPtr+fragsize)&(fragtotal-1);
	}

        hbusy=0;
        outportb(0x20,0x20);
        if(SBW->SBIRQ>=8)        
         outportb(0xA0,0x20);
}


static int Clear(SexyAL_device *device)
{
 SBWrap *SBW = device->private;
 SDL_LockAudio();

 SBW->BufferRead = SBW->BufferWrite = SBW->BufferIn = 0;

 SDL_UnlockAudio();
 return(1);
}

static uint32_t RawWrite(SexyAL_device *device, void *data, uint32_t len)
{
 SBWrap *SBW = device->private;
 uint32_t olen = len;

 while(len)
 {
  int tocopy = len;
  SDL_LockSound();
  if((tocopy + SBW->BufferIn) > SBW->BufferSize) tocopy = SBW->BufferSize - SBW->BufferIn;

  while(tocopy)
  {
   int maxcopy = tocopy;

   if((maxcopy + SBW->BufferWrite) > SBW->BufferSize)
    maxcopy = SBW->BufferSize - SBW->BufferWrite;

   memcpy((char*)SBW->Buffer + SBW->BufferWrite, data, maxcopy);
   SBW->BufferWrite = (SBW->BufferWrite + maxcopy) % SBW->BufferSize;

   SBW->BufferIn += maxcopy;

   data += maxcopy;
   tocopy -= maxcopy;
   len -= maxcopy;
  }
  SDL_UnlockAudio();
  if(len)
  {
   if(SBW->StartPaused)
   {
    SBW->StartPaused = 0;
    SDL_PauseAudio(0);
   }
   SDL_Delay(1);
  }
  //else
  // puts("nodelay");
 }
 return(olen);
}

static int RawCanWrite(SexyAL_device *device)
{
 SBWrap *SBW = device->private;

 if(hsmode)
  ResetDSP(SBW);                   /* High-speed mode requires a DSP reset. */
 else
  WriteDSP(SBW,format?0xD9:0xDA);    /* Exit auto-init DMA transfer mode. */ 
 WriteDSP(SBW,0xD3);                /* Turn speaker off. */

 outportb((SBW->SBIRQ>7)?0xA1:0x21,PICMask|(1<<(SBIRQ&7)));
 ResetVectors(SBW);
 outportb((SBW->SBIRQ>7)?0xA1:0x21,PICMask);
 KillDMABuffer(SBW);

 return(1);
}

static int Clear(SexyAL_device *device)
{
 SBWrap *SBW = device->private;

}

static int Pause(SexyAL_device *device, int state)
{
 return(0);
}

