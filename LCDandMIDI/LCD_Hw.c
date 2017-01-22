// ============================================================================
// Copyright (c) 2013 by Terasic Technologies Inc.
// ============================================================================
//
// Permission:
//
//   Terasic grants permission to use and modify this code for use
//   in synthesis for all Terasic Development Boards and Altera Development 
//   Kits made by Terasic.  Other use of this code, including the selling 
//   ,duplication, or modification of any portion is strictly prohibited.
//
// Disclaimer:
//
//   This VHDL/Verilog or C/C++ source code is intended as a design reference
//   which illustrates how these types of functions can be implemented.
//   It is the user's responsibility to verify their design for
//   consistency and functionality through the use of formal
//   verification methods.  Terasic provides no warranty regarding the use 
//   or functionality of this code.
//
// ============================================================================
//           
//  Terasic Technologies Inc
//  9F., No.176, Sec.2, Gongdao 5th Rd, East Dist, Hsinchu City, 30070. Taiwan
//  
//  
//                     web: http://www.terasic.com/  
//                     email: support@terasic.com
//
// ============================================================================




#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdarg.h> 
#include "LCD_HW.h"
#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"
#include "socal/alt_spim.h"
#include "socal/alt_rstmgr.h"

static void *lcd_virtual_base=NULL;


 // internal fucniton
//#define MY_DEBUG(msg, arg...) printf("%s:%s(%d): " msg, __FILE__, __FUNCTION__, __LINE__, ##arg)
#define MY_DEBUG(msg, arg...) 
void PIO_DC_Set(bool bIsData);
bool SPIM_IsTxFifoEmpty(void);
void SPIM_WriteTxData(uint8_t Data);


/////////////////////////////////////////////
/////	LCD Fucntion //////////////////////////
/////////////////////////////////////////////


#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

#define HPS_LCM_D_C_BIT_GPIObit62_GPIOreg2	    	( 0x00000010 )
#define HPS_LCM_RESETn_BIT_GPIObit48_GPIOreg1   	( 0x00080000 )  
#define HPS_LCM_BACKLIHGT_BIT_GPIObit40_GPIOreg1	( 0x00000800 )




