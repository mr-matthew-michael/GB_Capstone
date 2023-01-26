#include <stdio.h>
#include <stdlib.h>
#include "./include/gb_cpu_h.hpp"
#include "./include/mmu_h.hpp"
#include "./include/cartridge.h"
#include "./include/util.hpp"

int timer_clocksum = 0;
int div_clocksum = 0;
int cyc = 0;
bool unpaused = true;


void handleTimer(mmu_t *mmu, int cycle) 
{
    //	set divider
	div_clocksum += cycle;
	
	if (div_clocksum >= 256) 
	{
		div_clocksum -= 256;
		aluToMem(mmu, 0xff04, 1);
	}
	
	//	check if timer is on
	if ((mmu_read_byte(mmu, 0xff07) >> 2) & 0x1) {
		///printf("one: %0x \n", mmu_read_byte(mmu, 0xff07) >> 2);
		//	increase helper counter
		timer_clocksum += cycle * 4;

		//	set frequency
		int freq = 4096;				//	Hz
		if ((mmu_read_byte(mmu, 0xff07) & 3) == 1)		//	mask last 2 bits
			freq = 262144;
		else if ((mmu_read_byte(mmu, 0xff07) & 3) == 2)	//	mask last 2 bits
			freq = 65536;
		else if ((mmu_read_byte(mmu, 0xff07) & 3) == 3)	//	mask last 2 bits
			freq = 16384;

		//	increment the timer according to the frequency (synched to the processed opcodes)
		while (timer_clocksum >= (4194304 / freq)) {
			//	increase TIMA
			//printf("read from 0xff05: %0x\n", mmu_read_byte(mmu, 0xff05));
			aluToMem(mmu, 0xff05, 1);
			//	check TIMA for overflow
			if (mmu_read_byte(mmu, 0xff05) == 0x00) {
				//	set timer interrupt request
				mmu_write_byte(mmu, 0xff0f, mmu_read_byte(mmu, 0xff0f) | 4);
				//	reset timer to timer modulo
				mmu_write_byte(mmu, 0xff05, mmu_read_byte(mmu, 0xff06));
			}
			timer_clocksum -= (4194304 / freq);
		}
	}
}


void handleInterrupts(mmu_t *mmu, cpu_t *cpu) 
{
	//	unHALT the system, once we have any interrupt
	if (mmu_read_byte(mmu, 0xffff) & mmu_read_byte(mmu, 0xff0f) && cpu->halt) {
		cpu->halt = 0;
	}

	//	handle interrupts
	if (cpu->ime) {
		//	some interrupt is enabled and allowed
		if (mmu_read_byte(mmu, 0xffff) & mmu_read_byte(mmu, 0xff0f)) {
			//	handle interrupts by priority (starting at bit 0 - vblank)
			
			//	v-blank interrupt
			if ((mmu_read_byte(mmu, 0xffff) & 1) & (mmu_read_byte(mmu, 0xff0f) & 1)) 
			{
				cpu->reg.sp--;
				mmu_write_byte(mmu, cpu->reg.sp, cpu->reg.pc >> 8);
				cpu->reg.sp--;
				mmu_write_byte(mmu, cpu->reg.sp, cpu->reg.pc & 0xff);
				cpu->reg.pc = 0x40;
				mmu_write_byte(mmu,0xff0f, mmu_read_byte(mmu, 0xff0f) & ~1);
			}

			//	lcd stat interrupt
			if ((mmu_read_byte(mmu, 0xffff) & 2) & (mmu_read_byte(mmu, 0xff0f) & 2)) 
			{
				cpu->reg.sp--;
				mmu_write_byte(mmu, cpu->reg.sp, cpu->reg.pc >> 8);
				cpu->reg.sp--;
				mmu_write_byte(mmu, cpu->reg.sp, cpu->reg.pc & 0xff);
				cpu->reg.pc = 0x48;
				mmu_write_byte(mmu,0xff0f, mmu_read_byte(mmu, 0xff0f) & ~2);
			}

			//	lcd stat interrupt
			if ((mmu_read_byte(mmu, 0xffff) & 4) & (mmu_read_byte(mmu, 0xff0f) & 4)) 
			{
				cpu->reg.sp--;
				mmu_write_byte(mmu, cpu->reg.sp, cpu->reg.pc >> 8);
				cpu->reg.sp--;
				mmu_write_byte(mmu, cpu->reg.sp, cpu->reg.pc & 0xff);
				cpu->reg.pc = 0x50;
				mmu_write_byte(mmu,0xff0f, mmu_read_byte(mmu, 0xff0f) & ~4);
			}

			//	clear main interrupts_enable and corresponding flag
			cpu->ime = false;
		}
	}
}

int main()
{
    cpu_t *cpu;
    mmu_t* mmu = mmu_create();
	//mmu_load_bios(mmu);
    //09-op r, r.gb
    cartridge_t* cart = cartridge_load("/Users/Matt1/Desktop/UNCA_SPRING_2023/CSCI_481/Gameboy_Emulator/ROMS/gb-test-roms-master/instr_timing/instr_timing.gb");
    mmu_load_rom(mmu, cart);
    //mmu_write_byte(mmu, 0xff44, 0);
    cartridge_free(cart);
 
    cpu->reg.pc = 0x0101;
    cpu->reg.sp = 0x0000;
	
    cpu->reg.a = 0x0;
    cpu->reg.f = 0x0;
    cpu->reg.c = 0x0;
    cpu->reg.e = 0x0;
    cpu->reg.h = 0x0;
    cpu->reg.l = 0x0;
    cpu->reg.sp = 0x0;
	cpu->halt = 0x00;
	

    //	start CPU
while (1) {

	if (unpaused) 
	{
		//	step cpu if not halted
		if (!cpu->halt){
			cyc = stepCPU(cpu, mmu);
			printf("m val: %d ", cyc);
			printDebug(cpu, mmu);
		}
		//	if system is halted just idle, but still commence timers and condition for while loop
		else
			cyc = 1;

		//	handle timer
		handleTimer(mmu, cyc);

		//	handle interrupts
		//handleInterrupts(mmu, cpu);

	}
}
    return 0;
}