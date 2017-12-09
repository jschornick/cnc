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
#include "msp432p401r.h"
#include "gpio.h"

// The selected stepper controller
extern uint8_t tmc;

#define TMC_CLK  15000000L   /* 15MHz internal clock */

#define TMC_POLARITY_NORMAL 1
#define TMC_POLARITY_INVERT 0
extern uint8_t tmc_axis_conf[];

#define TMC_FWD 1
#define TMC_REV 0

// TMC0 (X axis)
#define TMC0_CS_PORT P7
#define TMC0_CS_PIN  PIN2

#define TMC0_EN_PORT P7
#define TMC0_EN_PIN  PIN1

#define TMC0_STEP_PORT P7
#define TMC0_STEP_PIN  PIN0

#define TMC0_DIR_PORT P9
#define TMC0_DIR_PIN  PIN4

// TMC1 (Y axis)
#define TMC1_CS_PORT P7
#define TMC1_CS_PIN  PIN5

#define TMC1_EN_PORT P7
#define TMC1_EN_PIN  PIN4

#define TMC1_STEP_PORT P7
#define TMC1_STEP_PIN  PIN7

#define TMC1_DIR_PORT P7
#define TMC1_DIR_PIN  PIN6

// TMC2 (Z axis)
#define TMC2_CS_PORT P3
#define TMC2_CS_PIN  PIN5

#define TMC2_EN_PORT P5
#define TMC2_EN_PIN  PIN2

#define TMC2_STEP_PORT P3
#define TMC2_STEP_PIN  PIN7

#define TMC2_DIR_PORT P3
#define TMC2_DIR_PIN  PIN6

typedef struct {
  DIO_PORT_Odd_Interruptable_Type *cs_port;
  uint8_t cs_pin;
  DIO_PORT_Odd_Interruptable_Type *en_port;
  uint8_t en_pin;
  DIO_PORT_Odd_Interruptable_Type *step_port;
  uint8_t step_pin;
  DIO_PORT_Odd_Interruptable_Type *dir_port;
  uint8_t dir_pin;
} tmc_pinout_t;


extern tmc_pinout_t tmc_pins[];


/////////////////////////////////////////
// DRVCONF register, structure definition

#define TMC_RDSEL_USTEP  0x0
#define TMC_RDSEL_SG     0x1
#define TMC_RDSEL_SGCS   0x2

#define TMC_VSENSE_LOW  0x0  /* Peak sense input: 290 - 330mV */
#define TMC_VSENSE_HIGH 0x1  /*                   153 - 180mV */

#define TMC_SD_ON     0x0  /* step/dir interface enabled */
#define TMC_SD_OFF    0x1

#define TMC_TS2G_32   0x0  /* short-to-ground detection, 3.2us (slow) */
#define TMC_TS2G_16   0x1
#define TMC_TS2G_12   0x2
#define TMC_TS2G_08   0x3

#define TMC_S2G_ON    0x0  /* short-to-ground not disabled */
#define TMC_S2G_OFF   0x1

#define TMC_SLP_MIN    0x0  /* slope control */
#define TMC_SLP_MIN_TC 0x1  /* NB: temperature compensation only affects high-side slope control */
#define TMC_SLP_MED    0x2
#define TMC_SLP_MAX    0x3

typedef union {
  struct {
    uint32_t RESERVED_0 : 4;
    uint32_t RDSEL      : 2;
    uint32_t VSENSE     : 1;
    uint32_t SDOFF      : 1;
    uint32_t TS2G       : 2;
    uint32_t DISS2G     : 1;
    uint32_t RESERVED_11: 1;
    uint32_t SLPL       : 2;
    uint32_t SLPH       : 2;
    uint32_t TST        : 1;
    uint32_t ADDR       : 3;  // 111b
  };
  uint32_t raw;
} drvconf_t;


/////////////////////////////////////////
// DRVCTL register, structure definition
// NOTE: step/dir mode only

#define TMC_MRES_256  0x0
#define TMC_MRES_128  0x1
#define TMC_MRES_64   0x2
#define TMC_MRES_32   0x3
#define TMC_MRES_16   0x4
#define TMC_MRES_8    0x5
#define TMC_MRES_4    0x6
#define TMC_MRES_2    0x7  /* half stepping */
#define TMC_MRES_1    0x8  /* full stepping */

#define TMC_RISING_EDGE 0x0
#define TMC_BOTH_EDGES  0x1  /* Do not use with INTPOL if step does not have 50% duty cycle! */

#define TMC_INTPOL_OFF 0x0
#define TMC_INTPOL_ON  0x1

typedef union {
  struct {
    uint32_t MRES       : 4;
    uint32_t RESERVED_4 : 4;
    uint32_t DEDGE      : 1;
    uint32_t INTPOL     : 1;
    uint32_t RESERVED_10: 8;
    uint32_t ADDR       : 2;  // 00b
  };
  uint32_t raw;
} drvctl_t;



/////////////////////////////////////////
// CHOPCONF register, structure definition
//
// Chopper configuration

#define TMC_CHOP_TBL_16 0x0  /* Blanking (sense ignore after switch) during, 16 clocks */
#define TMC_CHOP_TBL_24 0x1
#define TMC_CHOP_TBL_36 0x2
#define TMC_CHOP_TBL_54 0x3

#define TMC_CHOP_SPREAD  0x0  /* standard (spreadCycle) mode */
#define TMC_CHOP_CONST   0x1  /* constant t_off w/fast decay */

