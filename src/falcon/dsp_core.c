/*
	DSP M56001 emulation
	Host/Emulator <-> DSP glue

	(C) 2003-2008 ARAnyM developer team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "math.h"
#include "dsp_core.h"
#include "dsp_cpu.h"

#ifndef M_PI
#define M_PI	3.141592653589793238462643383279502
#endif

#define DEBUG 0

/* More disasm infos, if wanted */
#define DSP_DISASM_HOSTREAD 0	/* Dsp->Host transfer */
#define DSP_DISASM_HOSTWRITE 0	/* Host->Dsp transfer */
#define DSP_DISASM_STATE 0	/* State changes */

/* Init DSP emulation */
void dsp_core_init(dsp_core_t *dsp_core)
{
	int i;

#if DEBUG
	fprintf(stderr, "Dsp: core init\n");
#endif

	memset(dsp_core->ram, 0,sizeof(dsp_core->ram));
	memset(dsp_core->ramint, 0,sizeof(dsp_core->ramint));
	memset((void*)dsp_core->hostport, 0,sizeof(dsp_core->hostport));

	/* Initialize Y:rom[0x0100-0x01ff] with a sin table */
	for (i=0;i<256;i++) {
		float src = (((float) i)*M_PI)/128.0;
		Sint32 dest = (Sint32) (sin(src) * 8388608.0); /* 1<<23 */
		if (dest>8388607) {
			dest = 8388607;
		} else if (dest<-8388608) {
			dest = -8388608;
		}
		dsp_core->rom[DSP_SPACE_Y][0x100+i]=dest & 0x00ffffff;
	}

	/* Initialize X:rom[0x0100-0x017f] with a mu-law table */
	{
		const Uint16 mulaw_base[8]={
			0x7d7c, 0x3e7c, 0x1efc, 0x0f3c, 0x075c, 0x036c, 0x0174, 0x0078
		};

		Uint32 position = 0x0100;
		Uint32 offset = 0x040000;

		for(i=0;i<8;i++) {
			int j;
			Uint32 value = mulaw_base[i]<<8;

			for (j=0;j<16;j++) {
				dsp_core->rom[DSP_SPACE_X][position++]=value;
				value -= offset;
			}

			offset >>= 1;
		}
	}

	/* Initialize X:rom[0x0180-0x01ff] with a a-law table */
	{
		const Sint32 multiply_base[8]={
			0x1580, 0x0ac0, 0x5600, 0x2b00,
			0x1580, 0x0058, 0x0560, 0x02b0
		};
		const Sint32 multiply_col[4]={0x10, 0x01, 0x04, 0x02};
		const Sint32 multiply_line[4]={0x40, 0x04, 0x10, 0x08};
		const Sint32 base_values[4]={0, -1, 2, 1};
		Uint32 pos=0x0180;
		
		for (i=0;i<8;i++) {
			Sint32 alawbase, j;

			alawbase = multiply_base[i]<<8;
			for (j=0;j<4;j++) {
				Sint32 alawbase1, k;
				
				alawbase1 = alawbase + ((base_values[j]*multiply_line[i & 3])<<12);

				for (k=0;k<4;k++) {
					Sint32 alawbase2;

					alawbase2 = alawbase1 + ((base_values[k]*multiply_col[i & 3])<<12);

					dsp_core->rom[DSP_SPACE_X][pos++]=alawbase2;
				}
			}
		}
	}

	dsp_core->running = 0;
}

/* Shutdown DSP emulation */
void dsp_core_shutdown(dsp_core_t *dsp_core)
{
#if DEBUG
	fprintf(stderr, "Dsp: core shutdown\n");
#endif

	dsp_core->running = 0;
}

