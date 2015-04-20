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

#ifndef _X6502H

typedef void (FP_FASTAPASS(3) *writefunc)(void *private, uint32 A, uint8 V);
typedef uint8 (FP_FASTAPASS(3) *readfunc)(void *private, uint32 A, uint8 DB);

#include "sound.h"
#include "x6502struct.h"

void X6502_Run(X6502 *X, int32 cycles);

#define N_FLAG  0x80
#define V_FLAG  0x40
#define U_FLAG  0x20
#define B_FLAG  0x10
#define D_FLAG  0x08
#define I_FLAG  0x04
#define Z_FLAG  0x02
#define C_FLAG  0x01

#define NTSC_CPU 1789772.7272727272727272
#define PAL_CPU  1662607.125

#define FCEU_IQEXT      0x001
#define FCEU_IQEXT2     0x002
/* ... */
#define FCEU_IQRESET    0x020
#define FCEU_IQDPCM     0x100
#define FCEU_IQFCOUNT   0x200

X6502 *X6502_Init(uint8 *RAM, int PAL, void *private);
void X6502_Free(X6502 *X);
void X6502_Close(X6502 *X);

void X6502_Reset(X6502 *X);
void X6502_Power(X6502 *X);

void TriggerNMI(X6502 *X);

uint8 FASTAPASS(1) X6502_DMR(X6502 *X, uint32 A);
void FASTAPASS(2) X6502_DMW(X6502 *X, uint32 A, uint8 V);

void FASTAPASS(1) X6502_IRQBegin(X6502 *X, int w);
void FASTAPASS(1) X6502_IRQEnd(X6502 *X, int w);

void X6502_SpeedHack(X6502 *X, NESAPU *apu);

#define DECLFR(x) uint8 FP_FASTAPASS(3) x (void *private, uint32 A, uint8 DB)
#define DECLFW(x) void FP_FASTAPASS(3) x (void *private, uint32 A, uint8 V)
  
void SetReadHandler(X6502 *X, int32 start, int32 end, readfunc func, void *private);
void SetWriteHandler(X6502 *X, int32 start, int32 end, writefunc func, void *private);

#define _X6502H
#endif
