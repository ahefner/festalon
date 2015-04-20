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
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "../types.h"

static int chans;
static FILE *soundlog=0;
static long wsize;

void FCEU_WriteWaveData(float *Buffer, int Count)
{
 int16 temp[Count * chans];
 int16 *dest;
 int x;

 if(!soundlog) return;

 dest=temp;
 x=Count * chans;

 while(x--)
 {
  int16 tmp=*Buffer*65535 - 32767;

  *(uint8 *)dest=(((uint16)tmp)&255);
  *(((uint8 *)dest)+1)=(((uint16)tmp)>>8);
  dest++;
  Buffer++;
 }
 wsize+=fwrite(temp,1,Count*sizeof(int16)*chans,soundlog);
}

int FCEUI_EndWaveRecord(void)
{
 long s;

 if(!soundlog) return 0;
 s=ftell(soundlog)-8;
 fseek(soundlog,4,SEEK_SET);
 fputc(s&0xFF,soundlog);
 fputc((s>>8)&0xFF,soundlog);
 fputc((s>>16)&0xFF,soundlog);
 fputc((s>>24)&0xFF,soundlog);
 
 fseek(soundlog,0x28,SEEK_SET);
 s=wsize;
 fputc(s&0xFF,soundlog);
 fputc((s>>8)&0xFF,soundlog);
 fputc((s>>16)&0xFF,soundlog);
 fputc((s>>24)&0xFF,soundlog);
 
 fclose(soundlog);
 soundlog=0;
 return 1;
}


int FCEUI_BeginWaveRecord(uint32 rate, int channels, char *fn)
{
 int r;
 int tmpfd;

 tmpfd = open(fn, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

 if(tmpfd == -1)
 {
     printf(" Error opening %s: %s\n", fn, strerror(errno));
     return(0);
 }
 if(!(soundlog=fdopen(tmpfd,"wb")))
 {
     printf(" Error opening %s: %s\n", fn, strerror(errno));
     return 0;
 }
 wsize=0;

 chans = channels;
 /* Write the header. */
 fputs("RIFF",soundlog);
 fseek(soundlog,4,SEEK_CUR);  // Skip size
 fputs("WAVEfmt ",soundlog);

 fputc(0x10,soundlog);
 fputc(0,soundlog);
 fputc(0,soundlog);
 fputc(0,soundlog);

 fputc(1,soundlog);           // PCM
 fputc(0,soundlog);

 fputc(chans,soundlog);           // Monophonic
 fputc(0,soundlog);

 r=rate;
 fputc(r&0xFF,soundlog);
 fputc((r>>8)&0xFF,soundlog);
 fputc((r>>16)&0xFF,soundlog);
 fputc((r>>24)&0xFF,soundlog);
 r<<=1;
 fputc(r&0xFF,soundlog);
 fputc((r>>8)&0xFF,soundlog);
 fputc((r>>16)&0xFF,soundlog);
 fputc((r>>24)&0xFF,soundlog);
 fputc(2,soundlog);
 fputc(0,soundlog);
 fputc(16,soundlog);
 fputc(0,soundlog);
 
 fputs("data",soundlog);
 fseek(soundlog,4,SEEK_CUR);

 return(1);
}


