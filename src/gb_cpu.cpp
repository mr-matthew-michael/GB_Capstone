#include <stdio.h>
#include <stdlib.h>
#include "./include/gb_cpu_h.hpp"
#include "./include/mmu_h.hpp"
#include "./include/cartridge.h"
#include "./include/util.hpp"

#define DEBUG__

void cpu_flag_set_zero(cpu_t* cpu, const bool value)
{
    if (value)
        cpu->reg.f |= (0x01 << CPU_FLAG_ZERO_BIT);
    else
        cpu->reg.f &= ~(0x01 << CPU_FLAG_ZERO_BIT);
}

void cpu_flag_set_sub(cpu_t* cpu, const bool value)
{
    if (value)
        cpu->reg.f |= (0x01 << CPU_FLAG_SUB_BIT);
    else
        cpu->reg.f &= ~(0x01 << CPU_FLAG_SUB_BIT);
}
 
void cpu_flag_set_halfcarry(cpu_t* cpu, const bool value)
{
    if (value)
        cpu->reg.f |= (0x01 << CPU_FLAG_HC_BIT);
    else
        cpu->reg.f &= ~(0x01 << CPU_FLAG_HC_BIT);
}

void cpu_flag_set_carry(cpu_t* cpu, const bool value)
{
    if (value)
        cpu->reg.f |= (0x01 << CPU_FLAG_CARRY_BIT);
    else
        cpu->reg.f &= ~(0x01 << CPU_FLAG_CARRY_BIT);
}

bool cpu_flag(cpu_t* cpu, const uint8_t flag)
{
    return (cpu->reg.f & (1 << flag)) != 0;
}

bool cpu_check_condition(cpu_t* cpu, condition_e condition)
{
    bool c = false;
    switch (condition)
    {
        case CPU_CONDITION_C:
            c = cpu_flag(cpu, CPU_FLAG_CARRY_BIT);
            break;
        case CPU_CONDITION_NC:
            c = !cpu_flag(cpu, CPU_FLAG_CARRY_BIT);
            break;
        case CPU_CONDITION_Z:
            c = cpu_flag(cpu, CPU_FLAG_ZERO_BIT);
            break;
        case CPU_CONDITION_NZ:
            c = !cpu_flag(cpu, CPU_FLAG_ZERO_BIT);
            break;
        case CPU_CONDITION_ALWAYS:
            c = true;
            break;
    }

    return c;
}

uint16_t reg16(uint8_t reg_hi, uint8_t reg_lo)
{
    return (reg_hi<<8) | reg_lo;
}

void set_reg(uint16_t reg, uint8_t &reg_hi, uint8_t &reg_lo)
{
    reg_hi = (reg >> 8) & 0xff;
    reg_lo = (reg) & 0xff;
}

void print_registers (cpu_t* cpu)
{
    printf("af: $%04x, bc: $%04x, de: $%04x, hl: $%04x, sp: $%04x, pc: $%04x\n", reg16(cpu->reg.a, cpu->reg.f), reg16(cpu->reg.b, cpu->reg.c), 
            reg16(cpu->reg.d, cpu->reg.e), reg16(cpu->reg.h, cpu->reg.l), cpu->reg.sp, (cpu->reg.pc));
}

void printDebug(cpu_t* cpu, mmu_t *mmu) 
{
	printf(
		"PC: %04X, AF: %04X, BC: %04X, DE: %04X, HL: %04X, SP: %04X, STAT: %04X",
		cpu->reg.pc,
		(cpu->reg.a << 8)  | (cpu->reg.f),
		(cpu->reg.b << 8)  | cpu->reg.c,
		(cpu->reg.d << 8)  | cpu->reg.e,
		(cpu->reg.h << 8)  | cpu->reg.l,
		cpu->reg.sp,
		mmu_read_byte(mmu, 0xdff9)
	);

    printf("\t(%02X %02X %02X)\n",
		mmu_read_byte(mmu, cpu->reg.pc),
		mmu_read_byte(mmu, cpu->reg.pc + 1),
		mmu_read_byte(mmu, cpu->reg.pc + 2)
        //mu_read_byte(mmu, (cpu->reg.h << 8)  | cpu->reg.l)
	);
}

//GMB 8bit-Loadcommands ******************************************************************************************************************************************

int ldd_mem_HL_r(cpu_t *cpu, mmu_t *mmu, uint16_t reg_rr, uint8_t reg_r, char reg[4], char reg2[2], int m = 2, int step = 1) 
{
    uint16_t reg16_hl = reg16(cpu->reg.h, cpu->reg.l);

    #ifdef  DEBUG
        printf("LD %s, %s, 		; $%04x\n", reg, reg2, mmu_read_byte(mmu, reg16_hl));
    #endif

    mmu_write_byte(mmu, reg16_hl, cpu->reg.a);
    
    cpu->reg.pc += step;
    reg16_hl--;
    set_reg(reg16_hl, cpu->reg.h, cpu->reg.l);

    return m;
}
//ld(xy), z
int ld_mem_r(cpu_t *cpu, mmu_t *mmu, uint16_t reg_rr, uint8_t &reg_r, char reg[9], char reg2[2], int m = 2, int step = 1) 
{

    #ifdef  DEBUG
        printf("LD %s, %s, 		; (memory) $%04x\n", reg, reg2, reg_rr);
    #endif

    mmu_write_byte(mmu, reg_rr, reg_r);
    
    cpu->reg.pc += step;

    return m;
}

//ld (xy), u8
int ld_mem_u8(cpu_t *cpu, mmu_t *mmu, uint16_t reg_rr, uint8_t reg_r, char reg[9], char reg2[2], int m = 3, int step = 2) 
{

    #ifdef DEBUG
        printf("LD %s, %s, 		; (memory) $%04x\n", reg, reg2, reg_rr);
    #endif

    mmu_write_byte(mmu, reg_rr, reg_r);
    
    cpu->reg.pc += step;

    return m;
}

int ld_r_u8(cpu_t *cpu, mmu_t *mmu, uint8_t &reg8, uint8_t u8, char reg[4], int m = 2, int step = 2)
{
    #ifdef  DEBUG
        printf("LD %s, $%02x, 		;\n", reg, u8);
    #endif

    reg8 = u8;
    cpu->reg.pc+=step;

    return m;
}

int ld_r_r(cpu_t *cpu, mmu_t *mmu, uint8_t &reg8, uint8_t reg8_2, char reg[4], char reg2[4], int m = 1, int step = 1)
{
    #ifdef  DEBUG
        printf("LD %s, %s 		;\n", reg, reg2);
    #endif

    reg8 = reg8_2;
    cpu->reg.pc+=step;

    return m;
}

int ld_a_mem(cpu_t *cpu, mmu_t *mmu, uint8_t &reg8, uint16_t addr, char reg[4], char reg2[4], int m = 2, int step = 1)
{
    #ifdef  DEBUG
        printf("LD %s, %04x		;\n", reg, addr);
    #endif
    //printf("\n *in mem* addr8 pc+: %02x\n", mmu_read_byte(mmu, addr));
    cpu->reg.a = mmu_read_byte(mmu, addr);
    cpu->reg.pc+=step;

    return m;
}


//GMB 16bit-Loadcommands ***************************************************************************************************************************************
int ld_sp_nn(cpu_t* cpu, uint16_t &reg_sp, uint16_t nn, char reg[1], int m = 3, int step = 3) 
{
    #ifdef  DEBUG
        printf("LD %s, %04x			; $%04x\n", reg, nn, cpu->reg.pc);
    #endif

    reg_sp = nn;
    cpu->reg.pc += step;

	return m;
}

int ld_rr_nn(cpu_t* cpu, uint8_t &reg_hi, uint8_t &reg_lo, uint16_t nn, char reg[1], int m = 3, int step = 3) 
{
    #ifdef  DEBUG
        printf("LD %s, %04x			; $%04x\n", reg, nn, cpu->reg.pc);
    #endif

    set_reg(nn, reg_hi, reg_lo);

    cpu->reg.pc += step;
	return m;
}

int ld_hl_sp(cpu_t* cpu, mmu_t* mmu, uint16_t &reg, uint8_t val, int m = 3, int step = 2)
{
    #ifdef  DEBUG
        printf("LD HL, SP + i8			; $%04x\n", cpu->reg.pc);
    #endif

    cpu_flag_set_halfcarry(cpu, ((reg & 0xf) + (val & 0xf)) > 0xf);
    cpu_flag_set_carry(cpu, ((reg & 0xff) + val) > 0xff);

    cpu->reg.h = (reg + ((int8_t) val)) >> 8;
    cpu->reg.l = (reg + ((int8_t)val)) & 0xff;

    cpu_flag_set_zero(cpu, 0);
    cpu_flag_set_sub(cpu, 0);

    cpu->reg.pc += step;

    return m;
}

int push_rr(cpu_t* cpu, mmu_t* mmu, uint16_t rr, char reg[4], int m = 4, int step = 1) 
{
    #ifdef  DEBUG
        printf("Push %s,			; $%04x\n", reg, cpu->reg.pc);
    #endif

    // SP=SP-2 (SP)=rr 
    cpu->reg.sp -= 2;
    mmu_write_word(mmu, cpu->reg.sp, rr);
    //printf("\nmemory at sp $%04x,			; \n", mmu_read_word(mmu, cpu->reg.sp));
    cpu->reg.pc += step;
	return m;
}

int pop_rr(cpu_t* cpu, mmu_t* mmu, uint16_t rr, uint8_t &reg_hi, uint8_t &reg_lo, char reg[4], int m = 3, int step = 1) 
{
    #ifdef  DEBUG
        printf("POP %s,			; $%04x\n", reg, mmu_read_word(mmu, rr));
    #endif

    //rr=(SP) SP=SP+2
    reg_lo = mmu_read_byte(mmu, cpu->reg.sp);
    cpu->reg.sp++;
    reg_hi = mmu_read_byte(mmu, cpu->reg.sp);
    cpu->reg.sp++;

    cpu->reg.pc += step;
	return m;
}