#define TMC_CHOP_FIXED_TOFF 0x0
#define TMC_CHOP_RAND_TOFF  0x1

typedef union {
  struct {
    uint32_t TOFF       : 4;
    uint32_t HSTRT      : 3;
    uint32_t HEND       : 4;  // STD mode: hysteresis end value, CONST mode: sine offset
    uint32_t HDEC       : 2;
    uint32_t RNDTF      : 1;
    uint32_t CHM        : 1;
    uint32_t TBL        : 2;
    uint32_t ADDR       : 3;  // 100b
  };
  uint32_t raw;
} chopconf_t;


/////////////////////////////////////////
// SGCSCONF register, structure definition
//
// StallGuard2 configuration
//
// Current scaling:
//   R_sense = 0.07 Ohms
//   I_rms = (CSCALE+1)/32 * (V_FS/R_sense) * 1/sqrt(2)

#define TMC_SFILT_OFF 0x0
#define TMC_SFILT_ON  0x1

typedef union {
  struct {
    uint32_t CSCALE     : 5;  // current scaling, [1/32, 32/32]
    uint32_t RESERVED_5 : 3;
    int32_t  SGT        : 7;  // stallGuard threshold, signed value [-64, +63]
    uint32_t RESERVED_15: 1;
    uint32_t SFILT      : 1;  // filtering mode
    uint32_t ADDR       : 3;  // 110b
  };
  uint32_t raw;
} sgcsconf_t;


/////////////////////////////////////////
// SMARTEN register, structure definition
//
// SmartEnergy (coolStep) configuration

#define TMC_SE_DISABLED 0x0

#define TMC_SE_UP1      0x0
#define TMC_SE_UP2      0x1
#define TMC_SE_UP4      0x2
#define TMC_SE_UP8      0x3

#define TMC_SE_DOWN32   0x0
#define TMC_SE_DOWN8    0x1
#define TMC_SE_DOWN2    0x2
#define TMC_SE_DOWN1    0x3

#define TMC_SE_MIN_1_2  0x0  /* minimum is 1/2 of current scale */
#define TMC_SE_MIN_1_4  0x1  /* minimum is 1/4 of current scale */

typedef union {
  struct {
    uint32_t SEMIN      : 4;  // NB: coolStep disabled when 0
    uint32_t RESERVED_4 : 1;
    uint32_t SEUP       : 2;  // Number of steps per increment
    uint32_t RESERVED_7 : 1;
    uint32_t SEMAX      : 4;
    uint32_t RESERVED_12: 1;
    uint32_t SEDN       : 2;  // max current samples before decrement
    uint32_t SEIMIN     : 1;  // minimum current
    uint32_t RESERVED_16: 1;
    uint32_t ADDR       : 3;  // 101b
  };
  uint32_t raw;
} smarten_t;


typedef struct {
  drvconf_t drvconf;
  drvctl_t drvctl;
  chopconf_t chopconf;
  sgcsconf_t sgcsconf;
  smarten_t smarten;
} tmc_config_t;

extern tmc_config_t tmc_config[];


/////////////////////////////////////////////
// TMC 20-bit response, structure definition
//
// Response decoding type depends on RDSEL setting

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

typedef union {
  ustep_resp_t ustep;
  sg_resp_t sg;
  sgcs_resp_t sgcs;
} tmc_response_t;


////////////////////////////////////////////////////////////////
// Register definitions, bitmask style

// DRVCONF : Driver Configuration Register

#define DRVCONF              0xE0000

#define DRVCONF_RDSEL_MASK   0x00030

#define DRVCONF_RDSEL_USTEP  0x00000
#define DRVCONF_RDSEL_SG     0x00010
#define DRVCONF_RDSEL_SGCS   0x00020
#define DRVCONF_RDSEL_MASK   0x00030

#define DRVCONF_VSENSE_LOW   0x00000
#define DRVCONF_VSENSE_HIGH  0x00040

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
#define DRVCTL_MRES_MASK 0x0000f

#define DRVCTL_DEDGE_RISE 0x00000
#define DRVCTL_DEDGE_BOTH 0x00100  /* Do not use with INTPOL if step does not have 50% duty cycle! */

#define DRVCTL_INTPOL_OFF 0x00000
#define DRVCTL_INTPOL_ON  0x00200


// CHOPCONF : Chopper configuration
#define CHOPCONF 0x80000

#define CHOPCONF_TBL_16 0x00000
#define CHOPCONF_TBL_24 0x08000
#define CHOPCONF_TBL_36 0x10000
#define CHOPCONF_TBL_54 0x18000

#define CHOPCONF_CHM_SPREAD 0x00000
#define CHOPCONF_CHM_CONST  0x04000

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
#define SGCSCONF_CS_MASK 0x1f

// SMARTEN:
#define SMARTEN 0xA0000


////////////////////////////////////////////////
// Function declarations

void tmc_init(void);
uint32_t tmc_send(uint8_t tmc, uint32_t tx_data);
void tmc_reconfigure(uint8_t tmc, tmc_config_t *config);

uint8_t tmc_get_current_scale(uint8_t tmc);
void tmc_set_current_scale(uint8_t tmc, uint32_t value);

uint8_t tmc_get_microstep(uint8_t tmc);
void tmc_set_microstep(uint8_t tmc, uint32_t value);

void tmc_set_dir(uint8_t tmc, uint8_t dir);
int8_t tmc_get_dir(uint8_t tmc);

#endif /* _TMC_H */
