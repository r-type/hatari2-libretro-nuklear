/*
  Hatari - options.c

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

  Functions for showing and parsing all of Hatari's command line options.
  
  To add a new option:
  - Add option ID to the enum
  - Add the option information to corresponding place in HatariOptions[]
  - Add required actions for that ID to switch in Opt_ParseParameters()
*/
const char Main_rcsid[] = "Hatari $Id: options.c,v 1.3 2006-02-19 21:48:37 eerot Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "main.h"
#include "options.h"
#include "configuration.h"
#include "file.h"
#include "screen.h"
#include "video.h"
#include "vdi.h"

#include "uae-cpu/hatari-glue.h"


/*  List of supported options. */
enum {
	OPT_HELP,
	OPT_VERSION,
	OPT_MONO,
	OPT_FULLSCREEN,
	OPT_WINDOW,
	OPT_FRAMESKIP,
	OPT_DEBUG,
	OPT_JOYSTICK,
	OPT_NOSOUND,
	OPT_PRINTER,
	OPT_MIDI,
	OPT_RS232,
	OPT_HDIMAGE,
	OPT_HARDDRIVE,
	OPT_TOS,
	OPT_CARTRIDGE,
	OPT_CPULEVEL,
	OPT_COMPATIBLE,
	OPT_BLITTER,
	OPT_VDI,
	OPT_MEMSIZE,
	OPT_CONFIGFILE,
	OPT_SLOWFDC,
	OPT_MACHINE,
	OPT_NONE,
};

typedef struct {
	unsigned int id;	/* option ID */
	const char *chr;	/* short option */
	const char *str;	/* long option */
	const char *arg;	/* name for argument, if any */
	const char *desc;	/* option description */
} opt_t;

/* these have(!) to be in the same order as the enums */
static const opt_t HatariOptions[] = {
	{ OPT_HELP,      "-h", "--help",
	  NULL, "Print this help text and exit" },
	{ OPT_VERSION,   "-v", "--version",
	  NULL, "Print version number & help and exit" },
	{ OPT_MONO,      "-m", "--mono",
	  NULL, "Start in monochrome mode instead of color" },
	{ OPT_FULLSCREEN,"-f", "--fullscreen",
	  NULL, "Start emulator in fullscreen mode" },
	{ OPT_WINDOW,    "-w", "--window",
	  NULL, "Start emulator in window mode" },
	{ OPT_FRAMESKIP, NULL, "--frameskip",
	  NULL, "Skip every second frame (speeds up emulation!)" },
	{ OPT_DEBUG,     "-D", "--debug",
	  NULL, "Allow debug interface" },
	{ OPT_JOYSTICK,  "-j", "--joystick",
	  NULL, "Emulate a ST joystick with the cursor keys" },
	{ OPT_NOSOUND,   NULL, "--nosound",
	  NULL, "Disable sound (faster!)" },
	{ OPT_PRINTER,   NULL, "--printer",
	  NULL, "Enable printer support (experimental)" },
	{ OPT_MIDI,      NULL, "--midi",
	  "<file>", "Enable midi support and write midi data to <file>" },
	{ OPT_RS232,     NULL, "--rs232",
	  "<file>", "Use <file> as the serial port device" },
	{ OPT_HDIMAGE,   NULL, "--hdimage",
	  "<file>", "Emulate an ST harddrive with an image <file>" },
	{ OPT_HARDDRIVE, "-d", "--harddrive",
	  "<dir>", "Emulate an ST harddrive (<dir> = root directory)" },
	{ OPT_TOS,       "-t", "--tos",
	  "<file>", "Use TOS image <file>" },
	{ OPT_CARTRIDGE, NULL, "--cartridge",
	  "<file>", "Use ROM cartridge image <file>" },
	{ OPT_CPULEVEL,  NULL, "--cpulevel",
	  "<x>", "Set the CPU type (x => 680x0) (TOS 2.06 only!)" },
	{ OPT_COMPATIBLE,NULL, "--compatible",
	  NULL, "Use a more compatible (but slower) 68000 CPU mode" },
	{ OPT_BLITTER,   NULL, "--blitter",
	  NULL, "Enable blitter emulation (unstable!)" },
	{ OPT_VDI,       NULL, "--vdi",
	  NULL, "Use extended VDI resolution" },
	{ OPT_MEMSIZE,   "-s", "--memsize",
	  "<x>", "ST RAM size. x = size in MiB from 0 to 14, 0 for 512KiB" },
	{ OPT_CONFIGFILE,"-c", "--configfile",
	  "<file>", "Use <file> instead of the ~/.hatari.cfg config file" },
	{ OPT_SLOWFDC,   NULL, "--slowfdc",
	  NULL, "Slow down FDC emulation (very experimental!)" },
	{ OPT_MACHINE,   NULL, "--machine",
	  "<x>", "Select machine type (x = st/ste/tt)" },
	{ OPT_NONE, NULL, NULL, NULL, NULL }
};