int add_16_hl(cpu_t* cpu, mmu_t* mmu, uint16_t val, char cpu_reg[4], int m = 2, int step = 1)
{
    #ifdef  DEBUG
        printf("ADD HL, %s,			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    uint16_t double_reg = reg16(cpu->reg.h, cpu->reg.l);
    cpu_flag_set_sub(cpu, 0);
    uint16_t ans = val + double_reg;
    cpu_flag_set_carry(cpu, (double_reg + val) > 0xffff);
    cpu_flag_set_halfcarry(cpu, ((double_reg & 0x0FFF) + (val & 0x0FFF)) > 0x0fff);

    cpu->reg.h = (val + double_reg) >> 8;
    cpu->reg.l = (val + double_reg) & 0xff;
    cpu->reg.pc += step;

    return m;
}

int dec_16(cpu_t* cpu, mmu_t* mmu, uint8_t &reg_hi, uint8_t &reg_lo, char cpu_reg[4], int m = 2, int step = 1)
{
    #ifdef  DEBUG
        printf("DEC, %s,			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    uint16_t double_reg = reg16(reg_hi, reg_lo);
    double_reg--;
    reg_hi = double_reg >> 8 & 0xff;
    reg_lo = double_reg & 0xff;

    cpu->reg.pc += step;

    return m;
}

//GMB 8bit-Arithmetic/logical Commands ***************************************************************************************************************************************

int inc_r (cpu_t* cpu, uint8_t &reg, char cpu_reg[2], int m = 1, int step = 1)
{
    #ifdef  DEBUG
        printf("inc %s,			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    cpu_flag_set_sub(cpu, 0);
    cpu_flag_set_halfcarry(cpu, ((reg & 0xf) == 0xf));
    reg++;
    cpu_flag_set_zero(cpu, (reg==0));

    cpu->reg.pc += step;
    return m;
}

int dec_r (cpu_t* cpu, uint8_t &reg, char cpu_reg[2], int m = 1, int step = 1)
{

    #ifdef  DEBUG
        printf("dec %s,			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    cpu_flag_set_sub(cpu, 1);
    cpu_flag_set_halfcarry(cpu, ((reg & 0xf) == 0x0) ? 1 : 0);
    reg--;
    cpu_flag_set_zero(cpu, (reg==0));

    cpu->reg.pc += step;
    return m;
}

int inc_r_mem (cpu_t* cpu, mmu_t* mmu,  uint8_t reg, char cpu_reg[4], int m = 3, int step = 1)
{

    #ifdef  DEBUG
        printf("inc %s,			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    cpu_flag_set_sub(cpu, 0);
    cpu_flag_set_halfcarry(cpu, ((reg & 0xf) == 0xf));
    reg++;
    cpu_flag_set_zero(cpu, (reg==0));

    mmu_write_byte(mmu, reg16(cpu->reg.h, cpu->reg.l),reg);
    cpu->reg.pc += step;
    return m;
}

int dec_r_mem (cpu_t* cpu, mmu_t* mmu, uint8_t reg, char cpu_reg[4], int m = 3, int step = 1)
{

    #ifdef  DEBUG
        printf("dec %s,			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    cpu_flag_set_sub(cpu, 1);
    cpu_flag_set_halfcarry(cpu, ((reg & 0xf) == 0xf));
    reg--;
    cpu_flag_set_zero(cpu, (reg==0));

    cpu->reg.pc += step;
    return m;
}

int cp_r(cpu_t* cpu, uint8_t val, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef  DEBUG
        printf("CP %s,			; $%04x\n", cpu_reg, val);
    #endif

    cpu_flag_set_sub(cpu, true);
    cpu_flag_set_zero(cpu, cpu->reg.a == val);
    cpu_flag_set_halfcarry(cpu, ((cpu->reg.a & 0xf) - (val & 0xf)) < 0);
    cpu_flag_set_carry(cpu, cpu->reg.a < val);

    cpu->reg.pc += step;
    return m;
}

int or_r(cpu_t* cpu, uint8_t val, char cpu_reg[4], int m = 1, int step = 1)
{
    #ifdef  DEBUG
        printf("OR A, %s,			\n", cpu_reg);
    #endif
    cpu_flag_set_carry(cpu, false);
    cpu_flag_set_halfcarry(cpu, false);
    cpu_flag_set_sub(cpu, false);

    cpu->reg.a |= val;

    cpu_flag_set_zero(cpu, !cpu->reg.a);
    cpu->reg.pc += step;
    return m;
}

int add_r(cpu_t* cpu, mmu_t* mmu, uint8_t &reg, uint8_t val, char cpu_reg[4], int m = 1, int step = 1)
{
    #ifdef  DEBUG
        printf("ADD A, %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    cpu_flag_set_sub(cpu, false);

    uint8_t ans = reg + val;
    cpu_flag_set_zero(cpu, ans == 0);
    cpu_flag_set_carry(cpu, (reg + val) > 0xff);
    cpu_flag_set_halfcarry(cpu, ((reg & 0xf) + (val & 0xf)) > 0xf);

    reg = ans; 

    cpu->reg.pc += step;

    return m;
}

int adc_r(cpu_t* cpu, mmu_t* mmu, uint8_t &reg, uint8_t val, char cpu_reg[4], int m = 1, int step = 1)
{
    #ifdef  DEBUG
        printf("ADC A, %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    cpu_flag_set_sub(cpu, false);
    cpu_flag_set_halfcarry(cpu, (((val & 0xf) + (reg & 0xf) + cpu_flag(cpu, CPU_FLAG_CARRY_BIT)) > 0xf));
    int oldcarry = ((val + reg + cpu_flag(cpu, CPU_FLAG_CARRY_BIT)) > 0xff);
    reg += val + cpu_flag(cpu, CPU_FLAG_CARRY_BIT);
    cpu_flag_set_carry(cpu, oldcarry);
    cpu_flag_set_zero(cpu, reg==0);
    cpu->reg.pc += step;

    return m;
}

int sub_r(cpu_t* cpu, mmu_t* mmu, uint8_t reg, char cpu_reg[4], int m = 1, int step = 1)
{
    #ifdef  DEBUG
        printf("SUB A, %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    cpu_flag_set_carry(cpu, reg > cpu->reg.a);
	cpu_flag_set_halfcarry(cpu, (reg & 0xf) > (cpu->reg.a & 0xf)); 
	cpu->reg.a = cpu->reg.a - reg;
	cpu_flag_set_zero(cpu, cpu->reg.a == 0);
	cpu_flag_set_sub(cpu, true);
	cpu->reg.pc += step;

    return m;
}

int sbc_r(cpu_t* cpu, mmu_t* mmu, uint8_t reg, char cpu_reg[4], int m = 1, int step = 1)
{
    #ifdef  DEBUG
        printf("SBC A, %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    int newcf = (reg + cpu_flag(cpu, CPU_FLAG_CARRY_BIT)) > cpu->reg.a;
	cpu_flag_set_halfcarry(cpu, ((reg & 0xf) + cpu_flag(cpu, CPU_FLAG_CARRY_BIT)) > (cpu->reg.a & 0xf));
	cpu->reg.a = cpu->reg.a - (reg + cpu_flag(cpu, CPU_FLAG_CARRY_BIT));
	cpu_flag_set_carry(cpu, newcf);
	cpu_flag_set_zero(cpu, cpu->reg.a == 0);
	cpu_flag_set_sub(cpu, true);
	
	cpu->reg.pc += step;

    return m;
}

int and_r(cpu_t* cpu, mmu_t* mmu, uint8_t reg, char cpu_reg[4], int m = 1, int step = 1)
{
    #ifdef  DEBUG
        printf("AND A, %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif
    cpu_flag_set_carry(cpu, false);
    cpu_flag_set_halfcarry(cpu, true);
    cpu_flag_set_sub(cpu, false);

	cpu->reg.a &= reg;

	cpu_flag_set_zero(cpu, cpu->reg.a == 0);
	cpu->reg.pc += step;

    return m;
}

int xor_r(cpu_t* cpu, uint8_t reg, char cpu_reg[2], int m = 1, int step = 1) 
{
    #ifdef  DEBUG
        printf("XOR %s, %04x			; $%04x\n", cpu_reg, cpu->reg.f, cpu->reg.pc);
    #endif

    cpu_flag_set_carry(cpu, false);
    cpu_flag_set_halfcarry(cpu, false);
    cpu_flag_set_sub(cpu, false);

    cpu->reg.a ^= reg;

    cpu_flag_set_zero(cpu, !cpu->reg.a);
    cpu->reg.pc += step;

    return m;
}

/***check**/
int daa(cpu_t* cpu, mmu_t* mmu, int m = 1, int step = 1)
{
    #ifdef  DEBUG
        printf("DAA,			; $%04x\n", cpu->reg.pc);
    #endif

    
    if(!cpu_flag(cpu, CPU_FLAG_SUB_BIT))
    {
        if (cpu_flag(cpu, CPU_FLAG_CARRY_BIT) || cpu->reg.a > 0x99)
        {
            cpu->reg.a += 0x60;
            cpu_flag_set_carry(cpu, 1);
        }
        if (cpu_flag(cpu, CPU_FLAG_HC_BIT)|| (cpu->reg.a & 0x0f) > 0x09)
            cpu->reg.a += 0x06;
    }
    else
    {
        if (cpu_flag(cpu, CPU_FLAG_CARRY_BIT))
            cpu->reg.a -= 0x60;
        if (cpu_flag(cpu, CPU_FLAG_HC_BIT))
            cpu->reg.a -= 0x06;
    }
    cpu_flag_set_halfcarry(cpu, 0);
    cpu_flag_set_zero(cpu, cpu->reg.a == 0);
    cpu->reg.pc +=step;
    return m;
}

int cpl(cpu_t* cpu, mmu_t* mmu, int m = 1, int step = 1)
{
    #ifdef  DEBUG
        printf("CPL,			; $%04x\n", cpu->reg.pc);
    #endif
    
    cpu->reg.a = ~cpu->reg.a;
	cpu_flag_set_sub(cpu, 1);
    cpu_flag_set_halfcarry(cpu, 1);
	cpu->reg.pc += step;

	return m;

}

int add_sp(cpu_t* cpu, mmu_t* mmu, uint16_t &reg, uint8_t val, int m = 4, int step = 2)
{
    #ifdef  DEBUG
        printf("ADD SP, i8			; $%04x\n", cpu->reg.pc);
    #endif

    cpu_flag_set_zero(cpu, 0);
    cpu_flag_set_sub(cpu, 0);

    cpu_flag_set_halfcarry(cpu, ((reg & 0xf) + (val & 0xf)) > 0xf);
    cpu_flag_set_carry(cpu, ((reg & 0xff) + val) > 0xff);

    reg += (int8_t) val; 

    cpu->reg.pc += step;

    return m;
}

//16-bit Arithmetic Logic Unit ***************************************************************************************************************************************
int inc_rr (cpu_t* cpu, uint8_t &reg_hi, uint8_t &reg_lo, char cpu_reg[2], int m = 2, int step = 1)
{

    #ifdef  DEBUG
        printf("inc %s,			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    uint16_t reg_16 = reg16(reg_hi, reg_lo);
    reg_16++;
    set_reg(reg_16, reg_hi, reg_lo);

    cpu->reg.pc += step;
    return m;
}

int inc_sp (cpu_t* cpu, uint16_t &sp_reg, char cpu_reg[2], int m = 2, int step = 1)
{

    #ifdef  DEBUG
        printf("inc %s,			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    sp_reg++;

    cpu->reg.pc += step;
    return m;
}

//Rotate and Shift instructions ***************************************************************************************************************************************

int rlc_r(cpu_t* cpu, mmu_t* mmu, uint8_t &reg, char cpu_reg[2], int m = 1, int step = 1)
{
    #ifdef  DEBUG
        printf("RLC %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

        cpu_flag_set_carry(cpu, (reg>>7) & 0x01);
        cpu_flag_set_sub(cpu, false);
        cpu_flag_set_halfcarry(cpu, false);

        reg =  (reg << 1) | cpu_flag(cpu, CPU_FLAG_CARRY_BIT);

        cpu_flag_set_zero(cpu, 0);

        cpu->reg.pc += step;
        return m;
}

int rl_r(cpu_t* cpu, mmu_t* mmu, uint8_t &reg, char cpu_reg[2], int m = 2, int step = 2)
{
    #ifdef  DEBUG
        printf("RL %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    uint8_t carry_bit = cpu_flag(cpu, CPU_FLAG_CARRY_BIT);
    cpu_flag_set_carry(cpu, (reg >> 7) & 0x01);

    uint8_t res = (reg<<1)|carry_bit;

    cpu_flag_set_zero(cpu, 0);

    reg = res;  

    cpu_flag_set_sub(cpu, false);
    cpu_flag_set_halfcarry(cpu, false);

    cpu->reg.pc += step;
    return m;
}

int rrc_r(cpu_t* cpu, mmu_t* mmu, int m = 1, int step = 1)
{
    #ifdef  DEBUG
        printf("RRCA		; $%04x\n", cpu->reg.pc);
    #endif

    cpu_flag_set_carry(cpu, cpu->reg.a & 0x01);
    cpu_flag_set_halfcarry(cpu, 0);
    cpu_flag_set_sub(cpu, 0);
    cpu_flag_set_zero(cpu, 0);

    cpu->reg.a = (cpu->reg.a >> 1) | (cpu_flag(cpu, CPU_FLAG_CARRY_BIT) << 7);

    cpu->reg.pc += step;
    return m;
}

int rr_r(cpu_t* cpu, mmu_t* mmu, int m = 1, int step = 1)
{
    #ifdef  DEBUG
        printf("RRA		; $%04x\n", cpu->reg.pc);
    #endif

    int og_carry = cpu_flag(cpu, CPU_FLAG_CARRY_BIT);
    cpu_flag_set_carry(cpu, cpu->reg.a & 0x01);
    cpu_flag_set_halfcarry(cpu, 0);
    cpu_flag_set_sub(cpu, 0);
    cpu_flag_set_zero(cpu, 0);

    cpu->reg.a = (cpu->reg.a >> 1) | (og_carry << 7);

    cpu->reg.pc += step;
    return m;
}

//GMB Singlebit Operation Commands ***************************************************************************************************************************************
int bit_n_r(cpu_t* cpu, int shift_num, uint8_t reg, char cpu_reg[2], int m = 2, int step = 2) 
{
    #ifdef  DEBUG
        printf("BIT %d, %s			; $%04x\n",shift_num, cpu_reg, cpu->reg.pc);
    #endif

    cpu_flag_set_zero(cpu, util_check_bit(reg, shift_num) == 0);
    cpu_flag_set_sub(cpu, false);
    cpu_flag_set_halfcarry(cpu, true);

    cpu->reg.pc += step;

	return m;
}

//GMB Jumpcommands ***************************************************************************************************************************************

//jr   f,PC+dd
int jr_nz_nn (cpu_t* cpu, int8_t nn, condition_e c, int m = 2, int step = 2) 
{
    #ifdef  DEBUG
        printf("Jr nz, 			; $%04x\n", cpu->reg.pc);
    #endif

    if (cpu_check_condition(cpu, c))
    {
        cpu->reg.pc += nn;
        m = 3;
    }

    cpu->reg.pc += step;
    return m;
}

int jr_z_nn (cpu_t* cpu, int8_t nn, condition_e c, int m = 2, int step = 2) 
{
    #ifdef  DEBUG
        printf("Jr z, 			; $%04x\n", cpu->reg.pc);
    #endif

    if (cpu_check_condition(cpu, c))
    {
        cpu->reg.pc += nn;
        m = 3;
    }

    cpu->reg.pc += step;
    return m;
}

int jr_nc_nn (cpu_t* cpu, int8_t nn, condition_e c, int m = 2, int step = 2) 
{
    #ifdef  DEBUG
        printf("Jr nc, 			; $%04x\n", cpu->reg.pc);
    #endif

    if (cpu_check_condition(cpu, c))
    {
        cpu->reg.pc += nn;
        m = 3;
    }

    cpu->reg.pc += step;
    return m;
}

int jr_c_nn (cpu_t* cpu, int8_t nn, condition_e c, int m = 2, int step = 2) 
{
    #ifdef  DEBUG
        printf("Jr C, 			; $%04x\n", cpu->reg.pc);
    #endif

    if (cpu_check_condition(cpu, c))
    {
        cpu->reg.pc += nn;
        m = 3;
    }

    cpu->reg.pc += step;
    return m;
}

int jr_nn (cpu_t* cpu, int8_t nn, condition_e c, int m = 2, int step = 2) 
{
    #ifdef  DEBUG
        printf("Jr i8, 			; $%04x\n", cpu->reg.pc);
    #endif

    cpu->reg.pc += nn;
    m = 3;

    cpu->reg.pc += step;
    return m;
}

int call_nn (cpu_t* cpu, mmu_t* mmu, uint16_t u16, int m = 6, int step = 3)
{
    #ifdef  DEBUG
        printf("Call, $%04x 			; $%04x\n", u16, cpu->reg.pc);
    #endif

    //SP=SP-2, (SP)=PC, PC=nn
    cpu->reg.sp--;
    mmu_write_byte(mmu, cpu->reg.sp, (cpu->reg.pc + 3) >> 8);
    cpu->reg.sp--;
    mmu_write_byte(mmu, cpu->reg.sp, (cpu->reg.pc + 3) & 0xff);
    cpu->reg.pc = ((mmu_read_byte(mmu, cpu->reg.pc + 2) << 8) | mmu_read_byte(mmu, cpu->reg.pc + 1));

    //cpu->reg.pc += step;
    return m;
}

int call_nz_nn (cpu_t* cpu, mmu_t* mmu, uint16_t u16,  int m = 6, int step = 3)
{
    #ifdef  DEBUG
        printf("Call NZ, $%04x 			; $%04x\n", u16, cpu->reg.pc);
    #endif

    m = 0;

    if(cpu_check_condition(cpu, CPU_CONDITION_NZ))
    { 
        cpu->reg.sp--;
        mmu_write_byte(mmu, cpu->reg.sp, (cpu->reg.pc + 3) >> 8);
        cpu->reg.sp--;
        mmu_write_byte(mmu, cpu->reg.sp, (cpu->reg.pc + 3) & 0xff);
        cpu->reg.pc = ((mmu_read_byte(mmu, cpu->reg.pc + 2) << 8) | mmu_read_byte(mmu, cpu->reg.pc + 1));
        m = 6;
    }
    else
    {
        cpu->reg.pc += step;
        m = 3;
    }

    return m;
}

int call_nc_nn (cpu_t* cpu, mmu_t* mmu, uint16_t u16,  int m = 6, int step = 3)
{
    #ifdef  DEBUG
        printf("Call NC, $%04x 			; $%04x\n", u16, cpu->reg.pc);
    #endif

    m = 0;

    if(cpu_check_condition(cpu, CPU_CONDITION_NC))
    { 
        cpu->reg.sp--;
        mmu_write_byte(mmu, cpu->reg.sp, (cpu->reg.pc + 3) >> 8);
        cpu->reg.sp--;
        mmu_write_byte(mmu, cpu->reg.sp, (cpu->reg.pc + 3) & 0xff);
        cpu->reg.pc = ((mmu_read_byte(mmu, cpu->reg.pc + 2) << 8) | mmu_read_byte(mmu, cpu->reg.pc + 1));
        m = 6;
    }
    else
    {
        cpu->reg.pc += step;
        m = 3;
    }

    return m;
}

int call_c_nn (cpu_t* cpu, mmu_t* mmu, uint16_t u16,  int m = 6, int step = 3)
{
    #ifdef  DEBUG
        printf("Call C, $%04x 			; $%04x\n", u16, cpu->reg.pc);
    #endif

    m = 0;

    if(cpu_check_condition(cpu, CPU_CONDITION_C))
    { 
        cpu->reg.sp--;
        mmu_write_byte(mmu, cpu->reg.sp, (cpu->reg.pc + 3) >> 8);
        cpu->reg.sp--;
        mmu_write_byte(mmu, cpu->reg.sp, (cpu->reg.pc + 3) & 0xff);
        cpu->reg.pc = ((mmu_read_byte(mmu, cpu->reg.pc + 2) << 8) | mmu_read_byte(mmu, cpu->reg.pc + 1));
        m = 6;
    }
    else
    {
        cpu->reg.pc += step;
        m = 3;
    }

    return m;
}

int call_z_nn (cpu_t* cpu, mmu_t* mmu, uint16_t u16,  int m = 6, int step = 3)
{
    #ifdef  DEBUG
        printf("Call Z, $%04x 			; $%04x\n", u16, cpu->reg.pc);
    #endif

    m = 0;

    if(cpu_check_condition(cpu, CPU_CONDITION_Z))
    { 
        cpu->reg.sp--;
        mmu_write_byte(mmu, cpu->reg.sp, (cpu->reg.pc + 3) >> 8);
        cpu->reg.sp--;
        mmu_write_byte(mmu, cpu->reg.sp, (cpu->reg.pc + 3) & 0xff);
        cpu->reg.pc = ((mmu_read_byte(mmu, cpu->reg.pc + 2) << 8) | mmu_read_byte(mmu, cpu->reg.pc + 1));

        m = 6;
    }
    else
    {
        cpu->reg.pc += step;
        m = 3;
    }

    return m;
}

//PC=(SP), SP=SP+2
int ret(cpu_t* cpu, mmu_t* mmu, int m = 4, int step = 1)
{
    #ifdef  DEBUG
        printf("RET 			; $%04x\n", cpu->reg.pc);
    #endif

    cpu->reg.pc = (mmu_read_byte(mmu, ((cpu->reg.sp+1))) << 8)| mmu_read_byte(mmu, ((cpu->reg.sp)));
    cpu->reg.sp += 2;
    //cpu->reg.pc += step;
    return m;
}

int ret_nz(cpu_t* cpu, mmu_t* mmu, int m = 4, int step = 1)
{
    #ifdef  DEBUG
        printf("RET NZ			; $%04x\n", cpu->reg.pc);
    #endif

    m = 0;

    if(cpu_check_condition(cpu, CPU_CONDITION_NZ))
    {
        cpu->reg.pc = (mmu_read_byte(mmu, ((cpu->reg.sp+1))) << 8)| mmu_read_byte(mmu, ((cpu->reg.sp)));
        cpu->reg.sp += 2;
        //cpu->reg.pc += step;
        m = 5;
    }
    else
    {
        m = 2;
        cpu->reg.pc += step;
    }

    return m;
}

int ret_nc(cpu_t* cpu, mmu_t* mmu, int m = 4, int step = 1)
{
    #ifdef  DEBUG
        printf("RET NC			; $%04x\n", cpu->reg.pc);
    #endif

    m = 0;

    if(cpu_check_condition(cpu, CPU_CONDITION_NC))
    {
        cpu->reg.pc = (mmu_read_byte(mmu, ((cpu->reg.sp+1))) << 8)| mmu_read_byte(mmu, ((cpu->reg.sp)));
        cpu->reg.sp += 2;
        //cpu->reg.pc += step;
        m = 5;
    }
    else
    {
        m = 2;
        cpu->reg.pc += step;
    }

    return m;
}

int ret_c(cpu_t* cpu, mmu_t* mmu, int m = 4, int step = 1)
{
    #ifdef  DEBUG
        printf("RET C			; $%04x\n", cpu->reg.pc);
    #endif

    m = 0;

    if(cpu_check_condition(cpu, CPU_CONDITION_C))
    {
        cpu->reg.pc = (mmu_read_byte(mmu, ((cpu->reg.sp+1))) << 8)| mmu_read_byte(mmu, ((cpu->reg.sp)));
        cpu->reg.sp += 2;
        //cpu->reg.pc += step;
        m = 5;
    }
    else
    {
        m = 2;
        cpu->reg.pc += step;
    }

    return m;
}

int ret_z(cpu_t* cpu, mmu_t* mmu, int m = 4, int step = 1)
{
    #ifdef  DEBUG
        printf("RET Z			; $%04x\n", cpu->reg.pc);
    #endif

    m = 0;

    if(cpu_check_condition(cpu, CPU_CONDITION_Z))
    {
        cpu->reg.pc = (mmu_read_byte(mmu, ((cpu->reg.sp+1))) << 8)| mmu_read_byte(mmu, ((cpu->reg.sp)));
        cpu->reg.sp += 2;
        //cpu->reg.pc += step;
        m = 5;
    }
    else
    {
        m = 2;
        cpu->reg.pc += step;
    }

    return m;
}

int reti(cpu_t* cpu, mmu_t* mmu, int m = 4, int step = 1)
{
    #ifdef  DEBUG
        printf("RET 			; $%04x\n", cpu->reg.pc);
    #endif

    cpu->reg.pc = (mmu_read_byte(mmu, ((cpu->reg.sp+1))) << 8)| mmu_read_byte(mmu, ((cpu->reg.sp)));
    cpu->reg.sp += 2;
    //cpu->reg.pc += step;
    cpu->ime = 1;

    return m;
}

int jp_nn (cpu_t* cpu, uint16_t nn, int m = 4, int step = 3) 
{
    #ifdef  DEBUG
        printf("Jp nn, 			; $%04x\n", cpu->reg.pc);
    #endif

    cpu->reg.pc = nn;

    //cpu->reg.pc += step;
    return m;
}

int jp_nz_nn (cpu_t* cpu, uint16_t nn, int m = 4, int step = 3) 
{
    #ifdef  DEBUG
        printf("Jp NZ u16, 			; $%04x\n", cpu->reg.pc);
    #endif

    if (cpu_check_condition(cpu, CPU_CONDITION_NZ))
    {
        cpu->reg.pc = nn;
        m = 4;
    } else
    {
        m = 3;
        cpu->reg.pc += step;
    }

    return m;
}

int jp_nc_nn (cpu_t* cpu, uint16_t nn, int m = 4, int step = 3) 
{
    #ifdef  DEBUG
        printf("Jp NC u16, 			; $%04x\n", cpu->reg.pc);
    #endif

    if (cpu_check_condition(cpu, CPU_CONDITION_NC))
    {
        cpu->reg.pc = nn;
        m = 4;
    } else
    {
        m = 3;
        cpu->reg.pc += step;
    }

    return m;
}

int jp_c_nn (cpu_t* cpu, uint16_t nn, int m = 4, int step = 3) 
{
    #ifdef  DEBUG
        printf("Jp C u16, 			; $%04x\n", cpu->reg.pc);
    #endif

    if (cpu_check_condition(cpu, CPU_CONDITION_C))
    {
        cpu->reg.pc = nn;
        m = 4;
    } else
    {
        m = 3;
        cpu->reg.pc += step;
    }

    return m;
}

int jp_z_nn (cpu_t* cpu, uint16_t nn, int m = 4, int step = 3) 
{
    #ifdef  DEBUG
        printf("Jp Z u16, 			; $%04x\n", cpu->reg.pc);
    #endif

    if (cpu_check_condition(cpu, CPU_CONDITION_Z))
    {
        cpu->reg.pc = nn;
        m = 4;
    } else
    {
        m = 3;
        cpu->reg.pc += step;
    }

    return m;
}

int rst(cpu_t* cpu, mmu_t* mmu, uint8_t addr, int m = 4, int step = 1)
{
    #ifdef DEBUG
	    printf("RST");
    #endif 
    cpu->reg.sp--;
    mmu_write_byte(mmu, cpu->reg.sp, (cpu->reg.pc + 1) >> 8);
    cpu->reg.sp--;
    mmu_write_byte(mmu, cpu->reg.sp, (cpu->reg.pc + 1) & 0xff);
    cpu->reg.pc = addr;

    //cpu->reg.pc += step;
    return m;
}

//CPU Control instructions*************************************************************************************************************************************************************************************************************************************************

int disable_int (cpu_t* cpu, int m = 1, int step = 1) 
{
    #ifdef  DEBUG
        printf("Disable Interupt, 			; $%04x\n", cpu->reg.pc);
    #endif

    cpu->ime = false;
    cpu->reg.pc += step;

    return m;
}

int enable_int (cpu_t* cpu, int m = 1, int step = 1) 
{
    #ifdef  DEBUG
        printf("Enable Interupt, 			; $%04x\n", cpu->reg.pc);
    #endif

    cpu->ime = true;
    cpu->reg.pc += step;

    return m;
}

int scf(cpu_t* cpu, mmu_t *mmu, int m = 1, int step = 1)
{
    #ifdef DEBUG
	    printf("SCF");
    #endif

    cpu_flag_set_sub(cpu, 0);
    cpu_flag_set_halfcarry(cpu, 0);
    cpu_flag_set_carry(cpu, 1);
    cpu->reg.pc += step;

    return m;
}

int ccf(cpu_t* cpu, mmu_t *mmu, int m = 1, int step = 1)
{
    #ifdef DEBUG
	    printf("CCF");
    #endif

    cpu_flag_set_sub(cpu, 0);
    cpu_flag_set_halfcarry(cpu, 0);
    cpu_flag_set_carry(cpu, !cpu_flag(cpu, CPU_FLAG_CARRY_BIT));
    cpu->reg.pc += step;

    return m;
}
//Prefix cb ******************************************************************************************************************************************************************************************************************************************************************
int rlc_r_cb(cpu_t* cpu, mmu_t* mmu, uint8_t &reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef  DEBUG
        printf("CB_RLC %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

        cpu_flag_set_carry(cpu, (reg>>7) & 0x01);
        cpu_flag_set_sub(cpu, false);
        cpu_flag_set_halfcarry(cpu, false);

        reg =  (reg << 1) | cpu_flag(cpu, CPU_FLAG_CARRY_BIT);

        cpu_flag_set_zero(cpu, reg == 0);

        cpu->reg.pc += step;
        return m;
}

int rlc_mem_cb(cpu_t* cpu, mmu_t* mmu, uint8_t reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef  DEBUG
        printf("CB_RLC %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

        cpu_flag_set_carry(cpu, (reg>>7) & 0x01);
        cpu_flag_set_sub(cpu, false);
        cpu_flag_set_halfcarry(cpu, false);

        reg =  (reg << 1) | cpu_flag(cpu, CPU_FLAG_CARRY_BIT);

        cpu_flag_set_zero(cpu, reg == 0);

        mmu_write_byte(mmu, reg16(cpu->reg.h, cpu->reg.l), reg);
        cpu->reg.pc += step;
        return m;
}

int rrc_r_cb(cpu_t* cpu, mmu_t* mmu, uint8_t &reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef  DEBUG
        printf("CB_RRC %s		; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    cpu_flag_set_carry(cpu, reg & 0x01);
    cpu_flag_set_halfcarry(cpu, 0);
    cpu_flag_set_sub(cpu, 0);

    reg = (reg >> 1) | (cpu_flag(cpu, CPU_FLAG_CARRY_BIT) << 7);

    cpu_flag_set_zero(cpu, reg == 0);

    cpu->reg.pc += step;
    return m;
}

int rrc_mem_cb(cpu_t* cpu, mmu_t* mmu, uint8_t reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef  DEBUG
        printf("CB_RRC %s		; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    cpu_flag_set_carry(cpu, reg & 0x01);
    cpu_flag_set_halfcarry(cpu, 0);
    cpu_flag_set_sub(cpu, 0);

    reg = (reg >> 1) | (cpu_flag(cpu, CPU_FLAG_CARRY_BIT) << 7);

    cpu_flag_set_zero(cpu, reg == 0);

    mmu_write_byte(mmu, reg16(cpu->reg.h, cpu->reg.l), reg);
    cpu->reg.pc += step;
    return m;
}

int rl_r_cb(cpu_t* cpu, mmu_t* mmu, uint8_t &reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef  DEBUG
        printf("CB_RL %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    uint8_t carry_bit = cpu_flag(cpu, CPU_FLAG_CARRY_BIT);
    cpu_flag_set_carry(cpu, (reg >> 7) & 0x01);

    reg = (reg<<1)|carry_bit;

    cpu_flag_set_zero(cpu, reg == 0);
  
    cpu_flag_set_sub(cpu, false);
    cpu_flag_set_halfcarry(cpu, false);

    cpu->reg.pc += step;
    return m;
}

int rl_mem_cb(cpu_t* cpu, mmu_t* mmu, uint8_t reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef  DEBUG
        printf("CB_RL %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    uint8_t carry_bit = cpu_flag(cpu, CPU_FLAG_CARRY_BIT);
    cpu_flag_set_carry(cpu, (reg >> 7) & 0x01);

    reg = (reg<<1)|carry_bit;
    mmu_write_byte(mmu, reg16(cpu->reg.h, cpu->reg.l), reg);

    cpu_flag_set_zero(cpu, reg == 0);
  
    cpu_flag_set_sub(cpu, false);
    cpu_flag_set_halfcarry(cpu, false);

    cpu->reg.pc += step;
    return m;
}

int rr_r_cb(cpu_t* cpu, mmu_t* mmu, uint8_t &reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef  DEBUG
        printf("CB_RR %s		; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    int og_carry = cpu_flag(cpu, CPU_FLAG_CARRY_BIT);
    cpu_flag_set_carry(cpu, reg & 0x01);
    cpu_flag_set_halfcarry(cpu, 0);
    cpu_flag_set_sub(cpu, 0);
    cpu_flag_set_zero(cpu, 0);

    reg = (reg >> 1) | (og_carry << 7);
    cpu_flag_set_zero(cpu, reg == 0);

    cpu->reg.pc += step;
    return m;
}

int rr_mem_cb(cpu_t* cpu, mmu_t* mmu, uint8_t reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef  DEBUG
        printf("CB_RR %s		; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    int og_carry = cpu_flag(cpu, CPU_FLAG_CARRY_BIT);
    cpu_flag_set_carry(cpu, reg & 0x01);
    cpu_flag_set_halfcarry(cpu, 0);
    cpu_flag_set_sub(cpu, 0);
    cpu_flag_set_zero(cpu, 0);

    reg = (reg >> 1) | (og_carry << 7);
    mmu_write_byte(mmu, reg16(cpu->reg.h, cpu->reg.l), reg);
    cpu_flag_set_zero(cpu, reg == 0);

    cpu->reg.pc += step;
    return m;
}


int sla_r_cb(cpu_t* cpu, mmu_t* mmu, uint8_t &reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef  DEBUG
        printf("CB_SLA %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    cpu_flag_set_carry(cpu, (reg >> 7) & 0x01);
    cpu_flag_set_sub(cpu, false);
    cpu_flag_set_halfcarry(cpu, false);

    reg = reg << 1;

    cpu_flag_set_zero(cpu, reg == 0);

    cpu->reg.pc += step;
    return m;
}

int sla_mem_cb(cpu_t* cpu, mmu_t* mmu, uint8_t reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef  DEBUG
        printf("CB_SLA %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif

    cpu_flag_set_carry(cpu, (reg >> 7) & 0x01);
    cpu_flag_set_sub(cpu, false);
    cpu_flag_set_halfcarry(cpu, false);

    reg = reg << 1;

    cpu_flag_set_zero(cpu, reg == 0);

    mmu_write_byte(mmu, reg16(cpu->reg.h, cpu->reg.l), reg);

    cpu->reg.pc += step;
    return m;
}

int sra_r_cb(cpu_t* cpu, mmu_t* mmu, uint8_t &reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef DEBUG
	    printf("CB_SRA %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif 

	int carry = (reg >> 7) & 0x01;
	cpu_flag_set_carry(cpu, reg & 0x01);
	cpu_flag_set_sub(cpu, false);
    cpu_flag_set_halfcarry(cpu, false);

	reg = (reg >> 1) | (carry << 7);
	cpu_flag_set_zero(cpu, reg == 0);

    cpu->reg.pc += step;
    return m;      
}

int sra_mem_cb(cpu_t* cpu, mmu_t* mmu, uint8_t reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef DEBUG
	    printf("CB_SRA %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif 

	int carry = (reg >> 7) & 0x01;
	cpu_flag_set_carry(cpu, reg & 0x01);
	cpu_flag_set_sub(cpu, false);
    cpu_flag_set_halfcarry(cpu, false);

	reg = (reg >> 1) | (carry << 7);
	cpu_flag_set_zero(cpu, reg == 0);
    mmu_write_byte(mmu, reg16(cpu->reg.h, cpu->reg.l), reg);

    cpu->reg.pc += step;
    return m;      
}

int swap_r_cb(cpu_t* cpu, mmu_t* mmu, uint8_t &reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef DEBUG
	    printf("CB_SWAP %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif 

	int carry = (reg >> 7) & 0x01;
	cpu_flag_set_carry(cpu, reg & 0x01);
	cpu_flag_set_sub(cpu, false);
    cpu_flag_set_halfcarry(cpu, false);

	reg = (reg >> 1) | (carry << 7);
	cpu_flag_set_zero(cpu, reg == 0);

    cpu->reg.pc += step;
    return m;      
}

int swap_mem_cb(cpu_t* cpu, mmu_t* mmu, uint8_t reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef DEBUG
	    printf("CB_SWAP %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif 

	int carry = (reg >> 7) & 0x01;
	cpu_flag_set_carry(cpu, reg & 0x01);
	cpu_flag_set_sub(cpu, false);
    cpu_flag_set_halfcarry(cpu, false);

	reg = (reg >> 1) | (carry << 7);
	cpu_flag_set_zero(cpu, reg == 0);
    mmu_write_byte(mmu, reg16(cpu->reg.h, cpu->reg.l), reg);

    cpu->reg.pc += step;
    return m;      
}

int srl_r_cb(cpu_t* cpu, mmu_t* mmu, uint8_t &reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef DEBUG
	    printf("CB_SRL %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif 

	cpu_flag_set_carry(cpu, reg & 0x01);
	cpu_flag_set_sub(cpu, false);
    cpu_flag_set_halfcarry(cpu, false);

	reg = (reg >> 1);
	cpu_flag_set_zero(cpu, reg == 0);

    cpu->reg.pc += step;
    return m;      
}

int srl_mem_cb(cpu_t* cpu, mmu_t* mmu, uint8_t reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef DEBUG
	    printf("CB_SRL %s			; $%04x\n", cpu_reg, cpu->reg.pc);
    #endif 

	cpu_flag_set_carry(cpu, reg & 0x01);
	cpu_flag_set_sub(cpu, false);
    cpu_flag_set_halfcarry(cpu, false);

	reg = (reg >> 1);
	cpu_flag_set_zero(cpu, reg == 0);
    mmu_write_byte(mmu, reg16(cpu->reg.h, cpu->reg.l), reg);

    cpu->reg.pc += step;
    return m;      
}

int bit_num_r_cb(cpu_t* cpu, mmu_t* mmu, uint8_t num, uint8_t &reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef DEBUG
	    printf("CB_BIT %d, %s			; $%04x\n", num, cpu_reg, cpu->reg.pc);
    #endif 

    cpu_flag_set_zero(cpu, ~(reg >> num) & 0x1);
	cpu_flag_set_sub(cpu, 0);
    cpu_flag_set_halfcarry(cpu, 1);

	cpu_flag_set_zero(cpu, reg == 0);

    cpu->reg.pc += step;
    return m;      
}

int bit_num_mem_cb(cpu_t* cpu, mmu_t* mmu, uint8_t num, uint8_t reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef DEBUG
	    printf("CB_BIT %d, %s			; $%04x\n", num, cpu_reg, cpu->reg.pc);
    #endif 

    cpu_flag_set_zero(cpu, ~(reg >> num) & 0x1);
	cpu_flag_set_sub(cpu, 0);
    cpu_flag_set_halfcarry(cpu, 1);

	cpu_flag_set_zero(cpu, reg == 0);
    mmu_write_byte(mmu, reg16(cpu->reg.h, cpu->reg.l), reg);

    cpu->reg.pc += step;
    return m;      
}

int res_num_r_cb(cpu_t* cpu, mmu_t* mmu, uint8_t num, uint8_t &reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef DEBUG
	    printf("CB_RES %d, %s			; $%04x\n", num, cpu_reg, cpu->reg.pc);
    #endif 

    reg = reg & ~(0x1 << num);

    cpu->reg.pc += step;
    return m;      
}

int res_num_mem_cb(cpu_t* cpu, mmu_t* mmu, uint8_t num, uint8_t reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef DEBUG
	    printf("CB_RES %d, %s			; $%04x\n", num, cpu_reg, cpu->reg.pc);
    #endif 

    reg = reg & ~(0x1 << num);
    mmu_write_byte(mmu, reg16(cpu->reg.h, cpu->reg.l), reg);

    cpu->reg.pc += step;
    return m;      
}

int set_num_r_cb(cpu_t* cpu, mmu_t* mmu, uint8_t num, uint8_t &reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef DEBUG
	    printf("CB_SET %d, %s			; $%04x\n", num, cpu_reg, cpu->reg.pc);
    #endif 

    reg = reg | (0x1 << num);

    cpu->reg.pc += step;
    return m;      
}

int set_num_mem_cb(cpu_t* cpu, mmu_t* mmu, uint8_t num, uint8_t reg, char cpu_reg[4], int m = 2, int step = 2)
{
    #ifdef DEBUG
	    printf("CB_SET %d, %s			; $%04x\n", num, cpu_reg, cpu->reg.pc);
    #endif 

    reg = reg | (0x1 << num);
    mmu_write_byte(mmu, reg16(cpu->reg.h, cpu->reg.l), reg);

    cpu->reg.pc += step;
    return m;      
}

//****************************************************************************************************************************************************************************************************************************************************************************
int stepCPU(cpu_t *cpu, mmu_t *mmu) 
{
    #ifdef  DEBUG
        //print_registers(cpu);
        //printf("(SP) %04x, (pc) %04x\n", mmu_read_word(mmu, cpu->reg.sp), mmu_read_byte(mmu, cpu->reg.pc));
        printDebug(cpu, mmu); 
        printf("\n");
    #endif
    //printDebug(cpu, mmu);
    //printf("\n\n", mmu_read_byte(mmu, 0xff02));
    //    blarggs test - serial output
/*  
if (mmu_read_byte(mmu, 0xff02) == 0x81) {
    char c = mmu_read_word(mmu, 0xff02);
    //printf("THIS IS WRONG * %c \n", c);
    mmu_write_byte(mmu, 0xff02, 0x0);
}
*/  
	switch (mmu_read_byte(mmu, cpu->reg.pc)) 
    { 
        //NOP 
        case 0x00:
        {
            cpu->reg.pc++; 
            return 1; 
            break;
        }

        //LD BC,u16
        case 0x01:
        {
            int m = ld_rr_nn(cpu, cpu->reg.b, cpu->reg.c, (mmu_read_byte(mmu, cpu->reg.pc + 2) << 8 | mmu_read_byte(mmu, cpu->reg.pc + 1)), (char*)"BC");
            return m;
            break;
        }

        //LD (BC),A
        case 0x02:
        {
            uint16_t reg16_bc = reg16(cpu->reg.b, cpu->reg.c);
            int m = ld_mem_r(cpu, mmu, reg16_bc, cpu->reg.a, (char*) "(BC)", (char*) "A");

            set_reg(reg16_bc, cpu->reg.b, cpu->reg.c);

            return m;
            break;
        }

        //INC BC
        case 0x03:
        {
            int m = inc_rr(cpu, cpu->reg.b, cpu->reg.c, (char *) "BC");
            return m;   
            break;
        }

        //INC B
        case 0x04:
        {
            int m = inc_r(cpu, cpu->reg.b, (char *) "B");
            return m;   
            break;
        }

        //DEC B
        case 0x05:
        {
            int m = dec_r(cpu, cpu->reg.b, (char *) "B");
            return m;   
            break;
        }

        //LD B,u8
        case 0x06:
        {
            int m = ld_r_u8(cpu, mmu, cpu->reg.b, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*) "B");
            return m;  
            break; 
        }

        //RLCA
        case 0x07:
        {
            int m = rlc_r(cpu, mmu, cpu->reg.a, (char*) "A");
            return m;
            break;
        }

        //LD (u16),SP
        case 0x08:
        {
            uint16_t pc_16 = (mmu_read_byte(mmu, cpu->reg.pc + 2) << 8 | mmu_read_byte(mmu, cpu->reg.pc + 1));
            mmu_write_word(mmu, pc_16, cpu->reg.sp);
            cpu->reg.pc += 3;
            return 5;
            break;
        }

        //ADD HL,BC
        case 0x09:
        {
            int m = add_16_hl(cpu, mmu, reg16(cpu->reg.b, cpu->reg.c), (char*) "BC", 2, 1);
            return m;
            break;
        }

        //LD A,(BC)
        case 0x0a:
        {
            uint16_t reg16_bc = reg16(cpu->reg.b, cpu->reg.c);
            int m = ld_a_mem(cpu, mmu, cpu->reg.a, reg16_bc, (char*) "A", (char*) "(BC)");

            return m;
            break;
        }

        //DEC BC
        case 0x0b:
        {
            int m =  dec_16(cpu, mmu, cpu->reg.b, cpu->reg.c, (char*) "BC", 2, 1);
            return m;
            break;
        }

        //INC C
        case 0x0c:
        {
            int m = inc_r(cpu, cpu->reg.c, (char *) "C");
            return m;   
            break;
        }

        //DEC C
        case 0x0d:
        {
            int m = dec_r(cpu, cpu->reg.c, (char *) "C");
            return m;   
            break;
        }

        //LD C,u8
        case 0x0e:
        {
            int m = ld_r_u8(cpu, mmu, cpu->reg.c, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*) "C");
            return m; 
            break;  
        }

        //RRCA
        case 0x0f:
        {
            int m = rrc_r(cpu, mmu);
            return m;
            break;
        }

//************************************************************************************************************************************************************************
        
        //STOP
        case 0x10:
        {
            cpu->reg.pc++; 
            return 1; 
            break;
        }

        //LD DE, u16
        case 0x11:
        {
            int m = ld_rr_nn(cpu, cpu->reg.d, cpu->reg.e, (mmu_read_byte(mmu, cpu->reg.pc + 2) << 8 | mmu_read_byte(mmu, cpu->reg.pc + 1)), (char*)"DE");
            return m;
            break;
        }

        //LD (DE),A
        case 0x12:
        {
            uint16_t reg16_de = reg16(cpu->reg.d, cpu->reg.e);
            int m = ld_mem_r(cpu, mmu, reg16_de, cpu->reg.a, (char*) "(DE)", (char*) "A");

            set_reg(reg16_de, cpu->reg.d, cpu->reg.e);

            return m;
            break;
        }

        //INC DE
        case 0x13:
        {
            int m = inc_rr(cpu, cpu->reg.d, cpu->reg.e, (char *) "DE");
            return m;   
            break;
        }

        //INC D
        case 0x14:
        {
            int m = inc_r(cpu, cpu->reg.d, (char *) "D");
            return m;   
            break;
        }

        //DEC D
        case 0x15:
        {
            int m = dec_r(cpu, cpu->reg.d, (char *) "D");
            return m;   
            break;
        }

        //LD D,u8
        case 0x16:
        {
            int m = ld_r_u8(cpu, mmu, cpu->reg.d, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*) "D");
            return m;  
            break; 
        }

        //RLA
        case 0x17:
        {
            int m = rl_r(cpu, mmu, cpu->reg.a, (char *) "A", 1, 1);
            cpu_flag_set_zero(cpu, 0);
            return m;
            break;
        }

        //JR i8
        case 0x18:
        {
            int m = jr_nn(cpu, (int8_t) mmu_read_byte(mmu, cpu->reg.pc + 1), CPU_CONDITION_Z);
            return m;
            break;
        }

        //ADD HL,DE
        case 0x19:
        {
            int m = add_16_hl(cpu, mmu, reg16(cpu->reg.d, cpu->reg.e), (char*) "DE", 2, 1);
            return m;
            break;
        }

        //LD A,(DE)
        case 0x1a:
        {
            uint16_t reg16_de = reg16(cpu->reg.d, cpu->reg.e);
            int m = ld_a_mem(cpu, mmu, cpu->reg.a, reg16_de, (char*) "A", (char*) "(DE)");

            return m;
            break;
        }

        //DEC DE
        case 0x1b:
        {
            int m =  dec_16(cpu, mmu, cpu->reg.d, cpu->reg.e, (char*) "DE", 2, 1);
            return m;
            break;
        }

        //INC E
        case 0x1c:
        {
            int m = inc_r(cpu, cpu->reg.e, (char *) "E");
            return m;   
            break;
        }

        //DEC E
        case 0x1d:
        {
            int m = dec_r(cpu, cpu->reg.e, (char *) "E");
            return m;   
            break;
        }

        //LD E,u8
        case 0x1e:
        {
            int m = ld_r_u8(cpu, mmu, cpu->reg.e, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*) "E");
            return m; 
            break;  
        }

        //RRA
        case 0x1f:
        {
            int m = rr_r(cpu, mmu);
            return m;
            break;
        }

//************************************************************************************************************************************************************************

        //JR NZ,i8
        case 0x20:
        {
            int m = jr_nz_nn(cpu, (int8_t) mmu_read_byte(mmu, cpu->reg.pc + 1), CPU_CONDITION_NZ);
            return m;
            break;
        }

        //LD HL, u16
        case 0x21:
        {
            int m = ld_rr_nn(cpu, cpu->reg.h, cpu->reg.l, (mmu_read_byte(mmu, cpu->reg.pc + 2) << 8 | mmu_read_byte(mmu, cpu->reg.pc + 1)), (char*)"HL");
            return m;
            break;
        }

        //LD (HL+), A
        case 0x22:
        {
            uint16_t reg16_hl = reg16(cpu->reg.h, cpu->reg.l);
            int m = ld_mem_r(cpu, mmu, reg16_hl, cpu->reg.a, (char*) "(HL+)", (char*) "A");

            reg16_hl++;
            set_reg(reg16_hl, cpu->reg.h, cpu->reg.l);

            return m;
            break;
        }

        //INC HL
        case 0x23:
        {
            int m = inc_rr(cpu, cpu->reg.h, cpu->reg.l, (char*) "HL");

            return m;
            break;
        }

        //INC H
        case 0x24:
        {
            int m = inc_r(cpu, cpu->reg.h, (char *) "H");
            return m;   
            break;
        }

        //DEC H
        case 0x25:
        {
            int m = dec_r(cpu, cpu->reg.h, (char *) "H");
            return m;   
            break;
        }

        //LD H,u8
        case 0x26:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.h, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*) "H", (char*) "u8" , 2, 2);
            return m;  
            break; 
        }

        //DAA
        case 0x27:
        {
            int m = daa(cpu, mmu);
            return m;
            break;
        }

        //JR NZ,i8
        case 0x28:
        {
            int m = jr_z_nn(cpu, (int8_t) mmu_read_byte(mmu, cpu->reg.pc + 1), CPU_CONDITION_Z);
            return m;
            break;
        }

        //ADD HL,HL
        case 0x29:
        {
            int m = add_16_hl(cpu, mmu, reg16(cpu->reg.h, cpu->reg.l), (char*) "HL", 2, 1);
            return m;
            break;
        }

        //LD A,(HL+)
        case 0x2a:
        {
            uint16_t reg16_hl = reg16(cpu->reg.h, cpu->reg.l);
            int m = ld_a_mem(cpu, mmu, cpu->reg.a, reg16_hl, (char*) "A", (char*) "(HL+)");
            reg16_hl++;
            set_reg(reg16_hl, cpu->reg.h, cpu->reg.l);
            
            return m;
            break;
        }

        //DEC HL
        case 0x2b:
        {
            int m =  dec_16(cpu, mmu, cpu->reg.h, cpu->reg.l, (char*) "HL", 2, 1);
            return m;
            break;
        }

        //INC L
        case 0x2c:
        {
            int m = inc_r(cpu, cpu->reg.l, (char *) "L");
            return m;   
            break;
        }

        //DEC L
        case 0x2d:
        {
            int m = dec_r(cpu, cpu->reg.l, (char *) "l");
            return m;   
            break;
        }

        //LD L,u8
        case 0x2e:
        {
            int m = ld_r_u8(cpu, mmu, cpu->reg.l, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*) "l");
            return m; 
            break;  
        }

        //CPL
        case 0x2f:
        {
            int m = cpl(cpu, mmu);
            return m; 
            break;  
        }
//************************************************************************************************************************************************************************

        //JR NC,i8
        case 0x30:
        {
            int m = jr_nc_nn(cpu, (int8_t) mmu_read_byte(mmu, cpu->reg.pc + 1), CPU_CONDITION_NC);
            return m;
            break;
        }

        //LD SP, u16
        case 0x31:
        { 
            int m = ld_sp_nn(cpu, cpu->reg.sp, (mmu_read_byte(mmu, cpu->reg.pc + 2) << 8 | mmu_read_byte(mmu, cpu->reg.pc + 1)), (char*)"SP");
            return m;
            break;
        }

        //LD (HL-), A
        case 0x32:
        { 
            uint16_t reg16_hl = reg16(cpu->reg.h, cpu->reg.l);
            int m = ld_mem_r(cpu, mmu, reg16_hl, cpu->reg.a, (char*) "(HL-)", (char*) "A");

            reg16_hl--;
            set_reg(reg16_hl, cpu->reg.h, cpu->reg.l);

            return m;
            break;
        }

        //INC SP
        case 0x33:
        {
            int m = inc_sp(cpu, cpu->reg.sp, (char *) "SP");
            return m;   
            break;
        }

        //INC (HL)
        case 0x34:
        {
            int m = inc_r_mem(cpu, mmu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char *) "(HL)");
            return m;   
            break;
        }

        //DEC (HL)
        case 0x35:
        {
            #ifdef DEBUG
                printf("DEC (HL)");
            #endif // DEBUG
            cpu_flag_set_halfcarry(cpu, (mmu_read_byte(mmu, (cpu->reg.h << 8) | cpu->reg.l) & 0xf) == 0x0);
            mmu_write_byte(mmu, (cpu->reg.h << 8) | cpu->reg.l, mmu_read_byte(mmu, (cpu->reg.h << 8) | cpu->reg.l) - 1);
            cpu_flag_set_zero(cpu, mmu_read_byte(mmu, (cpu->reg.h << 8) | cpu->reg.l) == 0);
            cpu_flag_set_sub(cpu, 1);
            cpu->reg.pc += 1;
		
		    return 3;
            break;
        }

        //LD (HL),u8
        case 0x36:
        {
            int m = ld_mem_u8(cpu, mmu, reg16(cpu->reg.h, cpu->reg.l), mmu_read_byte(mmu, cpu->reg.pc + 1), (char *) "(HL)", (char *) "u8");
            return m;
            break;
        }

        //SCF
        case 0x37:
        {
            int m = scf(cpu, mmu);
            return m;
            break;
        }

        //JR C,i8
        case 0x38:
        {
            int m = jr_c_nn(cpu, (int8_t) mmu_read_byte(mmu, cpu->reg.pc + 1), CPU_CONDITION_C);
            return m;
            break;
        }

        //ADD HL,SP
        case 0x39:
        {
            int m = add_16_hl(cpu, mmu, cpu->reg.sp, (char*)"SP");
            return m;
            break;
        }

        //LD A,(HL-)
        case 0x3a:
        {
            uint16_t reg16_hl = reg16(cpu->reg.h, cpu->reg.l);
            int m = ld_a_mem(cpu, mmu, cpu->reg.a, reg16_hl, (char*) "A", (char*) "(HL-)");
            reg16_hl--;
            set_reg(reg16_hl, cpu->reg.h, cpu->reg.l);
            
            return m;
            break;
        }

        //DEC SP
        case 0x3b:
        {
            #ifdef  DEBUG
                printf("DEC, SP,			; $%04x\n", cpu->reg.pc);
            #endif

            cpu->reg.sp--;

            cpu->reg.pc ++;

            return 2;
            break;
        }

        //INC A
        case 0x3c:
        {
            int m = inc_r(cpu, cpu->reg.a, (char *) "A");
            return m;   
            break;
        }

        //DEC A
        case 0x3d:
        {
            int m = dec_r(cpu, cpu->reg.a, (char *) "A");
            return m;   
            break;
        }

        //LD A,u8
        case 0x3e:
        {
            int m = ld_r_u8(cpu, mmu, cpu->reg.a, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*) "A");
            return m;   

            break;
        }

        //CCF
        case 0x3f:
        {
            int m = ccf(cpu, mmu);
            return m; 
            break;  
        }

//************************************************************************************************************************************************************************
         //LD B,B
        case 0x40:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.b, cpu->reg.b, (char*) "B", (char*) "B");
            return m;
            break;
        }

         //LD B,C
        case 0x41:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.b, cpu->reg.c, (char*) "B", (char*) "C");
            return m;
            break;
        }

         //LD B,D
        case 0x42:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.b, cpu->reg.d, (char*) "B", (char*) "D");
            return m;
            break;
        }

         //LD B,E
        case 0x43:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.b, cpu->reg.e, (char*) "B", (char*) "E");
            return m;
            break;
        }

         //LD B,H
        case 0x44:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.b, cpu->reg.h, (char*) "B", (char*) "H");
            return m;
            break;
        }

         //LD B,L
        case 0x45:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.b, cpu->reg.l, (char*) "B", (char*) "L");
            return m;
            break;
        }

         //LD B,(HL)
        case 0x46:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.b, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), 
                    (char*) "B", (char*) "(HL)", m = 2);
            return m;
            break;
        }

         //LD B,A
        case 0x47:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.b, cpu->reg.a, (char*) "B", (char*) "A");
            return m;
            break;
        }

         //LD C,B
        case 0x48:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.c, cpu->reg.b, (char*) "C", (char*) "B");
            return m;
            break;
        }

         //LD C,C
        case 0x49:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.c, cpu->reg.c, (char*) "C", (char*) "C");
            return m;
            break;
        }

         //LD C,D
        case 0x4a:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.c, cpu->reg.d, (char*) "C", (char*) "D");
            return m;
            break;
        }

         //LD C,E
        case 0x4b:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.c, cpu->reg.e, (char*) "C", (char*) "E");
            return m;
            break;
        }

         //LD C,H
        case 0x4c:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.c, cpu->reg.h, (char*) "C", (char*) "H");
            return m;
            break;
        }

         //LD C,L
        case 0x4d:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.c, cpu->reg.l, (char*) "C", (char*) "L");
            return m;
            break;
        }

         //LD C,(HL)
        case 0x4e:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.c, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), 
                    (char*) "C", (char*) "(HL)", m = 2);
            return m;
            break;
        } 
        
        //LD C,A
        case 0x4f:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.c, cpu->reg.a, (char*) "C", (char*) "A");
            return m;
            break;
        }
