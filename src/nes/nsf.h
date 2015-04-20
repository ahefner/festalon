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

#ifndef __FESTAH_NSFH
#define __FESTAH_NSFH

#include "cart.h"
#include "sound.h"
#include "../festalon.h"

typedef struct {
                char ID[5]; /*NESM^Z*/
                uint8 Version;
                uint8 TotalSongs;
                uint8 StartingSong;
                uint8 LoadAddressLow;
                uint8 LoadAddressHigh;
                uint8 InitAddressLow;
                uint8 InitAddressHigh;
                uint8 PlayAddressLow;
                uint8 PlayAddressHigh;
                uint8 GameName[32];
                uint8 Artist[32];
                uint8 Copyright[32];
                uint8 NTSCspeed[2];              // Unused
                uint8 BankSwitch[8];
                uint8 PALspeed[2];               // Unused
                uint8 VideoSystem;
                uint8 SoundChip;
                uint8 Expansion[4];
        } NSF_HEADER;

typedef struct
{
	uint16 PlayAddr,InitAddr,LoadAddr;
	uint8 BankSwitch[8];
        int SoundChip;

	/* These are necessary for the NSF/NSFE creation functions to work right.  The
	   "NSFDATA" private variable is adjusted for internal use, and "NSFSize" may
	   have the same done to it in the future.

	   NOTE:  NSFRawData does not necessarily point to the value returned by malloc() or
	   calloc(), so it should not be passed to free()!
	*/
	uint8 *NSFRawData;
	uint32 NSFRawDataSize;

	/* Currently used only by the NSFE code.  All unrecognized chunks will be stuck in
	   here.
	*/
	uint8 *NSFExtra;
	uint32 NSFExtraSize;

	/* Private data follows.  Be careful if you decide to use this in code external
	   to the main Festalon code.
	*/
	int NSFNMIFlags;
	uint8 *NSFDATA;
	int NSFMaxBank;
	int NSFSize;
	uint8 BSon;
	int doreset;
	uint8 *ExWRAM;
	uint8 RAM[0x800];

	uint8 SongReload;
	int CurrentSong;

	uint8 PAL;
	X6502 *X;
	NESAPU *apu;
	NESCART *cart;

	uint8 *NSFROM;

	int doodoo;
} FESTALON_NSF;

float *FESTANSF_Emulate(FESTALON_NSF *fe, int *Count);
void FESTANSF_SongControl(FESTALON_NSF *fe, int which);
void FESTANSF_Close(FESTALON_NSF *festa);
FESTALON_NSF *FESTANSF_Load(FESTALON *fe, uint8 *buf, uint32 size);
void FESTANSF_FreeFileInfo(FESTALON_NSF *fe);
FESTALON_NSF *FESTANSF_GetFileInfo(FESTALON *fe, uint8 *buf, uint32 size, int type);
void FESTANSF_Disable(FESTALON_NSF *fe, int t);
void FESTANSF_SetVolume(FESTALON_NSF *fe, uint32 volume);
int FESTANSF_SetSound(FESTALON_NSF *nfe, uint32 rate, int quality);
int FESTANSF_SetLowpass(FESTALON_NSF *nfe, int on, uint32 corner, uint32 order);
#endif
