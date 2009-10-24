/*
  Hatari - dlgFloppy.c

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.
*/
const char DlgFloppy_fileid[] = "Hatari dlgFloppy.c : " __DATE__ " " __TIME__;

#include <assert.h>
#include "main.h"
#include "configuration.h"
#include "dialog.h"
#include "sdlgui.h"
#include "file.h"
#include "floppy.h"


#define FLOPPYDLG_EJECTA      3
#define FLOPPYDLG_BROWSEA     4
#define FLOPPYDLG_DISKA       5
#define FLOPPYDLG_EJECTB      7
#define FLOPPYDLG_BROWSEB     8
#define FLOPPYDLG_DISKB       9
#define FLOPPYDLG_IMGDIR      11
#define FLOPPYDLG_BROWSEIMG   12
#define FLOPPYDLG_AUTOB       13
#define FLOPPYDLG_SLOWFLOPPY  14
#define FLOPPYDLG_CREATEIMG   15
#define FLOPPYDLG_PROTOFF     17
#define FLOPPYDLG_PROTON      18
#define FLOPPYDLG_PROTAUTO    19
#define FLOPPYDLG_EXIT        20


/* The floppy disks dialog: */
static SGOBJ floppydlg[] =
{
	{ SGBOX, 0, 0, 0,0, 64,20, NULL },
	{ SGTEXT, 0, 0, 25,1, 12,1, "Floppy disks" },
	{ SGTEXT, 0, 0, 2,3, 8,1, "Drive A:" },
	{ SGBUTTON, 0, 0, 46,3, 7,1, "Eject" },
	{ SGBUTTON, 0, 0, 54,3, 8,1, "Browse" },
	{ SGTEXT, 0, 0, 3,4, 58,1, NULL },
	{ SGTEXT, 0, 0, 2,6, 8,1, "Drive B:" },
	{ SGBUTTON, 0, 0, 46,6, 7,1, "Eject" },
	{ SGBUTTON, 0, 0, 54,6, 8,1, "Browse" },
	{ SGTEXT, 0, 0, 3,7, 58,1, NULL },
	{ SGTEXT, 0, 0, 2,9, 32,1, "Default floppy images directory:" },
	{ SGTEXT, 0, 0, 3,10, 58,1, NULL },
	{ SGBUTTON, 0, 0, 54,9, 8,1, "Browse" },
	{ SGCHECKBOX, 0, 0, 2,12, 16,1, "Auto insert B" },
	{ SGCHECKBOX, 0, 0, 2,14, 21,1, "Slow floppy access" },
	{ SGBUTTON, 0, 0, 42,14, 20,1, "Create blank image" },
	{ SGTEXT, 0, 0, 2,16, 17,1, "Write protection:" },
	{ SGRADIOBUT, 0, 0, 21,16, 5,1, "Off" },
	{ SGRADIOBUT, 0, 0, 28,16, 5,1, "On" },
	{ SGRADIOBUT, 0, 0, 34,16, 6,1, "Auto" },
	{ SGBUTTON, SG_DEFAULT, 0, 22,18, 20,1, "Back to main menu" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};


/**
 * Let user browse given disk, insert disk if one selected.
 * return false if no disk selected, otherwise return true.
 */
static bool DlgDisk_BrowseDisk(char *dlgname, int drive, int diskid)
{
	char *selname, *zip_path;
	const char *tmpname;

	assert(drive >= 0 && drive < MAX_FLOPPYDRIVES);
	if (ConfigureParams.DiskImage.szDiskFileName[drive][0])
		tmpname = ConfigureParams.DiskImage.szDiskFileName[drive];
	else
		tmpname = ConfigureParams.DiskImage.szDiskImageDirectory;

	selname = SDLGui_FileSelect(tmpname, &zip_path, false);
	if (selname)
	{
		if (File_Exists(selname))
		{
			const char *realname;
			realname = Floppy_SetDiskFileName(drive, selname, zip_path);
			/* TODO: error dialog when this fails */
			if (realname)
			{
				File_ShrinkName(dlgname, realname, floppydlg[diskid].w);
			}
		}
		else
		{
			Floppy_SetDiskFileNameNone(drive);
			dlgname[0] = '\0';
		}
		if (zip_path)
			free(zip_path);
		free(selname);
		return true;
	}
	return false;
}


/**
 * Let user browse given directory, set directory if one selected.
 * return false if none selected, otherwise return true.
 */
static bool DlgDisk_BrowseDir(char *dlgname, char *confname, int maxlen)
{
	char *str, *selname;

	selname = SDLGui_FileSelect(confname, NULL, false);
	if (selname)
	{
		strcpy(confname, selname);
		free(selname);

		str = strrchr(confname, PATHSEP);
		if (str != NULL)
			str[1] = 0;
		File_CleanFileName(confname);
		File_ShrinkName(dlgname, confname, maxlen);
		return true;
	}
	return false;
}


/**
 * Show and process the floppy disk image dialog.
 */
void DlgFloppy_Main(void)
{
	int but, i;
	char dlgname[MAX_FLOPPYDRIVES][64], dlgdiskdir[64];

	SDLGui_CenterDlg(floppydlg);

	/* Set up dialog to actual values: */

	/* Disk name A: */
	if (EmulationDrives[0].bDiskInserted)
		File_ShrinkName(dlgname[0], ConfigureParams.DiskImage.szDiskFileName[0],
		                floppydlg[FLOPPYDLG_DISKA].w);
	else
		dlgname[0][0] = '\0';
	floppydlg[FLOPPYDLG_DISKA].txt = dlgname[0];

	/* Disk name B: */
	if (EmulationDrives[1].bDiskInserted)
		File_ShrinkName(dlgname[1], ConfigureParams.DiskImage.szDiskFileName[1],
		                floppydlg[FLOPPYDLG_DISKB].w);
	else
		dlgname[1][0] = '\0';
	floppydlg[FLOPPYDLG_DISKB].txt = dlgname[1];

	/* Default image directory: */
	File_ShrinkName(dlgdiskdir, ConfigureParams.DiskImage.szDiskImageDirectory,
	                floppydlg[FLOPPYDLG_IMGDIR].w);
	floppydlg[FLOPPYDLG_IMGDIR].txt = dlgdiskdir;

	/* Auto insert disk B: */
	if (ConfigureParams.DiskImage.bAutoInsertDiskB)
		floppydlg[FLOPPYDLG_AUTOB].state |= SG_SELECTED;
	else
		floppydlg[FLOPPYDLG_AUTOB].state &= ~SG_SELECTED;

	/* Write protection */
	for (i = FLOPPYDLG_PROTOFF; i <= FLOPPYDLG_PROTAUTO; i++)
	{
		floppydlg[i].state &= ~SG_SELECTED;
	}
	floppydlg[FLOPPYDLG_PROTOFF+ConfigureParams.DiskImage.nWriteProtection].state |= SG_SELECTED;

	/* Slow floppy access */
	if (ConfigureParams.DiskImage.bSlowFloppy)
		floppydlg[FLOPPYDLG_SLOWFLOPPY].state |= SG_SELECTED;
	else
		floppydlg[FLOPPYDLG_SLOWFLOPPY].state &= ~SG_SELECTED;

	/* Draw and process the dialog */
	do
	{
		but = SDLGui_DoDialog(floppydlg, NULL);
		switch (but)
		{
		 case FLOPPYDLG_EJECTA:                         /* Eject disk in drive A: */
			Floppy_SetDiskFileNameNone(0);
			dlgname[0][0] = '\0';
			break;
		 case FLOPPYDLG_BROWSEA:                        /* Choose a new disk A: */
			DlgDisk_BrowseDisk(dlgname[0], 0, FLOPPYDLG_DISKA);
			break;
		 case FLOPPYDLG_EJECTB:                         /* Eject disk in drive B: */
			Floppy_SetDiskFileNameNone(1);
			dlgname[1][0] = '\0';
			break;
		case FLOPPYDLG_BROWSEB:                         /* Choose a new disk B: */
			DlgDisk_BrowseDisk(dlgname[1], 1, FLOPPYDLG_DISKB);
			break;
		 case FLOPPYDLG_BROWSEIMG:
			DlgDisk_BrowseDir(dlgdiskdir,
			                 ConfigureParams.DiskImage.szDiskImageDirectory,
			                 floppydlg[FLOPPYDLG_IMGDIR].w);
			break;
		 case FLOPPYDLG_CREATEIMG:
			DlgNewDisk_Main();
			break;
		}
	}
	while (but != FLOPPYDLG_EXIT && but != SDLGUI_QUIT
	        && but != SDLGUI_ERROR && !bQuitProgram);

	/* Read values from dialog: */

	for (i = FLOPPYDLG_PROTOFF; i <= FLOPPYDLG_PROTAUTO; i++)
	{
		if (floppydlg[i].state & SG_SELECTED)
		{
			ConfigureParams.DiskImage.nWriteProtection = i-FLOPPYDLG_PROTOFF;
			break;
		}
	}

	ConfigureParams.DiskImage.bAutoInsertDiskB = (floppydlg[FLOPPYDLG_AUTOB].state & SG_SELECTED);
	ConfigureParams.DiskImage.bSlowFloppy = (floppydlg[FLOPPYDLG_SLOWFLOPPY].state & SG_SELECTED);
}