//************************************************************************************************************************************************************************
         //LD D,B
        case 0x50:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.d, cpu->reg.b, (char*) "D", (char*) "B");
            return m;
            break;
        }

         //LD D,C
        case 0x51:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.d, cpu->reg.c, (char*) "D", (char*) "C");
            return m;
            break;
        }

         //LD D,D
        case 0x52:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.d, cpu->reg.d, (char*) "D", (char*) "D");
            return m;
            break;
        }

         //LD D,E
        case 0x53:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.d, cpu->reg.e, (char*) "D", (char*) "E");
            return m;
            break;
        }

         //LD D,H
        case 0x54:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.d, cpu->reg.h, (char*) "D", (char*) "H");
            return m;
            break;
        }

         //LD D,L
        case 0x55:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.d, cpu->reg.l, (char*) "D", (char*) "L");
            return m;
            break;
        }

         //LD D,(HL)
        case 0x56:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.d, mmu_read_byte(mmu, (cpu->reg.h << 8) | cpu->reg.l), 
                    (char*) "D", (char*) "(HL)", m = 2);
            return m;
            break;
        }

         //LD D,A
        case 0x57:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.d, cpu->reg.a, (char*) "D", (char*) "A");
            return m;
            break;
        }

         //LD E,B
        case 0x58:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.e, cpu->reg.b, (char*) "E", (char*) "B");
            return m;
            break;
        }

         //LD E,C
        case 0x59:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.e, cpu->reg.c, (char*) "E", (char*) "C");
            return m;
            break;
        }

         //LD E,D
        case 0x5a:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.e, cpu->reg.d, (char*) "E", (char*) "D");
            return m;
            break;
        }

         //LD E,E
        case 0x5b:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.e, cpu->reg.e, (char*) "E", (char*) "E");
            return m;
            break;
        }

         //LD E,H
        case 0x5c:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.e, cpu->reg.h, (char*) "E", (char*) "H");
            return m;
            break;
        }

         //LD E,L
        case 0x5d:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.e, cpu->reg.l, (char*) "E", (char*) "L");
            return m;
            break;
        }

         //LD E,(HL)
        case 0x5e:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.e, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), 
                    (char*) "E", (char*) "(HL)", m = 2);
            return m;
            break;
        } 
        
        //LD E,A
        case 0x5f:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.e, cpu->reg.a, (char*) "E", (char*) "A");
            return m;
            break;
        }
