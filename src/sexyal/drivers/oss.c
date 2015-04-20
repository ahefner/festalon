#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>
#include <sys/soundcard.h>

#include "../sexyal.h"
#include "../md5.h"
#include "../smallc.h"

#include "oss.h"

SexyAL_enumdevice *SexyALI_OSS_EnumerateDevices(void)
{
 SexyAL_enumdevice *ret,*tmp,*last;
 struct stat buf;
 char fn[64];
 unsigned int n;

 n=0;

 ret = tmp = last = 0;

 for(;;)
 {
  strcpy(fn,"/dev/dsp");
  strcat(fn,sal_uinttos(n));
  if(stat(fn,&buf)!=0) break;

  tmp = malloc(sizeof(SexyAL_enumdevice));
  memset(tmp, 0, sizeof(SexyAL_enumdevice));

  tmp->name = strdup(fn);
  tmp->id = strdup(sal_uinttos(n));

  if(!ret) ret = tmp;
  if(last) last->next = tmp;

  last = tmp;
  n++;
 } 
 return(ret);
}

static int FODevice(char *id)
{
 char fn[64];

 strcpy(fn,"/dev/dsp");
 if(id)
  strcat(fn,id);
 return(open(fn,O_WRONLY));
}

unsigned int Log2(unsigned int value)
{
 int x=0;

 value>>=1;
 while(value)
 {
  value>>=1;
  x++;
 }
 return(x?x:1);
}

static uint32_t RawWrite(SexyAL_device *device, void *data, uint32_t len)
{
 ssize_t bytes;

 bytes = write(*(int *)device->private,data,len);
 if(bytes <= 0) return(0);                      /* FIXME:  What to do on -1? */
 return(bytes);
}

static uint32_t RawCanWrite(SexyAL_device *device)
{
 struct audio_buf_info ai;

 if(!ioctl(*(int *)device->private,SNDCTL_DSP_GETOSPACE,&ai))
 {
  //printf("%d\n\n",ai.bytes);
  if(ai.bytes < 0) ai.bytes = 0; // ALSA is weird
  return(ai.bytes);
 }
 else
  return(0);
}

static int Pause(SexyAL_device *device, int state)
{
 return(0);
}

static int Clear(SexyAL_device *device)
{
 ioctl(*(int *)device->private,SNDCTL_DSP_RESET,0);
 return(1);
}

static int RawClose(SexyAL_device *device)
{
 if(device)
 {
  if(device->private)
  {
   close(*(int*)device->private);
   free(device->private);
  }
  free(device);
  return(1);
 }
 return(0);
}

SexyAL_device *SexyALI_OSS_Open(char *id, SexyAL_format *format, SexyAL_buffering *buffering)
{
 SexyAL_device *device;
 int fd;
 unsigned int temp;

 if(!(fd=FODevice(id))) return(0);

 /* Set sample format. */
 /* TODO:  Handle devices with byte order different from native byte order. */
 /* TODO:  Fix fragment size calculation to work well with lower/higher playback rates,
    as reported by OSS.
 */

 if(format->sampformat == SEXYAL_FMT_PCMU8)
  temp=AFMT_U8;
 else if(format->sampformat == SEXYAL_FMT_PCMS8)
  temp=AFMT_S8;
 else if(format->sampformat == SEXYAL_FMT_PCMU16)
  temp=AFMT_U16_LE;
 else 
  temp=AFMT_S16_NE;

 format->byteorder=0;

 ioctl(fd,SNDCTL_DSP_SETFMT,&temp);
 switch(temp)
 {
  case AFMT_U8: format->sampformat = SEXYAL_FMT_PCMU8;break;
  case AFMT_S8: format->sampformat = SEXYAL_FMT_PCMS8;break;
  case AFMT_U16_LE: 
		    #ifndef LSB_FIRST
                    format->byteorder=1;
                    #endif
		    format->sampformat = SEXYAL_FMT_PCMU16;break;
  case AFMT_U16_BE: 
		    #ifdef LSB_FIRST
                    format->byteorder=1;
                    #endif
		    format->sampformat = SEXYAL_FMT_PCMU16;break;
  case AFMT_S16_LE: 
		    #ifndef LSB_FIRST
		    format->byteorder=1;
		    #endif
		    format->sampformat = SEXYAL_FMT_PCMS16;break;
  case AFMT_S16_BE: 
		    #ifdef LSB_FIRST
                    format->byteorder=1;
                    #endif
		    format->sampformat = SEXYAL_FMT_PCMS16;break;
  default: close(fd); return(0);
 }

 /* Set number of channels. */
 temp=format->channels;
 if(ioctl(fd,SNDCTL_DSP_CHANNELS,&temp)==-1)
 {
  close(fd);
  return(0);
 }

 if(temp<1 || temp>2)
 {
  close(fd);
  return(0);
 }

 format->channels=temp;

 /* Set frame rate. */
 temp=format->rate;
 if(ioctl(fd,SNDCTL_DSP_SPEED,&temp)==-1)
 {
  close(fd);
  return(0);
 }
 format->rate=temp;

 device=malloc(sizeof(SexyAL_device));
 memcpy(&device->format,format,sizeof(SexyAL_format));
 memcpy(&device->buffering,buffering,sizeof(SexyAL_buffering));

 if(buffering->fragcount == 0 || buffering->fragsize == 0)
 {
  buffering->fragcount=16;
  buffering->fragsize=64;
 }
 
 if(buffering->fragsizems)
  buffering->fragsize = buffering->fragsizems * format->rate / 1000;

 if(buffering->fragsize<32) buffering->fragsize=32;
 if(buffering->fragcount<2) buffering->fragcount=2;
 
 buffering->fragsize = 1 << Log2(buffering->fragsize);

 if(buffering->ms)
 {
  int64_t tc;

  //printf("%d\n",buffering->ms);
  
  /* 2*, >>1, |1 for crude rounding(it will always round 0.5 up, so it is a bit biased). */

  tc=2*buffering->ms * format->rate / 1000 / buffering->fragsize;
  //printf("%f\n",(double)buffering->ms * format->rate / 1000 / buffering->fragsize);
  buffering->fragcount=(tc>>1)+(tc&1); //1<<Log2(tc);
  //printf("%d\n",buffering->fragcount);
 }

 temp=Log2(buffering->fragsize*(format->sampformat>>4)*format->channels);
 temp|=buffering->fragcount<<16;
 ioctl(fd,SNDCTL_DSP_SETFRAGMENT,&temp);

 {
  audio_buf_info info;
  ioctl(fd,SNDCTL_DSP_GETOSPACE,&info);
  buffering->fragsize=info.fragsize/(format->sampformat>>4)/format->channels;
  buffering->fragcount=info.fragments;
  buffering->totalsize=buffering->fragsize*buffering->fragcount;
  //printf("Actual:  %d, %d\n",buffering->fragsize,buffering->fragcount);
 }
 device->private=malloc(sizeof(int));
 device->RawCanWrite = RawCanWrite;
 device->RawWrite = RawWrite;
 device->RawClose = RawClose;
 device->Clear = Clear;
 device->Pause = Pause;

 *(int*)device->private=fd;

 buffering->latency = RawCanWrite(device)/(format->sampformat>>4)/format->channels;
 return(device);
}

