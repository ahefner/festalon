#include <inttypes.h>

typedef struct
{
        int type;
        const char *name;
	const char *short_name;
} SexyAL_enumtype;

typedef struct
{
	uint32_t sampformat;
	uint32_t channels;	/* 1 = mono, 2 = stereo */
	uint32_t rate;		/* Number of frames per second, 22050, 44100, etc. */
	uint32_t byteorder;	/* 0 = Native(to CPU), 1 = Reversed.  PDP can go to hell. */
} SexyAL_format;

typedef struct
{
	uint32_t fragcount;
	uint32_t fragsize;
	uint32_t totalsize;	/* Shouldn't be filled in by application code. */

	uint32_t fragsizems;	/* Granularity of buffering in ms, approximate. */
	uint32_t ms;		/* Milliseconds of buffering, approximate. */

	uint32_t latency;	/* Estimated latency between Write() and sound output, in frames. */
} SexyAL_buffering;

#define SEXYAL_FMT_PCMU8	0x10
#define SEXYAL_FMT_PCMS8	0x11

#define SEXYAL_FMT_PCMU16       0x20
#define SEXYAL_FMT_PCMS16	0x21

#define SEXYAL_FMT_PCMFLOAT	0x40

typedef struct
{


} SexyAL_convert;

typedef struct __SexyAL_device {
	int (*SetConvert)(struct __SexyAL_device *, SexyAL_format *);
	uint32_t (*Write)(struct __SexyAL_device *, void *data, uint32_t frames);
	uint32_t (*CanWrite)(struct __SexyAL_device *);
        int (*Close)(struct __SexyAL_device *);

	int (*Pause)(struct __SexyAL_device *, int state);
	int (*Clear)(struct __SexyAL_device *);

        uint32_t (*RawWrite)(struct __SexyAL_device *, void *data, uint32_t bytes);
        uint32_t (*RawCanWrite)(struct __SexyAL_device *);
        int (*RawClose)(struct __SexyAL_device *);

	SexyAL_format format;
	SexyAL_format srcformat;
	SexyAL_buffering buffering;
	void *private;

	SexyAL_convert *convert;
} SexyAL_device;

typedef struct __SexyAL_enumdevice
{
        char *name;
        char *id;
        struct __SexyAL_enumdevice *next;
} SexyAL_enumdevice;

typedef struct
{
	uint32_t type;
	char *name;
	char *short_name;

	SexyAL_device * (*Open)(char *id, SexyAL_format *format, SexyAL_buffering *buffering);
	SexyAL_enumdevice *(*EnumerateDevices)(void);
} SexyAL_driver;

#define SEXYAL_TYPE_OSSDSP		0x001
#define SEXYAL_TYPE_ALSA		0x002	/* TODO */
#define SEXYAL_TYPE_DIRECTSOUND         0x010

#define SEXYAL_TYPE_SOUNDBLASTER	0x020	/* TODO */

#define SEXYAL_OSX_COREAUDIO		0x030	/* TODO */

#define SEXYAL_TYPE_ESOUND		0x100
#define SEXYAL_TYPE_JACK		0x101
#define SEXYAL_TYPE_SDL			0x102

#define SEXYAL_TYPE_LIBAO	        0x103

typedef struct __SexyAL {
        SexyAL_device * (*Open)(struct __SexyAL *, char *id, SexyAL_format *, SexyAL_buffering *buffering, int type);
	SexyAL_enumdevice *(*EnumerateDevices)(struct __SexyAL *, int type);

	SexyAL_enumtype * (*EnumerateTypes)(struct __SexyAL *);
	void (*Destroy)(struct __SexyAL *);
} SexyAL;

/* Initializes the library, requesting the interface of the version specified and output type. */
void *SexyAL_Init(int version);
