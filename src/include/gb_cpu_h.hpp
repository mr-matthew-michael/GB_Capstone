#include <stdio.h>
#include <stdlib.h>

#ifndef GB_DMG_CPU_H
#define GB_DMG_CPU_H

// Flag bits
#define CPU_FLAG_ZERO_BIT    7
#define CPU_FLAG_SUB_BIT     6
#define CPU_FLAG_HC_BIT      5
#define CPU_FLAG_CARRY_BIT   4

// Interrupts
#define CPU_INT_VBLANK      0x01
#define CPU_INT_LCD         (0x01 << 1)
#define CPU_INT_TIMER       (0x01 << 2)
#define CPU_INT_SERIAL      (0x01 << 3)
#define CPU_INT_JOYPAD      (0x01 << 4)

typedef struct _ppu_t ppu_t;
typedef struct _mmu_t mmu_t;

typedef enum _condition_e
{
    CPU_CONDITION_C,
    CPU_CONDITION_NC,
    CPU_CONDITION_Z,
    CPU_CONDITION_NZ,
    CPU_CONDITION_ALWAYS
} condition_e;

typedef struct clock_t
{
    uint32_t m;
    uint32_t t;
} clock_t;

typedef struct flag_t
{
    uint8_t z; 
    uint8_t n; 
    uint8_t h; 
    uint8_t c; 
    bool HALT;
} flag_t;

typedef union _reg16_t
{
    struct
    {
        uint8_t lo;
        uint8_t hi;
    };
    uint16_t word;
} reg16_t;

typedef struct _cpu_t
{    
    struct
    {
        uint8_t a, f, b, c, d, e, h, l;
        uint16_t sp, pc;
    } reg;

    bool ime;
    bool halt;

    uint16_t currop;
    uint16_t curropaddr;
} cpu_t;


int stepCPU(cpu_t *cpu, mmu_t *mmu);
void printDebug(cpu_t* cpu, mmu_t *mmu);

#endif
