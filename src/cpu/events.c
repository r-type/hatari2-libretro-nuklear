/*
 * events.c
 *
 * Event stuff - currently just simplified to the bare minimum
 * in Hatari, but we might want to extend this one day...
 */

#include "main.h"

#include "sysconfig.h"
#include "sysdeps.h"

#include "options_cpu.h"
#include "events.h"
#include "newcpu.h"

void do_cycles_slow (unsigned long cycles_to_add)
{
	currcycle += cycles_to_add;
}