/* Show Hatari options and exit().
 * If 'error' given, show that error message.
 * If 'option' != OPT_NONE, tells for which option the error is,
 * otherwise 'value' is show as the option user gave.
 */
static void Opt_ShowExit(int option, const char *value, const char *error)
{
	unsigned int i, len, maxlen;
	char buf[64];
	const opt_t *opt;
	
	printf("This is %s.\n", PROG_NAME);
	printf("This program is free software licensed under the GNU GPL.\n\n");
	printf("Usage:\n hatari [options] [disk image name]\n"
	       "Where options are:\n");
	if (option == OPT_VERSION) {
        	exit(0);
        }

	/* find largest option name and check option IDs */
	i = maxlen = 0;
	for (opt = HatariOptions; opt->id != OPT_NONE; opt++) {
		assert(opt->id == i++);
		len = strlen(opt->str);
		if (opt->arg) {
			len += strlen(opt->arg);
			len += 1;
			/* with arg, short options go to another line */
		} else {
			if (opt->chr) {
				/* ' or -c' */
				len += 6;
			}
		}
		if (len > maxlen) {
			maxlen = len;
		}
	}
	assert(maxlen < sizeof(buf));
	
	/* output all options */
	for (opt = HatariOptions; opt->id != OPT_NONE; opt++) {
		if (opt->arg) {
			sprintf(buf, "%s %s", opt->str, opt->arg);
			printf("  %-*s %s\n", maxlen, buf, opt->desc);
			if (opt->chr) {
				printf("    or %s %s\n", opt->chr, opt->arg);
			}
		} else {
			if (opt->chr) {
				sprintf(buf, "%s or %s", opt->str, opt->chr);
				printf("  %-*s %s\n", maxlen, buf, opt->desc);
			} else {
				printf("  %-*s %s\n", maxlen, opt->str, opt->desc);
			}
		}
	}
	if (error) {
		if (option != OPT_NONE) {
			fprintf(stderr, "\nError (%s): %s\n", HatariOptions[option].str, error);
		} else {
			fprintf(stderr, "\nError: %s (%s)\n", error, value);
		}
		exit(1);
	}
	exit(0);
}

/*
 * matches string under given index in the argv against all Hatari
 * short and long options. If match is found, returns ID for that,
 * otherwise shows help.
 * 
 * Checks also that if option is supposed to have argument,
 * whether there's one.
 */
static int Opt_WhichOption(int argc, char *argv[], int idx)
{
	const opt_t *opt;
	const char *str = argv[idx];

	for (opt = HatariOptions; opt->id != OPT_NONE; opt++) {
		
		if ((!strcmp(str, opt->str)) ||
		    (opt->chr && !strcmp(str, opt->chr))) {
			
			if (opt->arg && idx+1 >= argc) {
				Opt_ShowExit(opt->id, NULL, "Missing argument");
			}
			return opt->id;
		}
	}
	Opt_ShowExit(OPT_NONE, argv[idx], "Unrecognized option");
	return OPT_NONE;
}