//************************************************************************************************************************************************************************
         //LD H,B
        case 0x60:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.h, cpu->reg.b, (char*) "H", (char*) "B");
            return m;
            break;
        }

         //LD H,C
        case 0x61:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.h, cpu->reg.c, (char*) "H", (char*) "C");
            return m;
            break;
        }

         //LD H,D
        case 0x62:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.h, cpu->reg.d, (char*) "H", (char*) "D");
            return m;
            break;
        }

         //LD H,E
        case 0x63:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.h, cpu->reg.e, (char*) "H", (char*) "E");
            return m;
            break;
        }

         //LD H,H
        case 0x64:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.h, cpu->reg.h, (char*) "H", (char*) "H");
            return m;
            break;
        }

         //LD H,L
        case 0x65:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.h, cpu->reg.l, (char*) "H", (char*) "L");
            return m;
            break;
        }

         //LD H,(HL)
        case 0x66:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.h, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), 
                    (char*) "H", (char*) "(HL)", m = 2);
            return m;
            break;
        }

         //LD H,A
        case 0x67:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.h, cpu->reg.a, (char*) "H", (char*) "A");
            return m;
            break;
        }

         //LD L,B
        case 0x68:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.l, cpu->reg.b, (char*) "L", (char*) "B");
            return m;
            break;
        }

         //LD L,C
        case 0x69:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.l, cpu->reg.c, (char*) "L", (char*) "C");
            return m;
            break;
        }

         //LD L,D
        case 0x6a:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.l, cpu->reg.d, (char*) "L", (char*) "D");
            return m;
            break;
        }

         //LD L,E
        case 0x6b:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.l, cpu->reg.e, (char*) "L", (char*) "E");
            return m;
            break;
        }

         //LD L,H
        case 0x6c:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.l, cpu->reg.h, (char*) "L", (char*) "H");
            return m;
            break;
        }

         //LD L,L
        case 0x6d:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.l, cpu->reg.l, (char*) "L", (char*) "L");
            return m;
            break;
        }

         //LD L,(HL)
        case 0x6e:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.l, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), 
                    (char*) "L", (char*) "(HL)", m = 2);
            return m;
            break;
        } 
        
        //LD L,A
        case 0x6f:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.l, cpu->reg.a, (char*) "L", (char*) "A");
            return m;
            break;
        }