/* Reset */
void dsp_core_reset(dsp_core_t *dsp_core)
{
	int i;

#if DEBUG
	fprintf(stderr, "Dsp: core reset\n");
#endif

	/* Kill existing thread and semaphore */
	dsp_core_shutdown(dsp_core);

	/* Memory */
	memset((void*)dsp_core->periph, 0,sizeof(dsp_core->periph));
	memset(dsp_core->stack, 0,sizeof(dsp_core->stack));
	memset(dsp_core->registers, 0,sizeof(dsp_core->registers));

	dsp_core->bootstrap_pos = 0;
	
	/* Registers */
	dsp_core->pc = 0x0000;
	dsp_core->registers[DSP_REG_OMR]=0x02;
	for (i=0;i<8;i++) {
		dsp_core->registers[DSP_REG_M0+i]=0x00ffff;
	}

	/* Interruptions */
	dsp_core->interrupt_state = DSP_INTERRUPT_NONE;
	dsp_core->interrupt_instr_fetch = -1;
	dsp_core->interrupt_save_pc = -1;
	dsp_core->swi_inter = 0;

	/* host port init, dsp side */
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR]=(1<<DSP_HOST_HSR_HTDE);

	/* host port init, cpu side */
	dsp_core->hostport[CPU_HOST_CVR]=0x12;
	dsp_core->hostport[CPU_HOST_ISR]=(1<<CPU_HOST_ISR_TRDY)|(1<<CPU_HOST_ISR_TXDE);
	dsp_core->hostport[CPU_HOST_IVR]=0x0f;

	/* Other hardware registers */
	dsp_core->periph[DSP_SPACE_X][DSP_IPR]=0;
	dsp_core->periph[DSP_SPACE_X][DSP_BCR]=0xffff;

	/* Misc */
	dsp_core->loop_rep = 0;

#if DEBUG
	fprintf(stderr, "Dsp: reset done\n");
#endif

}


/* Process Host Interface peripheral code */
void dsp_core_process_host_interface(dsp_core_t *dsp_core)
{
	Uint8 dma_mode, hreq;

	/* DMA mode : 	00: Interrupt mode (DMA mode off)
			01: 24 bits DMA mode
			02: 16 bits DMA mode
			03;  8 bits DMA mode */
	dma_mode = dsp_core->hostport[CPU_HOST_ICR] & (0x3<<CPU_HOST_ICR_HM0); 

	/* TODO: DMA mode */
	dsp_core_host2dsp(dsp_core);
	dsp_core_dsp2host(dsp_core);

	/* Update ISR Host Request HREQ bit (interrupt mode only) */
	if (dma_mode == 0){
		hreq = (dsp_core->hostport[CPU_HOST_ICR] & 0x3) & (dsp_core->hostport[CPU_HOST_ISR] & 0x3);
		dsp_core->hostport[CPU_HOST_ISR] &= 0x7f; 
		dsp_core->hostport[CPU_HOST_ISR] |= (hreq?1:0) << CPU_HOST_ISR_HREQ; 
	}
}

/* Process SSI peripheral code */
void dsp_core_process_ssi_interface(dsp_core_t *dsp_core)
{
}

/* Process SCI peripheral code */
void dsp_core_process_sci_interface(dsp_core_t *dsp_core)
{
}


static void dsp_core_hostport_update_trdy(dsp_core_t *dsp_core)
{
	int trdy;

	/* Clear/set TRDY bit */
	dsp_core->hostport[CPU_HOST_ISR] &= 0xff-(1<<CPU_HOST_ISR_TRDY);
	trdy = (dsp_core->hostport[CPU_HOST_ISR]>>CPU_HOST_ISR_TXDE)
		& ~(dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR]>>DSP_HOST_HSR_HRDF);
	dsp_core->hostport[CPU_HOST_ISR] |= (trdy & 1)<< CPU_HOST_ISR_TRDY;
}

/* Host port transfer ? (dsp->host) */
void dsp_core_dsp2host(dsp_core_t *dsp_core)
{
	/* RXDF = 1 ==> host hasn't read the last value yet */
	if (dsp_core->hostport[CPU_HOST_ISR] & (1<<CPU_HOST_ISR_RXDF)) {
		return;
	}

	/* HTDE = 1 ==> nothing to tranfert from DSP port */
	if (dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] & (1<<DSP_HOST_HSR_HTDE)) {
		return;
	}

	dsp_core->hostport[CPU_HOST_RXL] =  (Uint8)(dsp_core->periph[DSP_SPACE_X][DSP_HOST_HTX] & 0xff);
	dsp_core->hostport[CPU_HOST_RXM] = (Uint8)((dsp_core->periph[DSP_SPACE_X][DSP_HOST_HTX]>>8)  & 0xff);
	dsp_core->hostport[CPU_HOST_RXH] = (Uint8)((dsp_core->periph[DSP_SPACE_X][DSP_HOST_HTX]>>16) & 0xff);
