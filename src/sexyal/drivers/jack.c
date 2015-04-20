#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <jack/jack.h>
#include <jack/ringbuffer.h>

#include "../sexyal.h"
#include "../md5.h"
#include "../smallc.h"

typedef struct
{
	jack_port_t *output_port;
	jack_client_t *client;
	jack_ringbuffer_t *tmpbuf;
	unsigned int BufferSize;

	const char **ports;
	int closed;
} JACKWrap;

static int process(jack_nframes_t nframes, void *arg)
{
 JACKWrap *jw = arg;
 jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer(jw->output_port, nframes);
 size_t canread;

 canread = jack_ringbuffer_read_space(jw->tmpbuf) / sizeof(jack_default_audio_sample_t);

 //printf("%d\n",canread);
 if(canread > nframes)
  canread = nframes;

 jack_ringbuffer_read(jw->tmpbuf,(char *)out,canread * sizeof(jack_default_audio_sample_t));
 //memset(out, 0, sizeof(jack_default_audio_sample_t) * nframes);
 nframes -= canread;

 if(nframes)    /* Buffer underflow.  Hmm. */
 {

 }
 return(1);
}

static uint32_t RawCanWrite(SexyAL_device *device)
{
 JACKWrap *jw = device->private;

 if(jw->closed) return(~0);
 return(jack_ringbuffer_write_space(jw->tmpbuf));
}

static uint32_t RawWrite(SexyAL_device *device, void *data, uint32_t len)
{
 JACKWrap *jw = device->private;

 if(jw->closed) return(0);

 while(jack_ringbuffer_write_space(jw->tmpbuf) < len) 
 {
  if(jw->closed) return(0);
  usleep(1000);
 }
 return(jack_ringbuffer_write(jw->tmpbuf, data, len));
}


static int Clear(SexyAL_device *device)
{
 JACKWrap *jw = device->private;
 //jack_ringbuffer_reset(jw->tmpbuf);
 return(1);
}

static int RawClose(SexyAL_device *device)
{
 if(device)
 {
  if(device->private)
  {
   JACKWrap *jw = device->private;
   if(jw->tmpbuf)
   {
    jack_ringbuffer_free(jw->tmpbuf);
   }
   if(jw->client && !jw->closed)
   {
    //jack_deactivate(jw->client);
    //jack_disconnect(jw->client, jack_port_name(jw->output_port), jw->ports[0]);
    jack_client_close(jw->client);
   }
   free(device->private);
  }
  free(device);
  return(1);
 }
 return(0);
}

static void DeadSound(void *arg)
{
 JACKWrap *jw = arg;
 jw->closed = 1;
 //puts("AGH!  Sound server hates us!  Let's go on a rampage.");
}

static int Pause(SexyAL_device *device, int state)
{
 return(0);
}

SexyAL_device *SexyALI_JACK_Open(char *id, SexyAL_format *format, SexyAL_buffering *buffering)
{
 SexyAL_device *device;
 JACKWrap *jw;

 jw = malloc(sizeof(JACKWrap));
 memset(jw, 0, sizeof(JACKWrap));

 device = malloc(sizeof(SexyAL_device));
 memset(device, 0, sizeof(SexyAL_device));
 device->private = jw;

 if(!(jw->client = jack_client_new("Festalon")))
 {
  RawClose(device);
  return(0);
 }

 jack_set_process_callback(jw->client, process, jw);
 jack_on_shutdown(jw->client, DeadSound, jw);

 format->channels = 1;
 format->rate = jack_get_sample_rate(jw->client);
 format->sampformat = SEXYAL_FMT_PCMFLOAT;

 if(!(jw->output_port = jack_port_register(jw->client, "output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)))
 {
  RawClose(device);
  return(0);
 }

 if(!buffering->ms) buffering->ms = 40;
 jw->BufferSize = format->rate * buffering->ms / 1000;
 buffering->totalsize = jw->BufferSize;
 if(!(jw->tmpbuf = jack_ringbuffer_create(jw->BufferSize * sizeof(jack_default_audio_sample_t))))
 {
  RawClose(device);
  return(0);
 }

 if(jack_activate(jw->client))
 {
  RawClose(device);
  return(0);
 }

 if(!(jw->ports = jack_get_ports(jw->client, NULL, NULL, JackPortIsPhysical | JackPortIsInput)))
 {
  RawClose(device);
  return(0);
 }
 jack_connect(jw->client, jack_port_name(jw->output_port), jw->ports[0]);
 jack_connect(jw->client, jack_port_name(jw->output_port), jw->ports[1]);
 free(jw->ports);

 buffering->latency = jw->BufferSize + jack_port_get_latency(jw->output_port);

 memcpy(&device->format,format,sizeof(SexyAL_format));
 memcpy(&device->buffering,buffering,sizeof(SexyAL_buffering));

 device->RawCanWrite = RawCanWrite;
 device->RawWrite = RawWrite;
 device->RawClose = RawClose;
 device->Clear = Clear;
 device->Pause = Pause;

 return(device);
}