void LCDHW_Init(void *virtual_base){
	
	lcd_virtual_base = virtual_base;
	

	//
	MY_DEBUG("virtual_base = %xh\r\n", (uint32_t)virtual_base);
	
	
	////////////////////////////////////////////////////
	////////////////////////////////////////////////////
	////////////////////////////////////////////////////
	//////// lcd reset
	// set the direction of the HPS GPIO1 bits attached to LCD RESETn to output
	alt_setbits_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO1_SWPORTA_DDR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), HPS_LCM_RESETn_BIT_GPIObit48_GPIOreg1 );
	// set the value of the HPS GPIO1 bits attached to LCD RESETn to zero
	alt_clrbits_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO1_SWPORTA_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), HPS_LCM_RESETn_BIT_GPIObit48_GPIOreg1 );
	usleep( 1000000 / 16 );	
	// set the value of the HPS GPIO1 bits attached to LCD RESETn to one
	alt_setbits_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO1_SWPORTA_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), HPS_LCM_RESETn_BIT_GPIObit48_GPIOreg1 );
	usleep( 1000000 / 16 );	
	
	////////////////////////////////////////////////////
	////////////////////////////////////////////////////
	////////////////////////////////////////////////////
	//////// turn-on backlight
	// set the direction of the HPS GPIO1 bits attached to LCD Backlight to output
	alt_setbits_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO1_SWPORTA_DDR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), HPS_LCM_BACKLIHGT_BIT_GPIObit40_GPIOreg1 );
	// set the value of the HPS GPIO1 bits attached to LCD Backlight to ZERO, turn OFF the Backlight
	alt_clrbits_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO1_SWPORTA_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), HPS_LCM_BACKLIHGT_BIT_GPIObit40_GPIOreg1 );
	
	
	
	////////////////////////////////////////////////////
	////////////////////////////////////////////////////
	////////////////////////////////////////////////////
	// set LCD-A0 pin as output pin (GPIO2 [5])
  // set the direction of the HPS GPIO2 bits attached to LCD-A0 to output
	MY_DEBUG("[GPIO2]Configure LCD-A0 as output pin\r\n");
	
	alt_setbits_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO2_SWPORTA_DDR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), HPS_LCM_D_C_BIT_GPIObit62_GPIOreg2 );
	// set HPS_LCM_D_C to 0
	alt_clrbits_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO2_SWPORTA_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), HPS_LCM_D_C_BIT_GPIObit62_GPIOreg2 );
	

	////////////////////////////////////////////////////
	////////////////////////////////////////////////////
	////////////////////////////////////////////////////
	// SPIM1 Init
	
	usleep( 1000000 / 16 );
	
	MY_DEBUG("[SPIM1]enable spim1 interface\r\n");
	// initialize the SPIM1 peripheral to talk to the LCM
	alt_clrbits_word( ( virtual_base + ( ( uint32_t )( ALT_RSTMGR_PERMODRST_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), ALT_RSTMGR_PERMODRST_SPIM1_SET_MSK );
	
	//===================
	// step 1: disable SPI
	//         writing 0 to the SSI Enable register (SSIENR).
	//
	
	MY_DEBUG("[SPIM1]spim1_spienr.spi_en = 0 # disable the SPI master\r\n");
	// [0] = 0, to disalbe SPI
	alt_clrbits_word( ( virtual_base + ( ( uint32_t )( ALT_SPIM1_SPIENR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), ALT_SPIM_SPIENR_SPI_EN_SET_MSK );
	
	//===================
	// step 2: setup 
	//         Write Control Register 0 (CTRLLR0).
	//         Write the Baud Rate Select Register (BAUDR)
	//         Write the Transmit and Receive FIFO Threshold Level registers (TXFTLR and RXFTLR) to set FIFO buffer threshold levels.
	//         Write the IMR register to set up interrupt masks.
	//         Write the Slave Enable Register (SER) register here to enable the target slave for selection......
	

	// Transmit Only: Transfer Mode [9:8], TXONLY = 0x01
	MY_DEBUG("[SPIM1]spim1_ctrlr0.tmod = 1  # TX only mode\r\n");
	alt_clrbits_word( ( virtual_base + ( ( uint32_t )( ALT_SPIM1_CTLR0_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), ALT_SPIM_CTLR0_TMOD_SET_MSK );
	alt_setbits_word( ( virtual_base + ( ( uint32_t )( ALT_SPIM1_CTLR0_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), ALT_SPIM_CTLR0_TMOD_SET( ALT_SPIM_CTLR0_TMOD_E_TXONLY ) );
	
	
	// 200MHz / 64 = 3.125MHz: [15:0] = 64
	MY_DEBUG("[SPIM1]spim1_baudr.sckdv = 64  # 200MHz / 64 = 3.125MHz\r\n");
	alt_clrbits_word( ( virtual_base + ( ( uint32_t )( ALT_SPIM1_BAUDR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), ALT_SPIM_BAUDR_SCKDV_SET_MSK );
	alt_setbits_word( ( virtual_base + ( ( uint32_t )( ALT_SPIM1_BAUDR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), ALT_SPIM_BAUDR_SCKDV_SET( 64 ) );



	// ss_n0 = 1, [3:0]
	MY_DEBUG("[SPIM1]spim1_ser.ser = 1  #ss_n0 = 1\r\n");
	alt_clrbits_word( ( virtual_base + ( ( uint32_t )( ALT_SPIM1_SER_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), ALT_SPIM_SER_SER_SET_MSK );
	alt_setbits_word( ( virtual_base + ( ( uint32_t )( ALT_SPIM1_SER_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), ALT_SPIM_SER_SER_SET( 1 ) );
	
	
	
	//===================
	// step 3: Enable the SPI master by writing 1 to the SSIENR register.
	// ALT_SPIM1_SPIENR_ADDR
	MY_DEBUG("[SPIM1]spim1_spienr.spi_en = 1  # ensable the SPI master\r\n");
	alt_setbits_word( ( virtual_base + ( ( uint32_t )( ALT_SPIM1_SPIENR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), ALT_SPIM_SPIENR_SPI_EN_SET_MSK );
	
	// step 4: Write data for transmission to the target slave into the transmit FIFO buffer (write DR)
	//alt_setbits_word( ( virtual_base + ( ( uint32_t )( ALT_SPIM1_DR_ADDR ) & ( uint32_t )( ALT_SPIM1_SPIENR_ADDR ) ) ), data16 );
	
	MY_DEBUG("[SPIM1]LCD_Init done\r\n");
	
}



void LCDHW_BackLight(bool bON){
	if (bON) 
		alt_setbits_word( ( lcd_virtual_base + ( ( uint32_t )( ALT_GPIO1_SWPORTA_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), HPS_LCM_BACKLIHGT_BIT_GPIObit40_GPIOreg1 );
	else
		alt_clrbits_word( ( lcd_virtual_base + ( ( uint32_t )( ALT_GPIO1_SWPORTA_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), HPS_LCM_BACKLIHGT_BIT_GPIObit40_GPIOreg1 );
}



void LCDHW_Write8(uint8_t bIsData, uint8_t Data){

    static uint8_t bPreIsData=0xFF;
 
    // set A0
    if (bPreIsData != bIsData){
        // Note. cannot change D_C until all tx dara(or command) are sent. i.e. fifo is empty
        //  while(!SPIM_IsTxEmpty()); // wait if buffer is not empty

        PIO_DC_Set(bIsData);
        bPreIsData = bIsData;
    }else{
        // wait buffer is not full
        //  while(SPIM_IsTxFull()); // wait if buffer is full 
    }


    SPIM_WriteTxData(Data);
}



//////////////////////////////////////////////////////////////
// internal funciton
//////////////////////////////////////////////////////////////

void PIO_DC_Set(bool bIsData){
	// D_C = "H": Data
	// D_C = "L": CMD
	// HPS_GPIO62: GPIO2[4]

	
	if (bIsData) // A0 = "H": Data
		alt_setbits_word( ( lcd_virtual_base + ( ( uint32_t )( ALT_GPIO2_SWPORTA_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), 0x00000010 );
	else
		alt_clrbits_word( ( lcd_virtual_base + ( ( uint32_t )( ALT_GPIO2_SWPORTA_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), 0x00000010 );
}






void SPIM_WriteTxData(uint8_t Data){
	
	while( ALT_SPIM_SR_TFE_GET( alt_read_word( ( lcd_virtual_base + ( ( uint32_t )( ALT_SPIM1_SR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ) ) ) != ALT_SPIM_SR_TFE_E_EMPTY );
	alt_write_word( ( lcd_virtual_base + ( ( uint32_t )( ALT_SPIM1_DR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ), ALT_SPIM_DR_DR_SET( Data ) );
	while( ALT_SPIM_SR_TFE_GET( alt_read_word( ( lcd_virtual_base + ( ( uint32_t )( ALT_SPIM1_SR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ) ) ) != ALT_SPIM_SR_TFE_E_EMPTY );
	while( ALT_SPIM_SR_BUSY_GET( alt_read_word( ( lcd_virtual_base + ( ( uint32_t )( ALT_SPIM1_SR_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ) ) ) != ALT_SPIM_SR_BUSY_E_INACT );
	
}