#ifndef _X6502STRUCTH

typedef struct __WriteMap
{
	writefunc func;
	void *private;
	struct __WriteMap *next;
} WriteMap;

typedef struct __X6502 {
        int32 tcount;           /* Temporary cycle counter */
        uint16 PC;              /* I'll change this to uint32 later... */
                                /* I'll need to AND PC after increments to 0xFFFF */
                                /* when I do, though.  Perhaps an IPC() macro? */
        uint8 A,X,Y,S,P,mooPI;
        uint8 jammed;

	int32 count;
        uint32 IRQlow;          /* Simulated IRQ pin held low(or is it high?).
                                   And other junk hooked on for speed reasons.*/
        uint8 DB;               /* Data bus "cache" for reads from certain areas */
	uint8 *RAM;
	int PAL;
	void *private;		/* Sent to the hook functions. */

	readfunc ARead[0x10000];
	WriteMap BWrite[0x10000];
	//writefunc BWrite[0x10000];
	void *AReadPrivate[0x10000];
	//void *BWritePrivate[0x10000];

	uint32 timestamp;
	uint64 timestampbase;
} X6502;
#define _X6502STRUCTH
#endif
