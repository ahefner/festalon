#ifndef _HES_H_
#define _HES_H_

/* PSG structure */
typedef struct {
	uint32 counter;
        uint16 frequency;       /* Channel frequency */
        uint8 control;          /* Channel enable, DDA, volume */
        uint8 balance;          /* Channel balance */
        uint8 waveform[32];     /* Waveform data */
        uint8 waveform_index;   /* Waveform data index */
        uint8 dda;              /* Recent data written to waveform buffer */
        uint8 noisectrl;        /* Noise enable/ctrl (channels 4,5 only) */
	uint32 noisecount;
	uint32 lfsr;
	int32 dda_cache[2];
} psg_channel;

typedef struct {
    uint8 select;               /* Selected channel (0-5) */
    uint8 globalbalance;        /* Global sound balance */
    uint8 lfofreq;              /* LFO frequency */
    uint8 lfoctrl;              /* LFO control */
    psg_channel channel[8];	// Really just 6...

    // 1789772.727272 / 60 ~= 29829
    int32 WaveHi[2][32768] __attribute__ ((aligned (16)));
    int16 WaveHi16[2][32768] __attribute__ ((aligned (16)));
    float *WaveFinal[2];
    float *WaveIL;
    uint32 WaveFinalLen;
    uint32 lastts;

    int32 inbuf;
    uint32 lastpoo;

    void *ff[2];
    int disabled;			/* User-disabled channels. */
}t_psg;

typedef struct
{
	uint8 IBP[0x2000];		/* Itty-bitty player thing. */

	uint8 ram[0x8000];          /* Work RAM */
	uint8 rom[0x100000];        /* HuCard ROM */


	uint8 dummy_read[0x2000];   /* For unmapped reads */
	uint8 dummy_write[0x2000];  /* For unmapped writes */
	uint8 *read_ptr[8];         /* Read pointers in logical address space */
	uint8 *write_ptr[8];        /* Write pointers in logical address space */

	uint8 mpr_start[8];

	/* I/O port data */
	struct {
	    uint8 sel;              /* Status of SEL line */
	    uint8 clr;              /* Status of CLR line */
	}ctrl;

	struct {
	    uint8 *read;            /* Read pointers in physical address space */
	    uint8 *write;           /* Write pointers in physical address space */
	} bank_map[0x100];


	/* VDC */
	uint16 reg[0x20];       /* VDC registers */
	uint8 latch;            /* VDC register latch */
	uint8 status;           /* Status flag */

	t_psg psg;

	void *h6280;
} FESTALON_HES;


float *FESTAHES_Emulate(FESTALON_HES *, int *Count);
void FESTAHES_SongControl(FESTALON_HES *, int which);
void FESTAHES_Close(FESTALON_HES *);
FESTALON_HES *FESTAHES_Load(FESTALON *, uint8 *buf, uint32 size);
void FESTAHES_FreeFileInfo(FESTALON_HES *);
FESTALON_HES *FESTAHES_GetFileInfo(FESTALON *, uint8 *buf, uint32 size, int type);
void FESTAHES_Disable(FESTALON_HES *, int t);
void FESTAHES_SetVolume(FESTALON_HES *, uint32 volume);
int FESTAHES_SetSound(FESTALON_HES *, uint32 rate, int quality);
int FESTAHES_SetLowpass(FESTALON_HES *, int on, uint32 corner, uint32 order);
#endif