//************************************************************************************************************************************************************************

        //LD (HL),B
        case 0x70:
        {
            uint16_t reg16_hl = reg16(cpu->reg.h, cpu->reg.l);
            int m = ld_mem_r(cpu, mmu, reg16_hl, cpu->reg.b, (char*) "(HL)", (char*) "B");
            
            return m;
            break;
        }

        //LD (HL),C
        case 0x71:
        {
            uint16_t reg16_hl = reg16(cpu->reg.h, cpu->reg.l);
            int m = ld_mem_r(cpu, mmu, reg16_hl, cpu->reg.c, (char*) "(HL)", (char*) "C");
            
            return m;
            break;
        }

        //LD (HL),D
        case 0x72:
        {
            uint16_t reg16_hl = reg16(cpu->reg.h, cpu->reg.l);
            int m = ld_mem_r(cpu, mmu, reg16_hl, cpu->reg.d, (char*) "(HL)", (char*) "D");
            
            return m;
            break;
        }

        //LD (HL),E
        case 0x73:
        {
            uint16_t reg16_hl = reg16(cpu->reg.h, cpu->reg.l);
            int m = ld_mem_r(cpu, mmu, reg16_hl, cpu->reg.e, (char*) "(HL)", (char*) "E");
            
            return m;
            break;
        }

        //LD (HL),H
        case 0x74:
        {
            uint16_t reg16_hl = reg16(cpu->reg.h, cpu->reg.l);
            int m = ld_mem_r(cpu, mmu, reg16_hl, cpu->reg.h, (char*) "(HL)", (char*) "H");

            return m;
            break;
        }

        //LD (HL),L
        case 0x75:
        {
            uint16_t reg16_hl = reg16(cpu->reg.h, cpu->reg.l);
            int m = ld_mem_r(cpu, mmu, reg16_hl, cpu->reg.l, (char*) "(HL)", (char*) "L");

            return m;
            break;
        }


         //HALT
        case 0x76:
        {
            cpu->halt = true;
            int m = 1;
            cpu->reg.pc++;
            return m;
            break;
        }


        //LD (HL),A
        case 0x77:
        {
            uint16_t reg16_hl = reg16(cpu->reg.h, cpu->reg.l);
            int m = ld_mem_r(cpu, mmu, reg16_hl, cpu->reg.a, (char*) "(HL)", (char*) "A");

            return m;
            break;
        }

        //LD A,B
        case 0x78:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.a, cpu->reg.b, (char*) "A", (char*) "B");
            return m;
            break;
        }

        //LD A,C
        case 0x79:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.a, cpu->reg.c, (char*) "A", (char*) "C");
            return m;
            break;
        }

        //LD A,D
        case 0x7a:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.a, cpu->reg.d, (char*) "A", (char*) "D");
            return m;
            break;
        }

        //LD A,E
        case 0x7b:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.a, cpu->reg.e, (char*) "A", (char*) "E");
            return m;
            break;
        }

        //LD A,H
        case 0x7c:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.a, cpu->reg.h, (char*) "A", (char*) "H");
            return m;
            break;
        }

        //LD A,L
        case 0x7d:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.a, cpu->reg.l, (char*) "A", (char*) "L");
            return m;
            break;
        }

        //LD A,(HL)
        case 0x7e:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.a, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), 
                    (char*) "A", (char*) "(HL)", m = 2);
            return m;
            break;
        } 
        
        //LD A,A
        case 0x7f:
        {
            int m = ld_r_r(cpu, mmu, cpu->reg.a, cpu->reg.a, (char*) "A", (char*) "A");
            return m;
            break;
        }
        
//************************************************************************************************************************************************************************
        //ADD A,B
        case 0x80:
        {
            int m = add_r(cpu, mmu, cpu->reg.a, cpu->reg.b, (char*) "B");
            return m;
            break;
        }

        //ADD A,C
        case 0x81:
        {
            int m = add_r(cpu, mmu, cpu->reg.a, cpu->reg.c, (char*) "C");
            return m;
            break;
        }

        //ADD A,D
        case 0x82:
        {
            int m = add_r(cpu, mmu, cpu->reg.a, cpu->reg.d, (char*) "D");
            return m;
            break;
        }

        //ADD A,E
        case 0x83:
        {
            int m = add_r(cpu, mmu, cpu->reg.a, cpu->reg.e, (char*) "E");
            return m;
            break;
        }

        //ADD A,H
        case 0x84:
        {
            int m = add_r(cpu, mmu, cpu->reg.a, cpu->reg.h, (char*) "H");
            return m;
            break;
        }

        //ADD A,L
        case 0x85:
        {
            int m = add_r(cpu, mmu, cpu->reg.a, cpu->reg.l, (char*) "L");
            return m;
            break;
        }

        //ADD A,(HL)
        case 0x86:
        {
            int m = add_r(cpu, mmu, cpu->reg.a, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*) "(HL)", 2);
            return m;
            break;
        }

        //ADD A,A
        case 0x87:
        {
            int m = add_r(cpu, mmu, cpu->reg.a, cpu->reg.a, (char*) "A");
            return m;
            break;
        }

        //ADC A,B
        case 0x88:
        {
            int m = adc_r(cpu, mmu,  cpu->reg.a, cpu->reg.b, (char*) "B");
            return m;
            break;
        }

        //ADC A,C
        case 0x89:
        {
            int m =  adc_r(cpu, mmu,  cpu->reg.a, cpu->reg.c, (char*) "C");
            return m;
            break;
        }

        //ADC A,D
        case 0x8a:
        {
            int m =  adc_r(cpu, mmu,  cpu->reg.a, cpu->reg.d, (char*) "D");
            return m;
            break;
        }

        //ADC A,E
        case 0x8b:
        {
            int m =  adc_r(cpu, mmu,  cpu->reg.a, cpu->reg.e, (char*) "E");
            return m;
            break;
        }

        //ADC A,H
        case 0x8c:
        {
            int m =  adc_r(cpu, mmu,  cpu->reg.a, cpu->reg.h, (char*) "H");
            return m;
            break;
        }

        //ADC A,L
        case 0x8d:
        {
            int m =  adc_r(cpu, mmu,  cpu->reg.a, cpu->reg.l, (char*) "L");
            return m;
            break;
        }

        //ADC A,(HL)
        case 0x8e:
        {
            int m = adc_r(cpu, mmu, cpu->reg.a, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*) "(HL)", 2);
            return m;
            break;
        } 
        
        //ADC A,A
        case 0x8f:
        {
            int m = adc_r(cpu, mmu,  cpu->reg.a, cpu->reg.a, (char*) "L");
            return m;
            break;
        }

//************************************************************************************************************************************************************************
        //SUB A,B
        case 0x90:
        {
            int m = sub_r(cpu, mmu, cpu->reg.b, (char*) "B");
            return m;
            break;
        }

        //SUB A,C
        case 0x91:
        {
            int m = sub_r(cpu, mmu, cpu->reg.c, (char*) "C");
            return m;
            break;
        }

        //SUB A,D
        case 0x92:
        {
            int m = sub_r(cpu, mmu, cpu->reg.d, (char*) "D");
            return m;
            break;
        }

        //SUB A,E
        case 0x93:
        {
            int m = sub_r(cpu, mmu, cpu->reg.e, (char*) "E");
            return m;
            break;
        }

        //SUB A,H
        case 0x94:
        {
            int m = sub_r(cpu, mmu, cpu->reg.h, (char*) "H");
            return m;
            break;
        }

        //SUB A,L
        case 0x95:
        {
            int m = sub_r(cpu, mmu, cpu->reg.l, (char*) "L");
            return m;
            break;
        }

        //SUB A,(HL)
        case 0x96:
        {
            int m = sub_r(cpu, mmu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*) "(HL)", 2);
            return m;
            break;
        }

        //SUB A,A
        case 0x97:
        {
            int m = sub_r(cpu, mmu, cpu->reg.a, (char*) "A");
            return m;
            break;
        }

        //SBC A,B
        case 0x98:
        {
            int m = sbc_r(cpu, mmu, cpu->reg.b, (char*) "B");
            return m;
            break;
        }

        //SBC A,C
        case 0x99:
        {
            int m =  sbc_r(cpu, mmu, cpu->reg.c, (char*) "C");
            return m;
            break;
        }

        //SBC A,D
        case 0x9a:
        {
            int m =  sbc_r(cpu, mmu, cpu->reg.d, (char*) "D");
            return m;
            break;
        }

        //SBC A,E
        case 0x9b:
        {
            int m =  sbc_r(cpu, mmu, cpu->reg.e, (char*) "E");
            return m;
            break;
        }

        //SBC A,H
        case 0x9c:
        {
            int m =  sbc_r(cpu, mmu, cpu->reg.h, (char*) "H");
            return m;
            break;
        }

        //SBC A,L
        case 0x9d:
        {
            int m =  sbc_r(cpu, mmu, cpu->reg.l, (char*) "L");
            return m;
            break;
        }

        //SBC A,(HL)
        case 0x9e:
        {
            int m = sbc_r(cpu, mmu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*) "(HL)", 2);
            return m;
            break;
        } 
        
        //SBC A,A
        case 0x9f:
        {
            int m = sbc_r(cpu, mmu, cpu->reg.a, (char*) "L");
            return m;
            break;
        }