/*-----------------------------------------------------------------------*/
/*
  Check for any passed parameters, return boot disk
*/
void Opt_ParseParameters(int argc, char *argv[],
			 char *bootdisk, size_t bootlen)
{
	int i;
	
	for(i = 1; i < argc; i++) {
		
		if (argv[i][0] != '-') {
			/* Possible passed disk image filename */
			if (argv[i][0] && File_Exists(argv[i]) &&
			    strlen(argv[i]) < bootlen) {
				strcpy(bootdisk, argv[i]);
				File_MakeAbsoluteName(bootdisk);
			} else {
				Opt_ShowExit(OPT_NONE, argv[i], "Not an option nor disk image");
			}
			continue;
		}
    
		/* WhichOption() checks also that there is an argument,
		 * so we don't need to check that below
		 */
		switch(Opt_WhichOption(argc, argv, i)) {

		case OPT_HELP:
			Opt_ShowExit(OPT_HELP, NULL, NULL);
			break;
			
		case OPT_VERSION:
			Opt_ShowExit(OPT_VERSION, NULL, NULL);
			break;
			
		case OPT_MONO:
			bUseHighRes = TRUE;
			ConfigureParams.Screen.bUseHighRes = TRUE;
			STRes = PrevSTRes = ST_HIGH_RES;
			break;
			
		case OPT_FULLSCREEN:
			ConfigureParams.Screen.bFullScreen = TRUE;
			break;
			
		case OPT_WINDOW:
			ConfigureParams.Screen.bFullScreen = FALSE;
			break;
			
		case OPT_JOYSTICK:
			ConfigureParams.Joysticks.Joy[1].nJoystickMode = JOYSTICK_KEYBOARD;
			break;
			
		case OPT_NOSOUND:
			ConfigureParams.Sound.bEnableSound = FALSE;
			break;
			
		case OPT_FRAMESKIP:
			ConfigureParams.Screen.bFrameSkip = TRUE;
			break;
			
		case OPT_DEBUG:
			bEnableDebug = TRUE;
			break;
			
		case OPT_PRINTER:
			/* FIXME: add more commandline configuration for printing */
			ConfigureParams.Printer.bEnablePrinting = TRUE;
			break;
			
		case OPT_MIDI:
			i += 1;
			if (strlen(argv[i]) < sizeof(ConfigureParams.Midi.szMidiOutFileName)) {
				ConfigureParams.Midi.bEnableMidi = TRUE;
				strcpy(ConfigureParams.Midi.szMidiOutFileName, argv[i]);
			} else {
				Opt_ShowExit(OPT_NONE, argv[i], "Midi file name too long!\n");
			}
			break;
      
		case OPT_RS232:
			i += 1;
			if (strlen(argv[i]) < sizeof(ConfigureParams.RS232.szOutFileName)) {
				ConfigureParams.RS232.bEnableRS232 = TRUE;
				strcpy(ConfigureParams.RS232.szOutFileName, argv[i]);
				strcpy(ConfigureParams.RS232.szInFileName, argv[i]);
			} else {
				Opt_ShowExit(OPT_NONE, argv[i], "RS232 file name too long!\n");
			}
			break;
			
		case OPT_HDIMAGE:
			i += 1;
			if (!File_Exists(argv[i])) {
				Opt_ShowExit(OPT_NONE, argv[i], "Given HD image file doesn't exist!\n");
			}
			if (strlen(argv[i]) < sizeof(ConfigureParams.HardDisk.szHardDiskImage)) {
				ConfigureParams.HardDisk.bUseHardDiskImage = TRUE;
				strcpy(ConfigureParams.HardDisk.szHardDiskImage, argv[i]);
			} else {
				Opt_ShowExit(OPT_NONE, argv[i], "HD image file name too long!\n");
			}
			break;
			
		case OPT_HARDDRIVE:
			i += 1;
			if(strlen(argv[i]) < sizeof(ConfigureParams.HardDisk.szHardDiskDirectories[0]))	{
				ConfigureParams.HardDisk.bUseHardDiskDirectories = TRUE;
				ConfigureParams.HardDisk.bBootFromHardDisk = TRUE;
				strcpy(ConfigureParams.HardDisk.szHardDiskDirectories[0], argv[i]);
			} else {
				Opt_ShowExit(OPT_NONE, argv[i], "HD directory name too long!\n");
			}
			break;
			
		case OPT_TOS:
			i += 1;
			if (!File_Exists(argv[i])) {
				Opt_ShowExit(OPT_NONE, argv[i], "Given TOS image file doesn't exist!\n");
			}
			if (strlen(argv[i]) < sizeof(ConfigureParams.Rom.szTosImageFileName)) {
				strcpy(ConfigureParams.Rom.szTosImageFileName, argv[i]);
			} else {
				Opt_ShowExit(OPT_NONE, argv[i], "TOS image file name too long!\n");
			}
			break;
      
		case OPT_CARTRIDGE:
			i += 1;
			if (!File_Exists(argv[i])) {
				Opt_ShowExit(OPT_NONE, argv[i], "Given Cartridge image file doesn't exist!\n");
			}
			if (strlen(argv[i]) < sizeof(ConfigureParams.Rom.szCartridgeImageFileName)) {
				strcpy(ConfigureParams.Rom.szCartridgeImageFileName, argv[i]);
			} else {
				Opt_ShowExit(OPT_NONE, argv[i], "Cartridge image file name too long!\n");
			}
			break;
			
		case OPT_CPULEVEL:
			/* UAE core uses cpu_level variable */
			cpu_level = atoi(argv[++i]);
			if(cpu_level < 0 || cpu_level > 4) {
				fprintf(stderr, "CPU level %d is invalid (valid: 0-4), set to 0.\n", cpu_level);
				cpu_level = 0;
			}
			ConfigureParams.System.nCpuLevel = cpu_level;
			break;
			
		case OPT_COMPATIBLE:
			cpu_compatible = TRUE;
			ConfigureParams.System.bCompatibleCpu = TRUE;
			break;
			
		case OPT_BLITTER:
			ConfigureParams.System.bBlitter = TRUE;
			break;
			
		case OPT_VDI:
			bUseVDIRes = ConfigureParams.Screen.bUseExtVdiResolutions = TRUE;
			break;
			
		case OPT_SLOWFDC:
			ConfigureParams.System.bSlowFDC = TRUE;
			break;
			
		case OPT_MEMSIZE:
			ConfigureParams.Memory.nMemorySize = atoi(argv[++i]);
			if (ConfigureParams.Memory.nMemorySize < 0 ||
			    ConfigureParams.Memory.nMemorySize > 14) {
				fprintf(stderr, "Memory size %d is invalid (valid: 0-14MB), set to 1.\n",
					ConfigureParams.Memory.nMemorySize);
				ConfigureParams.Memory.nMemorySize = 1;
			}
			break;
			
		case OPT_CONFIGFILE:
			i += 1;
			if (!File_Exists(argv[i])) {
				Opt_ShowExit(OPT_NONE, argv[i], "Given config file doesn't exist!\n");
			}
			if (strlen(argv[i]) < sizeof(sConfigFileName)) {
				strcpy(sConfigFileName, argv[i]);
				Configuration_Load(NULL);
			} else {
				Opt_ShowExit(OPT_NONE, argv[i], "Config file name too long!\n");
			}
			break;
			
		case OPT_MACHINE:
			i += 1;
			if (strcasecmp(argv[i], "st") == 0) {
				ConfigureParams.System.nMachineType = MACHINE_ST;
			} else if (strcasecmp(argv[i], "ste") == 0) {
				ConfigureParams.System.nMachineType = MACHINE_STE;
			} else if (strcasecmp(argv[i], "tt") == 0) {
				ConfigureParams.System.nMachineType = MACHINE_TT;
				ConfigureParams.System.nCpuLevel = cpu_level = 3;
				ConfigureParams.System.nCpuFreq = 32;
			} else {
				Opt_ShowExit(OPT_NONE, argv[i], "Unknown machine type");
			}
			break;
			
		default:
			Opt_ShowExit(OPT_NONE, argv[i], "Program didn't handle documented option");
		}
	}
	
	Configuration_WorkOnDetails(FALSE);
}