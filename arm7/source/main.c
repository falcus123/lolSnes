/*
    Copyright 2013 Mega-Mario

    This file is part of lolSnes.

    lolSnes is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    lolSnes is distributed in the hope that it will be useful, but WITHOUT ANY 
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along 
    with lolSnes. If not, see http://www.gnu.org/licenses/.
*/

#include <nds.h>

#include "spc700.h"


IPCStruct* IPC = 0;


void vblank()
{
	if (!IPC) return;
	
	IPC->Input_XY = *(vu8*)0x04000136;
}


int main() 
{
	readUserSettings();

	irqInit();
	fifoInit();

	installSystemFIFO();
	
	irqEnable(IRQ_VBLANK);
	irqSet(IRQ_VBLANK, vblank);
	
	// wait till the ARM9 has mapped VRAM for us
	for (;;)
	{
		u8 vrammap = *(vu8*)0x04000240;
		if (vrammap & 0x01) break;
	}
	
	// then proceed to copy shit
	int i;
	u32* src = (u32*)&SPC_ROM[0];
	u32* dst = (u32*)0x0600FFC0;
	for (i = 0; i < 64; i += 4)
		*dst++ = *src++;
	
	// set timer 0 to run at ~2000Hz
	// (16 samples are mixed each time)
	irqEnable(IRQ_TIMER0);
	*(vu16*)0x04000100 = 0xBEA0;
	*(vu16*)0x04000102 = 0x00C0;
	irqSet(IRQ_TIMER0, DSP_Mix);

	for (;;)
	{
		u32 val = fifoGetValue32(FIFO_USER_01);
		if (val)
		{
			switch (val)
			{
				case 1:
					SPC_Reset();
					break;
				
				case 2:
					swiWaitForVBlank();
					SPC_Run();
					break;
					
				case 3:
					while (!fifoCheckAddress(FIFO_USER_01));
					IPC = fifoGetAddress(FIFO_USER_01);
					break;
			}
		}
		
		swiWaitForVBlank();
	}

	return 0;
}