#if DSP_DISASM_HOSTWRITE
	fprintf(stderr, "Dsp: (D->H): Transfer 0x%06x\n", dsp_core->periph[DSP_SPACE_X][DSP_HOST_HTX]);
#endif
	/* Set HTDE bit to say that DSP can write */
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] |= 1<<DSP_HOST_HSR_HTDE;
#if DSP_DISASM_HOSTWRITE
	fprintf(stderr, "Dsp: (D->H): Dsp HTDE set\n");
#endif

	/* Set RXDF bit to say that host can read */
	dsp_core->hostport[CPU_HOST_ISR] |= 1<<CPU_HOST_ISR_RXDF;
#if DSP_DISASM_HOSTWRITE
	fprintf(stderr, "Dsp: (D->H): Host RXDF set\n");
#endif
}

/* Host port transfer ? (host->dsp) */
void dsp_core_host2dsp(dsp_core_t *dsp_core)
{
	/* TXDE = 1 ==> nothing to tranfert from host port */
	if (dsp_core->hostport[CPU_HOST_ISR] & (1<<CPU_HOST_ISR_TXDE)) {
		return;
	}
	
	/* HRDF = 1 ==> DSP hasn't read the last value yet */
	if (dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] & (1<<DSP_HOST_HSR_HRDF)) {
		return;
	}

	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HRX] = dsp_core->hostport[CPU_HOST_TXL];
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HRX] |= dsp_core->hostport[CPU_HOST_TXM]<<8;
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HRX] |= dsp_core->hostport[CPU_HOST_TXH]<<16;
#if DSP_DISASM_HOSTREAD
	fprintf(stderr, "Dsp: (H->D): Transfer 0x%06x\n", dsp_core->periph[DSP_SPACE_X][DSP_HOST_HRX]);
#endif

	/* Set HRDF bit to say that DSP can read */
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] |= 1<<DSP_HOST_HSR_HRDF;
#if DSP_DISASM_HOSTREAD
	fprintf(stderr, "Dsp: (H->D): Dsp HRDF set\n");
#endif

	/* Set TXDE bit to say that host can write */
	dsp_core->hostport[CPU_HOST_ISR] |= 1<<CPU_HOST_ISR_TXDE;
# if DSP_DISASM_HOSTREAD
	fprintf(stderr, "Dsp: (H->D): Host TXDE set\n");
# endif

	dsp_core_hostport_update_trdy(dsp_core);
}

void dsp_core_hostport_dspread(dsp_core_t *dsp_core)
{
	/* Clear HRDF bit to say that DSP has read */
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] &= 0xff-(1<<DSP_HOST_HSR_HRDF);
#if DSP_DISASM_HOSTREAD
	fprintf(stderr, "Dsp: (H->D): Dsp HRDF cleared\n");
#endif
	dsp_core_hostport_update_trdy(dsp_core);
}

void dsp_core_hostport_dspwrite(dsp_core_t *dsp_core)
{
	/* Clear HTDE bit to say that DSP has written */
	dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] &= 0xff-(1<<DSP_HOST_HSR_HTDE);
#if DSP_DISASM_HOSTWRITE
	fprintf(stderr, "Dsp: (D->H): Dsp HTDE cleared\n");
#endif
}


/* Read/writes on host port */

Uint8 dsp_core_read_host(dsp_core_t *dsp_core, int addr)
{
	Uint8 value;

	value = dsp_core->hostport[addr];
	if (addr == CPU_HOST_RXL) {
		/* Clear RXDF bit to say that CPU has read */
		dsp_core->hostport[CPU_HOST_ISR] &= 0xff-(1<<CPU_HOST_ISR_RXDF);
#if DSP_DISASM_HOSTWRITE
		fprintf(stderr, "Dsp: (D->H): Host RXDF cleared\n");
#endif
	}
	return value;
}

