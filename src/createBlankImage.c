/*
  Hatari - createBlankImage.c
 
  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.
 
  Create blank .ST/.MSA disk images.
*/
const char CreateBlankImage_fileid[] = "Hatari createBlankImage.c : " __DATE__ " " __TIME__;

#include "main.h"
#include "configuration.h"
#include "dim.h"
#include "file.h"
#include "floppy.h"
#include "log.h"
#include "msa.h"
#include "st.h"
#include "createBlankImage.h"

/*-----------------------------------------------------------------------*/
/*
           40 track SS   40 track DS   80 track SS   80 track DS
 0- 1   Branch instruction to boot program if executable
 2- 7   'Loader'
 8-10   24-bit serial number
11-12   BPS    512           512           512           512
13      SPC     1             2             2             2
14-15   RES     1             1             1             1
16      FAT     2             2             2             2
17-18   DIR     64           112           112           112
19-20   SEC    360           720           720          1440
21      MEDIA  $FC           $FD           $F8           $F9  (isn't used by ST-BIOS)
22-23   SPF     2             2             5             5
24-25   SPT     9             9             9             9
26-27   SIDE    1             2             1             2
28-29   HID     0             0             0             0
510-511 CHECKSUM
*/


/*-----------------------------------------------------------------------*/
/**
 * Calculate the size of a disk in dialog.
 */
static int CreateBlankImage_GetDiskImageCapacity(int nTracks, int nSectors, int nSides)
{
	/* Find size of disk image */
	return nTracks*nSectors*nSides*NUMBYTESPERSECTOR;
}


/*-----------------------------------------------------------------------*/
/**
 * Write a short integer to addr using little endian byte order
 * (needed for 16 bit values in the bootsector of the disk image).
 */
static inline void WriteShortLE(void *addr, Uint16 val)
{
	Uint8 *p = (Uint8 *)addr;

	p[0] = (Uint8)val;
	p[1] = (Uint8)(val >> 8);
}


/*-----------------------------------------------------------------------*/
/**
 * Create .ST/.MSA disk image according to 'Tracks,Sector,Sides' and save as filename
 */
void CreateBlankImage_CreateFile(char *pszFileName, int nTracks, int nSectors, int nSides)
{
	Uint8 *pDiskFile;
	unsigned long nDiskSize;
	unsigned short int SPC, nDir, MediaByte, SPF;
	bool bRet = false;

	/* Calculate size of disk image */
	nDiskSize = CreateBlankImage_GetDiskImageCapacity(nTracks, nSectors, nSides);

	/* Allocate space for our 'file', and blank */
	pDiskFile = malloc(nDiskSize);
	if (pDiskFile == NULL)
	{
		perror("Error while creating blank disk image");
		return;
	}
	memset(pDiskFile, 0, nDiskSize);                      /* Clear buffer */

	/* Fill in boot-sector */
	pDiskFile[0] = 0xE9;                                  /* Needed for MS-DOS compatibility */
	memset(pDiskFile+2, 0x4e, 6);                         /* 2-7 'Loader' */

	WriteShortLE(pDiskFile+8, rand());                    /* 8-10 24-bit serial number */
	pDiskFile[10] = rand();

	WriteShortLE(pDiskFile+11, NUMBYTESPERSECTOR);        /* 11-12 BPS */

	if ((nTracks == 40) && (nSides == 1))
		SPC = 1;
	else
		SPC = 2;
	pDiskFile[13] = SPC;                                  /* 13 SPC */

	WriteShortLE(pDiskFile+14, 1);                        /* 14-15 RES */
	pDiskFile[16] = 2;                                    /* 16 FAT */

	if (SPC==1)
		nDir = 64;
	else
		nDir = 112;
	WriteShortLE(pDiskFile+17, nDir);                     /* 17-18 DIR */

	WriteShortLE(pDiskFile+19, nTracks*nSectors*nSides);  /* 19-20 SEC */

	if (nTracks <= 42)
		MediaByte = 0xFC;
	else
		MediaByte = 0xF8;
	if (nSides == 2)
		MediaByte |= 0x01;
	pDiskFile[21] = MediaByte;                            /* 21 MEDIA */

	if (nTracks >= 80)
		SPF = 5;
	else
		SPF = 2;
	WriteShortLE(pDiskFile+22, SPF);                      /* 22-23 SPF */

	WriteShortLE(pDiskFile+24, nSectors);                 /* 24-25 SPT */
	WriteShortLE(pDiskFile+26, nSides);                   /* 26-27 SIDE */
	WriteShortLE(pDiskFile+28, 0);                        /* 28-29 HID */

	/* Set correct media bytes in the 1st FAT: */
	pDiskFile[512] = MediaByte;
	pDiskFile[513] = pDiskFile[514] = 0xFF;
	/* Set correct media bytes in the 2nd FAT: */
	pDiskFile[512 + SPF * 512] = MediaByte;
	pDiskFile[513 + SPF * 512] = pDiskFile[514 + SPF * 512] = 0xFF;

	/* Ask if OK to overwrite, if exists? */
	if (File_QueryOverwrite(pszFileName))
	{
		/* Save image to file */
		if (MSA_FileNameIsMSA(pszFileName, true))
			bRet = MSA_WriteDisk(pszFileName, pDiskFile, nDiskSize);
		else if (ST_FileNameIsST(pszFileName, true))
			bRet = ST_WriteDisk(pszFileName, pDiskFile, nDiskSize);
		else if (DIM_FileNameIsDIM(pszFileName, true))
			bRet = DIM_WriteDisk(pszFileName, pDiskFile, nDiskSize);

		/* Did create successfully? */
		if (bRet)
		{
			/* Say OK, */
			Log_AlertDlg(LOG_INFO, "Disk image has been created successfully.");
		}
		else
		{
			/* Warn user we were unable to create image */
			Log_AlertDlg(LOG_ERROR, "Unable to create disk image!");
		}
	}

	/* Free image */
	free(pDiskFile);
}
