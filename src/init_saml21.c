#include "uf2.h"

#define NVM_DFLL_COARSE_POS 26

volatile bool g_interrupt_enabled = true;


/**
 * \brief Set performance level
 */
static inline void _set_performance_level(const uint8_t level)
{
	PM->INTFLAG.reg = 0xFF;
    PM->PLCFG.reg = (PM->PLCFG.reg & ~PM_PLCFG_PLSEL_Msk) | PM_PLCFG_PLSEL(level);
	while (!PM->INTFLAG.reg) ;
}

void system_init(void) {
    uint32_t calib = 0;

    NVMCTRL->CTRLB.bit.RWS = 2; // Data says 2 Wait states at 48Mhz

    // Set Performance Level to 2. (High Performance)
    _set_performance_level(2);

#if defined(CRYSTALLESS)

    /********** 32khz HIGH ACCURACY INTERNAL OSCILLATOR ********/

    /* OSC32K calibration value at bit 12:6 of memory 0x00806020 */
    calib = (*((uint32_t *)NVMCTRL_OTP5) & 0x0001FC0) >> 6;

    OSC32KCTRL->OSC32K.reg = 
	    OSC32KCTRL_OSC32K_CALIB(calib) |
        OSC32KCTRL_OSC32K_STARTUP(0) | // 92us
        (1 << OSC32KCTRL_OSC32K_ONDEMAND_Pos) |
        (1 << OSC32KCTRL_OSC32K_ENABLE_Pos);

    /********** 32khz ULTRA LOW POWER INTERNAL OSCILLATOR ********/
    /* Already running and calibrated at reset. */
    /* Provides 1Khz Clock to RTC */

    /********** 16Mhz INTERNAL OSCILLATOR ************************/
	OSCCTRL->OSC16MCTRL.reg = (1 << OSCCTRL_OSC16MCTRL_ONDEMAND_Pos) |
	                          (1 << OSCCTRL_OSC16MCTRL_ENABLE_Pos) |
	                          OSCCTRL_OSC16MCTRL_FSEL(OSCCTRL_OSC16MCTRL_FSEL_16_Val);

	while (!(OSCCTRL->STATUS.reg & OSCCTRL_STATUS_OSC16MRDY)) ;

    /********** MASTER CLOCK GENERATOR ***************************/
    /* Backup Power Mode = MCLK / 8 */
    MCLK->BUPDIV.reg = MCLK_BUPDIV_BUPDIV(MCLK_BUPDIV_BUPDIV_DIV8_Val);
    /* Low Power Mode = MCLK / 4 */
	MCLK->LPDIV.reg = MCLK_LPDIV_LPDIV(MCLK_LPDIV_LPDIV_DIV4_Val);
    /* HIGH Power Mode = MCLK / 1 */
	MCLK->CPUDIV.reg = MCLK_CPUDIV_CPUDIV(MCLK_CPUDIV_CPUDIV_DIV1_Val);
    /* We set these because we may want to power down to save battery */
    /* while waiting for a USB connection */

    /********** GCLK + PLL INIT **********************************/
    /* Set up GCLK 7 First - Internal 32Khz Oscillator (Used by DPLL) */
    GCLK->GENCTRL[7].reg = 
            GCLK_GENCTRL_DIV(1) | 
            (1 << GCLK_GENCTRL_GENEN_Pos) | 
            GCLK_GENCTRL_SRC_OSC32K;

    /* Set up DFLL48M */
    calib = *((uint32_t *)(NVMCTRL_OTP5)) >> NVM_DFLL_COARSE_POS;

	OSCCTRL->DFLLCTRL.reg = OSCCTRL_DFLLCTRL_ENABLE;
	while (!(OSCCTRL->STATUS.reg & OSCCTRL_STATUS_DFLLRDY)) ;

    OSCCTRL->DFLLMUL.reg = OSCCTRL_DFLLMUL_CSTEP(7) | 
                           OSCCTRL_DFLLMUL_FSTEP(10) |
	                       OSCCTRL_DFLLMUL_MUL(48000);
	while (!(OSCCTRL->STATUS.reg & OSCCTRL_STATUS_DFLLRDY)) ;

	/* FINE is set to fixed value, which defined by DFLL48M Characteristics */
	OSCCTRL->DFLLVAL.reg = OSCCTRL_DFLLVAL_COARSE(calib) | 
                           OSCCTRL_DFLLVAL_FINE(512);

	OSCCTRL->DFLLCTRL.reg = 
	      (1 << OSCCTRL_DFLLCTRL_CCDIS_Pos) |
	      (1 << OSCCTRL_DFLLCTRL_USBCRM_Pos) |
	      (1 << OSCCTRL_DFLLCTRL_MODE_Pos) | 
          (1 << OSCCTRL_DFLLCTRL_ENABLE_Pos);

    uint32_t status_mask = OSCCTRL_STATUS_DFLLRDY | OSCCTRL_STATUS_DFLLLCKC;
	while ((OSCCTRL->STATUS.reg & status_mask) != status_mask) ;

	OSCCTRL->DFLLCTRL.reg |= OSCCTRL_DFLLCTRL_ONDEMAND;
	while (GCLK->SYNCBUSY.reg) ;

    /* GCLK 0 - 48Mhz - From DFLL48M */
    GCLK->GENCTRL[0].reg = 
	        GCLK_GENCTRL_DIV(1) | 
	        (1 << GCLK_GENCTRL_GENEN_Pos) | 
            GCLK_GENCTRL_SRC_DFLL48M;

    /* GCLK 2 - 8Mhz - From Internal 16Mhz Oscillator */
    GCLK->GENCTRL[2].reg = 
	        GCLK_GENCTRL_DIV(2) | 
	        (1 << GCLK_GENCTRL_GENEN_Pos) | 
            GCLK_GENCTRL_SRC_OSC16M;

    /* All other GCLKs 1,3,4,5,6,8 UNUSED */

#else
    #error "SAML21 with a Crystal NOT Implemented"
#endif

}