void dsp_core_write_host(dsp_core_t *dsp_core, int addr, Uint8 value)
{
	switch(addr) {
		case CPU_HOST_ICR:
			dsp_core->hostport[CPU_HOST_ICR]=value & 0xfb;
			/* Set HF1 and HF0 accordingly on the host side */
			dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] &=
					0xff-((1<<DSP_HOST_HSR_HF1)|(1<<DSP_HOST_HSR_HF0));
			dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] |=
					dsp_core->hostport[CPU_HOST_ICR] & ((1<<DSP_HOST_HSR_HF1)|(1<<DSP_HOST_HSR_HF0));
			break;
		case CPU_HOST_CVR:
			dsp_core->hostport[CPU_HOST_CVR]=value & 0x9f;
			/* if bit 7=1, host command . HSR(bit HCP) is set*/
			if (value & (1<<7)) {
				dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] |= (1<<DSP_HOST_HSR_HCP);
			}
			else{
				dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] &= 0xff - (1<<DSP_HOST_HSR_HCP);
			}
			break;
		case CPU_HOST_ISR:
		case CPU_HOST_TX0:
			/* Read only */
			break;
		case CPU_HOST_IVR:
		case CPU_HOST_TXH:
		case CPU_HOST_TXM:
			dsp_core->hostport[addr]=value;
			break;
		case CPU_HOST_TXL:
			dsp_core->hostport[addr]=value;

			if (!dsp_core->running) {
				dsp_core->ramint[DSP_SPACE_P][dsp_core->bootstrap_pos] =
					(dsp_core->hostport[CPU_HOST_TXH]<<16) |
					(dsp_core->hostport[CPU_HOST_TXM]<<8) |
					 dsp_core->hostport[CPU_HOST_TXL];
#if DEBUG
				fprintf(stderr, "Dsp: bootstrap p:0x%04x = 0x%06x\n",
					dsp_core->bootstrap_pos,
					dsp_core->ramint[DSP_SPACE_P][dsp_core->bootstrap_pos]);
#endif
				if (++dsp_core->bootstrap_pos == 0x200) {
#if DSP_DISASM_STATE
					fprintf(stderr, "Dsp: WAIT_BOOTSTRAP done\n");
#endif
					dsp_core->running = 1;
				}		
			} else {

				/* If TRDY is set, the tranfert is direct to DSP (Burst mode) */
				if (dsp_core->hostport[CPU_HOST_ISR] & (1<<CPU_HOST_ISR_TRDY)){
					dsp_core->periph[DSP_SPACE_X][DSP_HOST_HRX] = dsp_core->hostport[CPU_HOST_TXL];
					dsp_core->periph[DSP_SPACE_X][DSP_HOST_HRX] |= dsp_core->hostport[CPU_HOST_TXM]<<8;
					dsp_core->periph[DSP_SPACE_X][DSP_HOST_HRX] |= dsp_core->hostport[CPU_HOST_TXH]<<16;
#if DSP_DISASM_HOSTREAD
					fprintf(stderr, "Dsp: (H->D): Direct Transfer 0x%06x\n", dsp_core->periph[DSP_SPACE_X][DSP_HOST_HRX]);
#endif

					/* Set HRDF bit to say that DSP can read */
					dsp_core->periph[DSP_SPACE_X][DSP_HOST_HSR] |= 1<<DSP_HOST_HSR_HRDF;
#if DSP_DISASM_HOSTREAD
					fprintf(stderr, "Dsp: (H->D): Dsp HRDF set\n");
#endif
				}
				else{
					/* Clear TXDE to say that CPU has written */
					dsp_core->hostport[CPU_HOST_ISR] &= 0xff-(1<<CPU_HOST_ISR_TXDE);
#if DSP_DISASM_HOSTREAD
					fprintf(stderr, "Dsp: (H->D): Host TXDE cleared\n");
#endif
				}
				dsp_core_hostport_update_trdy(dsp_core);
			}
			break;
	}
}

/*
vim:ts=4:sw=4:
*/
