/*
  Hatari - dlgDevice.c

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

  Device (Printer etc.) setup dialog
*/
const char DlgDevice_rcsid[] = "Hatari $Id: dlgDevice.c,v 1.12 2007-12-11 19:02:19 eerot Exp $";

#include "main.h"
#include "configuration.h"
#include "dialog.h"
#include "sdlgui.h"
#include "file.h"
#include "screen.h"


#define DEVDLG_PRNENABLE       3
#define DEVDLG_PRNBROWSE       5
#define DEVDLG_PRNFILENAME     6
#define DEVDLG_RS232ENABLE     8
#define DEVDLG_RS232OUTBROWSE  10
#define DEVDLG_RS232OUTNAME    11
#define DEVDLG_RS232INBROWSE   13
#define DEVDLG_RS232INNAME     14
#define DEVDLG_MIDIENABLE      16
#define DEVDLG_MIDIBROWSE      18
#define DEVDLG_MIDIOUTNAME     19
#define DEVDLG_EXIT            20


#define MAX_DLG_FILENAME 46+1
static char dlgPrinterName[MAX_DLG_FILENAME];
static char dlgRs232OutName[MAX_DLG_FILENAME];
static char dlgRs232InName[MAX_DLG_FILENAME];
static char dlgMidiOutName[MAX_DLG_FILENAME];

/* The devices dialog: */
static SGOBJ devicedlg[] =
{
	{ SGBOX, 0, 0, 0,0, 52,22, NULL },
	{ SGTEXT, 0, 0, 20,1, 13,1, "Devices setup" },

	{ SGBOX, 0, 0, 1,3, 50,4, NULL },
 	{ SGCHECKBOX, 0, 0, 2,3, 28,1, "Enable printer emulation" },
 	{ SGTEXT, 0, 0, 2,5, 10,1, "Print to file:" },
 	{ SGBUTTON, 0, 0, 42,5, 8,1, "Browse" },
 	{ SGTEXT, 0, 0, 3,6, 46,1, dlgPrinterName },

	{ SGBOX, 0, 0, 1,8, 50,6, NULL },
 	{ SGCHECKBOX, 0, 0, 2,8, 28,1, "Enable RS232 emulation" },
 	{ SGTEXT, 0, 0, 2,10, 10,1, "Write RS232 output to file:" },
 	{ SGBUTTON, 0, 0, 42,10, 8,1, "Browse" },
 	{ SGTEXT, 0, 0, 3,11, 46,1, dlgRs232OutName },
 	{ SGTEXT, 0, 0, 2,12, 10,1, "Read RS232 input from file:" },
 	{ SGBUTTON, 0, 0, 42,12, 8,1, "Browse" },
 	{ SGTEXT, 0, 0, 3,13, 46,1, dlgRs232InName },

	{ SGBOX, 0, 0, 1,15, 50,4, NULL },
 	{ SGCHECKBOX, 0, 0, 2,15, 28,1, "Enable MIDI emulation" },
 	{ SGTEXT, 0, 0, 2,17, 10,1, "Write MIDI output to file:" },
 	{ SGBUTTON, 0, 0, 42,17, 8,1, "Browse" },
 	{ SGTEXT, 0, 0, 3,18, 46,1, dlgMidiOutName },

 	{ SGBUTTON, SG_DEFAULT, 0, 16,20, 20,1, "Back to main menu" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};


/*-----------------------------------------------------------------------*/
/*
  Show and process the "Device" dialog.
*/
void Dialog_DeviceDlg(void)
{
	int but;

	SDLGui_CenterDlg(devicedlg);

	/* Set up dialog from actual values: */

	if (DialogParams.Printer.bEnablePrinting)
		devicedlg[DEVDLG_PRNENABLE].state |= SG_SELECTED;
	else
		devicedlg[DEVDLG_PRNENABLE].state &= ~SG_SELECTED;
	File_ShrinkName(dlgPrinterName, DialogParams.Printer.szPrintToFileName, devicedlg[DEVDLG_PRNFILENAME].w);

	if (DialogParams.RS232.bEnableRS232)
		devicedlg[DEVDLG_RS232ENABLE].state |= SG_SELECTED;
	else
		devicedlg[DEVDLG_RS232ENABLE].state &= ~SG_SELECTED;
	File_ShrinkName(dlgRs232OutName, DialogParams.RS232.szOutFileName, devicedlg[DEVDLG_RS232OUTNAME].w);
	File_ShrinkName(dlgRs232InName, DialogParams.RS232.szInFileName, devicedlg[DEVDLG_RS232INNAME].w);

	if (DialogParams.Midi.bEnableMidi)
		devicedlg[DEVDLG_MIDIENABLE].state |= SG_SELECTED;
	else
		devicedlg[DEVDLG_MIDIENABLE].state &= ~SG_SELECTED;
	File_ShrinkName(dlgMidiOutName, DialogParams.Midi.szMidiOutFileName, devicedlg[DEVDLG_MIDIOUTNAME].w);

	/* The devices dialog main loop */
	do
	{
		but = SDLGui_DoDialog(devicedlg, NULL);

		switch(but)
		{
		 case DEVDLG_PRNBROWSE:                 /* Choose a new printer file */
			SDLGui_FileConfSelect(dlgPrinterName,
                                              DialogParams.Printer.szPrintToFileName,
                                              devicedlg[DEVDLG_PRNFILENAME].w,
                                              TRUE);
			break;
		 case DEVDLG_RS232OUTBROWSE:            /* Choose a new RS232 output file */
			SDLGui_FileConfSelect(dlgRs232OutName,
                                              DialogParams.RS232.szOutFileName,
                                              devicedlg[DEVDLG_RS232OUTNAME].w,
                                              TRUE);
			break;
		 case DEVDLG_RS232INBROWSE:             /* Choose a new RS232 input file */
			SDLGui_FileConfSelect(dlgRs232InName,
                                              DialogParams.RS232.szInFileName,
                                              devicedlg[DEVDLG_RS232INNAME].w,
                                              TRUE);
			break;
		 case DEVDLG_MIDIBROWSE:                /* Choose a new MIDI file */
			SDLGui_FileConfSelect(dlgMidiOutName,
                                              DialogParams.Midi.szMidiOutFileName,
                                              devicedlg[DEVDLG_MIDIOUTNAME].w,
                                              TRUE);
			break;
		}
	}
	while (but != DEVDLG_EXIT && but != SDLGUI_QUIT
	       && but != SDLGUI_ERROR && !bQuitProgram);

	/* Read values from dialog */
	DialogParams.Printer.bEnablePrinting = (devicedlg[DEVDLG_PRNENABLE].state & SG_SELECTED);
	DialogParams.RS232.bEnableRS232 = (devicedlg[DEVDLG_RS232ENABLE].state & SG_SELECTED);
	DialogParams.Midi.bEnableMidi = (devicedlg[DEVDLG_MIDIENABLE].state & SG_SELECTED);
}
