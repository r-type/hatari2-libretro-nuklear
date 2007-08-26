/*
  Hatari - configuration.c

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

  Configuration File

  The configuration file is now stored in an ASCII format to allow the user
  to edit the file manually.
*/
const char Configuration_rcsid[] = "Hatari $Id: configuration.c,v 1.66 2007-08-26 17:16:37 eerot Exp $";

#include <SDL_keysym.h>

#include "main.h"
#include "configuration.h"
#include "cfgopts.h"
#include "audio.h"
#include "file.h"
#include "log.h"
#include "m68000.h"
#include "memorySnapShot.h"
#include "screen.h"
#include "vdi.h"
#include "video.h"
#include "uae-cpu/hatari-glue.h"


BOOL bFirstTimeInstall = FALSE;             /* Has been run before? Used to set default joysticks etc... */
CNF_PARAMS ConfigureParams;                 /* List of configuration for the emulator */
char sConfigFileName[FILENAME_MAX];         /* Stores the name of the configuration file */


/* Used to load/save logging options */
static const struct Config_Tag configs_Log[] =
{
	{ "sLogFileName", String_Tag, ConfigureParams.Log.sLogFileName },
	{ "nTextLogLevel", Int_Tag, &ConfigureParams.Log.nTextLogLevel },
	{ "nAlertDlgLogLevel", Int_Tag, &ConfigureParams.Log.nAlertDlgLogLevel },
	{ NULL , Error_Tag, NULL }
};


