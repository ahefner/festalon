#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../types.h"
#include "../festalon.h"
#include "x6502.h"
#include "sound.h"
#include "cart.h"
#include "nsf.h"
#include "nsfe.h"


static uint32 ToU32(uint8 *buf)
{
 return(buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24));
}

static uint16 ToU16(uint8 *buf)
{
 return(buf[0] |(buf[1]<<8));
}


int LoadNSFE(FESTALON *fe, FESTALON_NSF *nfe, uint8 *buf, int32 size, int info_only)
{
 uint8 *nbuf = 0;

 size -= 4;
 buf += 4;

 while(size)
 {
  uint32 chunk_size;
  uint8 tb[4];

  if(size < 4) return(0);
  chunk_size = ToU32(buf);

  size -= 4;
  buf += 4;
  if(size < 4) return(0);

  memcpy(tb, buf, 4);

  buf += 4;
  size -= 4;

  //printf("\nChunk: %.4s %d\n", tb, chunk_size);
  if(!memcmp(tb, "INFO", 4))
  {
   if(chunk_size < 8) return(0);

   nfe->LoadAddr = ToU16(buf);
   buf+=2; size-=2;

   nfe->InitAddr = ToU16(buf);
   buf+=2; size-=2;

   nfe->PlayAddr = ToU16(buf);
   buf+=2; size-=2;

   fe->VideoSystem = *buf; buf++; size--;
   nfe->SoundChip = *buf; buf++; size--;

   chunk_size-=8;

   if(chunk_size) { fe->TotalSongs = *buf; buf++; size--; chunk_size--; }
   else fe->TotalSongs = 1;

   if(chunk_size) { fe->StartingSong = *buf; buf++; size--; chunk_size--; }
   else fe->StartingSong = 0;

   fe->SongNames = malloc(sizeof(char *) * fe->TotalSongs);
   memset(fe->SongNames, 0, sizeof(char *) * fe->TotalSongs);

   fe->SongLengths = malloc(sizeof(int32) * fe->TotalSongs);
   fe->SongFades = malloc(sizeof(int32) * fe->TotalSongs);
   {
    int x;
    for(x=0; x<fe->TotalSongs; x++) {fe->SongLengths[x] = -1; fe->SongFades[x] = -1; }
   }
  }
  else if(!memcmp(tb, "DATA", 4))
  {
   nfe->NSFSize=chunk_size;
   nbuf = buf;
  }
  else if(!memcmp(tb, "BANK", 4))
  {
   memcpy(nfe->BankSwitch, buf, (chunk_size > 8) ? 8 : chunk_size);
  }
  else if(!memcmp(tb, "NEND", 4))
  {
   if(chunk_size == 0 && nbuf)
   {
    nfe->NSFMaxBank=((nfe->NSFSize+(nfe->LoadAddr&0xfff)+4095)/4096);
    nfe->NSFMaxBank=uppow2(nfe->NSFMaxBank);

    if(!info_only)
    {
     if(!(nfe->NSFDATA=(uint8 *)malloc(nfe->NSFMaxBank*4096)))
      return 0;
        memset(nfe->NSFDATA,0x00,nfe->NSFMaxBank*4096);
     memcpy(nfe->NSFDATA+(nfe->LoadAddr&0xfff),nbuf,nfe->NSFSize);

     nfe->NSFRawData = nfe->NSFDATA + (nfe->LoadAddr & 0xFFF);
     nfe->NSFRawDataSize = nfe->NSFSize;
    }
    nfe->NSFMaxBank--;
    return(1);
   }
   else
    return(0);
  }
  else if(!memcmp(tb, "tlbl", 4))
  {
   int songcount = 0;
   if(!fe->TotalSongs) return(0);       // Out of order chunk.

   while(chunk_size > 0)
   {
    int slen = strlen(buf);

    fe->SongNames[songcount++] = FESTA_FixString(strdup(buf));

    buf += slen + 1;
    chunk_size -= slen + 1;
   }
  }
  else if(!memcmp(tb, "time", 4))
  {
   int count = chunk_size / 4;
   int ws = 0;
   chunk_size -= count * 4;

   while(count--)
   {
    fe->SongLengths[ws] = (int32)ToU32(buf);
    //printf("%d\n",fe->SongLengths[ws]/1000);
    buf += 4;
    ws++;
   }
  }
  else if(!memcmp(tb, "fade", 4))
  {
   int count = chunk_size / 4;
   int ws = 0;
   chunk_size -= count * 4;

   while(count--)
   {
    fe->SongFades[ws] = (int32)ToU32(buf);
    //printf("%d\n",fe->SongFades[ws]);
    buf += 4;
    ws++;
   }
  }
  else if(!memcmp(tb, "auth", 4))
  {
   int which = 0;
   while(chunk_size > 0)
   {
    int slen = strlen(buf);

    if(!which) fe->GameName = FESTA_FixString(strdup(buf));
    else if(which == 1) fe->Artist = FESTA_FixString(strdup(buf));
    else if(which == 2) fe->Copyright = FESTA_FixString(strdup(buf));
    else if(which == 3) fe->Ripper = FESTA_FixString(strdup(buf));

    which++;
    buf += slen +1;
    chunk_size -= slen + 1;
   }
  }
  else if(tb[0] >= 'A' && tb[0] <= 'Z') /* Unrecognized mandatory chunk */
  {
   //puts("unknown");
   return(0);
  }
  else	// mmm...store the unknown chunk in memory so it can be used by FESTAI_CreateNSFE()
	// if necessary.
  {
   //printf("Boop: %.4s\n",tb);
   nfe->NSFExtra = realloc(nfe->NSFExtra, nfe->NSFExtraSize + 8 + chunk_size);
   memcpy(nfe->NSFExtra + nfe->NSFExtraSize, buf - 8, 8 + chunk_size);
   nfe->NSFExtraSize += 8 + chunk_size;
  }
  buf += chunk_size;
  size -= chunk_size;
 }
 return(1);
}

