/*
  Hatari - nvram.c

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

  This file is partly based on GPL code taken from the Aranym project.
  - Copyright (c) 2001-2004 Petr Stehlik of ARAnyM dev team
  - Adaption to Hatari (c) 2006 by Thomas Huth

  Atari TT and Falcon NVRAM/RTC emulation code.
  This is a MC146818A or compatible chip with a non-volatile RAM area.

  These are the important bytes in the nvram array:

  Byte:    Description:

  14-15    Prefered operating system (TOS, Unix)
   20      Language
   21      Keyboard layout
   22      Format of date/time
   23      Separator for date
   24      Boot delay
  28-29    Video mode
   30      SCSI-ID in bits 0-2, bus arbitration flag in bit 7 (1=off, 0=on)
  62-63    Checksum

  All other cells are reserved / unused.
*/
const char NvRam_rcsid[] = "Hatari $Id: nvram.c,v 1.1 2007-02-13 19:32:29 thothy Exp $";

#include "main.h"
#include "configuration.h"
#include "ioMem.h"
#include "log.h"
#include "nvram.h"
#include "araglue.h"

#define DEBUG 0

#if DEBUG
#define D(x) x
#else
#define D(x)
#endif

// Defs for checksum
#define CKS_RANGE_START	14
#define CKS_RANGE_END	(14+47)
#define CKS_RANGE_LEN	(CKS_RANGE_END-CKS_RANGE_START+1)
#define CKS_LOC			(14+48)

#define NVRAM_START  14
#define NVRAM_LEN    50

static uint8 nvram[64] = { 48,255,21,255,23,255,1,25,3,33,42,14,112,128,
	0,0,0,0,0,0,0,0,17,46,32,1,255,0,1,10,135,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };


static Uint8 nvram_index;
static char nvram_filename[FILENAME_MAX];


/*-----------------------------------------------------------------------*/
/**
 * NvRam_Reset: Called during init and reset, used for resetting the
 * emulated chip.
 */
void NvRam_Reset(void)
{
	nvram_index = 0;
}


/*-----------------------------------------------------------------------*/
/**
 * Load NVRAM data from file.
 */
static BOOL NvRam_Load(void)
{
	BOOL ret = FALSE;
	FILE *f = fopen(nvram_filename, "rb");
	if (f != NULL)
	{
		uint8 fnvram[NVRAM_LEN];
		if (fread(fnvram, 1, NVRAM_LEN, f) == NVRAM_LEN)
		{
			memcpy(nvram+NVRAM_START, fnvram, NVRAM_LEN);
			ret = TRUE;
		}
		fclose(f);
		Log_Printf(LOG_DEBUG, "NVRAM loaded from '%s'\n", nvram_filename);
	}
	else
	{
		Log_Printf(LOG_INFO, "NVRAM not found at '%s'\n", nvram_filename);
	}

	return ret;
}


/*-----------------------------------------------------------------------*/
/**
 * Save NVRAM data to file
 */
static BOOL NvRam_Save(void)
{
	BOOL ret = FALSE;
	FILE *f = fopen(nvram_filename, "wb");
	if (f != NULL)
	{
		if (fwrite(nvram+NVRAM_START, 1, NVRAM_LEN, f) == NVRAM_LEN)
		{
			ret = TRUE;
		}
		fclose(f);
	}
	else
	{
		Log_Printf(LOG_ERROR, "ERROR: cannot store NVRAM to '%s'\n", nvram_filename);
	}

	return ret;
}


/*-----------------------------------------------------------------------*/
/**
 * Create NVRAM checksum. The checksum is over all bytes except the
 * checksum bytes themselves; these are at the very end.
 */
static void NvRam_SetChecksum(void)
{
	int i;
	unsigned char sum = 0;
	
	for(i = CKS_RANGE_START; i <= CKS_RANGE_END; ++i)
		sum += nvram[i];
	nvram[CKS_LOC] = ~sum;
	nvram[CKS_LOC+1] = sum;
}


/*-----------------------------------------------------------------------*/
/**
 * Initialization
 */
void NvRam_Init(void)
{
	const char sBaseName[] = ".hatari.nvram";
	// set up the nvram filename
	if (getenv("HOME") != NULL
	    && strlen(getenv("HOME"))+sizeof(sBaseName)+1 < sizeof(nvram_filename))
		sprintf(nvram_filename, "%s%c%s", getenv("HOME"), PATHSEP, sBaseName);
	else
		strcpy(nvram_filename, sBaseName);

	if (!NvRam_Load())		// load NVRAM file automatically
	{
		if (ConfigureParams.Screen.MonitorType == MONITOR_TYPE_VGA)   // VGA ?
		{
			nvram[29] |= 32;		// VGA mode
		}
		else
		{
			nvram[29] &= ~32;		// TV/RGB mode
		}
		NvRam_SetChecksum();
	}

	NvRam_Reset();
}


/*-----------------------------------------------------------------------*/
/**
 * De-Initialization
 */
void NvRam_UnInit(void)
{
	NvRam_Save();		// save NVRAM file upon exit automatically (should be conditionalized)
}


/*-----------------------------------------------------------------------*/
/**
 * Read from RTC/NVRAM offset selection register ($ff8961)
 */
void NvRam_Select_ReadByte(void)
{
	IoMem_WriteByte(0xff8961, nvram_index);
}


/*-----------------------------------------------------------------------*/
/**
 * Write to RTC/NVRAM offset selection register ($ff8961)
 */
void NvRam_Select_WriteByte(void)
{
	Uint8 value = IoMem_ReadByte(0xff8961);

	if (value < sizeof(nvram))
	{
		nvram_index = value;
	}
	else
	{
		Log_Printf(LOG_WARN, "NVRAM: trying to set out-of-bound position (%d)\n", value);
	}
}


/*-----------------------------------------------------------------------*/
/**
 * Read from RTC/NVRAM data register ($ff8963)
 */
void NvRam_Data_ReadByte(void)
{
	uint8 value = 0;

	if (nvram_index == 0 || nvram_index == 2 || nvram_index == 4
	    || (nvram_index >=7 && nvram_index <=9) )
	{
		time_t tim = time(NULL);
		struct tm *curtim = localtime(&tim);	// current time
		switch(nvram_index)
		{
			case 0:	value = curtim->tm_sec; break;
			case 2: value = curtim->tm_min; break;
			case 4: value = curtim->tm_hour; break;
			case 7: value = curtim->tm_mday; break;
			case 8: value = curtim->tm_mon+1; break;
			case 9: value = curtim->tm_year - 68; break;
		}
	}
	else if (nvram_index == 10)
	{
		static BOOL rtc_uip = TRUE;
		value = rtc_uip ? 0x80 : 0;
		rtc_uip = !rtc_uip;
	}
	else if (nvram_index == 13)
	{
		value = 0x80;   // Valid RAM and Time bit
	}
	else if (nvram_index < 14)
	{
		Log_Printf(LOG_DEBUG, "Read from unsupported RTC/NVRAM register 0x%x.\n", nvram_index);
		value = nvram[nvram_index];
	}
	else
	{
		value = nvram[nvram_index];
	}
	D(bug("Reading NVRAM data at %d = %d ($%02x)", nvram_index, value, value));

	IoMem_WriteByte(0xff8963, value);
}


/*-----------------------------------------------------------------------*/
/**
 * Write to RTC/NVRAM data register ($ff8963)
 */

void NvRam_Data_WriteByte(void)
{
	Uint8 value = IoMem_ReadByte(0xff8963);
	D(bug("Writing NVRAM data at %d = %d ($%02x)", nvram_index, value, value));
	nvram[nvram_index] = value;
}