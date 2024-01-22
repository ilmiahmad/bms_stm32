#ifndef PORTIO_H
#define PORTIO_H
/****************************************************************************
#ifdef DOC
File Name		:portio.h
Description		:
Remark			:
Date			:2020/10/28
Copyright		:Nuvoton Technology Singapore
#endif
*****************************************************************************/

/****************************************************************************
 define macro
*****************************************************************************/
#define	GPIO_PIN_RESET			0
#define	GPIO_PIN_SET			1

/*---------PORT DEFINITION-------------*/
//#define	GPIO_PORT_BMIC_SDO		PC7		/* pin18 SPI1_MISO */
//#define	GPIO_PORT_BMIC_SDI		PC6		/* pin19 SPI1_MOSI */
//#define	GPIO_PORT_BMIC_CLK		PA7		/* pin20 SPI1_CLK */
//#define	GPIO_PORT_BMIC_SEN		PA6		/* pin21 SPI1_SS (use as GPIO) */

//#define	GPIO_PORT_BMIC_ALARM1	PB5		/* pin2 input *//* BMIC ALARM1 output, H->L: Alarm1 (SCD) */
//#define	GPIO_PORT_BMIC_SHDN		PC4		/* pin36 output, L: active, H(1ms): shutdown */
//#define	GPIO_PORT_BMIC_FETOFF	PC1		/* pin39 output, L: normal, H: FET force off */
/*#define	GPIO_PORT_BMIC_VPC		PC0*/		/* pin40 output, fixed L after BMIC wake-up  */

//#define	GPIO_PORT_BMIC_GPIO1	PB8		/* pin63 input *//* BMIC MCU INT OR output, H->L: INT */
//#define	GPIO_PORT_BMIC_GPIO2	PB7		/* pin64 input *//* BMIC ALARM2 output, H->L: Alarm2 (UV/OV/OCD/OCC) */
//#define	GPIO_PORT_BMIC_GPIO3	PB6		/* pin1 input *//* BMIC ADIRQ2 output */

//#define	GPIO_PORT_LED1			PB4		/* pin3 output, LED1, L: on, H: off */
//#define	GPIO_PORT_LED2			PB3		/* pin4 output, LED2, L: on, H: off */
//#define	GPIO_PORT_LED3			PF6		/* pin12 output, LED3, L: on, H: off */
//#define	GPIO_PORT_LED4			PC5		/* pin35 output, LED4, L: on, H: off */
//
//#define	GPIO_PORT_DIP_SW1		PD0		/* pin44 input */
//#define	GPIO_PORT_DIP_SW2		PD1		/* pin43 input */
//#define	GPIO_PORT_DIP_SW3		PD2		/* pin42 input */
//#define	GPIO_PORT_DIP_SW4		PD3		/* pin41 input */
//
//#define	GPIO_PORT_CAN_TXD		PB11	/* pin60 CAN_TXD */
//#define	GPIO_PORT_CAN_RXD		PB10	/* pin61 CAN_RXD */
//#define	GPIO_PORT_CAN_STBY		PB9		/* pin62 output, H: active, L: inactive */
//#define	GPIO_PORT_RT_CTL_R		PB2		/* pin5 output, RT resistor ON/OFF control, L: OFF, H: ON */
//
//#define	GPIO_PORT_UART3_TXD		PC3		/* pin37 UART3_TXD */
//#define	GPIO_PORT_UART3_RXD		PC3		/* pin38 UART3_RXD */

/*---------PORT SETTING MACRO-------------*/
//#define	SPI_BMIC_SEN_H()	(GPIO_PORT_BMIC_SEN = GPIO_PIN_SET)
//#define	SPI_BMIC_SEN_L()	(GPIO_PORT_BMIC_SEN = GPIO_PIN_RESET)
//
//#define	LED1_LIGHT_ON()		(GPIO_PORT_LED1 = GPIO_PIN_RESET)
//#define	LED1_LIGHT_OFF()	(GPIO_PORT_LED1 = GPIO_PIN_SET)
//
//#define	LED2_LIGHT_ON()		(GPIO_PORT_LED2 = GPIO_PIN_RESET)
//#define	LED2_LIGHT_OFF()	(GPIO_PORT_LED2 = GPIO_PIN_SET)
//
//#define	LED3_LIGHT_ON()		(GPIO_PORT_LED3 = GPIO_PIN_RESET)
//#define	LED3_LIGHT_OFF()	(GPIO_PORT_LED3 = GPIO_PIN_SET)
//
//#define	LED4_LIGHT_ON()		(GPIO_PORT_LED4 = GPIO_PIN_RESET)
//#define	LED4_LIGHT_OFF()	(GPIO_PORT_LED4 = GPIO_PIN_SET)


#endif /* PORTIO_H */