uint8 *FESTAI_CreateNSFE(FESTALON *fe, FESTALON_NSF *nfe, uint32 *totalsize)
{
 uint32 cursize, malloced;
 uint8 *buffer;
 uint32 chunkbuf;
 int32 chunkbufsize;

 #define END_CHUNK()	{ buffer[chunkbuf+0] = chunkbufsize; buffer[chunkbuf+1]=chunkbufsize>>8;	\
			  buffer[chunkbuf+2] = chunkbufsize >> 16; buffer[chunkbuf+3] = chunkbufsize >> 24;	\
			}

 #define ANED(data,size){ \
	if((cursize + size) > malloced)	\
	{	\
	  uint32 additional = cursize + size - malloced;	\
	  if(additional < 8192) additional = 8192;	\
	  buffer = realloc(buffer, malloced + additional);	\
	  malloced += additional;	\
	}	\
	memcpy(buffer + cursize, data, size);	\
	chunkbufsize += size; cursize += size;	\
	}

  cursize = 0;
  malloced = 0;
  buffer = malloc(8192);;

  #define ANED8(v) { uint8 tmp=v; ANED(&tmp,1); }
  #define ANED32(v) { uint8 t32[4]; t32[0]=v; t32[1]=v>>8; t32[2]=v>>16; t32[3]=v>>24; ANED(t32,4); }
  #define ANED16(v) { uint8 t16[4]; t16[0]=v; t16[1]=v>>8; ANED(t16, 2); }
  #define ANEDS(str)  { if(str) {uint32 len = strlen(str) + 1; ANED(str, len);} else { ANED8(0); } }
  #define BEGIN_CHUNK(type)  { chunkbuf = cursize; chunkbufsize = -8; ANED32(0); ANED(type,4); }

  ANED("NSFE",4);

  BEGIN_CHUNK("INFO");
  ANED16(nfe->LoadAddr);
  ANED16(nfe->InitAddr);
  ANED16(nfe->PlayAddr);
  ANED8(fe->VideoSystem);
  ANED8(nfe->SoundChip);
  ANED8(fe->TotalSongs);
  ANED8(fe->StartingSong);
  END_CHUNK();

  {
   int x;
   for(x=0;x<8;x++)
    if(nfe->BankSwitch[x])
    {
     BEGIN_CHUNK("BANK");
     ANED(nfe->BankSwitch, 8);
     END_CHUNK();
     break;
    }
  }
  BEGIN_CHUNK("DATA");
  ANED(nfe->NSFRawData, nfe->NSFRawDataSize);
  END_CHUNK();

  if(fe->SongLengths)
  {
   int x;
   int max;

   for(max=fe->TotalSongs-1;max>=0;max--)
    if(fe->SongLengths[max] != -1)
     break;

   if(max >= 0)
   {
    BEGIN_CHUNK("time");
    for(x=0;x<=max;x++)
     ANED32(fe->SongLengths[x]);
    END_CHUNK();
   }
  }

  if(fe->SongFades)
  {
   int x;
   int max;

   for(max=fe->TotalSongs-1;max>=0;max--)
    if(fe->SongFades[max] != -1)
     break;

   if(max>=0)
   {
    BEGIN_CHUNK("fade");
    for(x=0;x<=max;x++)
     ANED32(fe->SongFades[x]);
    END_CHUNK();
   }
  }

  if(fe->SongNames)
  {
   int x;
   BEGIN_CHUNK("tlbl");
   for(x=0;x<fe->TotalSongs;x++)
    ANEDS(fe->SongNames[x]);
   END_CHUNK();
  }

  if(fe->Artist || fe->GameName || fe->Copyright || fe->Ripper)
  {
   BEGIN_CHUNK("auth");
   ANEDS(fe->GameName);
   ANEDS(fe->Artist);
   ANEDS(fe->Copyright);
   ANEDS(fe->Ripper);
   END_CHUNK();
  }

  if(nfe->NSFExtra)
   ANED(nfe->NSFExtra, nfe->NSFExtraSize);

  BEGIN_CHUNK("NEND");
  END_CHUNK();
 
  *totalsize = cursize;
  return(buffer);
}
