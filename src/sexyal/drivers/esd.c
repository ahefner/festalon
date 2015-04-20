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
#include <esd.h>

#include "../sexyal.h"
#include "../md5.h"
#include "../smallc.h"

#include "oss.h"

/*
void SexyALI_ESD_Enumerate(int (*func)(uint8_t *name, char *id, void *udata), void *udata)
{
 struct stat buf;
 char fn[64];
 unsigned int n;

 n=0;

 do
 {
  strcpy(fn,"/dev/dsp");
  strcat(fn,sal_uinttos(n));
  if(stat(fn,&buf)!=0) break;
 } while(func(fn,sal_uinttos(n),udata));
}
*/

static uint32_t RawWrite(SexyAL_device *device, void *data, uint32_t len)
{
 ssize_t bytes;

 bytes = write(*(int *)device->private,data,len);
 if(bytes <= 0) return(0);                      /* FIXME:  What to do on -1? */
 return(bytes);
}

static uint32_t RawCanWrite(SexyAL_device *device)
{
 return(1);
}

static int Pause(SexyAL_device *device, int state)
{
 return(0);
}

static int Clear(SexyAL_device *device)
{
 //ioctl(*(int *)device->private,SNDCTL_DSP_RESET,0);
 return(1);
}

static int RawClose(SexyAL_device *device)
{
 if(device)
 {
  if(device->private)
  {
   esd_close(*(int*)device->private);
   free(device->private);
  }
  free(device);
  return(1);
 }
 return(0);
}

SexyAL_device *SexyALI_ESD_Open(char *id, SexyAL_format *format, SexyAL_buffering *buffering)
{
 SexyAL_device *device;
 int fd;
 unsigned int tmpformat;

 tmpformat = ESD_STREAM | ESD_PLAY;

 if(format->channels == 2)
  tmpformat |= ESD_STEREO;
 else
  tmpformat |= ESD_MONO;

 tmpformat |= ESD_BITS16;

 if((fd = esd_open_sound(id)) < 0) return(0);

 format->rate = 44100;
 buffering->latency = esd_get_latency(fd);

 esd_close(fd);

 if((fd=esd_play_stream(tmpformat,format->rate,id,"festalon")) < 0) return(0);

 buffering->fragcount = buffering->fragsize = buffering->ms = 0;
 buffering->totalsize = ~0;

 format->byteorder = 0;
 format->sampformat = SEXYAL_FMT_PCMS16;

 device=malloc(sizeof(SexyAL_device));
 memcpy(&device->format,format,sizeof(SexyAL_format));
 memcpy(&device->buffering,buffering,sizeof(SexyAL_buffering));

 device->private=malloc(sizeof(int));
 device->RawCanWrite = RawCanWrite;
 device->RawWrite = RawWrite;
 device->RawClose = RawClose;
 device->Clear = Clear;
 device->Pause = Pause;

 *(int*)device->private=fd;

 return(device);
}

