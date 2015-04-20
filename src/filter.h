#ifndef FILTER_H
#define FILTER_H

#include <fidlib.h>

#define NCOEFFS	512

#define FFI_FLOAT	0
#define FFI_INT16	1

typedef struct {
	int input_format;

        uint32 mrindex;
        uint32 mrratio;
        double acc1,acc2;
        int soundq;
        int rate;
	float coeffs[NCOEFFS] __attribute__ ((aligned (16)));

	#ifdef ARCH_X86
	int16 coeffs_i16[NCOEFFS] __attribute__ ((aligned (8)));
	#elif ARCH_POWERPC
	int16 coeffs_i16[NCOEFFS] __attribute__ ((aligned (16)));
	#endif

	int32 SoundVolume;

	void *lrh;
	double lrhfactor;

	float boobuf[8192];	
	// 1789772.7272 / 16 / 60 = 1864
	// 1662607.1250 / 16 / 50 = 2078

	double imrate;	// Intermediate rate.

	FidFilter *fid;
	FidRun *fidrun;
	FidFunc *fidfuncp;
	void *fidbuf;

	void *realmem;

	uint32 cpuext;
} FESTAFILT;


int32 FESTAFILT_Do(FESTAFILT *ff, float *in, float *out, uint32 maxoutlen, uint32 inlen, int32 *leftover, int sinput);
FESTAFILT * FESTAFILT_Init(int32 rate, double clock, int PAL, int soundq);
int FESTAFILT_SetLowpass(FESTAFILT *ff, int on, uint32 corner, uint32 order);
void FESTAFILT_Kill(FESTAFILT *ff);
#endif