//************************************************************************************************************************************************************************    
        //AND A,B
        case 0xa0:
        {
            int m = and_r(cpu, mmu, cpu->reg.b, (char*) "B");
            return m;
            break;
        }

        //AND A,C
        case 0xa1:
        {
            int m = and_r(cpu, mmu, cpu->reg.c, (char*) "C");
            return m;
            break;
        }

        //AND A,D
        case 0xa2:
        {
            int m = and_r(cpu, mmu, cpu->reg.d, (char*) "D");
            return m;
            break;
        }

        //AND A,E
        case 0xa3:
        {
            int m = and_r(cpu, mmu, cpu->reg.e, (char*) "E");
            return m;
            break;
        }

        //AND A,H
        case 0xa4:
        {
            int m = and_r(cpu, mmu, cpu->reg.h, (char*) "H");
            return m;
            break;
        }

        //AND A,L
        case 0xa5:
        {
            int m = and_r(cpu, mmu, cpu->reg.l, (char*) "L");
            return m;
            break;
        }

        //AND A,(HL)
        case 0xa6:
        {
            int m = and_r(cpu, mmu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*) "(HL)", 2);
            return m;
            break;
        }

        //AND A,A
        case 0xa7:
        {
            int m = and_r(cpu, mmu, cpu->reg.a, (char*) "A");
            return m;
            break;
        }

        //XOR A,B
        case 0xa8:
        {
            int m = xor_r(cpu, cpu->reg.b, (char*) "B");
            return m;
            break;
        }

        //XOR A,C
        case 0xa9:
        {
            int m =  xor_r(cpu, cpu->reg.c, (char*) "C");
            return m;
            break;
        }

        //XOR A,D
        case 0xaa:
        {
            int m =  xor_r(cpu, cpu->reg.d, (char*) "D");
            return m;
            break;
        }

        //XOR A,E
        case 0xab:
        {
            int m =  xor_r(cpu, cpu->reg.e, (char*) "E");
            return m;
            break;
        }

        //XOR A,H
        case 0xac:
        {
            int m =  xor_r(cpu, cpu->reg.h, (char*) "H");
            return m;
            break;
        }

        //XOR A,L
        case 0xad:
        {
            int m =  xor_r(cpu, cpu->reg.l, (char*) "L");
            return m;
            break;
        }

        //XOR A,(HL)
        case 0xae:
        {
            int m = xor_r(cpu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*) "(HL)", 2);
            return m;
            break;
        } 
        
        //XOR A, A
        case 0xaf:
        { 
            int m = xor_r(cpu, cpu->reg.a, (char*)"A");
            return m;
            break;
        }
//************************************************************************************************************************************************************************
        
        //OR A,B
        case 0xb0:
        {
            int m = or_r(cpu, cpu->reg.b, (char*) "B");
            return m;
            break;
        }

        //OR A,C
        case 0xb1:
        {
            int m = or_r(cpu, cpu->reg.c, (char*) "C");
            return m;
            break;
        }

        //OR A,D
        case 0xb2:
        {
            int m = or_r(cpu, cpu->reg.d, (char*) "D");
            return m;
            break;
        }

        //OR A,E
        case 0xb3:
        {
            int m = or_r(cpu, cpu->reg.e, (char*) "E");
            return m;
            break;
        }

        //OR A,H
        case 0xb4:
        {
            int m = or_r(cpu, cpu->reg.h, (char*) "H");
            return m;
            break;
        }

        //OR A,L
        case 0xb5:
        {
            int m = or_r(cpu, cpu->reg.l, (char*) "L");
            return m;
            break;
        }

        //OR A,(HL)
        case 0xb6:
        {
            int m = or_r(cpu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*) "(HL)", 2);
            return m;
            break;
        }

        //OR A,A
        case 0xb7:
        {
            int m = or_r(cpu, cpu->reg.a, (char*) "A");
            return m;
            break;
        }

         //CP A,B
        case 0xb8:
        {
            int step = 0;
            int m = cp_r(cpu, cpu->reg.b, (char*) "B", m = 1, step = 1);
            return m;
            break;
        }

         //CP A,C
        case 0xb9:
        {
            int step = 0;
            int m = cp_r(cpu, cpu->reg.c, (char*) "C", m = 1, step = 1);
            return m;
            break;
        }

         //CP A,D
        case 0xba:
        {
            int step = 0;
            int m = cp_r(cpu, cpu->reg.d, (char*) "D", m = 1, step = 1);
            return m;
            break;
        }

         //CP A,E
        case 0xbb:
        {
            int step = 0;
            int m = cp_r(cpu, cpu->reg.e, (char*) "E", m = 1, step = 1);
            return m;
            break;
        }

         //CP A,H
        case 0xbc:
        {
            int step = 0;
            int m = cp_r(cpu, cpu->reg.h, (char*) "H", m = 1, step = 1);
            return m;
            break;
        }

         //CP A,L
        case 0xbd:
        {
            int step = 0;
            int m = cp_r(cpu, cpu->reg.l, (char*) "L", m = 1, step = 1);
            return m;
            break;
        }

         //CP A,(HL)
        case 0xbe:
        {
            int step = 0;
            int m = cp_r(cpu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*) "(HL)", m = 2, step = 1);
            return m;
            break;
        } 
        
        //CP A,A
        case 0xbf:
        {
            int step = 0;
            int m = cp_r(cpu, cpu->reg.a, (char*) "A", m = 1, step = 1);
            return m;
            break;
        }

