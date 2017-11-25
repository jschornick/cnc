// File       : tmc.h
// Author     : Jeff Schornick
//
// Trinamic TMC macro definitions for the TMC26x series.
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#ifndef _TMC_H
#define _TMC_H

#include <stdint.h>

extern uint32_t tmc_drvconf;
extern uint32_t tmc_drvctl;
extern uint32_t tmc_chopconf;
extern uint32_t tmc_sgcsconf;

#define TMC_CLK  15000000L   /* 15MHz internal clock */

// DRVCONF : Driver Configuration Register

#define DRVCONF              0xE0000

#define DRVCONF_RDSEL_MASK   0x00030

#define DRVCONF_RDSEL_USTEP  0x00000
#define DRVCONF_RDSEL_SG     0x00010
#define DRVCONF_RDSEL_SGCS   0x00020

#define DRVCONF_VSENSE_305   0x00000
#define DRVCONF_VSENSE_165   0x00040

#define DRVCONF_SDOFF_STEP   0x00000
#define DRVCONF_SDOFF_NOSTEP 0x00080

#define DRVCONF_TS2G_32      0x00000
#define DRVCONF_TS2G_16      0x00100
#define DRVCONF_TS2G_12      0x00200
#define DRVCONF_TS2G_08      0x00300

#define DRVCONF_DISS2G_DIS   0x00000
#define DRVCONF_DISS2G_EN    0x00400

#define DRVCONF_SLPL_MIN     0x00000
#define DRVCONF_SLPL_MED     0x02000
#define DRVCONF_SLPL_MAX     0x03000

#define DRVCONF_SLPH_MIN     0x00000
#define DRVCONF_SLPH_MED     0x08000
#define DRVCONF_SLPH_MAX     0x0C000


// DRVCTL : Driver Control Register
// Values depend on step mode set in DRVCONF!
#define DRVCTL 0x00000

// DRVCTL Step mode
#define DRVCTL_MRES_256  0x00000
#define DRVCTL_MRES_128  0x00001
#define DRVCTL_MRES_64   0x00002
#define DRVCTL_MRES_32   0x00003
#define DRVCTL_MRES_16   0x00004
#define DRVCTL_MRES_8    0x00005
#define DRVCTL_MRES_4    0x00006
#define DRVCTL_MRES_2    0x00007  /* half stepping */
#define DRVCTL_MRES_1    0x00008  /* full stepping */

#define DRVCTL_DEDGE_RISE 0x00000
#define DRVCTL_DEDGE_BOTH 0x00100

#define DRVCTL_INTPOL_OFF 0x00000
#define DRVCTL_INTPOL_ON  0x00200


// CHOPCONF : Chopper configuration
#define CHOPCONF 0x80000

#define CHOPCONF_TBL_16 0x00000
#define CHOPCONF_TBL_24 0x08000
#define CHOPCONF_TBL_36 0x10000
#define CHOPCONF_TBL_54 0x18000

#define CHOPCONF_CHM_STD   0x00000
#define CHOPCONF_CHM_CONST 0x04000

#define CHOPCONF_RNDTF_FIXED 0x00000
#define CHOPCONF_RNDTF_RAND  0x02000

// HDEC
// HEND
// HSTRT

#define CHOPCONF_TOFF_DISABLE 0x00000
#define CHOPCONF_TOFF(x) x  /* decay time = 12 + 32*TOFF clock period */


// SGCSCONF : StallGuard2 Control
#define SGCSCONF 0xC0000

#define SGCSCONF_SFILT_OFF 0x00000
#define SGCSCONF_SFILT_ON  0x10000

#define SGCSCONF_SGT(x) (x<<8)
#define SGCSCONF_CS(x) x

// SMARTEN:
#define SMARTEN 0xA0000


typedef struct {
  uint32_t SG   : 1;
  uint32_t OT   : 1;
  uint32_t OTPW : 1;
  uint32_t S2GA : 1;
  uint32_t S2GB : 1;
  uint32_t OLA  : 1;
  uint32_t OLB  : 1;
  uint32_t STST : 1;
  uint32_t RESRVED8 : 2;
  uint32_t MSTEP : 9;
  uint32_t POL   : 1;
} ustep_resp_t;

typedef struct {
  uint32_t SG   : 1;
  uint32_t OT   : 1;
  uint32_t OTPW : 1;
  uint32_t S2GA : 1;
  uint32_t S2GB : 1;
  uint32_t OLA  : 1;
  uint32_t OLB  : 1;
  uint32_t STST : 1;
  uint32_t RESRVED8 : 2;
  uint32_t SG10 : 10;
} sg_resp_t;

typedef struct {
  uint32_t SG   : 1;
  uint32_t OT   : 1;
  uint32_t OTPW : 1;
  uint32_t S2GA : 1;
  uint32_t S2GB : 1;
  uint32_t OLA  : 1;
  uint32_t OLB  : 1;
  uint32_t STST : 1;
  uint32_t RESRVED8 : 2;
  uint32_t SE5 : 5;
  uint32_t SG5 : 5;
} sgcs_resp_t;

void tmc_init(void);

#endif /* _TMC_H */
