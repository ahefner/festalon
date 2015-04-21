#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <ao/ao.h>

#include "../sexyal.h"



static int Pause(SexyAL_device *device, int state)
{
    return 0;
}

static int Clear(SexyAL_device *device)
{
    return 1;
}

static uint32_t RawCanWrite(SexyAL_device *device)
{
    // What this do?
    return 1;
}

static uint32_t RawWrite(SexyAL_device *device, void *data, uint32_t len)
{
    int tmp = ao_play(device->private, data, len);
    //printf("raw_write %u => %i\n", len, tmp);
    return tmp? len : 0;
}

static int RawClose(SexyAL_device *device)
{
    ao_close(device->private);
}

SexyAL_device *SexyALI_AO_Open(char *id, SexyAL_format *format, SexyAL_buffering *buffering)
{
    SexyAL_device *device;
    ao_device *aodev;
    ao_sample_format aofmt;

    ao_initialize();

    memset(&aofmt, 0, sizeof(aofmt));
    aofmt.bits = 16;
    aofmt.channels = format->channels;
    aofmt.rate = format->rate;
    aofmt.byte_format = AO_FMT_NATIVE;

    format->sampformat = SEXYAL_FMT_PCMS16;

    aodev = ao_open_live(ao_default_driver_id(), &aofmt, NULL);
    if (!device) return NULL;

    device = malloc(sizeof(SexyAL_device));
    memcpy(&device->format, format, sizeof(SexyAL_format));

    memset(&device->buffering, 0, sizeof(SexyAL_buffering));

    device->RawCanWrite = RawCanWrite;
    device->RawWrite = RawWrite;
    device->RawClose = RawClose;
    //device->Close = Close;
    device->Clear = Clear;
    device->Pause = Pause;

    device->private = aodev;

    return device;
}