/* Used to load/save screen options */
static const struct Config_Tag configs_Screen[] =
{
	{ "bFullScreen", Bool_Tag, &ConfigureParams.Screen.bFullScreen },
	{ "FrameSkips", Int_Tag, &ConfigureParams.Screen.FrameSkips },
	{ "bAllowOverscan", Bool_Tag, &ConfigureParams.Screen.bAllowOverscan },
	{ "bForce8Bpp", Bool_Tag, &ConfigureParams.Screen.bForce8Bpp },
	{ "bZoomLowRes", Bool_Tag, &ConfigureParams.Screen.bZoomLowRes },
	{ "MonitorType", Int_Tag, &ConfigureParams.Screen.MonitorType },
	{ "bUseExtVdiResolutions", Bool_Tag, &ConfigureParams.Screen.bUseExtVdiResolutions },
	{ "nVdiResolution", Int_Tag, &ConfigureParams.Screen.nVdiResolution },
	{ "nVdiWidth", Int_Tag, &ConfigureParams.Screen.nVdiWidth },
	{ "nVdiHeight", Int_Tag, &ConfigureParams.Screen.nVdiHeight },
	{ "nVdiColors", Int_Tag, &ConfigureParams.Screen.nVdiColors },
	{ "bCaptureChange", Bool_Tag, &ConfigureParams.Screen.bCaptureChange },
	{ "nFramesPerSecond", Int_Tag, &ConfigureParams.Screen.nFramesPerSecond },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save joystick 0 options */
static const struct Config_Tag configs_Joystick0[] =
{
	{ "nJoystickMode", Int_Tag, &ConfigureParams.Joysticks.Joy[0].nJoystickMode },
	{ "bEnableAutoFire", Bool_Tag, &ConfigureParams.Joysticks.Joy[0].bEnableAutoFire },
	{ "nJoyId", Int_Tag, &ConfigureParams.Joysticks.Joy[0].nJoyId },
	{ "nKeyCodeUp", Int_Tag, &ConfigureParams.Joysticks.Joy[0].nKeyCodeUp },
	{ "nKeyCodeDown", Int_Tag, &ConfigureParams.Joysticks.Joy[0].nKeyCodeDown },
	{ "nKeyCodeLeft", Int_Tag, &ConfigureParams.Joysticks.Joy[0].nKeyCodeLeft },
	{ "nKeyCodeRight", Int_Tag, &ConfigureParams.Joysticks.Joy[0].nKeyCodeRight },
	{ "nKeyCodeFire", Int_Tag, &ConfigureParams.Joysticks.Joy[0].nKeyCodeFire },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save joystick 1 options */
static const struct Config_Tag configs_Joystick1[] =
{
	{ "nJoystickMode", Int_Tag, &ConfigureParams.Joysticks.Joy[1].nJoystickMode },
	{ "bEnableAutoFire", Bool_Tag, &ConfigureParams.Joysticks.Joy[1].bEnableAutoFire },
	{ "nJoyId", Int_Tag, &ConfigureParams.Joysticks.Joy[1].nJoyId },
	{ "nKeyCodeUp", Int_Tag, &ConfigureParams.Joysticks.Joy[1].nKeyCodeUp },
	{ "nKeyCodeDown", Int_Tag, &ConfigureParams.Joysticks.Joy[1].nKeyCodeDown },
	{ "nKeyCodeLeft", Int_Tag, &ConfigureParams.Joysticks.Joy[1].nKeyCodeLeft },
	{ "nKeyCodeRight", Int_Tag, &ConfigureParams.Joysticks.Joy[1].nKeyCodeRight },
	{ "nKeyCodeFire", Int_Tag, &ConfigureParams.Joysticks.Joy[1].nKeyCodeFire },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save joystick 2 options */
static const struct Config_Tag configs_Joystick2[] =
{
	{ "nJoystickMode", Int_Tag, &ConfigureParams.Joysticks.Joy[2].nJoystickMode },
	{ "bEnableAutoFire", Bool_Tag, &ConfigureParams.Joysticks.Joy[2].bEnableAutoFire },
	{ "nJoyId", Int_Tag, &ConfigureParams.Joysticks.Joy[2].nJoyId },
	{ "nKeyCodeUp", Int_Tag, &ConfigureParams.Joysticks.Joy[2].nKeyCodeUp },
	{ "nKeyCodeDown", Int_Tag, &ConfigureParams.Joysticks.Joy[2].nKeyCodeDown },
	{ "nKeyCodeLeft", Int_Tag, &ConfigureParams.Joysticks.Joy[2].nKeyCodeLeft },
	{ "nKeyCodeRight", Int_Tag, &ConfigureParams.Joysticks.Joy[2].nKeyCodeRight },
	{ "nKeyCodeFire", Int_Tag, &ConfigureParams.Joysticks.Joy[2].nKeyCodeFire },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save joystick 3 options */
static const struct Config_Tag configs_Joystick3[] =
{
	{ "nJoystickMode", Int_Tag, &ConfigureParams.Joysticks.Joy[3].nJoystickMode },
	{ "bEnableAutoFire", Bool_Tag, &ConfigureParams.Joysticks.Joy[3].bEnableAutoFire },
	{ "nJoyId", Int_Tag, &ConfigureParams.Joysticks.Joy[3].nJoyId },
	{ "nKeyCodeUp", Int_Tag, &ConfigureParams.Joysticks.Joy[3].nKeyCodeUp },
	{ "nKeyCodeDown", Int_Tag, &ConfigureParams.Joysticks.Joy[3].nKeyCodeDown },
	{ "nKeyCodeLeft", Int_Tag, &ConfigureParams.Joysticks.Joy[3].nKeyCodeLeft },
	{ "nKeyCodeRight", Int_Tag, &ConfigureParams.Joysticks.Joy[3].nKeyCodeRight },
	{ "nKeyCodeFire", Int_Tag, &ConfigureParams.Joysticks.Joy[3].nKeyCodeFire },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save joystick 4 options */
static const struct Config_Tag configs_Joystick4[] =
{
	{ "nJoystickMode", Int_Tag, &ConfigureParams.Joysticks.Joy[4].nJoystickMode },
	{ "bEnableAutoFire", Bool_Tag, &ConfigureParams.Joysticks.Joy[4].bEnableAutoFire },
	{ "nJoyId", Int_Tag, &ConfigureParams.Joysticks.Joy[4].nJoyId },
	{ "nKeyCodeUp", Int_Tag, &ConfigureParams.Joysticks.Joy[4].nKeyCodeUp },
	{ "nKeyCodeDown", Int_Tag, &ConfigureParams.Joysticks.Joy[4].nKeyCodeDown },
	{ "nKeyCodeLeft", Int_Tag, &ConfigureParams.Joysticks.Joy[4].nKeyCodeLeft },
	{ "nKeyCodeRight", Int_Tag, &ConfigureParams.Joysticks.Joy[4].nKeyCodeRight },
	{ "nKeyCodeFire", Int_Tag, &ConfigureParams.Joysticks.Joy[4].nKeyCodeFire },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save joystick 5 options */
static const struct Config_Tag configs_Joystick5[] =
{
	{ "nJoystickMode", Int_Tag, &ConfigureParams.Joysticks.Joy[5].nJoystickMode },
	{ "bEnableAutoFire", Bool_Tag, &ConfigureParams.Joysticks.Joy[5].bEnableAutoFire },
	{ "nJoyId", Int_Tag, &ConfigureParams.Joysticks.Joy[5].nJoyId },
	{ "nKeyCodeUp", Int_Tag, &ConfigureParams.Joysticks.Joy[5].nKeyCodeUp },
	{ "nKeyCodeDown", Int_Tag, &ConfigureParams.Joysticks.Joy[5].nKeyCodeDown },
	{ "nKeyCodeLeft", Int_Tag, &ConfigureParams.Joysticks.Joy[5].nKeyCodeLeft },
	{ "nKeyCodeRight", Int_Tag, &ConfigureParams.Joysticks.Joy[5].nKeyCodeRight },
	{ "nKeyCodeFire", Int_Tag, &ConfigureParams.Joysticks.Joy[5].nKeyCodeFire },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save keyboard options */
static const struct Config_Tag configs_Keyboard[] =
{
	{ "bDisableKeyRepeat", Bool_Tag, &ConfigureParams.Keyboard.bDisableKeyRepeat },
	{ "nKeymapType", Int_Tag, &ConfigureParams.Keyboard.nKeymapType },
	{ "szMappingFileName", String_Tag, ConfigureParams.Keyboard.szMappingFileName },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save shortcut key bindings with modifiers options */
static const struct Config_Tag configs_ShortCutWithMod[] =
{
	{ "keyOptions",    Int_Tag, &ConfigureParams.Shortcut.withModifier[SHORTCUT_OPTIONS] },
	{ "keyFullScreen", Int_Tag, &ConfigureParams.Shortcut.withModifier[SHORTCUT_FULLSCREEN] },
	{ "keyMouseMode",  Int_Tag, &ConfigureParams.Shortcut.withModifier[SHORTCUT_MOUSEMODE] },
	{ "keyColdReset",  Int_Tag, &ConfigureParams.Shortcut.withModifier[SHORTCUT_COLDRESET] },
	{ "keyWarmReset",  Int_Tag, &ConfigureParams.Shortcut.withModifier[SHORTCUT_WARMRESET] },
	{ "keyScreenShot", Int_Tag, &ConfigureParams.Shortcut.withModifier[SHORTCUT_SCREENSHOT] },
	{ "keyBossKey",    Int_Tag, &ConfigureParams.Shortcut.withModifier[SHORTCUT_BOSSKEY] },
	{ "keyCursorEmu",  Int_Tag, &ConfigureParams.Shortcut.withModifier[SHORTCUT_CURSOREMU] },
	{ "keyMaxSpeed",   Int_Tag, &ConfigureParams.Shortcut.withModifier[SHORTCUT_MAXSPEED] },
	{ "keyRecAnim",    Int_Tag, &ConfigureParams.Shortcut.withModifier[SHORTCUT_RECANIM] },
	{ "keyRecSound",   Int_Tag, &ConfigureParams.Shortcut.withModifier[SHORTCUT_RECSOUND] },
	{ "keySound",      Int_Tag, &ConfigureParams.Shortcut.withModifier[SHORTCUT_SOUND] },
	{ "keyQuit",       Int_Tag, &ConfigureParams.Shortcut.withModifier[SHORTCUT_QUIT] },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save shortcut key bindings without modifiers options */
static const struct Config_Tag configs_ShortCutWithoutMod[] =
{
	{ "keyOptions",    Int_Tag, &ConfigureParams.Shortcut.withoutModifier[SHORTCUT_OPTIONS] },
	{ "keyFullScreen", Int_Tag, &ConfigureParams.Shortcut.withoutModifier[SHORTCUT_FULLSCREEN] },
	{ "keyMouseMode",  Int_Tag, &ConfigureParams.Shortcut.withoutModifier[SHORTCUT_MOUSEMODE] },
	{ "keyColdReset",  Int_Tag, &ConfigureParams.Shortcut.withoutModifier[SHORTCUT_COLDRESET] },
	{ "keyWarmReset",  Int_Tag, &ConfigureParams.Shortcut.withoutModifier[SHORTCUT_WARMRESET] },
	{ "keyScreenShot", Int_Tag, &ConfigureParams.Shortcut.withoutModifier[SHORTCUT_SCREENSHOT] },
	{ "keyBossKey",    Int_Tag, &ConfigureParams.Shortcut.withoutModifier[SHORTCUT_BOSSKEY] },
	{ "keyCursorEmu",  Int_Tag, &ConfigureParams.Shortcut.withoutModifier[SHORTCUT_CURSOREMU] },
	{ "keyMaxSpeed",   Int_Tag, &ConfigureParams.Shortcut.withoutModifier[SHORTCUT_MAXSPEED] },
	{ "keyRecAnim",    Int_Tag, &ConfigureParams.Shortcut.withoutModifier[SHORTCUT_RECANIM] },
	{ "keyRecSound",   Int_Tag, &ConfigureParams.Shortcut.withoutModifier[SHORTCUT_RECSOUND] },
	{ "keySound",      Int_Tag, &ConfigureParams.Shortcut.withoutModifier[SHORTCUT_SOUND] },
	{ "keyQuit",       Int_Tag, &ConfigureParams.Shortcut.withoutModifier[SHORTCUT_QUIT] },
	{ NULL , Error_Tag, NULL }
};


/* Used to load/save sound options */
static const struct Config_Tag configs_Sound[] =
{
	{ "bEnableSound", Bool_Tag, &ConfigureParams.Sound.bEnableSound },
	{ "nPlaybackQuality", Int_Tag, &ConfigureParams.Sound.nPlaybackQuality },
	{ "szYMCaptureFileName", String_Tag, ConfigureParams.Sound.szYMCaptureFileName },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save memory options */
static const struct Config_Tag configs_Memory[] =
{
	{ "nMemorySize", Int_Tag, &ConfigureParams.Memory.nMemorySize },
	{ "szMemoryCaptureFileName", String_Tag, ConfigureParams.Memory.szMemoryCaptureFileName },
	{ NULL , Error_Tag, NULL }
};


/* Used to load/save floppy options */
static const struct Config_Tag configs_Floppy[] =
{
	{ "bAutoInsertDiskB", Bool_Tag, &ConfigureParams.DiskImage.bAutoInsertDiskB },
	{ "nWriteProtection", Int_Tag, &ConfigureParams.DiskImage.nWriteProtection },
	{ "szDiskImageDirectory", String_Tag, ConfigureParams.DiskImage.szDiskImageDirectory },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save HD options */
static const struct Config_Tag configs_HardDisk[] =
{
	{ "bBootFromHardDisk", Bool_Tag, &ConfigureParams.HardDisk.bBootFromHardDisk },
	{ "bUseHardDiskDirectory", Bool_Tag, &ConfigureParams.HardDisk.bUseHardDiskDirectories },
	{ "szHardDiskDirectory", String_Tag, ConfigureParams.HardDisk.szHardDiskDirectories[DRIVE_C] },
	/*{ "szHardDiskDirD", String_Tag, ConfigureParams.HardDisk.szHardDiskDirectories[DRIVE_D] },*/
	/*{ "szHardDiskDirE", String_Tag, ConfigureParams.HardDisk.szHardDiskDirectories[DRIVE_E] },*/
	/*{ "szHardDiskDirF", String_Tag, ConfigureParams.HardDisk.szHardDiskDirectories[DRIVE_F] },*/
	{ "bUseHardDiskImage", Bool_Tag, &ConfigureParams.HardDisk.bUseHardDiskImage },
	{ "szHardDiskImage", String_Tag, ConfigureParams.HardDisk.szHardDiskImage },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save ROM options */
static const struct Config_Tag configs_Rom[] =
{
	{ "szTosImageFileName", String_Tag, ConfigureParams.Rom.szTosImageFileName },
	{ "szCartridgeImageFileName", String_Tag, ConfigureParams.Rom.szCartridgeImageFileName },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save RS232 options */
static const struct Config_Tag configs_Rs232[] =
{
	{ "bEnableRS232", Bool_Tag, &ConfigureParams.RS232.bEnableRS232 },
	{ "szOutFileName", String_Tag, ConfigureParams.RS232.szOutFileName },
	{ "szInFileName", String_Tag, ConfigureParams.RS232.szInFileName },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save printer options */
static const struct Config_Tag configs_Printer[] =
{
	{ "bEnablePrinting", Bool_Tag, &ConfigureParams.Printer.bEnablePrinting },
	{ "bPrintToFile", Bool_Tag, &ConfigureParams.Printer.bPrintToFile },
	{ "szPrintToFileName", String_Tag, ConfigureParams.Printer.szPrintToFileName },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save MIDI options */
static const struct Config_Tag configs_Midi[] =
{
	{ "bEnableMidi", Bool_Tag, &ConfigureParams.Midi.bEnableMidi },
	{ "szMidiOutFileName", String_Tag, ConfigureParams.Midi.szMidiOutFileName },
	{ NULL , Error_Tag, NULL }
};

/* Used to load/save system options */
static const struct Config_Tag configs_System[] =
{
	{ "nMinMaxSpeed", Int_Tag, &ConfigureParams.System.nMinMaxSpeed },
	{ "nCpuLevel", Int_Tag, &ConfigureParams.System.nCpuLevel },
	{ "nCpuFreq", Int_Tag, &ConfigureParams.System.nCpuFreq },
	{ "bCompatibleCpu", Bool_Tag, &ConfigureParams.System.bCompatibleCpu },
	{ "nMachineType", Int_Tag, &ConfigureParams.System.nMachineType },
	{ "bBlitter", Bool_Tag, &ConfigureParams.System.bBlitter },
	{ "nDSPType", Int_Tag, &ConfigureParams.System.nDSPType },
	{ "bRealTimeClock", Bool_Tag, &ConfigureParams.System.bRealTimeClock },
	{ "bPatchTimerD", Bool_Tag, &ConfigureParams.System.bPatchTimerD },
	{ "bSlowFDC", Bool_Tag, &ConfigureParams.System.bSlowFDC },
	{ NULL , Error_Tag, NULL }
};


/*-----------------------------------------------------------------------*/
/**
 * Set default configuration values.
 */
void Configuration_SetDefault(void)
{
	int i;
	char *homeDir;

	/* Assume first-time install */
	bFirstTimeInstall = TRUE;

	/* Clear parameters */
	memset(&ConfigureParams, 0, sizeof(CNF_PARAMS));

	/* Set defaults for logging */
	strcpy(ConfigureParams.Log.sLogFileName, "stderr");
	ConfigureParams.Log.nTextLogLevel = LOG_INFO;
	ConfigureParams.Log.nAlertDlgLogLevel = LOG_INFO;

	/* Set defaults for floppy disk images */
	ConfigureParams.DiskImage.bAutoInsertDiskB = TRUE;
	ConfigureParams.DiskImage.nWriteProtection = WRITEPROT_OFF;
	strcpy(ConfigureParams.DiskImage.szDiskImageDirectory, szWorkingDir);
	File_AddSlashToEndFileName(ConfigureParams.DiskImage.szDiskImageDirectory);

	/* Set defaults for hard disks */
	ConfigureParams.HardDisk.bBootFromHardDisk = FALSE;
	ConfigureParams.HardDisk.nHardDiskDir = DRIVE_C;
	ConfigureParams.HardDisk.bUseHardDiskDirectories = FALSE;
	for (i=0; i<MAX_HARDDRIVES; i++)
	{
		strcpy(ConfigureParams.HardDisk.szHardDiskDirectories[i], szWorkingDir);
		File_CleanFileName(ConfigureParams.HardDisk.szHardDiskDirectories[i]);
	}
	ConfigureParams.HardDisk.bUseHardDiskImage = FALSE;
	strcpy(ConfigureParams.HardDisk.szHardDiskImage, szWorkingDir);
	ConfigureParams.HardDisk.bUseIdeHardDiskImage = FALSE;
	strcpy(ConfigureParams.HardDisk.szIdeHardDiskImage, szWorkingDir);

	/* Set defaults for Joysticks */
	for (i = 0; i < JOYSTICK_COUNT; i++)
	{
		ConfigureParams.Joysticks.Joy[i].nJoystickMode = JOYSTICK_DISABLED;
		ConfigureParams.Joysticks.Joy[i].bEnableAutoFire = FALSE;
		ConfigureParams.Joysticks.Joy[i].nJoyId = i;
		ConfigureParams.Joysticks.Joy[i].nKeyCodeUp = SDLK_UP;
		ConfigureParams.Joysticks.Joy[i].nKeyCodeDown = SDLK_DOWN;
		ConfigureParams.Joysticks.Joy[i].nKeyCodeLeft = SDLK_LEFT;
		ConfigureParams.Joysticks.Joy[i].nKeyCodeRight = SDLK_RIGHT;
		ConfigureParams.Joysticks.Joy[i].nKeyCodeFire = SDLK_RCTRL;
	}
	ConfigureParams.Joysticks.Joy[1].nJoyId = 0;    /* ST Joystick #1 is default joystick */
	ConfigureParams.Joysticks.Joy[0].nJoyId = 1;

	/* Set defaults for Keyboard */
	ConfigureParams.Keyboard.bDisableKeyRepeat = TRUE;
	ConfigureParams.Keyboard.nKeymapType = KEYMAP_SYMBOLIC;
	strcpy(ConfigureParams.Keyboard.szMappingFileName, "");
  
	/* Set defaults for Shortcuts */
	ConfigureParams.Shortcut.withoutModifier[SHORTCUT_OPTIONS] = SDLK_F12;
	ConfigureParams.Shortcut.withoutModifier[SHORTCUT_FULLSCREEN] = SDLK_F11;
  
	ConfigureParams.Shortcut.withModifier[SHORTCUT_OPTIONS] = SDLK_o;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_FULLSCREEN] = SDLK_f;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_MOUSEMODE] = SDLK_m;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_COLDRESET] = SDLK_c;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_WARMRESET] = SDLK_r;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_SCREENSHOT] = SDLK_g;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_BOSSKEY] = SDLK_i;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_CURSOREMU] = SDLK_j;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_MAXSPEED] = SDLK_x;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_RECANIM] = SDLK_a;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_RECSOUND] = SDLK_y;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_SOUND] = SDLK_s;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_QUIT] = SDLK_q;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_LOADMEM] = SDLK_l;
	ConfigureParams.Shortcut.withModifier[SHORTCUT_SAVEMEM] = SDLK_k;
	
	/* Set defaults for Memory */
	ConfigureParams.Memory.nMemorySize = 1;     /* 1 MiB */
	sprintf(ConfigureParams.Memory.szMemoryCaptureFileName, "%s%chatari.sav", szWorkingDir,PATHSEP);

	/* Set defaults for Printer */
	ConfigureParams.Printer.bEnablePrinting = FALSE;
	ConfigureParams.Printer.bPrintToFile = TRUE;
	sprintf(ConfigureParams.Printer.szPrintToFileName, "%s%chatari.prn", szWorkingDir,PATHSEP);

	/* Set defaults for RS232 */
	ConfigureParams.RS232.bEnableRS232 = FALSE;
	strcpy(ConfigureParams.RS232.szOutFileName, "/dev/modem");
	strcpy(ConfigureParams.RS232.szInFileName, "/dev/modem");

	/* Set defaults for MIDI */
	ConfigureParams.Midi.bEnableMidi = FALSE;
	strcpy(ConfigureParams.Midi.szMidiOutFileName, "/dev/midi00");

	/* Set defaults for Screen */
	ConfigureParams.Screen.bFullScreen = FALSE;
	ConfigureParams.Screen.FrameSkips = 1;
	ConfigureParams.Screen.bAllowOverscan = TRUE;
	ConfigureParams.Screen.bForce8Bpp = FALSE;
	ConfigureParams.Screen.bZoomLowRes = FALSE;
	ConfigureParams.Screen.MonitorType = MONITOR_TYPE_RGB;
	ConfigureParams.Screen.bUseExtVdiResolutions = FALSE;
	ConfigureParams.Screen.nVdiResolution = GEMRES_640x480;
	ConfigureParams.Screen.nVdiWidth = 0;
	ConfigureParams.Screen.nVdiHeight = 0;
	ConfigureParams.Screen.nVdiColors = GEMCOLOUR_16;
	ConfigureParams.Screen.bCaptureChange = FALSE;
	ConfigureParams.Screen.nFramesPerSecond = 25;

	/* Set defaults for Sound */
	ConfigureParams.Sound.bEnableSound = TRUE;
	ConfigureParams.Sound.nPlaybackQuality = PLAYBACK_MEDIUM;
	sprintf(ConfigureParams.Sound.szYMCaptureFileName, "%s%chatari.wav", szWorkingDir,PATHSEP);

	/* Set defaults for Rom */
	sprintf(ConfigureParams.Rom.szTosImageFileName, "%s%ctos.img", DATADIR,PATHSEP);
	strcpy(ConfigureParams.Rom.szCartridgeImageFileName, "");

	/* Set defaults for System */
	ConfigureParams.System.nCpuLevel = 0;
	ConfigureParams.System.nCpuFreq = 8;
	ConfigureParams.System.bCompatibleCpu = FALSE;
	/*ConfigureParams.System.bAddressSpace24 = TRUE;*/
	ConfigureParams.System.nMachineType = MACHINE_ST;
	ConfigureParams.System.bBlitter = FALSE;
	ConfigureParams.System.nDSPType = DSP_TYPE_NONE;
	ConfigureParams.System.bPatchTimerD = TRUE;
	ConfigureParams.System.bRealTimeClock = TRUE;
	ConfigureParams.System.nMinMaxSpeed = MINMAXSPEED_MIN;
	ConfigureParams.System.bSlowFDC = FALSE;

	/* Initialize the configuration file name */
	homeDir = getenv("HOME");
	if (homeDir != NULL && homeDir[0] != 0 && strlen(homeDir) < sizeof(sConfigFileName)-13)
		sprintf(sConfigFileName, "%s%c.hatari.cfg", homeDir,PATHSEP);
	else
		strcpy(sConfigFileName, "hatari.cfg");

#if defined(__AMIGAOS4__)
	/* Fix default path names on Amiga OS */
	sprintf(ConfigureParams.Rom.szTosImageFileName, "%stos.img", DATADIR);
#endif
}


/*-----------------------------------------------------------------------*/
/**
 * Copy details from configuration structure into global variables for system,
 * clean file names, etc...  Called from main.c and dialog.c files.
 */
void Configuration_Apply(BOOL bReset)
{
	if (bReset)
	{
		/* Set resolution change */
		bUseVDIRes = ConfigureParams.Screen.bUseExtVdiResolutions;
		bUseHighRes = ((!bUseVDIRes) && ConfigureParams.Screen.MonitorType == MONITOR_TYPE_MONO)
			|| (bUseVDIRes && ConfigureParams.Screen.nVdiColors == GEMCOLOUR_2);
		if (bUseHighRes)
		{
			STRes = ST_HIGH_RES;
		}
		if (bUseVDIRes)
		{
			VDI_SetResolution(ConfigureParams.Screen.nVdiResolution,
			                  ConfigureParams.Screen.nVdiColors);
		}
	}

	/* Set playback frequency */
	if (ConfigureParams.Sound.bEnableSound)
		Audio_SetOutputAudioFreq(ConfigureParams.Sound.nPlaybackQuality);

	/* CPU settings */
	if (ConfigureParams.System.nCpuFreq < 12)
	{
		ConfigureParams.System.nCpuFreq = 8;
		nCpuFreqShift = 0;
	}
	else if (ConfigureParams.System.nCpuFreq > 26)
	{
		ConfigureParams.System.nCpuFreq = 32;
		nCpuFreqShift = 2;
	}
	else
	{
		ConfigureParams.System.nCpuFreq = 16;
		nCpuFreqShift = 1;
	}
	/* Change UAE cpu_level and cpu_compatible accordingly */
	check_prefs_changed_cpu(ConfigureParams.System.nCpuLevel,
				ConfigureParams.System.bCompatibleCpu);

	/* Clean file and directory names */
	File_MakeAbsoluteName(ConfigureParams.Rom.szTosImageFileName);
	if (strlen(ConfigureParams.Rom.szCartridgeImageFileName) > 0)
		File_MakeAbsoluteName(ConfigureParams.Rom.szCartridgeImageFileName);
	File_MakeAbsoluteName(ConfigureParams.HardDisk.szHardDiskImage);
	File_CleanFileName(ConfigureParams.HardDisk.szHardDiskDirectories[0]);
	File_MakeAbsoluteName(ConfigureParams.HardDisk.szHardDiskDirectories[0]);
	File_MakeAbsoluteName(ConfigureParams.Memory.szMemoryCaptureFileName);
	File_MakeAbsoluteName(ConfigureParams.Sound.szYMCaptureFileName);
	File_MakeAbsoluteName(ConfigureParams.Keyboard.szMappingFileName);
	File_MakeAbsoluteName(ConfigureParams.RS232.szOutFileName);
	File_MakeAbsoluteName(ConfigureParams.RS232.szInFileName);
	
	/* make path names absolute, but handle special file names */
	File_MakeAbsoluteSpecialName(ConfigureParams.Log.sLogFileName);
	File_MakeAbsoluteSpecialName(ConfigureParams.Midi.szMidiOutFileName);
	File_MakeAbsoluteSpecialName(ConfigureParams.Printer.szPrintToFileName);
}


/*-----------------------------------------------------------------------*/
/**
 * Load a settings section from the configuration file.
 */
static int Configuration_LoadSection(const char *pFilename, const struct Config_Tag configs[], const char *pSection)
{
	int ret;

	ret = input_config(pFilename, configs, pSection);

	if (ret < 0)
		fprintf(stderr, "Can not load configuration file %s (section %s).\n",
		        sConfigFileName, pSection);

	return ret;
}


/*-----------------------------------------------------------------------*/
/**
 * Load program setting from configuration file. If psFileName is NULL, use
 * the default (i.e. the users) configuration file.
 */
void Configuration_Load(const char *psFileName)
{
	if (psFileName == NULL)
		psFileName = sConfigFileName;

	if (!File_Exists(psFileName))
	{
		fprintf(stderr, "Configuration file %s not found.\n", psFileName);
		return;
	}

	bFirstTimeInstall = FALSE;

	Configuration_LoadSection(psFileName, configs_Log, "[Log]");
	Configuration_LoadSection(psFileName, configs_Screen, "[Screen]");
	Configuration_LoadSection(psFileName, configs_Joystick0, "[Joystick0]");
	Configuration_LoadSection(psFileName, configs_Joystick1, "[Joystick1]");
	Configuration_LoadSection(psFileName, configs_Joystick2, "[Joystick2]");
	Configuration_LoadSection(psFileName, configs_Joystick3, "[Joystick3]");
	Configuration_LoadSection(psFileName, configs_Joystick4, "[Joystick4]");
	Configuration_LoadSection(psFileName, configs_Joystick5, "[Joystick5]");
	Configuration_LoadSection(psFileName, configs_Keyboard, "[Keyboard]");
	Configuration_LoadSection(psFileName, configs_ShortCutWithMod, "[ShortcutsWithModifiers]");
	Configuration_LoadSection(psFileName, configs_ShortCutWithoutMod, "[ShortcutsWithoutModifiers]");
	Configuration_LoadSection(psFileName, configs_Sound, "[Sound]");
	Configuration_LoadSection(psFileName, configs_Memory, "[Memory]");
	Configuration_LoadSection(psFileName, configs_Floppy, "[Floppy]");
	Configuration_LoadSection(psFileName, configs_HardDisk, "[HardDisk]");
	Configuration_LoadSection(psFileName, configs_Rom, "[ROM]");
	Configuration_LoadSection(psFileName, configs_Rs232, "[RS232]");
	Configuration_LoadSection(psFileName, configs_Printer, "[Printer]");
	Configuration_LoadSection(psFileName, configs_Midi, "[Midi]");
	Configuration_LoadSection(psFileName, configs_System, "[System]");
}


/*-----------------------------------------------------------------------*/
/**
 * Save a settings section to configuration file
 */
static int Configuration_SaveSection(const char *pFilename, const struct Config_Tag configs[], const char *pSection)
{
	int ret;

	ret = update_config(pFilename, configs, pSection);

	if (ret < 0)
		fprintf(stderr, "Error while updating section %s\n", pSection);

	return ret;
}


/*-----------------------------------------------------------------------*/
/**
 * Save program setting to configuration file
 */
void Configuration_Save(void)
{
	if (Configuration_SaveSection(sConfigFileName, configs_Log, "[Log]") < 0)
	{
		fprintf(stderr, "Error saving config file.\n");
		return;
	}
	Configuration_SaveSection(sConfigFileName, configs_Screen, "[Screen]");
	Configuration_SaveSection(sConfigFileName, configs_Joystick0, "[Joystick0]");
	Configuration_SaveSection(sConfigFileName, configs_Joystick1, "[Joystick1]");
	Configuration_SaveSection(sConfigFileName, configs_Joystick2, "[Joystick2]");
	Configuration_SaveSection(sConfigFileName, configs_Joystick3, "[Joystick3]");
	Configuration_SaveSection(sConfigFileName, configs_Joystick4, "[Joystick4]");
	Configuration_SaveSection(sConfigFileName, configs_Joystick5, "[Joystick5]");
	Configuration_SaveSection(sConfigFileName, configs_Keyboard, "[Keyboard]");
	Configuration_SaveSection(sConfigFileName, configs_ShortCutWithMod, "[ShortcutsWithModifiers]");
	Configuration_SaveSection(sConfigFileName, configs_ShortCutWithoutMod, "[ShortcutsWithoutModifiers]");
	Configuration_SaveSection(sConfigFileName, configs_Sound, "[Sound]");
	Configuration_SaveSection(sConfigFileName, configs_Memory, "[Memory]");
	Configuration_SaveSection(sConfigFileName, configs_Floppy, "[Floppy]");
	Configuration_SaveSection(sConfigFileName, configs_HardDisk, "[HardDisk]");
	Configuration_SaveSection(sConfigFileName, configs_Rom, "[ROM]");
	Configuration_SaveSection(sConfigFileName, configs_Rs232, "[RS232]");
	Configuration_SaveSection(sConfigFileName, configs_Printer, "[Printer]");
	Configuration_SaveSection(sConfigFileName, configs_Midi, "[Midi]");
	Configuration_SaveSection(sConfigFileName, configs_System, "[System]");
}


/*-----------------------------------------------------------------------*/
/**
 * Save/restore snapshot of configuration variables
 * ('MemorySnapShot_Store' handles type)
 */
void Configuration_MemorySnapShot_Capture(BOOL bSave)
{
	MemorySnapShot_Store(ConfigureParams.Rom.szTosImageFileName, sizeof(ConfigureParams.Rom.szTosImageFileName));
	MemorySnapShot_Store(ConfigureParams.Rom.szCartridgeImageFileName, sizeof(ConfigureParams.Rom.szCartridgeImageFileName));

	MemorySnapShot_Store(&ConfigureParams.Memory.nMemorySize, sizeof(ConfigureParams.Memory.nMemorySize));

	MemorySnapShot_Store(&ConfigureParams.HardDisk.bUseHardDiskDirectories, sizeof(ConfigureParams.HardDisk.bUseHardDiskDirectories));
	MemorySnapShot_Store(ConfigureParams.HardDisk.szHardDiskDirectories[DRIVE_C], sizeof(ConfigureParams.HardDisk.szHardDiskDirectories[DRIVE_C]));
	MemorySnapShot_Store(&ConfigureParams.HardDisk.bUseHardDiskImage, sizeof(ConfigureParams.HardDisk.bUseHardDiskImage));
	MemorySnapShot_Store(ConfigureParams.HardDisk.szHardDiskImage, sizeof(ConfigureParams.HardDisk.szHardDiskImage));

	MemorySnapShot_Store(&ConfigureParams.Screen.MonitorType, sizeof(ConfigureParams.Screen.MonitorType));
	MemorySnapShot_Store(&ConfigureParams.Screen.bUseExtVdiResolutions, sizeof(ConfigureParams.Screen.bUseExtVdiResolutions));
	MemorySnapShot_Store(&ConfigureParams.Screen.nVdiResolution, sizeof(ConfigureParams.Screen.nVdiResolution));
	MemorySnapShot_Store(&ConfigureParams.Screen.nVdiWidth, sizeof(ConfigureParams.Screen.nVdiWidth));
	MemorySnapShot_Store(&ConfigureParams.Screen.nVdiHeight, sizeof(ConfigureParams.Screen.nVdiHeight));
	MemorySnapShot_Store(&ConfigureParams.Screen.nVdiColors, sizeof(ConfigureParams.Screen.nVdiColors));

	MemorySnapShot_Store(&ConfigureParams.System.nCpuLevel, sizeof(ConfigureParams.System.nCpuLevel));
	MemorySnapShot_Store(&ConfigureParams.System.nCpuFreq, sizeof(ConfigureParams.System.nCpuFreq));
	MemorySnapShot_Store(&ConfigureParams.System.bCompatibleCpu, sizeof(ConfigureParams.System.bCompatibleCpu));
	MemorySnapShot_Store(&ConfigureParams.System.nMachineType, sizeof(ConfigureParams.System.nMachineType));
	MemorySnapShot_Store(&ConfigureParams.System.bBlitter, sizeof(ConfigureParams.System.bBlitter));
	MemorySnapShot_Store(&ConfigureParams.System.nDSPType, sizeof(ConfigureParams.System.nDSPType));
	MemorySnapShot_Store(&ConfigureParams.System.bRealTimeClock, sizeof(ConfigureParams.System.bRealTimeClock));
	MemorySnapShot_Store(&ConfigureParams.System.bPatchTimerD, sizeof(ConfigureParams.System.bPatchTimerD));
	MemorySnapShot_Store(&ConfigureParams.System.bSlowFDC, sizeof(ConfigureParams.System.bSlowFDC));

	if (!bSave)
		Configuration_Apply(TRUE);
}

