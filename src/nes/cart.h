#ifndef CART_H
#define CART_H

#include "types.h"
#include "x6502.h"

typedef struct {
	uint8 *Page[32];
	uint8 *PRGptr[32]; 
	uint32 PRGsize[32];
	uint8 PRGIsRAM[32];
	int PRGram[32];

	uint32 PRGmask2[32]; 
	uint32 PRGmask4[32]; 
	uint32 PRGmask8[32]; 
	uint32 PRGmask16[32];
	uint32 PRGmask32[32];
} NESCART;

NESCART *FESTAC_Init(void);
void FESTAC_Kill(NESCART *ca);

void FESTAC_SetupPRG(NESCART *, int chip, uint8 *p, uint32 size, int ram);

DECLFR(CartBROB);
DECLFR(CartBR);
DECLFW(CartBW);

void FASTAPASS(2) setprg2(NESCART *ca, uint32 A, uint32 V);
void FASTAPASS(2) setprg4(NESCART *ca, uint32 A, uint32 V);
void FASTAPASS(2) setprg8(NESCART *ca, uint32 A, uint32 V);
void FASTAPASS(2) setprg16(NESCART *ca, uint32 A, uint32 V);
void FASTAPASS(2) setprg32(NESCART *ca, uint32 A, uint32 V);

void FASTAPASS(3) setprg2r(NESCART *ca, int r, unsigned int A, unsigned int V);
void FASTAPASS(3) setprg4r(NESCART *ca, int r, unsigned int A, unsigned int V);
void FASTAPASS(3) setprg8r(NESCART *ca, int r, unsigned int A, unsigned int V);
void FASTAPASS(3) setprg16r(NESCART *ca, int r, unsigned int A, unsigned int V);
void FASTAPASS(3) setprg32r(NESCART *ca, int r, unsigned int A, unsigned int V);

#endif
