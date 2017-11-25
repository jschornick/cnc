// File       : tmc.c
// Author     : Jeff Schornick
//
// Functions to control the Trinamic TMC-26x series
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include "spi.h"
#include "tmc.h"

uint32_t tmc_drvconf;
uint32_t tmc_drvctl;
uint32_t tmc_chopconf;
uint32_t tmc_sgcsconf;
uint32_t tmc_smarten;

void tmc_init(void)
{

  tmc_drvconf = DRVCONF
    | DRVCONF_VSENSE_305
    | DRVCONF_SDOFF_STEP;

  tmc_drvctl = DRVCTL
    | DRVCTL_MRES_2
    | DRVCTL_DEDGE_BOTH
    | DRVCTL_INTPOL_OFF;

  tmc_chopconf = CHOPCONF
    | CHOPCONF_CHM_CONST
    | CHOPCONF_TOFF(8);

  // sense: 0.07 Ohms
  // I_rms = (CS+1)/32 * (V_FS/R_sense) * 1/sqrt(2)
  tmc_sgcsconf = SGCSCONF
    | SGCSCONF_CS(15);

  tmc_smarten = SMARTEN;

  spi1_send(tmc_drvconf);
  spi1_send(tmc_drvctl);
  spi1_send(tmc_chopconf);
  spi1_send(tmc_sgcsconf);

}
