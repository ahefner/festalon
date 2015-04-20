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

#ifndef SOUND_H
#define SOUND_H

#include "../filter.h"
#include "x6502struct.h"

typedef struct {
	   void (*HiFill)(void *private);
	   void (*HiSync)(void *private, int32 ts);
	   void (*Kill)(void *private);
	   void (*Disable)(void *private, int mask);
	   void *private;
	   int Channels;
} EXPSOUND;

typedef struct {
        uint8 Speed;
        uint8 Mode;     /* Fixed volume(1), and loop(2) */
        uint8 DecCountTo1;
        uint8 decvolume;
        int reloaddec;
} ENVUNIT;

typedef struct {
        uint32 wlookup1[32];
        uint32 wlookup2[203];
 	
	int32 WaveHi[40000] __attribute__ ((aligned (16)));
	float *WaveFinal;
	uint32 WaveFinalLen;

        uint8 TriCount;
        uint8 TriMode;

        int32 tristep;

        int32 wlcount[4];        /* Wave length counters.        */

        uint8 IRQFrameMode;      /* $4017 / xx000000 */
        uint8 PSG[0x10];
        uint8 RawDALatch;        /* $4011 0xxxxxxx */
        uint8 EnabledChannels;          /* Byte written to $4015 */
        
        ENVUNIT EnvUnits[3];
        
        int32 RectDutyCount[2];
        uint8 sweepon[2];
        int32 curfreq[2];
        uint8 SweepCount[2];
        
        uint16 nreg;
        
        uint8 fcnt;
        int32 fhcnt;
        int32 fhinc;
        
        int32 lengthcount[4];
        
        int32 DMCacc;
        int32 DMCPeriod;
        uint8 DMCBitCount;
        uint8 DMCAddressLatch,DMCSizeLatch; /* writes to 4012 and 4013 */
        uint8 DMCFormat; /* Write to $4010 */
        
        uint32 DMCAddress;
        int32 DMCSize;
        uint8 DMCShift;
        uint8 SIRQStat;
        
        int DMCHaveDMA;
        uint8 DMCDMABuf;
        int DMCHaveSample;

        uint32 ChannelBC[5];
        
        int32 inbuf;
        uint32 lastpoo;

	X6502 *X;
	FESTAFILT *ff;
	int disabled;

	EXPSOUND *exp[16];
	int expcount;

	void *realmem;
} NESAPU;


void FCEUSND_Disable(NESAPU *apu, int d);
void FCEUSND_AddExp(NESAPU *, EXPSOUND *);

void FCEUSND_Power(NESAPU *apu);
void FCEUSND_Reset(NESAPU *apu);
int FCEUSND_FlushEmulate(NESAPU *apu);

NESAPU *FCEUSND_Init(X6502 *X);
void FCEUSND_Kill(NESAPU *apu);

void FASTAPASS(2) FCEU_SoundCPUHook(void *private, int);
#endif