//************************************************************************************************************************************************************************
        //RET NZ
        case 0xc0:
        {
            int m = ret_nz(cpu, mmu);
            return m;
            break;
        }

        //POP BC
        case 0xc1:
        {
            int m = pop_rr(cpu, mmu, cpu->reg.sp, cpu->reg.b, cpu->reg.c, (char*) "BC");
            return m;
            break;
        }

        //JP nz u16
        case 0xc2:
        {
            int m = jp_nz_nn(cpu, mmu_read_word(mmu, cpu->reg.pc + 1));
            return m;
            break;
        }

        //JP u16
        case 0xc3:
        {
            int m = jp_nn(cpu, (mmu_read_byte(mmu, cpu->reg.pc + 2) << 8) | (mmu_read_byte(mmu, cpu->reg.pc + 1) & 0xff));
            return m;
            break;
        }

        //CALL NZ,u16
        case 0xc4:
        {
            int m = call_nz_nn(cpu, mmu, mmu_read_word(mmu, cpu->reg.pc + 1));
            return m;
            break;
        }

        //PUSH BC
        case 0xc5:
        {
            int m = push_rr(cpu, mmu, reg16(cpu->reg.b, cpu->reg.c), (char*) "BC");
            return m;
            break;
        }

        //ADD A,u8
        case 0xc6:
        {
            int m = add_r(cpu, mmu, cpu->reg.a, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*)"u8", 2 ,2);
            return m;
            break;
        }

        //RST 00h
        case 0xc7:
        {
            int m =  rst(cpu, mmu, 0x00);
            return m;
            break;
        }

        //RET Z
        case 0xc8:
        {
            int m = ret_z(cpu, mmu);
            return m;
            break;
        }

        //RET
        case 0xc9:
        {
            int m = ret(cpu,mmu);
            return m;
            break;
        }

        //JP Z,u16
        case 0xca:
        {
            int m = jp_z_nn(cpu, mmu_read_word(mmu, cpu->reg.pc +1));
            return m;
            break;
        }

        //CB cases
        case 0xcb:
        {
            switch(mmu_read_byte(mmu, cpu->reg.pc + 1))
            {
                //RLC B 
                case 0x00:
                {
                    int m = rlc_r_cb(cpu, mmu, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //RLC C
                case 0x01:
                {
                    int m = rlc_r_cb(cpu, mmu, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //RLC D
                case 0x02:
                {
                    int m = rlc_r_cb(cpu, mmu, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //RLC E
                case 0x03:
                {
                    int m = rlc_r_cb(cpu, mmu, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //RLC H
                case 0x04:
                {
                    int m = rlc_r_cb(cpu, mmu, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //RLC L
                case 0x05:
                {
                    int m = rlc_r_cb(cpu, mmu, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //RLC (HL)
                case 0x06:
                {
                    int m = rlc_mem_cb(cpu, mmu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 4);
                    return m;  
                    break; 
                }

                //RLC A
                case 0x07:
                {
                    int m = rlc_r_cb(cpu, mmu, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //RRC B
                case 0x08:
                {
                    int m = rrc_r_cb(cpu, mmu, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //RRC C
                case 0x09:
                {
                    int m = rrc_r_cb(cpu, mmu, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //RRC D
                case 0x0a:
                {
                    int m = rrc_r_cb(cpu, mmu, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //RRC E
                case 0x0b:
                {
                    int m = rrc_r_cb(cpu, mmu, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //RRC H
                case 0x0c:
                {
                    int m = rrc_r_cb(cpu, mmu, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //RRC L
                case 0x0d:
                {
                    int m = rrc_r_cb(cpu, mmu, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //RRC (HL)
                case 0x0e:
                {
                    int m = rrc_mem_cb(cpu, mmu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 4);  
                    return m;
                    break;
                }

                //RRC A
                case 0x0f:
                {
                    int m = rrc_r_cb(cpu, mmu, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }
                
                //************************************************************************************************************************************************************************
                //RL B 
                case 0x10:
                {
                    int m = rl_r_cb(cpu, mmu, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //RL C
                case 0x11:
                {
                    int m = rl_r_cb(cpu, mmu, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //RL D
                case 0x12:
                {
                    int m = rl_r_cb(cpu, mmu, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //RL E
                case 0x13:
                {
                    int m = rl_r_cb(cpu, mmu, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //RL H
                case 0x14:
                {
                    int m = rl_r_cb(cpu, mmu, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //RL L
                case 0x15:
                {
                    int m = rl_r_cb(cpu, mmu, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //RL (HL)
                case 0x16:
                {
                    int m = rl_mem_cb(cpu, mmu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 4);
                    return m;  
                    break; 
                }

                //RL A
                case 0x17:
                {
                    int m = rl_r_cb(cpu, mmu, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //RR B
                case 0x18:
                {
                    int m = rr_r_cb(cpu, mmu, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //RR C
                case 0x19:
                {
                    int m = rr_r_cb(cpu, mmu, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //RR D
                case 0x1a:
                {
                    int m = rr_r_cb(cpu, mmu, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //RR E
                case 0x1b:
                {
                    int m = rr_r_cb(cpu, mmu, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //RR H
                case 0x1c:
                {
                    int m = rr_r_cb(cpu, mmu, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //RR L
                case 0x1d:
                {
                    int m = rr_r_cb(cpu, mmu, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //RR (HL)
                case 0x1e:
                {
                    int m = rr_mem_cb(cpu, mmu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 4);  
                    return m;
                    break;
                }

                //RR A
                case 0x1f:
                {
                    int m = rr_r_cb(cpu, mmu, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                } 

                //************************************************************************************************************************************************************************

                //SLA B 
                case 0x20:
                {
                    int m = sla_r_cb(cpu, mmu, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //SLA C
                case 0x21:
                {
                    int m = sla_r_cb(cpu, mmu, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //SLA D
                case 0x22:
                {
                    int m = sla_r_cb(cpu, mmu, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //SLA E
                case 0x23:
                {
                    int m = sla_r_cb(cpu, mmu, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //SLA H
                case 0x24:
                {
                    int m = sla_r_cb(cpu, mmu, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //SLA L
                case 0x25:
                {
                    int m = sla_r_cb(cpu, mmu, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //SLA (HL)
                case 0x26:
                {
                    int m = sla_mem_cb(cpu, mmu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 4);
                    return m;  
                    break; 
                }

                //SLA A
                case 0x27:
                {
                    int m = sla_r_cb(cpu, mmu, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //SRA B
                case 0x28:
                {
                    int m = sra_r_cb(cpu, mmu, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //SRA C
                case 0x29:
                {
                    int m = sra_r_cb(cpu, mmu, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //SRA D
                case 0x2a:
                {
                    int m = sra_r_cb(cpu, mmu, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //SRA E
                case 0x2b:
                {
                    int m = sra_r_cb(cpu, mmu, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //SRA H
                case 0x2c:
                {
                    int m = sra_r_cb(cpu, mmu, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //SRA L
                case 0x2d:
                {
                    int m = sra_r_cb(cpu, mmu, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //SRA (HL)
                case 0x2e:
                {
                    int m = sra_mem_cb(cpu, mmu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 4);  
                    return m;
                    break;
                }

                //SRA A
                case 0x2f:
                {
                    int m = sra_r_cb(cpu, mmu, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }

                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************

                //SWAP B 
                case 0x30:
                {
                    int m = swap_r_cb(cpu, mmu, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //SWAP C
                case 0x31:
                {
                    int m = swap_r_cb(cpu, mmu, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //SWAP D
                case 0x32:
                {
                    int m = swap_r_cb(cpu, mmu, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //SWAP E
                case 0x33:
                {
                    int m = swap_r_cb(cpu, mmu, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //SWAP H
                case 0x34:
                {
                    int m = swap_r_cb(cpu, mmu, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //SWAP L
                case 0x35:
                {
                    int m = swap_r_cb(cpu, mmu, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //SWAP (HL)
                case 0x36:
                {
                    int m = swap_mem_cb(cpu, mmu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 4);
                    return m;  
                    break; 
                }

                //SWAP A
                case 0x37:
                {
                    int m = swap_r_cb(cpu, mmu, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //SRL B
                case 0x38:
                {
                    int m = srl_r_cb(cpu, mmu, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //SRL C
                case 0x39:
                {
                    int m = srl_r_cb(cpu, mmu, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //SRL D
                case 0x3a:
                {
                    int m = srl_r_cb(cpu, mmu, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //SRL E
                case 0x3b:
                {
                    int m = srl_r_cb(cpu, mmu, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //SRL H
                case 0x3c:
                {
                    int m = srl_r_cb(cpu, mmu, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //SRL L
                case 0x3d:
                {
                    int m = srl_r_cb(cpu, mmu, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //SRL (HL)
                case 0x3e:
                {
                    int m = srl_mem_cb(cpu, mmu, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 4);  
                    return m;
                    break;
                }
        
                //SRL A
                case 0x3f:
                {
                    int m = srl_r_cb(cpu, mmu, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }

                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************
                
                //BIT 0, B 
                case 0x40:
                {
                    int m = bit_num_r_cb(cpu, mmu, 0, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //BIT 0, C
                case 0x41:
                {
                    int m = bit_num_r_cb(cpu, mmu, 0, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //BIT 0, D
                case 0x42:
                {
                    int m = bit_num_r_cb(cpu, mmu, 0, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //BIT 0, E
                case 0x43:
                {
                    int m = bit_num_r_cb(cpu, mmu, 0, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //BIT 0, H
                case 0x44:
                {
                    int m = bit_num_r_cb(cpu, mmu, 0, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //BIT 0, L
                case 0x45:
                {
                    int m = bit_num_r_cb(cpu, mmu, 0, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //BIT 0, (HL)
                case 0x46:
                {
                    int m = bit_num_mem_cb(cpu, mmu, 0, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 3);
                    return m;  
                    break; 
                }

                //BIT 0, A
                case 0x47:
                {
                    int m = bit_num_r_cb(cpu, mmu, 0, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //BIT 1, B
                case 0x48:
                {
                    int m = bit_num_r_cb(cpu, mmu, 1, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //BIT 1, C
                case 0x49:
                {
                    int m = bit_num_r_cb(cpu, mmu, 1, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //BIT 1, D
                case 0x4a:
                {
                    int m = bit_num_r_cb(cpu, mmu, 1, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //BIT 1, E
                case 0x4b:
                {
                    int m = bit_num_r_cb(cpu, mmu, 1, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //BIT 1, H
                case 0x4c:
                {
                    int m = bit_num_r_cb(cpu, mmu, 1, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //BIT 1, L
                case 0x4d:
                {
                    int m = bit_num_r_cb(cpu, mmu, 1, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //BIT 1, (HL)
                case 0x4e:
                {
                    int m = bit_num_mem_cb(cpu, mmu, 1, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 3);  
                    return m;
                    break;
                }

                //BIT 1, A
                case 0x4f:
                {
                    int m = bit_num_r_cb(cpu, mmu, 1, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }

                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************
                
                                //BIT 2, B 
                case 0x50:
                {
                    int m = bit_num_r_cb(cpu, mmu, 2, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //BIT 2, C
                case 0x51:
                {
                    int m = bit_num_r_cb(cpu, mmu, 2, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //BIT 2, D
                case 0x52:
                {
                    int m = bit_num_r_cb(cpu, mmu, 2, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //BIT 2, E
                case 0x53:
                {
                    int m = bit_num_r_cb(cpu, mmu, 2, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //BIT 2, H
                case 0x54:
                {
                    int m = bit_num_r_cb(cpu, mmu, 2, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //BIT 2, L
                case 0x55:
                {
                    int m = bit_num_r_cb(cpu, mmu, 2, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //BIT 2, (HL)
                case 0x56:
                {
                    int m = bit_num_mem_cb(cpu, mmu, 2, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 3);
                    return m;  
                    break; 
                }

                //BIT 2, A
                case 0x57:
                {
                    int m = bit_num_r_cb(cpu, mmu, 2, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //BIT 3, B
                case 0x58:
                {
                    int m = bit_num_r_cb(cpu, mmu, 3, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //BIT 3, C
                case 0x59:
                {
                    int m = bit_num_r_cb(cpu, mmu, 3, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //BIT 3, D
                case 0x5a:
                {
                    int m = bit_num_r_cb(cpu, mmu, 3, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //BIT 3, E
                case 0x5b:
                {
                    int m = bit_num_r_cb(cpu, mmu, 3, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //BIT 3, H
                case 0x5c:
                {
                    int m = bit_num_r_cb(cpu, mmu, 3, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //BIT 3, L
                case 0x5d:
                {
                    int m = bit_num_r_cb(cpu, mmu, 3, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //BIT 3, (HL)
                case 0x5e:
                {
                    int m = bit_num_mem_cb(cpu, mmu, 3, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 3);  
                    return m;
                    break;
                }

                //BIT 3, A
                case 0x5f:
                {
                    int m = bit_num_r_cb(cpu, mmu, 3, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }
                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************

                //BIT 4, B 
                case 0x60:
                {
                    int m = bit_num_r_cb(cpu, mmu, 4, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //BIT 4, C
                case 0x61:
                {
                    int m = bit_num_r_cb(cpu, mmu, 4, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //BIT 4, D
                case 0x62:
                {
                    int m = bit_num_r_cb(cpu, mmu, 4, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //BIT 4, E
                case 0x63:
                {
                    int m = bit_num_r_cb(cpu, mmu, 4, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //BIT 4, H
                case 0x64:
                {
                    int m = bit_num_r_cb(cpu, mmu, 4, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //BIT 4, L
                case 0x65:
                {
                    int m = bit_num_r_cb(cpu, mmu, 4, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //BIT 4, (HL)
                case 0x66:
                {
                    int m = bit_num_mem_cb(cpu, mmu, 4, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 3);
                    return m;  
                    break; 
                }

                //BIT 4, A
                case 0x67:
                {
                    int m = bit_num_r_cb(cpu, mmu, 4, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //BIT 5, B
                case 0x68:
                {
                    int m = bit_num_r_cb(cpu, mmu, 5, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //BIT 5, C
                case 0x69:
                {
                    int m = bit_num_r_cb(cpu, mmu, 5, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //BIT 5, D
                case 0x6a:
                {
                    int m = bit_num_r_cb(cpu, mmu, 5, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //BIT 5, E
                case 0x6b:
                {
                    int m = bit_num_r_cb(cpu, mmu, 5, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //BIT 5, H
                case 0x6c:
                {
                    int m = bit_num_r_cb(cpu, mmu, 5, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //BIT 5, L
                case 0x6d:
                {
                    int m = bit_num_r_cb(cpu, mmu, 5, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //BIT 5, (HL)
                case 0x6e:
                {
                    int m = bit_num_mem_cb(cpu, mmu, 5, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 3);  
                    return m;
                    break;
                }

                //BIT 5, A
                case 0x6f:
                {
                    int m = bit_num_r_cb(cpu, mmu, 5, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }

                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************

                //BIT 6, B 
                case 0x70:
                {
                    int m = bit_num_r_cb(cpu, mmu, 6, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //BIT 6, C
                case 0x71:
                {
                    int m = bit_num_r_cb(cpu, mmu, 6, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //BIT 6, D
                case 0x72:
                {
                    int m = bit_num_r_cb(cpu, mmu, 6, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //BIT 6, E
                case 0x73:
                {
                    int m = bit_num_r_cb(cpu, mmu, 6, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //BIT 6, H
                case 0x74:
                {
                    int m = bit_num_r_cb(cpu, mmu, 6, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //BIT 6, L
                case 0x75:
                {
                    int m = bit_num_r_cb(cpu, mmu, 6, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //BIT 6, (HL)
                case 0x76:
                {
                    int m = bit_num_mem_cb(cpu, mmu, 6, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 3);
                    return m;  
                    break; 
                }

                //BIT 6, A
                case 0x77:
                {
                    int m = bit_num_r_cb(cpu, mmu, 6, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //BIT 7, B
                case 0x78:
                {
                    int m = bit_num_r_cb(cpu, mmu, 7, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //BIT 7, C
                case 0x79:
                {
                    int m = bit_num_r_cb(cpu, mmu, 7, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //BIT 7, D
                case 0x7a:
                {
                    int m = bit_num_r_cb(cpu, mmu, 7, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //BIT 7, E
                case 0x7b:
                {
                    int m = bit_num_r_cb(cpu, mmu, 7, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //BIT 7, H
                case 0x7c:
                {
                    int m = bit_num_r_cb(cpu, mmu, 7, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //BIT 7, L
                case 0x7d:
                {
                    int m = bit_num_r_cb(cpu, mmu, 7, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //BIT 7, (HL)
                case 0x7e:
                {
                    int m = bit_num_mem_cb(cpu, mmu, 7, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 3);  
                    return m;
                    break;
                }

                //BIT 7, A
                case 0x7f:
                {
                    int m = bit_num_r_cb(cpu, mmu, 7, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }
                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************
                
                //RES 0, B 
                case 0x80:
                {
                    int m = res_num_r_cb(cpu, mmu, 0, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //RES 0, C
                case 0x81:
                {
                    int m = res_num_r_cb(cpu, mmu, 0, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //RES 0, D
                case 0x82:
                {
                    int m = res_num_r_cb(cpu, mmu, 0, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //RES 0, E
                case 0x83:
                {
                    int m = res_num_r_cb(cpu, mmu, 0, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //RES 0, H
                case 0x84:
                {
                    int m = res_num_r_cb(cpu, mmu, 0, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //RES 0, L
                case 0x85:
                {
                    int m = res_num_r_cb(cpu, mmu, 0, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //RES 0, (HL)
                case 0x86:
                {
                    int m = res_num_mem_cb(cpu, mmu, 0, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 4);
                    return m;  
                    break; 
                }

                //RES 0, A
                case 0x87:
                {
                    int m = res_num_r_cb(cpu, mmu, 0, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //RES 1, B
                case 0x88:
                {
                    int m = res_num_r_cb(cpu, mmu, 1, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //RES 1, C
                case 0x89:
                {
                    int m = res_num_r_cb(cpu, mmu, 1, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //RES 1, D
                case 0x8a:
                {
                    int m = res_num_r_cb(cpu, mmu, 1, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //RES 1, E
                case 0x8b:
                {
                    int m = res_num_r_cb(cpu, mmu, 1, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //RES 1, H
                case 0x8c:
                {
                    int m = res_num_r_cb(cpu, mmu, 1, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //RES 1, L
                case 0x8d:
                {
                    int m = res_num_r_cb(cpu, mmu, 1, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //RES 1, (HL)
                case 0x8e:
                {
                    int m = res_num_mem_cb(cpu, mmu, 1, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 4);  
                    return m;
                    break;
                }

                //RES 1, A
                case 0x8f:
                {
                    int m = res_num_r_cb(cpu, mmu, 1, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }

                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************

                //RES 2, B 
                case 0x90:
                {
                    int m = res_num_r_cb(cpu, mmu, 2, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //RES 2, C
                case 0x91:
                {
                    int m = res_num_r_cb(cpu, mmu, 2, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //RES 2, D
                case 0x92:
                {
                    int m = res_num_r_cb(cpu, mmu, 2, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //RES 2, E
                case 0x93:
                {
                    int m = res_num_r_cb(cpu, mmu, 2, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //RES 2, H
                case 0x94:
                {
                    int m = res_num_r_cb(cpu, mmu, 2, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //RES 2, L
                case 0x95:
                {
                    int m = res_num_r_cb(cpu, mmu, 2, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //RES 2, (HL)
                case 0x96:
                {
                    int m = res_num_mem_cb(cpu, mmu, 2, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 4);
                    return m;  
                    break; 
                }

                //RES 2, A
                case 0x97:
                {
                    int m = res_num_r_cb(cpu, mmu, 2, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //RES 3, B
                case 0x98:
                {
                    int m = res_num_r_cb(cpu, mmu, 3, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //RES 3, C
                case 0x99:
                {
                    int m = res_num_r_cb(cpu, mmu, 3, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //RES 3, D
                case 0x9a:
                {
                    int m = res_num_r_cb(cpu, mmu, 3, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //RES 3, E
                case 0x9b:
                {
                    int m = res_num_r_cb(cpu, mmu, 3, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //RES 3, H
                case 0x9c:
                {
                    int m = res_num_r_cb(cpu, mmu, 3, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //RES 3, L
                case 0x9d:
                {
                    int m = res_num_r_cb(cpu, mmu, 3, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //RES 3, (HL)
                case 0x9e:
                {
                    int m = res_num_mem_cb(cpu, mmu, 3, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 4);  
                    return m;
                    break;
                }

                //RES 3, A
                case 0x9f:
                {
                    int m = res_num_r_cb(cpu, mmu, 3, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }

                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************

                //RES 4, B 
                case 0xa0:
                {
                    int m = res_num_r_cb(cpu, mmu, 4, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //RES 4, C
                case 0xa1:
                {
                    int m = res_num_r_cb(cpu, mmu, 4, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //RES 4, D
                case 0xa2:
                {
                    int m = res_num_r_cb(cpu, mmu, 4, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //RES 4, E
                case 0xa3:
                {
                    int m = res_num_r_cb(cpu, mmu, 4, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //RES 4, H
                case 0xa4:
                {
                    int m = res_num_r_cb(cpu, mmu, 4, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //RES 4, L
                case 0xa5:
                {
                    int m = res_num_r_cb(cpu, mmu, 4, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //RES 4, (HL)
                case 0xa6:
                {
                    int m = res_num_mem_cb(cpu, mmu, 4, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 4);
                    return m;  
                    break; 
                }

                //RES 4, A
                case 0xa7:
                {
                    int m = res_num_r_cb(cpu, mmu, 4, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //RES 5, B
                case 0xa8:
                {
                    int m = res_num_r_cb(cpu, mmu, 5, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //RES 5, C
                case 0xa9:
                {
                    int m = res_num_r_cb(cpu, mmu, 5, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //RES 5, D
                case 0xaa:
                {
                    int m = res_num_r_cb(cpu, mmu, 5, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //RES 5, E
                case 0xab:
                {
                    int m = res_num_r_cb(cpu, mmu, 5, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //RES 5, H
                case 0xac:
                {
                    int m = res_num_r_cb(cpu, mmu, 5, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //RES 5, L
                case 0xad:
                {
                    int m = res_num_r_cb(cpu, mmu, 5, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //RES 5, (HL)
                case 0xae:
                {
                    int m = res_num_mem_cb(cpu, mmu, 5, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 4);  
                    return m;
                    break;
                }

                //RES 5, A
                case 0xaf:
                {
                    int m = res_num_r_cb(cpu, mmu, 5, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }

                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************
                //RES 6, B 
                case 0xb0:
                {
                    int m = res_num_r_cb(cpu, mmu, 6, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //RES 6, C
                case 0xb1:
                {
                    int m = res_num_r_cb(cpu, mmu, 6, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //RES 6, D
                case 0xb2:
                {
                    int m = res_num_r_cb(cpu, mmu, 6, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //RES 6, E
                case 0xb3:
                {
                    int m = res_num_r_cb(cpu, mmu, 6, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //RES 6, H
                case 0xb4:
                {
                    int m = res_num_r_cb(cpu, mmu, 6, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //RES 6, L
                case 0xb5:
                {
                    int m = res_num_r_cb(cpu, mmu, 6, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //RES 6, (HL)
                case 0xb6:
                {
                    int m = res_num_mem_cb(cpu, mmu, 6, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 4);
                    return m;  
                    break; 
                }

                //RES 6, A
                case 0xb7:
                {
                    int m = res_num_r_cb(cpu, mmu, 6, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //RES 7, B
                case 0xb8:
                {
                    int m = res_num_r_cb(cpu, mmu, 7, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //RES 7, C
                case 0xb9:
                {
                    int m = res_num_r_cb(cpu, mmu, 7, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //RES 7, D
                case 0xba:
                {
                    int m = res_num_r_cb(cpu, mmu, 7, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //RES 7, E
                case 0xbb:
                {
                    int m = res_num_r_cb(cpu, mmu, 7, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //RES 7, H
                case 0xbc:
                {
                    int m = res_num_r_cb(cpu, mmu, 7, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //RES 7, L
                case 0xbd:
                {
                    int m = res_num_r_cb(cpu, mmu, 7, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //RES 7, (HL)
                case 0xbe:
                {
                    int m = res_num_mem_cb(cpu, mmu, 7, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 4);  
                    return m;
                    break;
                }

                //RES 7, A
                case 0xbf:
                {
                    int m = res_num_r_cb(cpu, mmu, 7, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }

                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************

                //SET 0, B 
                case 0xc0:
                {
                    int m = set_num_r_cb(cpu, mmu, 0, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //SET 0, C
                case 0xc1:
                {
                    int m = set_num_r_cb(cpu, mmu, 0, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //SET 0, D
                case 0xc2:
                {
                    int m = set_num_r_cb(cpu, mmu, 0, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //SET 0, E
                case 0xc3:
                {
                    int m = set_num_r_cb(cpu, mmu, 0, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //SET 0, H
                case 0xc4:
                {
                    int m = set_num_r_cb(cpu, mmu, 0, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //SET 0, L
                case 0xc5:
                {
                    int m = set_num_r_cb(cpu, mmu, 0, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //SET 0, (HL)
                case 0xc6:
                {
                    int m = set_num_mem_cb(cpu, mmu, 0, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 4);
                    return m;  
                    break; 
                }

                //SET 0, A
                case 0xc7:
                {
                    int m = set_num_r_cb(cpu, mmu, 0, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //SET 1, B
                case 0xc8:
                {
                    int m = set_num_r_cb(cpu, mmu, 1, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //SET 1, C
                case 0xc9:
                {
                    int m = set_num_r_cb(cpu, mmu, 1, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //SET 1, D
                case 0xca:
                {
                    int m = set_num_r_cb(cpu, mmu, 1, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //SET 1, E
                case 0xcb:
                {
                    int m = set_num_r_cb(cpu, mmu, 1, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //SET 1, H
                case 0xcc:
                {
                    int m = set_num_r_cb(cpu, mmu, 1, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //SET 1, L
                case 0xcd:
                {
                    int m = set_num_r_cb(cpu, mmu, 1, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //SET 1, (HL)
                case 0xce:
                {
                    int m = set_num_mem_cb(cpu, mmu, 1, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 4);  
                    return m;
                    break;
                }

                //SET 1, A
                case 0xcf:
                {
                    int m = set_num_r_cb(cpu, mmu, 1, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }

                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************

                //SET 2, B 
                case 0xd0:
                {
                    int m = set_num_r_cb(cpu, mmu, 2, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //SET 2, C
                case 0xd1:
                {
                    int m = set_num_r_cb(cpu, mmu, 2, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //SET 2, D
                case 0xd2:
                {
                    int m = set_num_r_cb(cpu, mmu, 2, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //SET 2, E
                case 0xd3:
                {
                    int m = set_num_r_cb(cpu, mmu, 2, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //SET 2, H
                case 0xd4:
                {
                    int m = set_num_r_cb(cpu, mmu, 2, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //SET 2, L
                case 0xd5:
                {
                    int m = set_num_r_cb(cpu, mmu, 2, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //SET 2, (HL)
                case 0xd6:
                {
                    int m = set_num_mem_cb(cpu, mmu, 2, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 4);
                    return m;  
                    break; 
                }

                //SET 2, A
                case 0xd7:
                {
                    int m = set_num_r_cb(cpu, mmu, 2, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //SET 3, B
                case 0xd8:
                {
                    int m = set_num_r_cb(cpu, mmu, 3, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //SET 3, C
                case 0xd9:
                {
                    int m = set_num_r_cb(cpu, mmu, 3, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //SET 3, D
                case 0xda:
                {
                    int m = set_num_r_cb(cpu, mmu, 3, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //SET 3, E
                case 0xdb:
                {
                    int m = set_num_r_cb(cpu, mmu, 3, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //SET 3, H
                case 0xdc:
                {
                    int m = set_num_r_cb(cpu, mmu, 3, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //SET 3, L
                case 0xdd:
                {
                    int m = set_num_r_cb(cpu, mmu, 3, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //SET 3, (HL)
                case 0xde:
                {
                    int m = set_num_mem_cb(cpu, mmu, 3, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 4);  
                    return m;
                    break;
                }

                //SET 3, A
                case 0xdf:
                {
                    int m = set_num_r_cb(cpu, mmu, 3, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }

                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************

                //SET 4, B 
                case 0xe0:
                {
                    int m = set_num_r_cb(cpu, mmu, 4, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //SET 4, C
                case 0xe1:
                {
                    int m = set_num_r_cb(cpu, mmu, 4, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //SET 4, D
                case 0xe2:
                {
                    int m = set_num_r_cb(cpu, mmu, 4, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //SET 4, E
                case 0xe3:
                {
                    int m = set_num_r_cb(cpu, mmu, 4, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //SET 4, H
                case 0xe4:
                {
                    int m = set_num_r_cb(cpu, mmu, 4, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //SET 4, L
                case 0xe5:
                {
                    int m = set_num_r_cb(cpu, mmu, 4, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //SET 4, (HL)
                case 0xe6:
                {
                    int m = set_num_mem_cb(cpu, mmu, 4, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 4);
                    return m;  
                    break; 
                }

                //SET 4, A
                case 0xe7:
                {
                    int m = set_num_r_cb(cpu, mmu, 4, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //SET 5, B
                case 0xe8:
                {
                    int m = set_num_r_cb(cpu, mmu, 5, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //SET 5, C
                case 0xe9:
                {
                    int m = set_num_r_cb(cpu, mmu, 5, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //SET 5, D
                case 0xea:
                {
                    int m = set_num_r_cb(cpu, mmu, 5, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //SET 5, E
                case 0xeb:
                {
                    int m = set_num_r_cb(cpu, mmu, 5, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //SET 5, H
                case 0xec:
                {
                    int m = set_num_r_cb(cpu, mmu, 5, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //SET 5, L
                case 0xed:
                {
                    int m = set_num_r_cb(cpu, mmu, 5, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //SET 5, (HL)
                case 0xee:
                {
                    int m = set_num_mem_cb(cpu, mmu, 5, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 4);  
                    return m;
                    break;
                }

                //SET 5, A
                case 0xef:
                {
                    int m = set_num_r_cb(cpu, mmu, 5, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }

                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************

                                //SET 6, B 
                case 0xf0:
                {
                    int m = set_num_r_cb(cpu, mmu, 6, cpu->reg.b, (char*)"B");
                    return m; 
                    break;
                }

                //SET 6, C
                case 0xf1:
                {
                    int m = set_num_r_cb(cpu, mmu, 6, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //SET 6, D
                case 0xf2:
                {
                    int m = set_num_r_cb(cpu, mmu, 6, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //SET 6, E
                case 0xf3:
                {
                    int m = set_num_r_cb(cpu, mmu, 6, cpu->reg.e, (char*)"E");
                    return m;   
                    break;
                }

                //SET 6, H
                case 0xf4:
                {
                    int m = set_num_r_cb(cpu, mmu, 6, cpu->reg.h, (char*)"H");
                    return m;   
                    break;
                }

                //SET 6, L
                case 0xf5:
                {
                    int m = set_num_r_cb(cpu, mmu, 6, cpu->reg.l, (char*)"L");
                    return m;   
                    break;
                }

                //SET 6, (HL)
                case 0xf6:
                {
                    int m = set_num_mem_cb(cpu, mmu, 6, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"L", 4);
                    return m;  
                    break; 
                }

                //SET 6, A
                case 0xf7:
                {
                    int m = set_num_r_cb(cpu, mmu, 6, cpu->reg.a, (char*)"A");
                    return m;
                    break;
                }

                //SET 7, B
                case 0xf8:
                {
                    int m = set_num_r_cb(cpu, mmu, 7, cpu->reg.b, (char*)"B");
                    return m;
                    break;
                }

                //SET 7, C
                case 0xf9:
                {
                    int m = set_num_r_cb(cpu, mmu, 7, cpu->reg.c, (char*)"C");
                    return m;
                    break;
                }

                //SET 7, D
                case 0xfa:
                {
                    int m = set_num_r_cb(cpu, mmu, 7, cpu->reg.d, (char*)"D");
                    return m;
                    break;
                }

                //SET 7, E
                case 0xfb:
                {
                    int m = set_num_r_cb(cpu, mmu, 7, cpu->reg.e, (char*)"E");
                    return m;
                    break;
                }

                //SET 7, H
                case 0xfc:
                {
                    int m = set_num_r_cb(cpu, mmu, 7, cpu->reg.h, (char*)"H");
                    return m;
                    break;
                }

                //SET 7, L
                case 0xfd:
                {
                    int m = set_num_r_cb(cpu, mmu, 7, cpu->reg.l, (char*)"L");
                    return m;
                    break;
                }

                //SET 7, (HL)
                case 0xfe:
                {
                    int m = set_num_mem_cb(cpu, mmu, 7, mmu_read_byte(mmu, reg16(cpu->reg.h, cpu->reg.l)), (char*)"(HL)", 4);  
                    return m;
                    break;
                }

                //SET 7, A
                case 0xff:
                {
                    int m = set_num_r_cb(cpu, mmu, 7, cpu->reg.a, (char*)"A");
                    return m; 
                    break;
                }

                //************************************************************************************************************************************************************************//************************************************************************************************************************************************************************

                default:
			        printf("Unsupported CB-prefixed opcode: 0x%02x at 0x%04x \n\n\n", mmu_read_byte(mmu, cpu->reg.pc + 1), cpu->reg.pc);
                    exit(EXIT_FAILURE);
			        break;
            }

            return 0;
        }

        //CALL Z,u16
        case 0xcc:
        {
            int m = call_z_nn(cpu, mmu, mmu_read_word(mmu, cpu->reg.pc + 1));
            return m;
            break;
        }

        //CALL u16
        case 0xcd:
        {
            int m = call_nn(cpu, mmu, (mmu_read_byte(mmu, cpu->reg.pc + 2) << 8 | mmu_read_byte(mmu, cpu->reg.pc + 1)));

            return m;
            break;
        }

        //ADC A,u8
        case 0xce:
        {
            int m = adc_r(cpu, mmu, cpu->reg.a, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*)"u8", 2, 2);
            return m;
            break;
        }

        //RST 00h
        case 0xcf:
        {
            int m =  rst(cpu, mmu, 0x08);
            return m;
            break;
        }
//************************************************************************************************************************************************************************

        //RET NC
        case 0xd0:
        {
            int m = ret_nc(cpu, mmu);
            return m;
            break;
        }

        //POP DE
        case 0xd1:
        {
            int m = pop_rr(cpu, mmu, cpu->reg.sp, cpu->reg.d, cpu->reg.e, (char*) "DE");
            return m;
            break;
        }

        //JP NC,u16
        case 0xd2:
        {
            int m = jp_nc_nn(cpu, mmu_read_word(mmu, cpu->reg.pc + 1));
            return m;
            break;
        }

        //CALL NC,u16
        case 0xd4:
        {
            int m = call_nc_nn(cpu, mmu, mmu_read_word(mmu, cpu->reg.pc + 1));
            return m;
            break;
        }

        //PUSH DE
        case 0xd5:
        {
            int m = push_rr(cpu, mmu, reg16(cpu->reg.d, cpu->reg.e), (char*) "DE");
            return m;
            break;
        }

        //SUB A,u8
        case 0xd6:
        {
            int m = sub_r(cpu, mmu, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*)"u8", 2 ,2);

            return m;
            break;
        }

        //RST 10h
        case 0xd7:
        {
            int m =  rst(cpu, mmu, 0x10);
            return m;
            break;
        }

        //RET C
        case 0xd8:
        {
            int m = ret_c(cpu, mmu);
            return m;
            break;
        }

        //RETI
        case 0xd9:
        {
            int m = reti(cpu,mmu);
            return m;
            break;
        }

        //JP C,u16
        case 0xda:
        {
            int m = jp_c_nn(cpu, mmu_read_word(mmu, cpu->reg.pc +1));
            return m;
            break;
        }

        //CALL c,u16
        case 0xdc:
        {
            int m = call_c_nn(cpu, mmu, mmu_read_word(mmu, cpu->reg.pc + 1));
            return m;
            break;
        }

        //SBC A,u8
        case 0xde:
        {
            int m = sbc_r(cpu, mmu, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*)"u8", 2, 2);
            return m;
            break;
        }

        //RST 18h
        case 0xdf:
        {
            int m =  rst(cpu, mmu, 0x18);
            return m;
            break;
        }

//************************************************************************************************************************************************************************

        //LD (FF00+u8),A -> load this section of memory with what is in register 'a'
        case 0xe0:
        { 
            int m = ld_mem_u8(cpu, mmu, (0xff00 + mmu_read_byte(mmu, cpu->reg.pc+1)), cpu->reg.a, (char*)"(FF00+u8)", (char*)"A" );

            return m;
            break;
        }

        //POP HL
        case 0xe1:
        {
            int m = pop_rr(cpu, mmu, cpu->reg.sp, cpu->reg.h, cpu->reg.l, (char*) "HL");
            return m;
            break;
        }

        //LD (FF00+C),A -> load this section of memory with what is in register 'a'
        case 0xe2:
        { 
            int m = ld_mem_r(cpu, mmu, (0xff00 + cpu->reg.c), cpu->reg.a, (char*)"(FF00+C)", (char*)"A" );

            return m;
            break;
        }

        //PUSH HL
        case 0xe5:
        {
            int m = push_rr(cpu, mmu, reg16(cpu->reg.h, cpu->reg.l), (char*) "HL");
            return m;
            break;
        }
        
        //AND A,u8
        case 0xe6:
        {
            int m = and_r(cpu, mmu, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*) "u8", 2, 2);
            return m;
            break;
        }
        
        //RST 20h
        case 0xe7:
        {
            int m =  rst(cpu, mmu, 0x20);
            return m;
            break;
        }

        //ADD SP,i8
        case 0xe8:
        {
            int m = add_sp(cpu, mmu, cpu->reg.sp, mmu_read_byte(mmu, cpu->reg.pc + 1));
            return m;
            break;
        }

        case 0xe9:
        {
            uint16_t reg_hl = reg16(cpu->reg.h, cpu->reg.l);
            cpu->reg.pc = reg_hl;
            return 1;
            break;
        }

        //LD (u16),A
        case 0xea:
        {
            uint16_t reg16_hl = reg16(cpu->reg.h, cpu->reg.l);
            int step = 0;
            int m = ld_mem_r(cpu, mmu,mmu_read_word(mmu, cpu->reg.pc + 1) , cpu->reg.a, (char*) "(u16)", (char*) "A", m = 4, step = 3);

            return m;
            break;
        }

        //XOR A,u8
        case 0xee:
        {
            int m = xor_r(cpu, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*)"u8", 2, 2);
            return m;
            break;
        }

        //RST 28h
        case 0xef:
        {
            int m =  rst(cpu, mmu, 0x28);
            return m;
            break;
        }

//*****************************************************************************************************************************************************************************
        //LD A,(FF00+u8)
        case 0xf0:
        { 
            int m = ld_a_mem(cpu, mmu, cpu->reg.a, 0xff00 + mmu_read_byte(mmu,cpu->reg.pc+1), (char*) "A", (char*) "(FF00+U8)", 3, 2);
            //printf("\naddr8 pc+: %02x\n", mmu_read_byte(mmu, 0xff00));
            return m;
            break;
        }

        //POP AF
        case 0xf1:
        {
            //printf("\nPOP AF \n");
            cpu_flag_set_zero(cpu, (mmu_read_byte(mmu, cpu->reg.sp) >> 7) & 0x1);
            cpu_flag_set_sub(cpu, (mmu_read_byte(mmu, cpu->reg.sp) >> 6) & 0x1);
            cpu_flag_set_halfcarry(cpu, (mmu_read_byte(mmu, cpu->reg.sp) >> 5) & 0x1);
            cpu_flag_set_carry(cpu, (mmu_read_byte(mmu, cpu->reg.sp) >> 4) & 0x1);
            cpu->reg.sp++;
            cpu->reg.a = mmu_read_byte(mmu, cpu->reg.sp);
            cpu->reg.sp++;
            cpu->reg.pc++;
            return 3;
            break;
        }


        //LD A,(FF00+C)
        case 0xf2:
        { 
            int m = ld_r_r(cpu, mmu, cpu->reg.a, mmu_read_byte(mmu, 0xff00 + cpu->reg.c), (char*) "A", (char*) "(FF00+C)", 2, 1);
            //printf("\naddr8 pc+: %02x\n", mmu_read_byte(mmu, 0xff00));
            return m;
            break;
        }

        //DI - 0xF3
        case 0xf3:
        {
            int m = disable_int(cpu);
            return m;
            break;
        }

        //PUSH AF
        case 0xf5:
        {
            int m = push_rr(cpu, mmu, reg16(cpu->reg.a, cpu->reg.f), (char*) "AF");
            return m;
            break;
        }

        //OR A,u8
        case 0xf6:
        {
            int m = or_r(cpu, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*) "u8", 2, 2);
            return m;
            break;
        }

        //RST 30h
        case 0xf7:
        {
            int m = rst(cpu, mmu, 0x30);
            return m;
            break;
        }

        //LD HL,SP+i8
        case 0xf8:
        {
            int m = ld_hl_sp(cpu, mmu, cpu->reg.sp, mmu_read_byte(mmu, cpu->reg.pc + 1));
            return m;
            break;
        }

        case 0xf9:
        {
            uint16_t reg_hl = reg16(cpu->reg.h, cpu->reg.l);
            cpu->reg.sp = reg_hl;
            cpu->reg.pc++;
            return 2;
            break;
        }

        //LD A,(u16)
        case 0xfa:
        {
            int m = ld_a_mem(cpu, mmu, cpu->reg.a,(mmu_read_byte(mmu, cpu->reg.pc+2) << 8) | mmu_read_byte(mmu, cpu->reg.pc+1), (char*) "A", (char*) "(u16)", 4, 3);

            return m;
            break;
        }

        case 0xfb:
        {
            int m =  enable_int(cpu);
            return m;
            break;
        }

        //CP A,u8
        case 0x0fe:
        {
            int m = cp_r(cpu, mmu_read_byte(mmu, cpu->reg.pc + 1), (char*) "u8");

            return m;
            break;
        }

        //RST 38h
        case 0xff:
        {
            int m =  rst(cpu, mmu, 0x38);
            return m;
            break;
        }
//************************************************************************************************************************************************************************
        default:
	    	printf("Unsupported opcode: 0x%02x at 0x%04x, \n\n\n", mmu_read_byte(mmu, cpu->reg.pc), cpu->reg.pc) ;
	    	exit(EXIT_FAILURE);
            cpu->reg.pc++;
    		return 0;
	    	break;
	}
}
/*
int main()
{
    cpu_t *cpu;
    mmu_t* mmu = mmu_create();
	mmu_load_bios(mmu);
    //09-op r, r.gb
    cartridge_t* cart = cartridge_load("/Users/Matt1/Desktop/UNCA_SPRING_2023/CSCI_481/Gameboy_Emulator/ROMS/gb-test-roms-master/cpu_instrs/individual/11-op a,(hl).gb");
    mmu_load_rom(mmu, cart);
    //mmu_write_byte(mmu, 0xff44, 0);
    cartridge_free(cart);
 
    cpu->reg.pc = 0x0100;
    //cpu->reg.sp = 0x0000;
    cpu->reg.a = 0x01;
    cpu->reg.f = 0xb0;
    cpu->reg.c = 0x13;
    cpu->reg.e = 0xd8;
    cpu->reg.h = 0x01;
    cpu->reg.l = 0x4d;
    cpu->reg.sp = 0xfffe;


    while(1)
    {
        stepCPU(cpu, mmu);
    }
    return 0;
}
*/