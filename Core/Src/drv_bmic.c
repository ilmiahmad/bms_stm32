/****************************************************************************
#ifdef DOC
File Name		:drv_bmic.c
Description		:BMIC 49517/522 device driver
Remark			:
Date			:2020/10/28
Copyright		:Nuvoton Technology Singapore
#endif
*****************************************************************************/
#include "main.h"
#include <stdbool.h>
#include "sys_type.h"
#include "main.h"
#include "drv_bmic.h"
#include "drv_spi.h"
#include "portio.h"

/* define hspi1
 *
 */
extern SPI_HandleTypeDef hspi1;
/****************************************************************************
 define macro
*****************************************************************************/
#define	AVERAGE_TIMES			(6u)

#define SCD_P_mA				(12500u)			/* P=12.5A	*/
#define OCD_P_mA				(6250u)				/* P=6.25A	*/
#define OCC_P_mA				(2500u)				/* P=2.5A	*/
#if(1)
#define SCD_Detect_mV			(20u)				/* 0.020V	*/
#define OCD_Detect_mV			(10u)				/* 0.010V	*/
#define OCC_Detect_mV			(5u)				/* 0.005V	*/
#else
#define SCD_Detect_mV			(50u)				/* 0.05V	*/
#define OCD_Detect_mV			(25u)				/* 0.025V	*/
#define OCC_Detect_mV			(10u)				/* 0.01V	*/
#endif

/*#define SHUNT_mohm				(1u)*/				/* 0.001ohm */

/* CB cell mask	*/
#if(BMIC_49522)
#define	CELL_ODD_MASK_ODDCELL	(0x00255555u)
#define	CELL_ODD_MASK_EVENCELL	(0x00155555u)
#else	/* BMIC_49517 */
#define	CELL_ODD_MASK_ODDCELL	(0x00015555u)
#define	CELL_ODD_MASK_EVENCELL	(0x00009555u)
#endif	/* BMIC_49522 */

#define NON_CELL_BALANCE		(0u)
#define ODD_CELL_BALANCE		(1u)
#define EVEN_CELL_BALANCE		(2u)
#define CELL_BALANCE_OFF		(3u)

/*#define NON_CBL_REQ				(0u)*/
/*#define ODD_CBL_REQ				(1u)*/
/*#define EVEN_CBL_REQ			(2u)*/
/*#define BOTH_CBL_REQ			(3u)*/
/*#define CUR_SUM_MIN_LEVEL_AD	(10u*SHUNT_mohm)*/		/* 10mA	*/
#define	CUR_SUM_MIN_LEVEL		(10u)					/* 10A */
#define BMIC_DEAD_DETECT_CNT	(10u)


/* BMIC 49517/22 Register declaration */
/* 					 			((UCHAR)0x00u) */	/* Reserved */
#define		PWR_CTRL_ADDR		((UCHAR)0x01u)	/* Operation mode control, ADC operation setting, LDM/VPC pin control */
#define		REG_INT_EN_ADDR		((UCHAR)0x02u)	/* INTx control, REG_EXT & VDD55 setting when standby/sleep/intermitten mode */
#define		SPIWD_CTRL_ADDR		((UCHAR)0x03u)	/* SPI watchdog timer setting/control, REG_EXT status when WDT expire */
#define		FDRV_CTRL_ADDR		((UCHAR)0x04u)	/* FDRV control settings */

#define		CVSEL1_ADDR			((UCHAR)0x05u)	/* Cell 01 ~ Cell 16 voltage measurement on/off select */
#define		CVSEL2_ADDR			((UCHAR)0x06u)	/* Cell 17 ~ Cell 22 voltage measurement on/off select */
#define		GVSEL_ADDR			((UCHAR)0x07u)	/* VPACK, TMONI1~5, VDD55, GPIO1~2, VDD18, REGEXT, VREF2 voltage measurement on/off select */

#define		FUSE_CTL1_ADDR		((UCHAR)0x08u)	/* Chemical fuse control setting1 */
#define		FUSE_CTL2_ADDR		((UCHAR)0x09u)	/* Chemical fuse control setting2 */

#define		OUVCTL1_ADDR		((UCHAR)0x0Au)	/* OV/UV detection threshold, current threshold for fuse cut */
#define		OUVCTL2_ADDR		((UCHAR)0x0Bu)	/* OV/UV hysteresis & delay setting */
#define		OP_MODE_ADDR		((UCHAR)0x0Cu)	/* OV/UV detection on/off, CB on/off, ADC trigger, ADC result latch */

#define		GPIO_CTRL1_ADDR		((UCHAR)0x0Du) 	/* GPIO1 settings */
#define		GPIO_CTRL2_ADDR		((UCHAR)0x0Eu)	/* GPIO2 settings */
#define		GPIO_CTRL3_ADDR		((UCHAR)0x0Fu)	/* GPIO3 settings */
#define		GPOH_CTL_ADDR		((UCHAR)0x10u)	/* GPOH settings */
#define		GPIO_CTRL4_ADDR		((UCHAR)0x11u)	/* ADC scan cycle during intermitten mode, TMONI1~5 pull-up setting, OSCH_DIV/OSCL_DIV setting etc */

#define		ALARM_CTRL1_ADDR	((UCHAR)0x12u)	/* ALRAM1 pin select, SCD/OCD/OCC/CP enable */
#define		ALARM_CTRL2_ADDR	((UCHAR)0x13u)	/* SCD/OCD/OCC detection threshold settings */
#define		ALARM_CTRL3_ADDR	((UCHAR)0x14u)	/* SCD/OCD/OCC delay settings */

#define		CBSEL_ADDR			((UCHAR)0x15u)	/* Cell balance select: Cell 01 ~ Cell 16 */
#define		CBSEL_22_ADDR		((UCHAR)0x16u)	/* Cell balance select: Cell 17 ~ Cell 22 */

#define		OTHCTL_ADDR			((UCHAR)0x17u)	/* SPI SEN/SCL/SDI pull-down settings, VDD55 regulator setting */

#define		ADCTL_ADDR			((UCHAR)0x18u)	/* HS/LS Current ADC on/off select, V-I Sync setting, HS/LS ADC latch timigng setting */

#define		INRCV1_ADDR			((UCHAR)0x19u)	/* Selection of Cell 01 ~ Cell 16 for open detection */
#define		INRCV2_ADDR			((UCHAR)0x1Au)	/* Selection of Cell 17 ~ Cell 22 for open detection */

#define		INR_CTL_DIAG_EN_ADDR	((UCHAR)0x1Bu)	/* cell open detection on/off control, HS/LS current ADC diagnosis on/off, charge/discharge FET diagnosis on/off, Cell selection for V-I sync */

#define		STAT1_ADDR			((UCHAR)0x1Cu)	/* operation mode status, HS/LS current ADC & voltage ADC completion flag, charge/discharge FET status, GPOH1/2 state, GPIO1~3 input state */
#define		STAT2_ADDR			((UCHAR)0x1Du)	/* Cell 01 ~ Cell 16 voltage measurement on/off status */
#define		STAT3_ADDR			((UCHAR)0x1Eu)	/* Cell 17 ~ Cell 22 voltage measurement on/off status */
#define		STAT4_ADDR			((UCHAR)0x1Fu)	/* VREF1/2, REGEXT, VDD18, VPACK, GPIO1/2, TMOIN1~5 voltage measurement on/off status */

#define		ANA_CTL_ADDR		((UCHAR)0x20u)	/* LDM current drive selection */

#define		OTHSTAT_ADDR		((UCHAR)0x21u)	/* CHG/DIS FET ON/OFF diagnosis result, TSD detection flag, OCC/OCD/SCD detection flags, sequence diagnosis result, SPI error flag */

#define		OVSTAT1_ADDR		((UCHAR)0x22u)	/* Cell 01 ~ Cell 16 OV detection status */
#define		OVSTAT2_ADDR		((UCHAR)0x23u)	/* Cell 17 ~ Cell 22 OV detection status */
#define		UVSTAT1_ADDR		((UCHAR)0x24u)	/* Cell 01 ~ Cell 16 UV detection status */
#define		UVSTAT2_ADDR		((UCHAR)0x25u)	/* Cell 17 ~ Cell 22 UV detection status */

#define		BIASSTAT_ADDR		((UCHAR)0x26u)	/* VDD55, REG_EXT, VDD18, VREF1/2 OV/UV status flag */

#define		STAT5_ADDR			((UCHAR)0x27u)	/* OV/UV status, BIAS fault status, other fault status, FUSE blow status, VPC/LDM event status, current event status, WDT timeout status */

#define		CV01_AD_ADDR		((UCHAR)0x28u)	/* Cell01 Voltage measurement result */
#define		CV02_AD_ADDR		((UCHAR)0x29u)	/* Cell02 Voltage measurement result */
#define		CV03_AD_ADDR		((UCHAR)0x2Au)	/* Cell03 Voltage measurement result */
#define		CV04_AD_ADDR		((UCHAR)0x2Bu)	/* Cell04 Voltage measurement result */
#define		CV05_AD_ADDR		((UCHAR)0x2Cu)	/* Cell05 Voltage measurement result */
#define		CV06_AD_ADDR		((UCHAR)0x2Du)	/* Cell06 Voltage measurement result */
#define		CV07_AD_ADDR		((UCHAR)0x2Eu)	/* Cell07 Voltage measurement result */
#define		CV08_AD_ADDR		((UCHAR)0x2Fu)	/* Cell08 Voltage measurement result */
#define		CV09_AD_ADDR		((UCHAR)0x30u)	/* Cell09 Voltage measurement result */
#define		CV10_AD_ADDR		((UCHAR)0x31u)	/* Cell10 Voltage measurement result */
#define		CV11_AD_ADDR		((UCHAR)0x32u)	/* Cell11 Voltage measurement result */
#define		CV12_AD_ADDR		((UCHAR)0x33u)	/* Cell12 Voltage measurement result */
#define		CV13_AD_ADDR		((UCHAR)0x34u)	/* Cell13 Voltage measurement result */
#define		CV14_AD_ADDR		((UCHAR)0x35u)	/* Cell14 Voltage measurement result */
#define		CV15_AD_ADDR		((UCHAR)0x36u)	/* Cell15 Voltage measurement result */
#define		CV16_AD_ADDR		((UCHAR)0x37u)	/* Cell16 Voltage measurement result */
#define		CV17_AD_ADDR		((UCHAR)0x38u)	/* Cell17 Voltage measurement result */
#define		CV18_AD_ADDR		((UCHAR)0x39u)	/* Cell18 Voltage measurement result */
#define		CV19_AD_ADDR		((UCHAR)0x3Au)	/* Cell19 Voltage measurement result */
#define		CV20_AD_ADDR		((UCHAR)0x3Bu)	/* Cell20 Voltage measurement result */
#define		CV21_AD_ADDR		((UCHAR)0x3Cu)	/* Cell21 Voltage measurement result */
#define		CV22_AD_ADDR		((UCHAR)0x3Du)	/* Cell22 Voltage measurement result */
#define		VPACK_AD_ADDR		((UCHAR)0x3Eu)	/* VPACK Voltage measurement result */
#define		TMONI1_AD_ADDR		((UCHAR)0x3Fu)	/* TMONI1 Voltage measurement result */
#define		TMONI2_AD_ADDR		((UCHAR)0x40u)	/* TMONI2 Voltage measurement result */
#define		TMONI3_AD_ADDR		((UCHAR)0x41u)	/* TMONI3 Voltage measurement result */
#define		TMONI4_AD_ADDR		((UCHAR)0x42u)	/* TMONI4 Voltage measurement result */
#define		TMONI5_AD_ADDR		((UCHAR)0x43u)	/* TMONI5 Voltage measurement result */
#define		VDD55_AD_ADDR		((UCHAR)0x44u)	/* VDD55 Voltage measurement result */
#define		GPIO1_AD_ADDR		((UCHAR)0x45u)	/* GPIO1 Voltage measurement result */
#define		GPIO2_AD_ADDR		((UCHAR)0x46u)	/* GPIO2 Voltage measurement result */
#define		CVIH_AD_ADDR		((UCHAR)0x47u)	/* High-speed current ADC measurement result */
#define		CVIL_AD_ADDR		((UCHAR)0x48u)	/* Low-speed current ADC measurement result */
#define		VDD18_AD_ADDR		((UCHAR)0x49u)	/* VDD18 Voltage measurement result */
#define		REGEXT_AD_ADDR		((UCHAR)0x4Au)	/* REGEXT Voltage measurement result */
#define		VREF2_AD_ADDR		((UCHAR)0x4Bu)	/* VREF2 Voltage measurement result */
/*#define						((UCHAR)0x4Cu)*//* Reserved */

#define		OVL_STAT1_ADDR		((UCHAR)0x4Du)	/* Cell OV detection flag (Cell 1 ~ Cell 16) */
#define		OVL_STAT2_ADDR		((UCHAR)0x4Eu)	/* Cell OV detection flag (Cell 17 ~ Cell 22) */

#define		UVL_STAT1_ADDR		((UCHAR)0x4Fu)	/* Cell UV detection flag (Cell 1 ~ Cell 16) */
#define		UVL_STAT2_ADDR		((UCHAR)0x50u)	/* Cell UV detection flag (Cell 17 ~ Cell 22) */

#define		CBSTAT1_ADDR		((UCHAR)0x51u)	/* Individual cell balance control status (Cell 1 ~ Cell 16) */
#define		CBSTAT2_ADDR		((UCHAR)0x52u)	/* Individual cell balance control status (Cell 17 ~ Cell 22) */

#define		FUSE_BLOW_ADDR		((UCHAR)0x53u)	/* Fuse blow function activation */
#define		AUTO_ITHU_ADDR		((UCHAR)0x54u)	/* Reserved */
#define		AUTO_ITHL_ADDR		((UCHAR)0x55u)	/* Settings for detection current level to enter low power auto mode */
#define		VDD55_CTL_ADDR		((UCHAR)0x56u)	/* NPN Hfe(Gain) adjustment, NPN Temp coefficient adjustment  */
#define		TMONI1_ADDR			((UCHAR)0x57u)	/* TMONI1 pin pull-up resistance value (absolute value) */
#define		TMONI23_ADDR		((UCHAR)0x58u)	/* TMON2 & TMON3 pin pull-up resistance value (difference) */
#define		TMONI45_ADDR		((UCHAR)0x59u)	/* TMON4 & TMON5 pin pull-up resistance value (difference) */

#define		BMIC_REG_MAX_ADDR	TMONI45_ADDR

/* PWR_CTRL_ADDR(0x01u) */
#define		NPD_RST					((USHORT)0x0000u)		/* b0: Soft Reset */
#define		NPD_RST_MASK			((USHORT)0x0001u)		/* b0: Soft Reset Mask */
#define		MSET_SHDN				((USHORT)0x0002u)		/* b1: Shutdown control  */
#define		MSET_SHDN_MASK			((USHORT)0x0002u)		/* b1: Shutdown control MASK */
#define		MSET_SLP				((USHORT)0x0004u)		/* b2: Sleep mode control */
#define		MSET_SLP_MASK			((USHORT)0x0004u)		/* b2: Sleep mode control MASK */
#define		MSET_LP					((USHORT)0x0008u)		/* b3: Low power mode control */
#define		MSET_LP_MASK			((USHORT)0x0008u)		/* b3: Low power mode control MASK */
#define		MSET_STB				((USHORT)0x0010u)		/* b4: Low power mode control */
#define		MSET_STB_MASK			((USHORT)0x0010u)		/* b4: Low power mode control MASK */

#define		VPC_STB_EN				((USHORT)0x0080u)		/* b7: enable return to active mode from LP/STB when VPC high */
#define		LDM_STB_EN				((USHORT)0x0100u)		/* b8: enable return to active mode form LP/STB when LDM high */
#define		VPC_SLP_EN				((USHORT)0x0200u)		/* b9: enable return to active mode form SLP when VPC high */
#define		LDM_SLP_EN				((USHORT)0x0400u)		/* b10: enable return to active mode form SLP when LDM high */

#define		ADC_CONT				((USHORT)0x8000u)		/* b15: ADC Operation Setting, 1 - Continuous mode (Voltage and HS current measurement done repeatedly during Active mode), 0 - Oneshot mode (Voltage and HS current measurement only when ADC_TRG is set) */

/* FDRV_CTRL_ADDR(0x04u) */
#define		FDRV_STBY_OFF				((USHORT)0x0000u)		/* b8: FET standby mode : OFF (Normal)(default) */
#define		FDRV_STBY_ON				((USHORT)0x0100u)		/* b8: FET standby mode : ON (Standby) */
#define		FDRV_STBY_MASK				((USHORT)0x0100u)		/* b8: FET standby MASK */
#define		FDRV_DISCHARGE_FET_OFF		((USHORT)0x0000u)		/* b9: DIS FET control : OFF(default) */
#define		FDRV_DISCHARGE_FET_ON		((USHORT)0x0200u)		/* b9: DIS FET control : ON */
#define		FDRV_DISCHARGE_FET_MASK		((USHORT)0x0200u)		/* b9: DIS FET MASK */
#define		FDRV_CHARGE_FET_OFF			((USHORT)0x0000u)		/* b10: GHG FET control : OFF(default) */
#define		FDRV_CHARGE_FET_ON			((USHORT)0x0400u)		/* b10: CHG FET control : ON */
#define		FDRV_CHARGE_FET_MASK		((USHORT)0x0400u)		/* b10: CHG FET MASK */
#define		FDRV_ALM_CLR				((USHORT)0x2000)		/* b13: FDRV_ALM_CLR 1: CHG/DIS FET and GPOH pins recover */

/* OP_MODE_ADDR(0x0Cu) */
#define		OP_MODE_INIT		((USHORT)0x0000u)		/* 0x0C Register initial value */
#define		ADV_LATCH_ON		((USHORT)0x0001u)		/* b0: ADC value latch request : Cell voltage ON */
#define		ADIH_LATCH_ON		((USHORT)0x0002u)		/* b1: ADC value latch request : High speed current ON */
#define		ADIL_LATCH_ON		((USHORT)0x0004u)		/* b2: ADC value latch request : Low speed current ON */
#define		ADC_TRG_ON			((USHORT)0x0010u)		/* b4: ADC 1 shot trigger : ON */
#define		OVMSK_ON			((USHORT)0x0000u)		/* b6: OV detection : ON (default) */
#define		OVMSK_OFF			((USHORT)0x0040u)		/* b6: OV detection : OFF */
#define		UVMSK_ON			((USHORT)0x0000u)		/* b7: UV detection : ON (default) */
#define		UVMSK_OFF			((USHORT)0x0080u)		/* b7: UV detection : OFF */
#define		CB_SET_ON			((USHORT)0x0100u)		/* b8: Cell balancing operation : ON */
#define		CB_SET_OFF			((USHORT)0x0000u)		/* b8: Cell balancing operation : OFF (default) */

/* OTHCTL_ADDR(0x17u) */
#define		NPD_CB_NORMAL		((USHORT)0x0001u)		/* b0: NPD_CB cell balance control power down, 1 - Normal */
#define		NPD_CB_POWERDOWN	((USHORT)0x0000u)		/* b0: NPD_CB cell balance control power down, 0 - Power down(default) */
#define		NPD_CB_MASK			((USHORT)0x0001u)		/* b0: NPD_CB MASK */
#define		PD_REG55_ON			((USHORT)0x0040u)		/* b6: VDD55 regulator power down ON */
#define		PD_REG55_OFF		((USHORT)0x0000u)		/* b6: VDD55 regulator power down OFF (normal) */
#define		PD_REG55_MASK		((USHORT)0x0040u)		/* b6: VDD55 regulator power down */



/* GPOH_CTL_ADDR(0x10u) */
#define		GPOH1_LOW			((USHORT)0x0001u)		/* b0 GPOH1 control : LOW */
#define		GPOH1_HIZ			((USHORT)0x0000u)		/* b0 GPOH1 control : HiZ */
#define		GPOH1_MASK			((USHORT)0x0001u)		/* b0 GPOH1 MASK */
#define		GPOH2_LOW			((USHORT)0x0002u)		/* b1 GPOH2 control : LOW */
#define		GPOH2_HIZ			((USHORT)0x0000u)		/* b1 GPOH2 control : HiZ */
#define		GPOH2_MASK			((USHORT)0x0002u)		/* b1 GPOH2 MASK */
#define		GPOH_FET_MASK		((USHORT)0x0004u)		/* b2 GPOH FET MASK */
#define		GPOH_FET_FET		((USHORT)0x0004u)		/* b2 GPOH FET : USE_FET */
#define		GPOH1_ALM_ST_LOW	((USHORT)0x0010u)		/* b4 GPOH1 data during ALARM: LOW */
#define		GPOH1_ALM_ST_HIZ	((USHORT)0x0000u)		/* b4 GPOH1 data during ALARM: HiZ */
#define		GPOH1_ALM_ST_MASK	((USHORT)0x0010u)		/* b4 GPOH1 data during ALARM MASK */
#define		GPOH2_ALM_ST_LOW	((USHORT)0x0020u)		/* b5 GPOH2 data during ALARM: LOW */
#define		GPOH2_ALM_ST_HIZ	((USHORT)0x0000u)		/* b5 GPOH2 data during ALARM: HiZ */
#define		GPOH2_ALM_ST_MASK	((USHORT)0x0020u)		/* b5 GPOH2 data during ALARM MASK */

/* STAT1_ADDR(0x1Cu) */
#define	MODE_ST_MASK		((USHORT)0x001Fu)	/* STAT1(0x1Cu) : Bit[4~0] operation mode flags */
#define	MODE_ST_ACT			((USHORT)0x0001u)	/* STAT1(0x1Cu) : Bit[0] Active Mode */
#define	MODE_ST_STBY		((USHORT)0x0002u)	/* STAT1(0x1Cu) : Bit[1] Standby Mode */
#define	MODE_ST_SDWN		((USHORT)0x0004u)	/* STAT1(0x1Cu) : Bit[2] Shutdown Mode */
#define	MODE_ST_LP			((USHORT)0x0008u)	/* STAT1(0x1Cu) : Bit[3] Low Power Mode */
#define	MODE_ST_INTM		((USHORT)0x0010u)	/* STAT1(0x1Cu) : Bit[4] Intermitten Mode */
#define	VAD_DONE			((USHORT)0x0020u)	/* STAT1(0x1Cu) : Bit[5] Voltage measurement ADC completion flag */
#define	IADH_DONE			((USHORT)0x0040u)	/* STAT1(0x1Cu) : Bit[6] High speed current measurement ADC completion flag */
#define	IADS_DONE			((USHORT)0x0080u)	/* STAT1(0x1Cu) : Bit[7] Low speed current measurement ADC completion flag */
#define	GPOH1_ST_MASK		((USHORT)0x0100u)	/* STAT1(0x1Cu)	: Bit[8] GPOH1_ST */
#define	GPOH2_ST_MASK		((USHORT)0x0200u)	/* STAT1(0x1Cu)	: Bit[9] GPOH2_ST */
#define	FDRV_CHG_ST_MASK	((USHORT)0x0400u)	/* STAT1(0x1Cu)	: Bit[10] FDRV_CHG_ST */
#define	FDRV_DIS_ST_MASK	((USHORT)0x0800u)	/* STAT1(0x1Cu)	: Bit[11] FDRV_DIS_ST */
#define	ST_GPIO1_MASK		((USHORT)0x2000u)	/* STAT1(0x1Cu)	: Bit[13] ST_GPIO1 */
#define	ST_GPIO2_MASK		((USHORT)0x4000u)	/* STAT1(0x1Cu)	: Bit[14] ST_GPIO2 */
#define	ST_GPIO3_MASK		((USHORT)0x8000u)	/* STAT1(0x1Cu)	: Bit[15] ST_GPIO3 */

/* OTHSTAT_ADDR(0x21u) */
#define	SPI_F_MASK			((USHORT)0x1000u)	/* OTHSTAT(0x21u) : Bit[12] SPI_F    					*/
#define	ST_OCC_MASK			((USHORT)0x2000u)	/* OTHSTAT(0x21u) : Bit[13] ST_OCC						*/
#define	ST_OCD_MASK			((USHORT)0x4000u)	/* OTHSTAT(0x21u) : Bit[14] ST_OCD						*/
#define	ST_SCD_MASK			((USHORT)0x8000u)	/* OTHSTAT(0x21u) : Bit[15] ST_SCD						*/
#define	OTHSTAT_SCD_OCD_OCC_MASK	((USHORT)0xE000u)	/* OTHSTAT(0x21u) : Bit[15~13] ST_SCD, ST_OCD, ST_OCC	*/

/* STAT5_ADDR(0x27u) */
#define	ST_UV_MASK					((USHORT)0x0001u)	/* STAT5(0x27u): Bit[0] ST_UV			*/
#define	ST_OV_MASK					((USHORT)0x0002u)	/* STAT5(0x27u): Bit[1] ST_OV			*/
#define	STAT5_OVUV_MASK				((USHORT)0x0003u)	/* STAT5(0x27u): Bit[1~0] ST_OV, ST_UV	*/
#define	ST_BIAS						((USHORT)0x0004u)	/* STAT5(0x27u): Bit[2] ST_BIAS		*/
#define	ST_OTH						((USHORT)0x0008u)	/* STAT5(0x27u): Bit[3] ST_OTH		*/
#define	FUSEB_F						((USHORT)0x0010u)	/* STAT5(0x27u): Bit[4] FUSEB_F		*/
#define	VPC_L_F						((USHORT)0x0100u)	/* STAT5(0x27u): Bit[8] VPC_L_F		*/
#define	VPC_H_F						((USHORT)0x0200u)	/* STAT5(0x27u): Bit[9] VPC_H_F		*/
#define	LDM_L_F						((USHORT)0x0400u)	/* STAT5(0x27u): Bit[10] LDM_L_F	*/
#define	LDM_H_F						((USHORT)0x0800u)	/* STAT5(0x27u): Bit[11] LDM_H_F	*/
#define	CUR_H_F						((USHORT)0x1000u)	/* STAT5(0x27u): Bit[12] CUR_H_F	*/
#define	WDT_F						((USHORT)0x2000u)	/* STAT5(0x27u): Bit[13] WDT_F		*/
#define	STAT5_VPC_LDM_CUR_WDT_MASK	((USHORT)0x3F00u)	/* STAT5(0x27u): Bit[13~8] VPC, LDM, CUR, WDT flgs		*/
#define	VPC_DET_F					((USHORT)0x4000u)	/* STAT5(0x27u): Bit[14] VPC_DET_F*/
#define	LDM_DET_F					((USHORT)0x8000u)	/* STAT5(0x27u): Bit[15] LDM_DET_F*/

/* Control Register Initial value */
#define	PWR_CTRL_NPD_RST	(0x0780)	/* default settings: ADC_CONT OFF and NPD_RST "0" for soft reset */
#define	PWR_CTRL_INI	(0x8781u)		/* initial settings: ADC_CONT ON */

/*#define	REG_INT_EN_INI	(0x0500u)*/		/* initial setting: default settings */
#define	REG_INT_EN_INI	(0x1D00u)		/* initial settng: Enable INT1(charger detection) & INT2(load detection) */

#ifdef USE_BMIC_SPI_WDT
#define	SPIWD_CTRL_INI	(0x1045u)		/* SPIWD used */
#else
#define	SPIWD_CTRL_INI	(0x0045u)		/* SPIWD not use	*/
#endif

/*#define	FDRV_CTRL_INI	(0x1000u) */	/* initial setting: CHG/DIS not on */
/*#define	FDRV_CTRL_INI	(0x1600u) */	/* initial setting: CHG/DIS on */
#define	FDRV_CTRL_INI	(0xD600u)	/* initial setting: CHG/DIS on, FDRV_ALM_SD, FDRV_ALM_RCV on */

#define	CVSEL1_INI		(0xFFFFu)
#if(BMIC_49522)
#define	CVSEL2_INI		(0x003Fu)
#else	/* BMIC_49517 */
#define	CVSEL2_INI		(0x0001u)
#endif	/* BMIC_49522 */

/*#define	GVSEL_INI		(0xF07Fu) */				/**/
#define	GVSEL_INI		(0x707Fu)		/* bp15 - reserved */

#define	FUSE_CTL1_INI	(0x0000u)		/* default settings */
#define	FUSE_CTL2_INI	(0x000Au)		/* default settings	*/

/*#define	OUVCTL1_INI		(0x0BECu)*/		/* OCTH: 1000mA(default), OVTH: 4.25V, UVTH: 2.7V */
#define	OUVCTL1_INI		(0x0BC0u)		/* OCTH: 1000mA(default), OVTH: 4.25V, UVTH: 0.5V */
#define	OUVCTL2_INI		(0x3333u)		/* OV_HYS: 100mV(default), UV_HYS: 100ms(default), OV_DLY: 800ms(default), UV_DLY: 800ms(default)*/

#define	OP_MODE_INI		(0x0000u)		/* UVMSK, OVMSK: 0(ON) - default	*/

#define	GPIO_CTRL1_INI	(0x0D00u)		/* GPIO1: output select - MCU INT OR  */
#define	GPIO_CTRL2_INI	(0x0B00u)		/* GPIO2: output select - ALARM2 */
#define	GPIO_CTRL3_INI	(0x0400u)		/* GPIO3: output select - ADIRQ2 */
#define	GPIO_CTRL4_INI	(0x63F9u)		/* default 0x6301 | 0x00F8 (TMONI1~5 pull-up resistor: ON */

#define	ALARM_CTRL1_INI	(0x800Fu)		/* ALARM1: ALARM for SCD(ALARM2 for OV/UV/OCD/OCC), SCD/OCD/OCC/CP detection: ON */

#define	ALARM_CTRL2_INI	(0x0421u)		/* SCD TH: 40mV(default), OCD TH: 20mV(default), OCC TH: 10mV(default) */
#define	ALARM_CTRL3_INI	(0x0400u)		/* SCD delay: 31.25us(default), OCD delay: 10ms(default), OCC delay: 10ms (default) */

#define	CBSEL_INI		(0x0000u)
#define	CBSEL_22_INI	(0x0000u)
#define	OTHCTL_INI 		(0x0700u)		/* SDI, SCL, SEN pull-down ON, PD_REG55: VDD55 normal, NPD_CB: CB control power off */

/*#define	ADCTL_INI		(0x3213u)*/		/* HS ADC/LS ADC enable, data latch: instant, V-I sync: OFF */
#define	ADCTL_INI		(0x3203u)		/* HS ADC/LS ADC enable, data latch: instant, V-I sync: OFF */
/*#define	ADCTL_INI		(0x0213u)*/

#define	GPOH_CTL_INI	(0x0000u)		/* Default */

#define	INRCV1_INI		(0x0000u)
#define	INRCV2_INI		(0x0000u)

#define	INR_CTL_DIAG_EN_INI		(0x0000u)



#define	BMIC_PACKET_BUF_SIZE	(200u)	/* 517 registers: 0x00 ~ 0x59 => 90, x2 => 180 byte, reserved 200 byte */

#define	BMIC_RECEIVE_STATUS_REGISTER_SIZE	((UCHAR)62u)		/* 517 Stat registers: 0x1C ~ 0x59 => 62u */
#define	BMIC_RECEIVE_PACKET_BUF_SIZE		((UCHAR)BMIC_RECEIVE_STATUS_REGISTER_SIZE*2u)
#define	DATA_ELEMENTSIZE		((UCHAR)0x60u)			/* 517 registers: 0x00 ~ 0x59 (90 elements), reserved 0x60 (96 elements) */

#define	SEND_REQ_DISCHARGE		((UCHAR)0x0001u)
#define	SEND_REQ_CHARGE			((UCHAR)0x0002u)
#define	SEND_REQ_PRECHARGE		((UCHAR)0x0004u)

/* usBMIC_AdState	*/
#define NOT_FIN_ADV_LATCH		(0x01u)		/* not finish ADV Latch		*/
#define NOT_FIN_ADIH_LATCH	(0x02u)		/* not finish ADIH Latch  */
#define AD_AVE_DATA_NG			(0x04u)		/* SPI NG to receive AD   */

#define OFFSET40(val)			((val) + 40)
#define COMP_MAX(val, max)		do {\
									if((val)>(max)){\
										(max) = (val);\
									}\
								}while(0)
#define COMP_MIN(val, min)		do {\
									if((min)>(val)){\
										(min) = (val);\
									}\
								}while(0)

#define CALIB_AD_COUNT			(8u)

/*usSPI_ComAtLeastOnceFlg	*/
#define COMM_MULTI_READ			(0x0001u)
#define COMM_AD_CELLV			(0x0002u)
#define COMM_AD_TMONI			(0x0004u)
#define COMM_AD_IHIGH			(0x0008u)
#define COMM_AD_ILOW			(0x0010u)
#define COMM_AD_OTHER			(0x0020u)
#define COMM_RECEIVE_ALL		(0x003Fu)

/*Cell connect bit pattern*/
#if(BMIC_49522)
#define BITPAT_4CELL			(0x00300003u)
#define BITPAT_5CELL			(0x00300007u)
#define BITPAT_6CELL			(0x0030000Fu)
#define BITPAT_7CELL			(0x0030001Fu)
#define BITPAT_8CELL			(0x0030003Fu)
#define BITPAT_9CELL			(0x0030007Fu)
#define BITPAT_10CELL			(0x003000FFu)
#define BITPAT_11CELL			(0x003001FFu)
#define BITPAT_12CELL			(0x003003FFu)
#define BITPAT_13CELL			(0x003007FFu)
#define BITPAT_14CELL			(0x00300FFFu)
#define BITPAT_15CELL			(0x00301FFFu)
#define BITPAT_16CELL			(0x00303FFFu)
#define BITPAT_17CELL			(0x00307FFFu)
#define BITPAT_18CELL			(0x0030FFFFu)
#define BITPAT_19CELL			(0x0031FFFFu)
#define BITPAT_20CELL			(0x0033FFFFu)
#define BITPAT_21CELL			(0x0037FFFFu)
#define BITPAT_22CELL			(0x003FFFFFu)
#else	/* BMIC_49517 */
#define BITPAT_4CELL			(0x00018003u)
#define BITPAT_5CELL			(0x00018007u)
#define BITPAT_6CELL			(0x0001800Fu)
#define BITPAT_7CELL			(0x0001801Fu)
#define BITPAT_8CELL			(0x0001803Fu)
#define BITPAT_9CELL			(0x0001807Fu)
#define BITPAT_10CELL			(0x000180FFu)
#define BITPAT_11CELL			(0x000181FFu)
#define BITPAT_12CELL			(0x000183FFu)
#define BITPAT_13CELL			(0x000187FFu)
#define BITPAT_14CELL			(0x00018FFFu)
#define BITPAT_15CELL			(0x00019FFFu)
#define BITPAT_16CELL			(0x0001BFFFu)
#define BITPAT_17CELL			(0x0001FFFFu)
#endif	/* BMIC_49522 */

#define CELL_CONNECT_NG_DET_TH_mV (1000u)

#define CURRENT_ERR_CLEAR_TIME_1S (2u)

#define	BMIC_WAIT_1US						((USHORT)4u)
#define	MS_100US								(10u)							/* 1000us/100us = 10 */
#define BMIC_SPIWAIT_TIMEOUT		(1000u)						/* ~250us */

/****************************************************************************
 define type (STRUCTURE, UNION)
*****************************************************************************/
/*--------------------------------------------*/
typedef struct {
	UCHAR	ucTxBuf[BMIC_PACKET_BUF_SIZE];
	UCHAR	ucRxBuf[BMIC_PACKET_BUF_SIZE];

	USHORT	usRegData[DATA_ELEMENTSIZE];

	UCHAR	ucCellList[MAX_CELL_NUM];			/* List of connect cell	*/
	UCHAR	ucOpenCellList[MAX_CELL_NUM];		/* List of open cell	*/
	ULONG	ulCVSumAD[MAX_CELL_NUM];			/* Total cell voltage */
	USHORT	usCVAD_max[MAX_CELL_NUM];			/* Max voltage of each cell */
	USHORT	usCVAD_min[MAX_CELL_NUM];			/* Min voltage of each cell	*/
	USHORT	usCVAveAD[MAX_CELL_NUM];			/* Average cell voltage */

	ULONG	ulVpackSumAD;						/* Total VPACK voltage */
	USHORT	usVpackAD_max;						/* Max voltage of VPACK */
	USHORT	usVpackAD_min;						/* Min voltage of VPACK */
	USHORT	usVpackAveAD;						/* Average VPACK voltage */

	ULONG	ulVddSumAD;							/* Total Vdd55 voltage */
	USHORT	usVddAD_max;						/* Max Vdd55 voltage */
	USHORT	usVddAD_min;						/* Min Vdd55 voltage	*/
	USHORT	usVddAveAD;							/* Average Vdd55 voltage	*/

	ULONG	ulVdd18SumAD;						/* Total Vdd18 voltage	*/
	USHORT	usVdd18AD_max;						/* Max Vdd18 voltage	*/
	USHORT	usVdd18AD_min;						/* Min Vdd18 voltage	*/
	USHORT	usVdd18AveAD;						/* Average Vdd18 voltage	*/

	ULONG	ulVREGEXTSumAD;						/* Total REGEXT voltage */
	USHORT	usVREGEXTAD_max;					/* Max REGEXT voltage	*/
	USHORT	usVREGEXTAD_min;					/* Min REGEXT voltage	*/
	USHORT	usVREGEXTAveAD;						/* Average REGEXT voltage	*/

	ULONG	ulVREF2SumAD;						/* Total VREF2 voltage	*/
	USHORT	usVREF2AD_max;						/* Max VREF2 voltage	*/
	USHORT	usVREF2AD_min;						/* Min VREF2 voltage	*/
	USHORT	usVREF2AveAD;						/* Average VREF2 voltage	*/

	#if(0)	/* VREF1 not used */
	ULONG	ulVREF1SumAD;						/* Total VREF1 voltage	*/
	USHORT	usVREF1AD_max;						/* Max VREF1 voltage	*/
	USHORT	usVREF1AD_min;						/* Min VREF1 voltage	*/
	USHORT	usVREF1AveAD;						/* Average VREF1 voltage	*/
	#endif

	LONG	lCISumAD;							/* Total current	*/
	SHORT	sCIAD_max;							/* max current	*/
	SHORT	sCIAD_min;							/* min current	*/
	SHORT	sCIAveAD;							/* Average current	*/

	ULONG	ulTmoniSumAD[THERMISTOR_NUM];		/* Total Thermistor voltage	*/
	USHORT	usTmoniAD_max[THERMISTOR_NUM];		/* max Thermistor voltage */
	USHORT	usTmoniAD_min[THERMISTOR_NUM];		/* min Thermistor voltage */
	USHORT	usTmoniAveAD[THERMISTOR_NUM];		/* Average Thermistor voltage	*/
	USHORT	usTmoniC_40[THERMISTOR_NUM];		/* Temperature(Celsius) offset 40C	*/

	UCHAR	ucBMIC_Ctrl_Status;					/* BMIC Control Status */

	UCHAR	ucBMIC_Cell_Balancer;				/* Target line of cell balance */
	UCHAR	ucCellBalancingCounter;				/* Cell balance control time	*/
	ULONG	ulBMIC_Cell_Balancetgt;				/* Target cell of cell balance request	*/
	ULONG	ulBMIC_Cell_Balancetgt_Mnt;			/* Target cell of cell balance request in maintenance mode	*/
	ULONG	ulBMIC_Cell_Balancetgt_1sec;		/* Target cell of cell balance at 1sec	*/
	ULONG	ulBMIC_CBL_Odd_Mask;

	ULONG	ulRfuse[MAX_THERMISTOR_NUM];

	UCHAR	ucBMIC_AdState;

	UCHAR	ucBMIC_Cnt_1sec;
	UCHAR	ucBMIC_Cnt_1sec_CB;
	UCHAR	ucTmoni_read_req;

	USHORT	usSPI_ComAtLeastOnceFlg;
	UCHAR	ucBMIC_SPI_status;
	UCHAR	ucBMIC_SPI_err_count;
	UCHAR	ucBMIC_SPI_err_detail;
	UCHAR	ucBMIC_ComOffCount;
	UCHAR	ucBMIC_ShutdownReq;
	ULONG	ulBMIC_ChkSpiMisoCount;
	bool	bBMIC_NgSpiMiso;
	ULONG	ulBMIC_ConnectNgCell;
	ULONG	ulBMIC_ConnectNgCell_buff;
}TBMIC_Drv;


/****************************************************************************
 define type (Enumerated type ENUM)
*****************************************************************************/
/****************************************************************************
 Variable declaration
*****************************************************************************/
static TBMIC_Drv	BMIC_Drv;
static UCHAR		BMIC_SpiBusy;
TBMICCellConf		BMICCellConf;
TBMIC_Info			BMIC_Info;
TBMICSetParam 		BMICSetParam;

#ifdef BATTERY_THERMISTOR
static UCHAR ucBatOvTempDischrgStat;		/* over temaparature state(BAT)	*/
static UCHAR ucBatLowTempDischrgStat;		/* low temaparature state(BAT)	*/
static UCHAR ucBatOvTempChrgStat;			/* over temaparature state(BAT)	*/
static UCHAR ucBatLowTempChrgStat;		/* low temaparature state(BAT)	*/
#endif	/* BATTERY_THERMISTOR */
static USHORT usChkTempBuff;
static USHORT usChkTempFixBuff;

static USHORT usBMIC_CurClbCount;
static LONG lBMIC_CurClbSum;
static LONG lBMIC_CurClbReference_1000;
static float fBMIC_CurClbGain;
static UCHAR ucBMIC_CurClbFin;
static bool bRegChkEnable = true;
static ULONG ulCurErr_ClrTime;

/*static USHORT	SHUNT_mohm = 1u;*/
static USHORT	SHUNT_uohm = 1000u;			/* default: 1000uOhm (1mOhm), 500/1000/1500/2000/3000/5000 uOhm, please match actual Shunt Resistor value */
/****************************************************************************
 Prototype declaration
*****************************************************************************/
static void vBMIC_ExeCurCalibration(void);

static void vBMIC_Register_Init(void);
void		vBMIC_InitParams(void);
void 		vBMIC_Start_Init(void);
static UCHAR 	ucBMIC_Make_CellList(void);
UCHAR		ucBMIC_CellConf_Init(void);
void		vBMIC_Ctrl_Startup(void);
void		vBMIC_Ctrl_Normal(void);
void		vBMIC_Ctrl_SpiErr(void);
void		vBMIC_Ctrl_Restart(void);
void		vBMIC_Ctrl_Shutdown(void);
void		vBMIC_ShutdownRequest(void);

void		vBMIC_SetCtrlStatus(void);
UCHAR	getBMIC_Ctrl_Status(void);

static void		vBMIC_ClearADSum(void);
static void		vBMIC_ClearADMinMax(void);
static void		vBMIC_ADIL_Latch(void);
static void		vBMIC_Start_ADC(void);
static void		vBMIC_CalcCVReadSum(void);
static void		vBMIC_CalcAdAve(void);
static void		vBMIC_CalcMeasuredVol(void);
static void		vBMIC_CalcMeasuredTemp(void);
static void		vBMIC_CalcMeasuredCur(void);
void		vBMIC_Error_Ctrl(void);
void		vBMIC_Register_ReadCtrl(void);
bool		bBMIC_ChkOverTemparature(SHORT temp, USHORT thresh, USHORT recover, UCHAR *status);
bool		bBMIC_ChkLowTemparature(SHORT temp, USHORT thresh, USHORT recover, UCHAR *status);
USHORT	usBMIC_Chk_Temparature(void);
UCHAR 	ucChk_Cellbalance_Req(void);
void		vBMIC_CBL_Control(void);
void		vBMICRegInitRead(void);
UCHAR		ucGetBMIC_SDO_In(void);
UCHAR 	ucGetBMIC_ADIRQ2_In(void);
void		Shutdown_Ctrl(UCHAR output);
void		Wakeup_Ctrl(void);
void		vBMICWriteReg(UCHAR addr, USHORT data);
void		vBMICWriteReg_Mask(UCHAR addr, USHORT usdata, USHORT usMask, UCHAR ucdata);
USHORT	usBMICReadReg(UCHAR addr);
void 		vBMICReadReg_Multi(UCHAR addr, UCHAR num);
UCHAR		ucBMIC_Check_SPI_status(void);
UCHAR		ucBMIC_Check_SPI_status_crc(UCHAR crc_chk);
bool		bBMICCheckSpiResult(void);
USHORT	usBMICReferRegData(UCHAR addr);
void		vBMICInputRecvData(UCHAR addr, USHORT usdata);
UCHAR		ucBMICRegInitChk(void);
UCHAR		ucBMICChk_StatusReg(void);
UCHAR		ucBMICChk_CtrlReg(void);
UCHAR		ucBMICReadFuse(void);
ULONG	ulBMICCalcRfuse(SHORT sfuse);
SHORT		sCalcTemperature_01deg(USHORT vtm_ad, UCHAR num);
SHORT		iLookupTemp_01deg(ULONG rth);
UCHAR		ucGetBMICcrc(UCHAR *buf, UCHAR size);
void		vBMIC_Wait(USHORT usdata);
void		vBMIC_Register_Check(void);
void		vBMIC_CellCount_Check(void);
void		vBMIC_CBL_Init(void);
bool		bBMICGetRegChkState(void);
void		Wait_msec(USHORT time_inms);
void		BMIC_SpiRelease(void);
bool		bBMIC_SpiTake(USHORT timeout);
/****************************************************************************
 define data table
*****************************************************************************/
/*--------------------------------------------*/
typedef struct {
	UCHAR m_ucreg;				/* Write Address */
	USHORT m_usdata;				/* Write Data or Read count */
}TYPE_INITTBL;

/*--------------------------------------------*/
const static TYPE_INITTBL stCtrlReg_Init_BMIC[] ={
	{ PWR_CTRL_ADDR,	PWR_CTRL_INI	},
	{ REG_INT_EN_ADDR,	REG_INT_EN_INI	},
	{ SPIWD_CTRL_ADDR,	SPIWD_CTRL_INI	},
	{ FDRV_CTRL_ADDR,	FDRV_CTRL_INI	},
	{ CVSEL1_ADDR,		CVSEL1_INI		},
	{ CVSEL2_ADDR,		CVSEL2_INI		},
	{ GVSEL_ADDR,		GVSEL_INI		},

	{ FUSE_CTL1_ADDR,	FUSE_CTL1_INI	},
	{ FUSE_CTL2_ADDR,	FUSE_CTL2_INI	},

	{ OUVCTL1_ADDR,		OUVCTL1_INI		},
	{ OUVCTL2_ADDR,		OUVCTL2_INI		},

	{ OP_MODE_ADDR, 	OP_MODE_INI		},

	{ GPIO_CTRL1_ADDR,	GPIO_CTRL1_INI	},
	{ GPIO_CTRL2_ADDR,	GPIO_CTRL2_INI	},
	{ GPIO_CTRL3_ADDR,	GPIO_CTRL3_INI	},

	{ GPOH_CTL_ADDR,	GPOH_CTL_INI	},

	{ GPIO_CTRL4_ADDR,	GPIO_CTRL4_INI	},

	{ ALARM_CTRL1_ADDR, ALARM_CTRL1_INI	},
	{ ALARM_CTRL2_ADDR, ALARM_CTRL2_INI	},
	{ ALARM_CTRL3_ADDR, ALARM_CTRL3_INI	},

	{ CBSEL_ADDR, 		CBSEL_INI		},
	{ CBSEL_22_ADDR, 	CBSEL_22_INI	},
	{ OTHCTL_ADDR, 		OTHCTL_INI 		},

	{ ADCTL_ADDR, 		ADCTL_INI		},

	{ INRCV1_ADDR, 		INRCV1_INI	 	},
	{ INRCV2_ADDR, 		INRCV2_INI	 	},

	{ INR_CTL_DIAG_EN_ADDR, 		INR_CTL_DIAG_EN_INI	 	},
};


/****************************************************************************
FUNCTION	: vBMIC_CtrlParameter_Init
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
static void vBMIC_CtrlParameter_Init(void)
{
	UCHAR i = 0u;

	BMIC_Drv.ucBMIC_Ctrl_Status = BMIC_STARTUP;
	BMIC_Drv.ucTmoni_read_req = 1u;

	BMIC_Info.BMICMeasured = false;
	BMIC_Info.ullChgSum_AD = 0u;
	BMIC_Info.ullDisChgSum_AD = 0u;
	BMIC_Info.ucMaintenanceMode = 0u;
	BMIC_Info.ulBatPackErr = 0u;
	BMIC_Info.ulBatPackErrLog = 0u;

	fBMIC_CurClbGain = 1.0;
	ucBMIC_CurClbFin = 0u;
	BMIC_Drv.ulBMIC_Cell_Balancetgt = 0u;

	for(i=0; i< THERMISTOR_NUM; i++){
		BMIC_Drv.ulTmoniSumAD[i] = 0u;
		BMIC_Drv.usTmoniAveAD[i] = 0u;
		BMIC_Drv.usTmoniC_40[i] = 0u;
	}

	BMICSetParam.usOCInfo_cur = 1000u;				/* OC threshold before fuse cut: 1000mA */
/*	BMICSetParam.usOVInfo_vol = 4250u;*/			/* OV threshold: 4250mV */
	BMICSetParam.usOVInfo_vol = 3500u;				/* OV threshold: 3500mV */
	BMICSetParam.usOVInfo_delay = 800u;				/* OV delay: 800ms */
	BMICSetParam.usOVInfo_hys = 100u;				/* OV hysteresis: 100mV */
/*	BMICSetParam.usUVInfo_vol = 2700u;*/			/* UV threshold: 2700mV */
	BMICSetParam.usUVInfo_vol = 500u;				/* UV threshold: 500mV */
	BMICSetParam.usUVInfo_delay = 800u;				/* UV delay: 800ms */
	BMICSetParam.usUVInfo_hys = 100u;				/* UV hysteresis: 100mV */

/*	BMICSetParam.usOCCInfo_cur = 10u;*/				/* OCC threshold: 10A */
	BMICSetParam.usOCCInfo_cur = 20u;				/* OCC threshold: 20A */
	BMICSetParam.usOCCInfo_delay = 10u;				/* OCC delay: 10ms */
/*	BMICSetParam.usOCDInfo_cur = 20u;*/				/* OCD threshold: */
	BMICSetParam.usOCDInfo_cur = 40u;				/* OCD threshold: 40A */
	BMICSetParam.usOCDInfo_delay = 10u;				/* OCD delay: 10ms */
/*	BMICSetParam.usSCDInfo_cur = 40u;*/				/* SCD threshold: 40A */
	BMICSetParam.usSCDInfo_cur = 60u;				/* SCD threshold: 60A */
	BMICSetParam.ulSCDInfo_delay = 31250u;			/* SCD delay: 31250nsec (31.25us) */

#if(BMS_STATE_CTRL)
	/* temperature error check in application vTsk_sys() */
#else
	BMICSetParam.usOTBatInfo_temp = (USHORT)OFFSET40(60u);
	BMICSetParam.usOTBatInfo_temp_recover = (USHORT)OFFSET40(50u);
	BMICSetParam.usLTBatInfo_temp = (USHORT)OFFSET40(-15);
	BMICSetParam.usLTBatInfo_temp_recover = (USHORT)OFFSET40(-13);
	BMICSetParam.usOTBatChrgInfo_temp = (USHORT)OFFSET40(42u);
	BMICSetParam.usOTBatChrgInfo_temp_recover = (USHORT)OFFSET40(40u);
	BMICSetParam.usLTBatChrgInfo_temp = (USHORT)OFFSET40(5u);
	BMICSetParam.usLTBatChrgInfo_temp_recover = (USHORT)OFFSET40(7u);
#endif	/* BMS_STATE_CTRL */

#if(BMIC_49522)

	#if(1)
	BMICCellConf.ulCellPos = 0x003FFFFFu;			/* assume connected 22 cells */
	BMICCellConf.ucSeriesCount = 22u;				/* assume connected 22 cells */
	#endif

	#if(0)
	BMICCellConf.ulCellPos = 0x00300003u;			/* assume connected 4 cells */
	BMICCellConf.ucSeriesCount = 4u;				/* assume connected 4 cells */
	#endif

#else

	#if(1)	/* 17cells */
	BMICCellConf.ulCellPos = 0x0001FFFFu;			/* assume connected 17 cells */
	BMICCellConf.ucSeriesCount = 17u;				/* assume connected 17 cells */
	#endif

	#if(0)	/* 4cells */
	BMICCellConf.ulCellPos = 0x00018003u;			/* assume connected 4 cells */
	BMICCellConf.ucSeriesCount = 4u;				/* assume connected 4 cells */
	#endif

#endif	/* BMIC_49522 */

	BMICCellConf.ucCellTempCount = 5u;				/* Cell temperature measurement: 5 */
	BMICCellConf.ulCellTempPos = 0x1Fu;				/* assume TMONI1, 2, 3, 4 & 5 to measure temperature */

}

/****************************************************************************
FUNCTION	: usBMIC_Calc_OUVCTL1
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/13
*****************************************************************************/
static USHORT usBMIC_Calc_OUVCTL1(void)
{
	USHORT ret = OUVCTL1_INI;
	UCHAR valid = 1u;
	USHORT ovth = 0u, uvth = 0u;
	USHORT octh = 0u;
	USHORT dat = 0u;

	switch(BMICSetParam.usOCInfo_cur){
		case 1000u:
			octh = 0u;					/* OCTH: 1000mA (default)	*/
			break;
		case 600u:
			octh = 1u;					/* OCTH: 600mA 	*/
			break;
		case 300u:
			octh = 2u;					/* OCTH: 300mA 	*/
			break;
		case 100u:
			octh = 3u;					/* OCTH: 100mA 	*/
			break;
		default:
			valid = 0u;
			break;
	}

	if(BMICSetParam.usOVInfo_vol >= 2000u){
		dat = BMICSetParam.usOVInfo_vol;
		if(dat > 4500u){
			dat = 4500u;
		}
		ovth = (dat - 2000u) / 50u + 2u;					/* OVTH = (usOVInfo_vol(mV) - 2000(mV)) / 50(mV) + 2(offset) */
	} else {
		valid = 0u;
	}
	if(BMICSetParam.usUVInfo_vol >= 500u){
		dat = BMICSetParam.usUVInfo_vol;
		if(dat > (USHORT)3000){
			dat = 3000u;
		}
		uvth = (dat - 500u) / 50u;							/* UVTH = (usUVInfo_vol(mV) - 2000(mV)) / 50(mV) */
	} else {
		valid = 0u;
	}

	if(valid == 1u){
		ret = (USHORT)(octh << 12u)| (USHORT)(ovth << 6u) | uvth;
	}
	return ret;
}

/****************************************************************************
FUNCTION	: vBMIC_Send_OUVCTL1
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/12/18
*****************************************************************************/
void vBMIC_Send_OUVCTL1(void)
{
	USHORT data = 0u;
	data = usBMIC_Calc_OUVCTL1();
	ucBMIC_Send_Req(OUVCTL1_ADDR, data, 0xFFFFu, BMIC_DATA_INPUT_NEED);
}

/****************************************************************************
FUNCTION	: usBMIC_Calc_OUVCTL2
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/13
*****************************************************************************/
static USHORT usBMIC_Calc_OUVCTL2(void)
{
	USHORT dat = 0u;
	USHORT ov_hys = 0u;
	USHORT uv_hys = 0u;
	USHORT ov_dly = 0u;
	USHORT uv_dly = 0;

	dat = BMICSetParam.usOVInfo_hys;
	if(dat < 25u){
		dat = 25u;
	} else if(dat > 200u){
		dat = 200u;
	}
	ov_hys = (dat - 25u) / 25u;						/* ov_hys = (usOVInfo_hys(mv) - 25mV) / 25u */

	dat = BMICSetParam.usUVInfo_hys;
	if(dat < 25u){
		dat = 25u;
	} else if(dat > 200u){
		dat = 200u;
	}
	uv_hys = (dat - 25u) / 25u;						/* uv_hys = (usUVInfo_hys(mv) - 25mV) / 25u */

	dat = BMICSetParam.usOVInfo_delay;
	if(dat > 6000u){
		dat = 6000u;
	} else if (dat < 200u){
		dat = 200u;
	}
	if(dat >= (USHORT)1500){
		ov_dly = (dat - 1500u) / 1500u + 4u;		/* ov_dly = (usOVInfo_delay(msec) - 1500msec)/ 1500u + 4u */
	}else{
		ov_dly = (dat - 200u) / 200u;				/* ov_dly = (usOVInfo_delay(msec) - 200msec)/ 200u */
	}

	dat = BMICSetParam.usUVInfo_delay;
	if(dat > 6000u){
		dat = 6000u;
	} else if (dat < 200u){
		dat = 200u;
	}
	if(dat >= (USHORT)1500){
		uv_dly = (dat - 1500u) / 1500u + 4u;		/* uv_dly = (usUVInfo_delay(msec) - 1500msec)/ 1500u - 4u	*/
	}else{
		uv_dly = (dat - 200u) / 200u;				/* uv_dly = (usUVInfo_delay(msec) - 200msec)/ 200u	*/
	}

	USHORT ret = (USHORT)(ov_hys << 12u) | (USHORT)(uv_hys << 8u) | (USHORT)(ov_dly << 4u) | uv_dly;

	return ret;
}

/****************************************************************************
FUNCTION	: vBMIC_Send_OUVCTL2
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/12/18
*****************************************************************************/
void vBMIC_Send_OUVCTL2(void)
{
	USHORT data = usBMIC_Calc_OUVCTL2();
	ucBMIC_Send_Req(OUVCTL2_ADDR, data, 0xFFFFu, BMIC_DATA_INPUT_NEED);
}

/****************************************************************************
FUNCTION	: usBMIC_Calc_ALARM_CTL2
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/13
*****************************************************************************/
static USHORT usBMIC_Calc_ALARM_CTL2(void)
{
	USHORT scd_d = 0u;
	USHORT ocd_d = 0u;
	USHORT occ_d = 0u;
	USHORT ret = 0u;

	if(BMICSetParam.usSCDInfo_cur == 0u){
		scd_d = 0u;
/*	} else if((BMICSetParam.usSCDInfo_cur * SHUNT_mohm % 20u) == 0){	*/
/*		scd_d = BMICSetParam.usSCDInfo_cur * SHUNT_mohm / 20u - 1u;     */
/*	} else {                                                            */
/*		scd_d = BMICSetParam.usSCDInfo_cur * SHUNT_mohm / 20u;          */
/*	}                                                                   */
	} else if((BMICSetParam.usSCDInfo_cur * SHUNT_uohm % 20000u) == 0){
		scd_d = BMICSetParam.usSCDInfo_cur * SHUNT_uohm / 20000u - 1u;
	} else {
		scd_d = BMICSetParam.usSCDInfo_cur * SHUNT_uohm / 20000u;
	}
	if(scd_d > 0x1Fu){														/* max 5bit	*/
		scd_d = 0x1F;
	}

	if(BMICSetParam.usOCDInfo_cur == 0u){
		ocd_d = 0u;
/*	} else if((BMICSetParam.usOCDInfo_cur * SHUNT_mohm % 10u) == 0){	*/
/*		ocd_d = BMICSetParam.usOCDInfo_cur * SHUNT_mohm / 10u - 1u;     */
/*	} else {                                                            */
/*		ocd_d = BMICSetParam.usOCDInfo_cur * SHUNT_mohm / 10u;          */
/*	}                                                                   */
	} else if((BMICSetParam.usOCDInfo_cur * SHUNT_uohm % 10000u) == 0){
		ocd_d = BMICSetParam.usOCDInfo_cur * SHUNT_uohm / 10000u - 1u;
	} else {
		ocd_d = BMICSetParam.usOCDInfo_cur * SHUNT_uohm / 10000u;
	}
	if(ocd_d > 0x1Fu){														/* max 5bit	*/
		ocd_d = 0x1F;
	}

	if(BMICSetParam.usOCCInfo_cur == 0u){
		occ_d = 0u;
/*	} else if((BMICSetParam.usOCCInfo_cur * SHUNT_mohm % 5u) == 0){		*/
/*		occ_d = BMICSetParam.usOCCInfo_cur * SHUNT_mohm / 5u - 1u;      */
/*	} else {                                                            */
/*		occ_d = BMICSetParam.usOCCInfo_cur * SHUNT_mohm / 5u;           */
/*	}                                                                   */
	} else if((BMICSetParam.usOCCInfo_cur * SHUNT_uohm % 5000u) == 0){
		occ_d = BMICSetParam.usOCCInfo_cur * SHUNT_uohm / 5000u - 1u;
	} else {
		occ_d = BMICSetParam.usOCCInfo_cur * SHUNT_uohm / 5000u;
	}
	if(occ_d > 0x1Fu){														/* max 5bit	*/
		occ_d = 0x1F;
	}

	ret = (USHORT)(scd_d << 10u) | (USHORT)(ocd_d << 5u) | occ_d;
	return ret;

}

/****************************************************************************
FUNCTION	: vBMIC_Send_ALARM_CTL2(void)
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/12/18
*****************************************************************************/
void vBMIC_Send_ALARM_CTL2(void)
{
	USHORT data = usBMIC_Calc_ALARM_CTL2();
	ucBMIC_Send_Req(ALARM_CTRL2_ADDR, data, 0xFFFFu, BMIC_DATA_INPUT_NEED);
}

/****************************************************************************
FUNCTION	: usBMIC_Calc_ALARM_CTL3
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/13
*****************************************************************************/
static USHORT usBMIC_Calc_ALARM_CTL3(void)
{
	ULONG scddelay_dat = BMICSetParam.ulSCDInfo_delay;
	USHORT scd_dly = 0u;
	USHORT dat = 0u;
	USHORT ocd_dly = 0u;
	USHORT occ_dly = 0u;
	USHORT ret = 0u;

	if(scddelay_dat == 0u){
		scd_dly = 0u;
	} else {
		scd_dly = (USHORT)(scddelay_dat / 31250u);					/* (nsec)/31250(nsec)	*/
	}
	if(scd_dly  > 0x1Fu){									/* max 5bit	*/
		scd_dly = 0x1F;
	}

	dat = BMICSetParam.usOCDInfo_delay;
	if(dat == 0u){
		ocd_dly = 0u;
	} else {
		ocd_dly = dat / (USHORT)10 - 1u;							/* (msec)/10(msec) - 1	*/
	}
	if(ocd_dly > 0x1Fu){									/* max 5bit	*/
		ocd_dly = 0x1F;
	}

	dat = BMICSetParam.usOCCInfo_delay;
	if(dat == 0u){
		occ_dly = 0u;
	} else {
		occ_dly = dat / (USHORT)10 - 1u;							/* (msec)/10(msec) - 1	*/
	}
	if(occ_dly > 0x1Fu){									/* max 5bit	*/
		occ_dly = 0x1F;
	}

	ret = (USHORT)(scd_dly << 10u) | (USHORT)(ocd_dly << 5u) | occ_dly;
	return ret;

}

/****************************************************************************
FUNCTION	: vBMIC_Send_ALARM_CTL3(void)
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/12/18
*****************************************************************************/
void vBMIC_Send_ALARM_CTL3(void)
{
	USHORT data = usBMIC_Calc_ALARM_CTL3();
	ucBMIC_Send_Req(ALARM_CTRL3_ADDR, data, 0xFFFFu, BMIC_DATA_INPUT_NEED);
}

/****************************************************************************
FUNCTION	: vBMIC_Register_Init
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
static void vBMIC_Register_Init(void)
{
	/* Controll Register Initial Setting */
	vBMICWriteReg(SPIWD_CTRL_ADDR, SPIWD_CTRL_INI);
	vBMICWriteReg(REG_INT_EN_ADDR, REG_INT_EN_INI);
	vBMICWriteReg(PWR_CTRL_ADDR, PWR_CTRL_INI);
	vBMICWriteReg(FDRV_CTRL_ADDR, FDRV_CTRL_INI);
	vBMICWriteReg(CVSEL1_ADDR, CVSEL1_INI);
	vBMICWriteReg(CVSEL2_ADDR, CVSEL2_INI);
	vBMICWriteReg(GVSEL_ADDR, GVSEL_INI);
	vBMICWriteReg(OUVCTL1_ADDR, OUVCTL1_INI);
	vBMICWriteReg(OUVCTL2_ADDR, OUVCTL2_INI);

	vBMICWriteReg(GPIO_CTRL2_ADDR, GPIO_CTRL2_INI);
	vBMICWriteReg(GPIO_CTRL1_ADDR, GPIO_CTRL1_INI);
	vBMICWriteReg(GPIO_CTRL3_ADDR, GPIO_CTRL3_INI);
	vBMICWriteReg(GPIO_CTRL4_ADDR, GPIO_CTRL4_INI);

	vBMICWriteReg(GPOH_CTL_ADDR, GPOH_CTL_INI);

	vBMICWriteReg(ALARM_CTRL1_ADDR, ALARM_CTRL1_INI);
	vBMICWriteReg(ALARM_CTRL2_ADDR, ALARM_CTRL2_INI);
	vBMICWriteReg(ALARM_CTRL3_ADDR, ALARM_CTRL3_INI);

	vBMICWriteReg(CBSEL_ADDR, CBSEL_INI);
	vBMICWriteReg(OTHCTL_ADDR, OTHCTL_INI);

	vBMICWriteReg(ADCTL_ADDR, ADCTL_INI);

	vBMICWriteReg(INRCV1_ADDR, INRCV1_INI);
	vBMICWriteReg(INRCV2_ADDR, INRCV2_INI);

	vBMICWriteReg(INR_CTL_DIAG_EN_ADDR, INR_CTL_DIAG_EN_INI);

	vBMICWriteReg(OTHSTAT_ADDR, 0);
	vBMICWriteReg(OVL_STAT1_ADDR, 0);			/* clear any flag in OVL_STAT1 register */
	vBMICWriteReg(OVL_STAT2_ADDR, 0);			/* clear any flag in OVL_STAT2 register */
	vBMICWriteReg(UVL_STAT1_ADDR, 0);			/* clear any flag in UVL_STAT1 register */
	vBMICWriteReg(UVL_STAT2_ADDR, 0);			/* clear any flag in UVL_STAT2 register */
}


/****************************************************************************
FUNCTION	: ucBMIC_Make_CellList
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
static UCHAR ucBMIC_Make_CellList(void)
{
#if(BMIC_49522)
	const ULONG bitpat_tbl[19] = {
		BITPAT_4CELL,
		BITPAT_5CELL,
		BITPAT_6CELL,
		BITPAT_7CELL,
		BITPAT_8CELL,
		BITPAT_9CELL,
		BITPAT_10CELL,
		BITPAT_11CELL,
		BITPAT_12CELL,
		BITPAT_13CELL,
		BITPAT_14CELL,
		BITPAT_15CELL,
		BITPAT_16CELL,
		BITPAT_17CELL,
		BITPAT_18CELL,
		BITPAT_19CELL,
		BITPAT_20CELL,
		BITPAT_21CELL,
		BITPAT_22CELL,
	};
#else     	/* BMIC_49517 */
	const ULONG bitpat_tbl[14] = {
		BITPAT_4CELL,
		BITPAT_5CELL,
		BITPAT_6CELL,
		BITPAT_7CELL,
		BITPAT_8CELL,
		BITPAT_9CELL,
		BITPAT_10CELL,
		BITPAT_11CELL,
		BITPAT_12CELL,
		BITPAT_13CELL,
		BITPAT_14CELL,
		BITPAT_15CELL,
		BITPAT_16CELL,
		BITPAT_17CELL,
	};
#endif	/* BMIC_49522 */

	UCHAR chk = 0u;
	UCHAR cellnum = BMICCellConf.ucSeriesCount;
	UCHAR i = 0u;

	for(i=0u; i<MAX_CELL_NUM; i++){
		BMIC_Drv.ucCellList[i] = 0u;
		BMIC_Drv.ucOpenCellList[i] = 0u;
	}

	if((MIN_CELL_NUM <= cellnum)&&(cellnum <= MAX_CELL_NUM)){
		UCHAR cellcnt = 0u;
		UCHAR open_cellcnt = 0u;
		ULONG bitpat = bitpat_tbl[BMICCellConf.ucSeriesCount - MIN_CELL_NUM];

		BMICCellConf.ulCellPos = (ULONG)bitpat;
		chk = 1u;

		for(i=0; i<MAX_CELL_NUM; i++ ){
			if((bitpat & 0x1u) != 0){
				BMIC_Drv.ucCellList[cellcnt] = i;
				cellcnt++;
			} else {
				BMIC_Drv.ucOpenCellList[open_cellcnt] = i;
				open_cellcnt++;
			}
			bitpat = bitpat >> 1u;
		}
		vBMIC_CBL_Init();
	}


	return chk;
}


/****************************************************************************
FUNCTION	: vBMIC_InitParams
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_InitParams(void)
{
	USHORT i;

	BMIC_Drv.ucBMIC_ShutdownReq = 0u;
	BMIC_Drv.ulBMIC_ChkSpiMisoCount = 0u;
	BMIC_Drv.bBMIC_NgSpiMiso = false;
	BMIC_Drv.usSPI_ComAtLeastOnceFlg = 0u;
	BMIC_Drv.ulBMIC_ConnectNgCell = 0u;
	BMIC_Drv.ulBMIC_ConnectNgCell_buff = 0u;
	BMIC_Drv.ucBMIC_SPI_err_detail = 0u;

	vBMIC_CtrlParameter_Init();

	for(i=0u; i<DATA_ELEMENTSIZE; i++){
		BMIC_Drv.usRegData[i]=(USHORT)0u;
	}

	#ifdef BATTERY_THERMISTOR
	ucBatOvTempDischrgStat = 0u;		/* over temaparature state(BAT) */
	ucBatLowTempDischrgStat = 0u;		/* low temaparature state(BAT)  */
	ucBatOvTempChrgStat = 0u;				/* over temaparature state(BAT) */
	ucBatLowTempChrgStat = 0u;			/* low temaparature state(BAT)  */
	#endif	/* BATTERY_THERMISTOR */

	usChkTempBuff = 0u;
	usChkTempFixBuff = 0u;

	BMIC_SpiRelease();

}


/****************************************************************************
FUNCTION	: ucBMIC_CellConf_Init
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
UCHAR ucBMIC_CellConf_Init(void)
{
	USHORT data;
	UCHAR chk = ucBMIC_Make_CellList();
	volatile bool spi_result;
	UCHAR cell_conf_stat = OK;
	if(chk == OK){
		data = usBMIC_Calc_OUVCTL2();
		vBMICWriteReg_Mask(OUVCTL2_ADDR, data, 0xFFFFu, BMIC_DATA_INPUT_NEED);
		spi_result = bBMICCheckSpiResult();
		cell_conf_stat &= (UCHAR)spi_result;

		data = usBMIC_Calc_OUVCTL1();
		vBMICWriteReg_Mask(OUVCTL1_ADDR, data, 0xFFFFu, BMIC_DATA_INPUT_NEED);
		spi_result = bBMICCheckSpiResult();
		cell_conf_stat &= (UCHAR)spi_result;

		data = usBMIC_Calc_ALARM_CTL2();
		vBMICWriteReg_Mask(ALARM_CTRL2_ADDR, data, 0xFFFFu, BMIC_DATA_INPUT_NEED);
		spi_result = bBMICCheckSpiResult();
		cell_conf_stat &= (UCHAR)spi_result;

		data = usBMIC_Calc_ALARM_CTL3();
		vBMICWriteReg_Mask(ALARM_CTRL3_ADDR, data, 0xFFFFu, BMIC_DATA_INPUT_NEED);
		spi_result = bBMICCheckSpiResult();
		cell_conf_stat &= (UCHAR)spi_result;
	} else {
		cell_conf_stat = NG;
	}
	return cell_conf_stat;
}


/****************************************************************************
FUNCTION	: vBMIC_Ctrl_Startup
DESCRIPTION	:
INPUT		: None
OUTPUT		: None
UPDATE		: 2021/05/12
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_Ctrl_Startup(void)
{
	UCHAR ret;

	/* BMIC soft reset start: Perform soft reset on BMIC for CVDD=REGEXT use case */
	vBMICWriteReg(PWR_CTRL_ADDR, PWR_CTRL_NPD_RST);			/* Set NPD_RST to 0: Reset */
	Wait_msec(1u);											/* wait 1msec before check SDO */
	/* BMIC soft reset end */

	ret = ucGetBMIC_SDO_In();

	if(ret == HIGH){
		UCHAR fuse_jdg = 0u;
		UCHAR cellconf_jdg = 0u;

		BMIC_Drv.bBMIC_NgSpiMiso = false;
		/* BMIC register initial setting */
		vBMIC_Register_Init();
		vBMICWriteReg(OTHSTAT_ADDR, OTHSTAT_SCD_OCD_OCC_MASK | SPI_F_MASK);	/* clear OCC, OCD, SCD alarm flag, SPI_F flag */
		vBMICWriteReg(STAT5_ADDR, STAT5_VPC_LDM_CUR_WDT_MASK);		/* clear WDT, CUR_H, LDM_H, LDM_L, VPC_H, VPC_L flags */

		/* BMIC register initial read */
		vBMICRegInitRead();

		/* BMIC register initial check */
		ret = ucBMICRegInitChk();

		fuse_jdg = ucBMICReadFuse();
		cellconf_jdg = ucBMIC_CellConf_Init();
		if((fuse_jdg == NG) || (cellconf_jdg == NG)){
			ret = NG;
		}
	} else {
		if(BMIC_Drv.ulBMIC_ChkSpiMisoCount >= BMIC_DEAD_DETECT_CNT){
			BMIC_Drv.bBMIC_NgSpiMiso = true;
		} else {
			BMIC_Drv.ulBMIC_ChkSpiMisoCount++;
		}
	}

	if( ret == OK ){
		BMIC_Drv.ucBMIC_Ctrl_Status = BMIC_NORMAL;
		BMIC_Drv.ulBMIC_ChkSpiMisoCount = 0u;
		BMIC_Drv.ucBMIC_SPI_err_detail = 0u;
	} else {	/* BMIC Restart */
		BMIC_Drv.ucBMIC_Ctrl_Status = BMIC_RESTART;
		BMIC_Drv.ucBMIC_ComOffCount = 5u;
	}

}


/****************************************************************************
FUNCTION	: vBMIC_Ctrl_Normal
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_Ctrl_Normal(void)
{
	UCHAR i;
	vBMIC_ClearADSum();
	vBMIC_ClearADMinMax();
	vBMIC_ADIL_Latch();

	for(i = 0u; i<AVERAGE_TIMES; i++){
		vBMIC_Start_ADC();
		vBMIC_CalcCVReadSum();
	}

	vBMIC_CalcAdAve();
	vBMIC_CalcMeasuredVol();
	vBMIC_CalcMeasuredCur();
	vBMIC_CalcMeasuredTemp();

	vBMIC_Register_ReadCtrl();
	vBMIC_Error_Ctrl();
	vBMIC_CBL_Control();
	vBMIC_ExeCurCalibration();

	vBMIC_CellCount_Check();
	vBMIC_Register_Check();
	vBMIC_SetCtrlStatus();
}

/****************************************************************************
FUNCTION	: vBMIC_Ctrl_SpiErr
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_Ctrl_SpiErr(void)
{
	if(BMIC_Drv.ucBMIC_ComOffCount != (UCHAR)0){
		BMIC_Drv.ucBMIC_ComOffCount--;
	} else{		/* 500msec�o��	*/
		UCHAR i;
		USHORT usdata = 0;
		for(i=0u; i < (UCHAR)5; i++){
			usdata = usBMICReadReg(OTHSTAT_ADDR);
		}

		if(BMIC_Drv.ucBMIC_SPI_status == 1u){
			BMIC_Drv.ucBMIC_Ctrl_Status = BMIC_RESTART;
			BMIC_Drv.ucBMIC_ComOffCount = 5u;
		} else{
			if((usdata & SPI_F_MASK) == SPI_F_MASK){
				vBMICWriteReg_Mask(OTHSTAT_ADDR, SPI_F_MASK, SPI_F_MASK, BMIC_DATA_INPUT_NEED);		/* Write SPI_F "1" to clear SPI error flg */
			}
			BMIC_Drv.ucBMIC_Ctrl_Status = BMIC_NORMAL;
			BMIC_Drv.ucBMIC_SPI_err_detail = 0u;
		}
	}
}

/****************************************************************************
FUNCTION	: vBMIC_Ctrl_Restart
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2020/10/12
*****************************************************************************/
void vBMIC_Ctrl_Restart(void)
{
	USHORT usdata = 0;

	if(BMIC_Drv.ucBMIC_ComOffCount != 0u){
		BMIC_Drv.ucBMIC_ComOffCount--;

		usdata = usBMICReadReg(OTHSTAT_ADDR);

		if(bBMICCheckSpiResult() == true){							/* SDO high & CRC check OK? */
			if((usdata & SPI_F_MASK) == SPI_F_MASK){
				vBMICWriteReg_Mask(OTHSTAT_ADDR, SPI_F_MASK, SPI_F_MASK, BMIC_DATA_INPUT_NEED);		/* Write SPI_F "1" to clear SPI error flg */
			}else{
				BMIC_Drv.ucBMIC_Ctrl_Status = BMIC_STARTUP;
			}
		}
	} else{
		/* SPI error persist, go to STARTUP to confirm SDO */
		BMIC_Drv.ucBMIC_Ctrl_Status = BMIC_STARTUP;
	}
}


/****************************************************************************
FUNCTION	: vBMIC_Ctrl_Shutdown
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_Ctrl_Shutdown(void)
{
	if(BMIC_Drv.ucBMIC_ComOffCount != 0u){
		if(BMIC_Drv.ucBMIC_ComOffCount == 5u){
			vBMIC_SHDN_Out(HIGH);
			Wait_msec(1u);						/* SHDN = High for minimum 1ms to put BMIC shutdown */
			vBMIC_SHDN_Out(LOW);
		}
		BMIC_Drv.ucBMIC_ComOffCount--;
	} else{
		BMIC_Drv.ucBMIC_Ctrl_Status = BMIC_STARTUP;
	}

}


/****************************************************************************
FUNCTION	: vBMIC_ShutdownRequest
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_ShutdownRequest(void)
{
	BMIC_Drv.ucBMIC_ShutdownReq = 1u;
}


/****************************************************************************
FUNCTION	: vBMIC_CellCount_Check
DESCRIPTION	: Check open cell's voltage based on CVxx_AD Register data
			: If voltage > CELL_CONNECT_NG_DET_TH_mV, judge as cell connection NG
			: CVSEL for all cells need enable if this routine use for CellCount check
INPUT		: None
OUTPUT		: None
UPDATE		: 2020/10/23
DATE		: 2020/1/10
*****************************************************************************/
void vBMIC_CellCount_Check(void)
{
	UCHAR cnt = MAX_CELL_NUM - BMICCellConf.ucSeriesCount;
	ULONG connect_ng_cell_buff = 0u;
	UCHAR i = 0u;
	ULONG ultemp = 0u;
	ULONG mask = 0u;

	if(bBMIC_ChkCellbalance() == false){
		for(i=0u; i<cnt; i++) {
			ultemp = (ULONG)usBMICReferRegData(BMIC_Drv.ucOpenCellList[i]+CV01_AD_ADDR);
			ultemp *= 5000u;
			ultemp /= 16384u;
			if(ultemp > CELL_CONNECT_NG_DET_TH_mV) {
				connect_ng_cell_buff |= (ULONG)((ULONG)1u << i);
			}
		}

		mask = (BMIC_Drv.ulBMIC_ConnectNgCell_buff ^ connect_ng_cell_buff);
		BMIC_Drv.ulBMIC_ConnectNgCell = (~mask & connect_ng_cell_buff) | (mask & BMIC_Drv.ulBMIC_ConnectNgCell);
		BMIC_Drv.ulBMIC_ConnectNgCell_buff = connect_ng_cell_buff;
	}
}

/****************************************************************************
FUNCTION	: vBMIC_Register_Check
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2020/1/10
*****************************************************************************/
void vBMIC_Register_Check(void)
{
	UCHAR ucjdg = OK;
	USHORT usregdata;

	if (bBMICGetRegChkState() == true){
		usregdata = usBMICReferRegData(GVSEL_ADDR);
		if ( usregdata != GVSEL_INI ) {
			ucjdg = NG;
		}

		USHORT us_ouvctl1 = usBMIC_Calc_OUVCTL1();
		usregdata = usBMICReferRegData(OUVCTL1_ADDR);
		if ( usregdata != us_ouvctl1 ) {
			ucjdg = NG;
		}

		usregdata = usBMICReferRegData(ALARM_CTRL1_ADDR);
		if ( usregdata != ALARM_CTRL1_INI ) {
			ucjdg = NG;
		}

		usregdata = usBMICReferRegData(ADCTL_ADDR);
		if ( usregdata != ADCTL_INI ) {
			ucjdg = NG;
		}

		if( ucjdg == NG ) {
			BMIC_Drv.ucBMIC_ShutdownReq = 1;
		}
	}
}

/****************************************************************************
FUNCTION	: vBMIC_SetCtrlStatus
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_SetCtrlStatus(void)
{
	/*SPI Err	*/
	if(BMIC_Drv.ucBMIC_SPI_status == 1u){
		BMIC_Drv.ucBMIC_SPI_status = 0u;
		BMIC_Drv.ucBMIC_Ctrl_Status = BMIC_SPIERR;
		BMIC_Drv.ucBMIC_ComOffCount = 5u;
		BMIC_Info.BMICMeasured = false;
	}
	if(BMIC_Drv.ucBMIC_ShutdownReq == 1u){
		BMIC_Drv.ucBMIC_ShutdownReq = 0u;
		BMIC_Drv.ucBMIC_Ctrl_Status = BMIC_SHUTDOWN;
		BMIC_Drv.ucBMIC_ComOffCount = 5u;
		BMIC_Info.BMICMeasured = false;
	}
}

/****************************************************************************
FUNCTION	: getBMIC_Ctrl_Status
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
UCHAR getBMIC_Ctrl_Status(void)
{
	return	BMIC_Drv.ucBMIC_Ctrl_Status;
}


/****************************************************************************
FUNCTION	: vBMIC_ClearADSum
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_ClearADSum(void)
{
	UCHAR i;
	for(i=0u; i<MAX_CELL_NUM; i++){
		BMIC_Drv.ulCVSumAD[i] = 0u;
	}
	BMIC_Drv.ulVpackSumAD = 0u;
	BMIC_Drv.ulVddSumAD = 0u;
	BMIC_Drv.ulVdd18SumAD = 0u;
	BMIC_Drv.ulVREGEXTSumAD = 0u;
	BMIC_Drv.ulVREF2SumAD = 0u;
	#if(0)	/* VREF1 not used */
	BMIC_Drv.ulVREF1SumAD = 0u;
	#endif
	BMIC_Drv.lCISumAD = 0;	/* signed	*/

	for(i=0u; i<THERMISTOR_NUM; i++){
		BMIC_Drv.ulTmoniSumAD[i] = 0u;
	}

	BMIC_Drv.ucBMIC_AdState = 0u;
}

/****************************************************************************
FUNCTION	: vBMIC_ClearADMinMax(void)
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2020/02/03
*****************************************************************************/
static void vBMIC_ClearADMinMax(void)
{
	UCHAR i;
	for(i=0u; i<MAX_CELL_NUM; i++){
		BMIC_Drv.usCVAD_max[i] = 0u;
		BMIC_Drv.usCVAD_min[i] = 0xFFFFu;
	}
	BMIC_Drv.usVpackAD_max = 0u;
	BMIC_Drv.usVpackAD_min = 0xFFFFu;

	BMIC_Drv.usVddAD_max = 0u;
	BMIC_Drv.usVddAD_min = 0xFFFFu;

	BMIC_Drv.usVdd18AD_max = 0u;
	BMIC_Drv.usVdd18AD_min = 0xFFFFu;

	BMIC_Drv.usVREGEXTAD_max = 0u;
	BMIC_Drv.usVREGEXTAD_min = 0xFFFFu;

	BMIC_Drv.usVREF2AD_max = 0u;
	BMIC_Drv.usVREF2AD_min = 0xFFFFu;

	#if(0)	/* VREF1 not used */
	BMIC_Drv.usVREF1AD_max = 0u;
	BMIC_Drv.usVREF1AD_min = 0xFFFFu;
	#endif

	BMIC_Drv.sCIAD_max = (SHORT)-32768;
	BMIC_Drv.sCIAD_min = (SHORT)32767;

	for(i=0u; i<THERMISTOR_NUM; i++){
		BMIC_Drv.usTmoniAD_max[i] = 0u;
		BMIC_Drv.usTmoniAD_min[i] = 0xFFFFu;
	}
}


/****************************************************************************
FUNCTION	: vBMIC_ADIL_Latch
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:2020/10/14
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_ADIL_Latch(void)
{
	#if(0)
	/* Monitor ADIRQ2 to latch ADIL (BMIC GPIO port need to select ADIRQ2 output)*/
	if(ucGetBMIC_ADIRQ2_In() == HIGH){
		ucBMIC_Send_Req(OP_MODE_ADDR, ADIL_LATCH_ON, ADIL_LATCH_ON, BMIC_DATA_INPUT_NONE);		/* NOT INUPUT */
	}

	#else

	/* Monitor IADS_DONE status to latch ADIL */
	USHORT usdata;
	volatile bool spi_result = true;
	UCHAR	iads_complete = 0;

	usdata = usBMICReadReg(STAT1_ADDR);
	spi_result = bBMICCheckSpiResult();
	if(spi_result){
		if((usdata & IADS_DONE) == IADS_DONE){					/* Check IADS_DONE flg */
			iads_complete = 1;
		}
	}

	if(iads_complete){
		ucBMIC_Send_Req(STAT1_ADDR, IADS_DONE, IADS_DONE, BMIC_DATA_INPUT_NONE);							/* clear IADS_DONE */
		ucBMIC_Send_Req(OP_MODE_ADDR, ADIL_LATCH_ON, ADIL_LATCH_ON, BMIC_DATA_INPUT_NONE);		/* NOT INUPUT */
	}

	#endif
}


/****************************************************************************
FUNCTION	: vBMIC_Start_ADC
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:2020/10/14
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_Start_ADC(void)
{
	USHORT usdata;
	volatile bool spi_result = true;
	UCHAR	adc_complete = 0;
	UCHAR	ucChkCnt = 0;

	if((usBMICReferRegData(PWR_CTRL_ADDR) & ADC_CONT) == 0u){		/* PWR_CTRL_ADDR bit15: ADC_CONT, if 0 (Oneshot mode), set OP_MODE_ADDR bit4 ADC_TRG to start ADC */
		ucBMIC_Send_Req(OP_MODE_ADDR, ADC_TRG_ON, ADC_TRG_ON, BMIC_DATA_INPUT_NONE);				/* NOT INUPUT */
		Wait_msec(1u);												/* wait 1ms before check status */
	}

	while(ucChkCnt < (UCHAR)20){										/* maximum check 20 times, ~ 2ms */
		usdata = usBMICReadReg(STAT1_ADDR);
		spi_result = bBMICCheckSpiResult();
		if(spi_result){
			if((usdata & (VAD_DONE | IADH_DONE)) == (VAD_DONE | IADH_DONE)){			/* Check VAD_DONE flg & IADH_DONE flg */
				adc_complete = 1;
				break;
			}
		}else{
			break;
		}
		ucChkCnt ++;
	}

	if(adc_complete){
		ucBMIC_Send_Req(STAT1_ADDR, VAD_DONE | IADH_DONE, VAD_DONE | IADH_DONE, BMIC_DATA_INPUT_NONE);		/* clear VAD_DONE, IADH_DONE */
	}

	ucBMIC_Send_Req(OP_MODE_ADDR, ADV_LATCH_ON | ADIH_LATCH_ON, ADV_LATCH_ON | ADIH_LATCH_ON, BMIC_DATA_INPUT_NONE);		/* NOT INPUT */
}

/****************************************************************************
FUNCTION	: vBMIC_CalcCVReadSum
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_CalcCVReadSum(void)
{
	USHORT usdata;
	UCHAR i;
	volatile bool spi_result = true;
	UCHAR ad_stat = OK;
	SHORT sdata;

	usdata = usBMICReadReg(OP_MODE_ADDR);
	spi_result = bBMICCheckSpiResult();
	ad_stat &= (UCHAR)spi_result;
	if((usdata & ADV_LATCH_ON) != 0u){
		BMIC_Drv.ucBMIC_AdState |= NOT_FIN_ADV_LATCH;
	}
	if((usdata & ADIH_LATCH_ON) != 0u){
		BMIC_Drv.ucBMIC_AdState |= NOT_FIN_ADIH_LATCH;
	}

	/* Read Cell voltage measurement result */
	for (i=0; i < BMICCellConf.ucSeriesCount; i++) {
		usdata = usBMICReadReg(BMIC_Drv.ucCellList[i]+CV01_AD_ADDR);
		spi_result = bBMICCheckSpiResult();
		ad_stat &= (UCHAR)spi_result;
		BMIC_Drv.ulCVSumAD[i] += usdata;
		COMP_MAX(usdata, BMIC_Drv.usCVAD_max[i]);
		COMP_MIN(usdata, BMIC_Drv.usCVAD_min[i]);
	}

	/* Read HS current measurement result */
	sdata = (SHORT)usBMICReadReg(CVIH_AD_ADDR);
	spi_result = bBMICCheckSpiResult();
	ad_stat &= (UCHAR)spi_result;
	BMIC_Drv.lCISumAD += (LONG)sdata;
	COMP_MAX(sdata, BMIC_Drv.sCIAD_max);
	COMP_MIN(sdata, BMIC_Drv.sCIAD_min);

	/* Read VPACK voltage measurement result */
	usdata = usBMICReadReg(VPACK_AD_ADDR);
	spi_result = bBMICCheckSpiResult();
	ad_stat &= (UCHAR)spi_result;
	BMIC_Drv.ulVpackSumAD += usdata;
	COMP_MAX(usdata, BMIC_Drv.usVpackAD_max);
	COMP_MIN(usdata, BMIC_Drv.usVpackAD_min);

	/* Read VDD55 voltage measurement result */
	usdata = usBMICReadReg(VDD55_AD_ADDR);
	spi_result = bBMICCheckSpiResult();
	ad_stat &= (UCHAR)spi_result;
	BMIC_Drv.ulVddSumAD += usdata;
	COMP_MAX(usdata, BMIC_Drv.usVddAD_max);
	COMP_MIN(usdata, BMIC_Drv.usVddAD_min);

	/* Read VDD18 voltage measurement result */
	usdata = usBMICReadReg(VDD18_AD_ADDR);
	spi_result = bBMICCheckSpiResult();
	ad_stat &= (UCHAR)spi_result;
	BMIC_Drv.ulVdd18SumAD += usdata;
	COMP_MAX(usdata, BMIC_Drv.usVdd18AD_max);
	COMP_MIN(usdata, BMIC_Drv.usVdd18AD_min);

	/* Read REGEXT voltage measurement result */
	usdata = usBMICReadReg(REGEXT_AD_ADDR);
	spi_result = bBMICCheckSpiResult();
	ad_stat &= (UCHAR)spi_result;
	BMIC_Drv.ulVREGEXTSumAD += usdata;
	COMP_MAX(usdata, BMIC_Drv.usVREGEXTAD_max);
	COMP_MIN(usdata, BMIC_Drv.usVREGEXTAD_min);

	/* Read VREF2 voltage measurement result */
	usdata = usBMICReadReg(VREF2_AD_ADDR);
	spi_result = bBMICCheckSpiResult();
	ad_stat &= (UCHAR)spi_result;
	BMIC_Drv.ulVREF2SumAD += usdata;
	COMP_MAX(usdata, BMIC_Drv.usVREF2AD_max);
	COMP_MIN(usdata, BMIC_Drv.usVREF2AD_min);

	#if(0)	/* VREF1 not used */
	/* Read VREF1 voltage measurement result */
	usdata = usBMICReadReg(VREF1_AD_ADDR);
	spi_result = bBMICCheckSpiResult();
	ad_stat &= (UCHAR)spi_result;
	BMIC_Drv.ulVREF1SumAD += usdata;
	COMP_MAX(usdata, BMIC_Drv.usVREF1AD_max);
	COMP_MIN(usdata, BMIC_Drv.usVREF1AD_min);
	#endif

	/* Read TMONIn voltage measurement result */
	if(BMIC_Drv.ucTmoni_read_req == 1u){
		for (i=0; i < THERMISTOR_NUM; i++) {
			usdata = usBMICReadReg(i+TMONI1_AD_ADDR);
			spi_result = bBMICCheckSpiResult();
			ad_stat &= (UCHAR)spi_result;
			BMIC_Drv.ulTmoniSumAD[i] += usdata;
			COMP_MAX(usdata, BMIC_Drv.usTmoniAD_max[i]);
			COMP_MIN(usdata, BMIC_Drv.usTmoniAD_min[i]);
		}
	}

	if(ad_stat == NG){
		BMIC_Drv.ucBMIC_AdState |= AD_AVE_DATA_NG;
	}
}

/****************************************************************************
FUNCTION	: vBMIC_CalcAdAve
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/11
*****************************************************************************/
void vBMIC_CalcAdAve(void)
{
	UCHAR ad_state = BMIC_Drv.ucBMIC_AdState;
	UCHAR i;

	if((ad_state & AD_AVE_DATA_NG) == 0u){
		if((ad_state & NOT_FIN_ADV_LATCH) == 0u){
			if(BMIC_Drv.ucBMIC_Cell_Balancer == NON_CELL_BALANCE ){
				for (i=0; i<BMICCellConf.ucSeriesCount; i++) {
					BMIC_Drv.usCVAveAD[i] = (USHORT)((BMIC_Drv.ulCVSumAD[i] - BMIC_Drv.usCVAD_max[i] - BMIC_Drv.usCVAD_min[i]) >> 2u);
					BMIC_Drv.usSPI_ComAtLeastOnceFlg |= COMM_AD_CELLV;
				}
			}
			if(BMIC_Drv.ucTmoni_read_req == 1u){
				for (i=0; i<THERMISTOR_NUM; i++) {
					BMIC_Drv.usTmoniAveAD[i] = (USHORT)((BMIC_Drv.ulTmoniSumAD[i] - BMIC_Drv.usTmoniAD_max[i] - BMIC_Drv.usTmoniAD_min[i]) >> 2u);
					BMIC_Drv.usSPI_ComAtLeastOnceFlg |= COMM_AD_TMONI;
				}
			}
			BMIC_Drv.usVpackAveAD = (USHORT)((BMIC_Drv.ulVpackSumAD - BMIC_Drv.usVpackAD_max - BMIC_Drv.usVpackAD_min) >> 2u);

			BMIC_Drv.usVddAveAD = (USHORT)((BMIC_Drv.ulVddSumAD - BMIC_Drv.usVddAD_max - BMIC_Drv.usVddAD_min) >> 2u);

			BMIC_Drv.usVdd18AveAD = (USHORT)((BMIC_Drv.ulVdd18SumAD - BMIC_Drv.usVdd18AD_max - BMIC_Drv.usVdd18AD_min) >> 2u);

			BMIC_Drv.usVREGEXTAveAD = (USHORT)((BMIC_Drv.ulVREGEXTSumAD - BMIC_Drv.usVREGEXTAD_max - BMIC_Drv.usVREGEXTAD_min) >> 2u);

			BMIC_Drv.usVREF2AveAD = (USHORT)((BMIC_Drv.ulVREF2SumAD - BMIC_Drv.usVREF2AD_max - BMIC_Drv.usVREF2AD_min) >> 2u);
			#if(0)	/* VREF1 not used */
			BMIC_Drv.usVREF1AveAD = (USHORT)((BMIC_Drv.ulVREF1SumAD - BMIC_Drv.usVREF1AD_max - BMIC_Drv.usVREF1AD_min) >> 2u);
			#endif
			BMIC_Drv.usSPI_ComAtLeastOnceFlg |= COMM_AD_OTHER;
		}

		if((ad_state & NOT_FIN_ADIH_LATCH) == 0u){
			BMIC_Drv.sCIAveAD = (SHORT)((BMIC_Drv.lCISumAD - BMIC_Drv.sCIAD_max - BMIC_Drv.sCIAD_min) / (SHORT)4u);
			BMIC_Drv.usSPI_ComAtLeastOnceFlg |= COMM_AD_IHIGH;
		}
	}
}

/****************************************************************************
FUNCTION	: vBMIC_Error_Ctrl
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_Error_Ctrl(void)
{
	USHORT err1_data = 0u, err2_data = 0u, err_data = 0u;
	ULONG ul_tmp = BMIC_Info.ulBatPackErrLog;		/* get previous error info */
	USHORT tmp_data = 0u;
	USHORT chk_data = 0u;
	ULONG ul_curr_clear_cnt = 0u;

	err1_data = usBMICReferRegData(STAT5_ADDR) & STAT5_OVUV_MASK;	/* STAT5 bit1~0: ST_OV,ST_UV  */
	err2_data |= usBMICReferRegData(OTHSTAT_ADDR) & OTHSTAT_SCD_OCD_OCC_MASK;	/* OTHSTAT bit15~13: ST_SCD,ST_OCD,ST_OCC */
	err_data = (USHORT)(err1_data << 8u) | (USHORT)(err2_data >> 9u);		/* STAT5 bit1~0: ST_OV,ST_UV => err_data bit 9~8, OTHSTAT bit15~13: ST_SCD,ST_OCD,ST_OCC => err_data bit 6~4  */

	tmp_data = err_data & 0x0370u;
	err_data &= 0x0370u;						/* keep bp9,8,6,5,4		*/

	chk_data = (err2_data & OTHSTAT_SCD_OCD_OCC_MASK);	/* bit15~13	*/
	if(chk_data != 0u){
		vBMICWriteReg(OTHSTAT_ADDR,chk_data);			/* clear OTHSTAT register bit15~13: ST_SCD, ST_OCD, ST_OCC	*/
	}

	chk_data = err1_data & ST_UV_MASK;					/* bit0 ST_UV	*/
	if(chk_data != 0u){
		vBMICWriteReg(UVL_STAT1_ADDR,0x0000u);		/* clear UVL_STAT1 register	*/
		vBMICWriteReg(UVL_STAT2_ADDR,0x0000u);		/* clear UVL_STAT2 register */
	}
	else{
		/* When ST_UV is clear */
		err2_data = usBMICReferRegData(UVL_STAT1_ADDR);
		if(err2_data != 0u){
			vBMICWriteReg(UVL_STAT1_ADDR,0x0000u);		/* clear UVL_STAT1 register	*/
		}
		err2_data = usBMICReferRegData(UVL_STAT2_ADDR);
		if(err2_data != 0u){
			vBMICWriteReg(UVL_STAT2_ADDR,0x0000u);		/* clear UVL_STAT2 register	*/
		}
	}

	chk_data = err1_data & ST_OV_MASK;					/* bit1 ST_OV	*/
	if(chk_data != 0u){
		vBMICWriteReg(OVL_STAT1_ADDR,0x0000u);		/* clear OVL_STAT1 register	*/
		vBMICWriteReg(OVL_STAT2_ADDR,0x0000u);		/* clear OVL_STAT2 register */
	}
	else{
		/* When ST_OV is clear */
		err2_data = usBMICReferRegData(OVL_STAT1_ADDR);
		if(err2_data != 0u){
			vBMICWriteReg(OVL_STAT1_ADDR,0x0000u);		/* clear OVL_STAT1 register	if any */
		}
		err2_data = usBMICReferRegData(OVL_STAT2_ADDR);
		if(err2_data != 0u){
			vBMICWriteReg(OVL_STAT2_ADDR,0x0000u);		/* clear OVL_STAT2 register if any */
		}
	}

	chk_data = usBMICReferRegData(STAT5_ADDR) & STAT5_VPC_LDM_CUR_WDT_MASK;		/* STAT5 bit13~8: WDT_F, CUR_H_F, LDM_H_F, LDM_L_H, VPC_H_F, VPC_L_F */
	if(chk_data != 0u){
		vBMICWriteReg(STAT5_ADDR,chk_data);			/* clear STAT5 bit13~8: WDT_F, CUR_H_F, LDM_H_F, LDM_L_H, VPC_H_F, VPC_L_F	*/
	}

	/* MODE_STAT	*/
	chk_data = usBMICReferRegData(STAT1_ADDR);
	chk_data &= ERR_IC_SHUTDOWN;				/* STAT1 bit2 ST_SDWN */
	err_data = (err_data | chk_data);						/* set bp2	*/

	/* CONNECT ERROR	*/
	if(BMIC_Drv.ulBMIC_ConnectNgCell != 0u){
		err_data = (err_data | ERR_CELL_CONNECT_NG);	/* set bp7	*/
	}

#if(BMS_STATE_CTRL)
	/* temperature error check in application vTsk_sys() */
#else
	if(BMIC_Drv.ucTmoni_read_req == 1u){
		/* TEMPARATURE	*/
		chk_data = usBMIC_Chk_Temparature();
		err_data = (err_data | chk_data);					/* set bp13,12,11,10,1,0	*/
	} else {
		err_data &= (USHORT)~MASK_TEMPARATURE_ERR;
		err_data |= (USHORT)(ul_tmp & MASK_TEMPARATURE_ERR);
	}
#endif	/* BMS_STATE_CTRL */

	BMIC_Info.ulBatPackErr = (ULONG)err_data;

	/*ul_curr_clear_cnt = RtcDrv.ulGetRtcFrc();*/
	ul_curr_clear_cnt = ulGetMainSecCnt();
	if((err_data & MASK_CURRENT_ERR) != 0u) {
		ulCurErr_ClrTime = ul_curr_clear_cnt + CURRENT_ERR_CLEAR_TIME_1S;
	}

	err_data &= (USHORT)(~(USHORT)MASK_CURRENT_ERR);
	err_data |= (USHORT)(ul_tmp & MASK_CURRENT_ERR) | (tmp_data & MASK_CURRENT_ERR);

	if(ul_curr_clear_cnt >= ulCurErr_ClrTime) {
		err_data &= (USHORT)(~(USHORT)MASK_CURRENT_ERR);
	}
	BMIC_Info.ulBatPackErrLog = (ULONG)err_data;
}

/****************************************************************************
FUNCTION	: vBMIC_CalcMeasuredVol
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
static void vBMIC_CalcMeasuredVol(void)
{
	ULLONG ulltemp = 0u;
	ULONG ulsum_uV = 0u;
	USHORT usmin = 0xFFFFu;
	USHORT usmax = 0u;
	ULONG ulmax_uV = 0u;
	ULONG ulmin_uV = 0u;
	UCHAR i = 0u;

#if(BMS_STATE_CTRL)	/* CVSEL1&2 can change directly from GUI, if uncheck from GUI, the cell shall not update for minimum AD of block */
	ULONG ulCVSEL = 0u;
	ULONG ulChk_bit;
#endif	/* BMS_STATE_CTRL */

	/*	VPACK voltage	*/
	ulltemp = BMIC_Drv.usVpackAveAD;
	ulltemp *= 110000000u;												/* VPACK max: 110V	*/
	ulltemp /= 16384u;
	BMIC_Info.ulVpackV_uV = (ULONG)ulltemp;		/* Vin = AD * 110000000uV / 16384	*/

	/* VDD55 voltage	*/
	ulltemp = BMIC_Drv.usVddAveAD;
	ulltemp *= 7500000u;													/* VDD55 max: 7.5V	*/
	ulltemp /= 16384u;
	BMIC_Info.ulVdd55V_uV = (ULONG)ulltemp;		/* Vin = AD * 7500000uV / 16384	*/

	/* VDD18 voltage	*/
	ulltemp = BMIC_Drv.usVdd18AveAD;
	ulltemp *= 5000000u;													/* VDD18 max: 5.0V	*/
	ulltemp /= 16384u;
	BMIC_Info.ulVdd18V_uV = (ULONG)ulltemp;		/* Vin = AD * 5000000uV / 16384	*/

	/* REGEXT voltage	*/
	ulltemp = BMIC_Drv.usVREGEXTAveAD;
	ulltemp *= 7500000u;													/* REGEXT max: 7.5V	*/
	ulltemp /= 16384u;
	BMIC_Info.ulVRegextV_uV = (ULONG)ulltemp;	/* Vin = AD * 7500000uV / 16384	*/

	/* VREF2 voltage	*/
	ulltemp = BMIC_Drv.usVREF2AveAD;
	ulltemp *= 5000000u;													/* VREF2 max: 5.0V	*/
	ulltemp /= 16384u;
	BMIC_Info.ulVref2V_uV = (ULONG)ulltemp;		/* Vin = AD * 5000000uV / 16384	*/

	#if(0)	/* VREF1 not used */
	/* VREF1 voltage	*/
	ulltemp = BMIC_Drv.usVREF1AveAD;
	ulltemp *= 5000000u;													/* VREF1 max: 5.0V	*/
	ulltemp /= 16384u;
	BMIC_Info.ulVref1V_uV = (ULONG)ulltemp;		/* Vin = AD * 5000000uV / 16384	*/
	#endif

#if(BMS_STATE_CTRL)	/* CVSEL1&2 can change directly from GUI, if uncheck from GUI, the cell shall not update for minimum AD of block */
	ulCVSEL = ((ULONG)usBMICReferRegData(CVSEL1_ADDR) & (ULONG)0x0000FFFFu) | ((((ULONG)usBMICReferRegData(CVSEL2_ADDR)) & (ULONG)0x0000FFFFu) << 16u);
#endif	/* BMS_STATE_CTRL */

	for(i=0u; i<BMICCellConf.ucSeriesCount; i++){
		ulltemp = BMIC_Drv.usCVAveAD[i];
		ulltemp *= 5000000u;
		ulltemp /= 16384u;
		BMIC_Info.ulBlkVol_uV[i] = (ULONG)ulltemp;

#if(BMS_STATE_CTRL)	/* CVSEL1&2 can change directly from GUI, if uncheck from GUI, the cell shall not update for minimum AD of block */
		ulChk_bit = (ULONG)0x00000001u << BMIC_Drv.ucCellList[i];
		if(ulChk_bit & ulCVSEL){
#endif	/* BMS_STATE_CTRL */
		if(usmin > BMIC_Drv.usCVAveAD[i]){					/* calc Minimum AD of Block voltage	*/
			usmin = BMIC_Drv.usCVAveAD[i];
			ulmin_uV = (ULONG)ulltemp;
		}
#if(BMS_STATE_CTRL) /* CVSEL1&2 can change directly from GUI, if uncheck from GUI, the cell shall not update for minimum AD of block */
		}
#endif	/* BMS_STATE_CTRL */
		if(usmax < BMIC_Drv.usCVAveAD[i]){					/* calc Max AD of Block voltage	*/
			usmax = BMIC_Drv.usCVAveAD[i];
			ulmax_uV = (ULONG)ulltemp;
		}
		ulsum_uV += (ULONG)ulltemp;
	}

	/* �p�b�N�d��, �ő�u���b�N�d���i���n��j, �ŏ��u���b�N�d���i���n��j	*		*/
	BMIC_Info.ulVpackSumV_uV = ulsum_uV;					/* Vbat(sum)[mV] = 1000x AD Sum(All cell) * 5V/2^14 [mV]	*/
	BMIC_Info.ulBatPackMinBlkVol_uV = ulmin_uV;		/* Vin = AD * 5000000uV / 16384	*/
	BMIC_Info.ulBatPackMaxBlkVol_uV = ulmax_uV;
}

/****************************************************************************
FUNCTION	: vBMIC_CalcMeasuredTemp
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
static void vBMIC_CalcMeasuredTemp(void)
{
	/*�T�[�~�X�^AD -> �ێ����x�ϊ� thermistor AD -> Celsisus temperature conversion	*/
	if( BMIC_Drv.ucTmoni_read_req == 1u ){
		USHORT cnt = 0u;
		UCHAR i = 0u;
		SHORT stemp_01C;
		for (i=0; i<THERMISTOR_NUM; i++) {
			if ((BMICCellConf.ulCellTempPos & ((ULONG)1 << i)) != 0u) {
				stemp_01C = sCalcTemperature_01deg(BMIC_Drv.usTmoniAveAD[i], i);
				if(stemp_01C != 0x7FFF){
					BMIC_Info.sTemp_01Cdeg[i] = stemp_01C;
				}
				cnt++;
				if (cnt >= BMICCellConf.ucCellTempCount) {
					break;
				}
			}
		}
	}
}

/****************************************************************************
FUNCTION	: vBMIC_CalcMeasuredCur
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		: 2020/10/19
DATE		: 2019/11/01
*****************************************************************************/
static void vBMIC_CalcMeasuredCur(void)
{
	float ftmp;
	SHORT sSlowIAD;
	bool spi_result;
	USHORT usdata = 0;
/*	USHORT CUR_SUM_MIN_LEVEL_AD = (USHORT)(CUR_SUM_MIN_LEVEL*SHUNT_mohm);*/
	USHORT CUR_SUM_MIN_LEVEL_AD = (USHORT)(CUR_SUM_MIN_LEVEL*SHUNT_uohm/1000u);

	/* �p�b�N�d��, �u���b�N�d�� pack current, block current	*/
/*	BMIC_Info.lBatPackFastCur_100uA = (sCIAveAD * 360) / 65536 / SHUNT_mohm;*/ /* High Speed Current AD	*/
	ftmp = fBMIC_CurClbGain;
/*	ftmp *= 6.103515625;	*/										/* 360 * 10000 /65536 / 3 / 3 = 6.10351526, so again * 3 will be actual current */
/*	ftmp *= SHUNT_mohm;   */
/*	ftmp *= 54.931640625;	*/										/* 360 * 10000 / 65536 / 1 = 54.931640625 */
/*	ftmp *= 54.931640625 / (float)SHUNT_mohm;*/						/* 360 * 10000 / 65536 / SHUNT_mohm = 54.931640625 / SHUNT_mohm */
	ftmp *= (float)54931.640625 / (float)SHUNT_uohm;						/* 360 * 10000 * 1000 / 65536 / SHUNT_uohm = 54931.640625 / SHUNT_uohm */
	ftmp *= (float)BMIC_Drv.sCIAveAD;
	BMIC_Info.lBatPackFastCur_100uA = (LONG)ftmp;				/* fast AD */


/*	BMIC_Info.lBatPackCur_100uA = (sSlowIAD * 360) / 65536 / SHUNT_mohm; */	/* Low Speed Current AD	*/
	sSlowIAD = (SHORT)usBMICReadReg(CVIL_AD_ADDR);				/* Low Speed Current AD	*/
	spi_result = bBMICCheckSpiResult();
	if(spi_result == true){
		LONG lCurrent_10mA;
		BMIC_Drv.usSPI_ComAtLeastOnceFlg |= COMM_AD_ILOW;
		ftmp = fBMIC_CurClbGain;
/*		ftmp *= 6.103515625;		*/
/*		ftmp *= SHUNT_mohm;     */
/*	 	ftmp *= 54.931640625;	*/									/* 360 * 10000 / 65536 / 1 = 54.931640625 */
/*	 	ftmp *= 54.931640625 / (float)SHUNT_mohm;*/					/* 360 * 10000 / 65536 / SHUNT_mohm = 54.931640625/SHUNT_mohm */
		ftmp *= (float)54931.640625 / (float)SHUNT_uohm;					/* 360 * 10000 * 1000 / 65536 / SHUNT_uohm = 54931.640625 / SHUNT_uohm */
		ftmp *= (float)sSlowIAD;
		BMIC_Info.lBatPackCur_100uA = (LONG)ftmp;				/* slow AD */

	/*	LONG lCurrent_10mA = BMIC_Info.lBatPackCur_100uA / 100;	*/			/* slow AD */
		lCurrent_10mA = BMIC_Info.lBatPackFastCur_100uA / 100;			/* fast AD	*/
		BMIC_Info.usBatPackCur_10mA_300A = (USHORT)(lCurrent_10mA + 30000);	/* Pack current[10mA] =Pack current[10mA] + offset(300A * 100[10mA])	*/

		/*�ώZ���d��, �ώZ�[�d��	cumulative discharge amount, cumulative charge amount	*/
		if(sSlowIAD >= 0){			/* (+):Charge, (-):Discharge	*/
			if ((USHORT)sSlowIAD >= (USHORT)CUR_SUM_MIN_LEVEL_AD) {
				BMIC_Info.ullChgSum_AD += (ULLONG)sSlowIAD;				/* [10mA*100msec] = [10mA*100msec]+[10mA*100msec]*/
			}
		} else {
			usdata = (USHORT)(0-sSlowIAD);
			if (usdata >= (USHORT)CUR_SUM_MIN_LEVEL_AD) {
				BMIC_Info.ullDisChgSum_AD += (ULLONG)usdata;						/* [10mA*100msec] = [10mA*100msec]+[10mA*100msec]	*/
			}
		}
	}

}


/****************************************************************************
FUNCTION	: vBMIC_Register_ReadCtrl
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_Register_ReadCtrl(void)
{
	/*	vBMICReadReg_Multi(STAT1_ADDR, BMIC_RECEIVE_STATUS_REGISTER_SIZE);	*/
	vBMICReadReg_Multi(PWR_CTRL_ADDR, 89);				/* Read register 0x01 ~ 0x59 (89) */

	if(BMIC_Drv.usSPI_ComAtLeastOnceFlg == COMM_RECEIVE_ALL) {
		BMIC_Info.BMICMeasured = true;
	}
}

/****************************************************************************
FUNCTION	: ucBMIC_ChkOverTemparature
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
bool bBMIC_ChkOverTemparature(SHORT temp, USHORT thresh, USHORT recover, UCHAR *status)
{
	bool ret = false;
	SHORT threshC = (SHORT)(thresh*10u - 400);
	SHORT recoverC = (SHORT)(recover*10u - 400);

	if(temp > threshC){
		*status = 1u;
	} else if(temp < recoverC) {
		*status = 0u;
	} else {
		;
	}
	if(*status == 1u){
		ret = true;
	}
	return ret;
}

/****************************************************************************
FUNCTION	: ucBMIC_ChkLowTemparature
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
bool bBMIC_ChkLowTemparature(SHORT temp, USHORT thresh, USHORT recover, UCHAR *status)
{
	bool ret = false;
	SHORT threshC = (SHORT)(thresh*10u - 400);
	SHORT recoverC = (SHORT)(recover*10u - 400);

	if(temp < threshC){
		*status = 1u;
	} else if(temp > recoverC) {
		*status = 0u;
	} else {
		;
	}
	if(*status == 1u){
		ret = true;
	}
	return ret;
}

/****************************************************************************
FUNCTION	: usBMIC_Chk_Temparature
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/08
*****************************************************************************/
USHORT usBMIC_Chk_Temparature(void)
{
	USHORT ret = 0u;
	SHORT max_cell_temp = -400;
	SHORT min_cell_temp = 1000;
#ifdef BATTERY_THERMISTOR
	UCHAR ovtemp, lowtemp;
#endif
	USHORT mask = 0u;
	USHORT fix_ret = 0u;

	for(UCHAR i=0; i<THERMISTOR_NUM; i++){
		if ((BMICCellConf.ulCellTempPos & ((ULONG)1 << i)) != 0u) {
			if(max_cell_temp < BMIC_Info.sTemp_01Cdeg[i]){
				max_cell_temp = BMIC_Info.sTemp_01Cdeg[i];
			}
			if(min_cell_temp > BMIC_Info.sTemp_01Cdeg[i]){
				min_cell_temp = BMIC_Info.sTemp_01Cdeg[i];
			}
		}
	}
#ifdef BATTERY_THERMISTOR
	ovtemp = bBMIC_ChkOverTemparature(max_cell_temp, BMICSetParam.usOTBatChrgInfo_temp, BMICSetParam.usOTBatChrgInfo_temp_recover, &ucBatOvTempChrgStat);
	lowtemp = bBMIC_ChkLowTemparature(min_cell_temp, BMICSetParam.usLTBatChrgInfo_temp, BMICSetParam.usLTBatChrgInfo_temp_recover, &ucBatLowTempChrgStat);
	if(ovtemp == true){
		ret |= ERR_OVER_TEMP_BATTERY_CHARGE;
	}
	if(lowtemp == true){
		ret |= ERR_LOW_TEMP_BATTERY_CHARGE;
	}

	ovtemp = bBMIC_ChkOverTemparature(max_cell_temp, BMICSetParam.usOTBatInfo_temp, BMICSetParam.usOTBatInfo_temp_recover, &ucBatOvTempDischrgStat);
	lowtemp = bBMIC_ChkLowTemparature(min_cell_temp, BMICSetParam.usLTBatInfo_temp, BMICSetParam.usLTBatInfo_temp_recover, &ucBatLowTempDischrgStat);
	if(ovtemp == true){
		ret |= ERR_OVER_TEMP_BATTERY_DISCHARGE;
	}
	if(lowtemp == true){
		ret |= ERR_LOW_TEMP_BATTERY_DISCHARGE;
	}
#endif

	mask = usChkTempBuff ^ ret;
	fix_ret = ((USHORT)(~mask) & ret) | (mask & usChkTempFixBuff);
	usChkTempBuff = ret;
	usChkTempFixBuff = fix_ret;

	return fix_ret;
}

/****************************************************************************
FUNCTION	: vBMIC_CBL_Control
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		: 2020/12/01
DATE		: 2019/11/21
*****************************************************************************/
void vBMIC_CBL_Control(void){
	ULONG req_state;
	UCHAR next_cell_balancer = BMIC_Drv.ucBMIC_Cell_Balancer;
	UCHAR cbl_off_req = 0u;
	UCHAR cbl_counter = BMIC_Drv.ucCellBalancingCounter;

	if(BMIC_Info.ucMaintenanceMode == 1u){
		req_state = BMIC_Drv.ulBMIC_Cell_Balancetgt_Mnt;
	} else{
		req_state = BMIC_Drv.ulBMIC_Cell_Balancetgt;
	}

	switch(BMIC_Drv.ucBMIC_Cell_Balancer){
	case NON_CELL_BALANCE:
		if(req_state != 0u){
			BMIC_Drv.ulBMIC_Cell_Balancetgt_1sec = req_state;
			next_cell_balancer = ODD_CELL_BALANCE;
			cbl_counter = 4u;
		}
		break;
	case ODD_CELL_BALANCE:
		if(cbl_counter == 0u){
			next_cell_balancer = EVEN_CELL_BALANCE;
			cbl_off_req = 1u;
			cbl_counter = 4u;
		}
		break;
	case EVEN_CELL_BALANCE:
		if(cbl_counter == 0u){
			next_cell_balancer = CELL_BALANCE_OFF;
			cbl_counter = 1u;
		}
		break;
	case CELL_BALANCE_OFF:
		if(cbl_counter == 0u){
			next_cell_balancer = NON_CELL_BALANCE;
		}
		break;
	}
	if (BMIC_Info.ulBatPackErrLog != 0u){
		next_cell_balancer = NON_CELL_BALANCE;
		cbl_off_req = 0u;
		cbl_counter = 0u;
	}

	if(cbl_counter > 0u){
		cbl_counter--;
	} else {
		cbl_counter = 0u;
	}

	if(BMIC_Drv.ucBMIC_Cell_Balancer != next_cell_balancer){
		USHORT send_data1;
		USHORT send_data2;
		ULONG cell_balance_target = BMIC_Drv.ulBMIC_Cell_Balancetgt_1sec;
		volatile UCHAR spi_result;
		UCHAR stat = OK;
/*		USHORT usregdata = 0u;*/
		if(next_cell_balancer == ODD_CELL_BALANCE){
			send_data1 = (USHORT)((cell_balance_target & BMIC_Drv.ulBMIC_CBL_Odd_Mask) & 0x0000FFFF);
			send_data2 = (USHORT)(((cell_balance_target & BMIC_Drv.ulBMIC_CBL_Odd_Mask) >> 16) & 0x0000FFFF);
		} else if(next_cell_balancer == EVEN_CELL_BALANCE){
			send_data1 = (USHORT)((cell_balance_target & (~BMIC_Drv.ulBMIC_CBL_Odd_Mask)) & 0x0000FFFF);
			send_data2 = (USHORT)(((cell_balance_target & (~BMIC_Drv.ulBMIC_CBL_Odd_Mask)) >> 16) & 0x0000FFFF);
		} else {
			send_data1 = 0u;
			send_data2 = 0u;
		}

		if((next_cell_balancer == ODD_CELL_BALANCE) || (next_cell_balancer == EVEN_CELL_BALANCE)){
			if(cbl_off_req == (UCHAR)1){
				spi_result = ucBMIC_Send_Req(OP_MODE_ADDR, CB_SET_OFF, CB_SET_ON, BMIC_DATA_INPUT_NONE);		/* CB off */
				stat &= spi_result;
				spi_result = ucBMIC_Send_Req(CBSEL_ADDR, send_data1, 0xFFFFu, BMIC_DATA_INPUT_NEED);			/* Select cell for CB1~16 */
				stat &= spi_result;
				spi_result = ucBMIC_Send_Req(CBSEL_22_ADDR, send_data2, 0xFFFFu, BMIC_DATA_INPUT_NEED);			/* Select cell for CB17~22 */
				stat &= spi_result;
				Wait_msec(2u);
				if(stat == OK){
					ucBMIC_Send_Req(OP_MODE_ADDR, CB_SET_ON, CB_SET_ON, BMIC_DATA_INPUT_NEED);					/* CB on, OV/UV detection off */
				} else {
					ucBMIC_Send_Req(CBSEL_ADDR, 0x0000, 0xFFFFu, BMIC_DATA_INPUT_NEED);					/* Deselect cell for CB1~16 */
					ucBMIC_Send_Req(CBSEL_22_ADDR, 0x0000, 0xFFFFu, BMIC_DATA_INPUT_NEED);				/* Deselect cell for CB17~22 */
					ucBMIC_Send_Req(OP_MODE_ADDR, CB_SET_OFF|OVMSK_ON|UVMSK_ON, CB_SET_ON|OVMSK_OFF|UVMSK_OFF, BMIC_DATA_INPUT_NEED);			/* CB off, OV/UV detection on */
					ucBMIC_Send_Req(OTHCTL_ADDR, NPD_CB_POWERDOWN, NPD_CB_MASK, BMIC_DATA_INPUT_NEED);				/* OTHCTL: NPD_CB OFF */
					next_cell_balancer = NON_CELL_BALANCE;
					cbl_counter = 0u;
				}
			} else {
/*				usregdata = usBMICReferRegData(OTHCTL_ADDR);*/
/*				if((usregdata & PD_REG55_MASK) == PD_REG55_ON){	*/
/*					spi_result = ucBMIC_Send_Req(OTHCTL_ADDR, PD_REG55_OFF|NPD_CB_NORMAL, PD_REG55_MASK|NPD_CB_MASK, BMIC_DATA_INPUT_NEED);	*/
/*					stat &= spi_result;	*/
/*				}	*/
				spi_result = ucBMIC_Send_Req(OTHCTL_ADDR, NPD_CB_NORMAL, NPD_CB_MASK, BMIC_DATA_INPUT_NEED);		/* OTHCTL: NPD_CB ON */
				stat &= spi_result;
				spi_result = ucBMIC_Send_Req(CBSEL_ADDR, send_data1, 0xFFFFu, BMIC_DATA_INPUT_NEED);				/* Select cell for CB1~16 */
				stat &= spi_result;
				spi_result = ucBMIC_Send_Req(CBSEL_22_ADDR, send_data2, 0xFFFFu, BMIC_DATA_INPUT_NEED);				/* Select cell for CB17~22 */
				stat &= spi_result;
				if(stat == OK){
					ucBMIC_Send_Req(OP_MODE_ADDR, CB_SET_ON|OVMSK_OFF|UVMSK_OFF, CB_SET_ON|OVMSK_OFF|UVMSK_OFF, BMIC_DATA_INPUT_NEED);			/* CB on, OV/UV detection off  */
				} else {
					ucBMIC_Send_Req(CBSEL_ADDR, 0x0000, 0xFFFFu, BMIC_DATA_INPUT_NEED);					/* Deselect cell for CB1~16 */
					ucBMIC_Send_Req(CBSEL_22_ADDR, 0x0000, 0xFFFFu, BMIC_DATA_INPUT_NEED);				/* Deselect cell for CB17~22 */
					ucBMIC_Send_Req(OP_MODE_ADDR, CB_SET_OFF|OVMSK_ON|UVMSK_ON, CB_SET_ON|OVMSK_OFF|UVMSK_OFF, BMIC_DATA_INPUT_NEED);			/* CB off, OV/UV detection on */
					ucBMIC_Send_Req(OTHCTL_ADDR, NPD_CB_POWERDOWN, NPD_CB_MASK, BMIC_DATA_INPUT_NEED);					/* OTHCTL: NPD_CB OFF */
					next_cell_balancer = NON_CELL_BALANCE;
					cbl_counter = 0u;
				}
			}
		}else{
			ucBMIC_Send_Req(CBSEL_ADDR, send_data1, 0xFFFFu, BMIC_DATA_INPUT_NEED);					/* Deselect cell for CB1~16 */
			ucBMIC_Send_Req(CBSEL_22_ADDR, send_data2, 0xFFFFu, BMIC_DATA_INPUT_NEED);				/* Deselect cell for CB17~22 */
			ucBMIC_Send_Req(OP_MODE_ADDR, CB_SET_OFF|OVMSK_ON|UVMSK_ON, CB_SET_ON|OVMSK_OFF|UVMSK_OFF, BMIC_DATA_INPUT_NEED);		/* CB off, OV/UV detection on */
			ucBMIC_Send_Req(OTHCTL_ADDR, NPD_CB_POWERDOWN, NPD_CB_MASK, BMIC_DATA_INPUT_NEED);											/* OTHCTL: NPD_CB OFF */
		}
	}
	BMIC_Drv.ucCellBalancingCounter = cbl_counter;
	BMIC_Drv.ucBMIC_Cell_Balancer = next_cell_balancer;
}

/****************************************************************************
FUNCTION	: vBMIC_CBL_Init
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2020/2/19
*****************************************************************************/
void vBMIC_CBL_Init(void)
{
	BMIC_Drv.ucBMIC_Cell_Balancer = NON_CELL_BALANCE;
	BMIC_Drv.ucCellBalancingCounter = 0u;
	BMIC_Drv.ulBMIC_Cell_Balancetgt = 0u;
	BMIC_Drv.ulBMIC_Cell_Balancetgt_Mnt = 0u;
	BMIC_Drv.ulBMIC_Cell_Balancetgt_1sec = 0u;

	if((BMICCellConf.ucSeriesCount % (UCHAR)2) == (UCHAR)0) {
		BMIC_Drv.ulBMIC_CBL_Odd_Mask = CELL_ODD_MASK_EVENCELL;
	} else {
		BMIC_Drv.ulBMIC_CBL_Odd_Mask = CELL_ODD_MASK_ODDCELL;
	}
}

/****************************************************************************
FUNCTION	: vBMIC_SetCellbalanceReq
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_SetCellbalanceReq(ULONG target)
{
	ULONG target_tmp = 0u;
	if (BMIC_Info.ulBatPackErrLog == 0u) {
		UCHAR all_bit_set = 1u;
		ULONG chk_bit = 0u;
		UCHAR shift = 0u;
		UCHAR i;

		for (i = 0u; i < BMICCellConf.ucSeriesCount; i++) {
			chk_bit = ((ULONG)0x00000001 << i);
			if ((target & chk_bit) != 0u) {
				shift = BMIC_Drv.ucCellList[i];
				target_tmp |= (ULONG)((ULONG)0x00000001u << shift);
			} else {
				all_bit_set = 0u;
			}
		}
		if(all_bit_set == 1u){
			target_tmp = 0u;
		} else {;}
	}
	BMIC_Drv.ulBMIC_Cell_Balancetgt = target_tmp;
}

/****************************************************************************
FUNCTION	: vBMIC_SetCellbalanceReq_Mnt
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_SetCellbalanceReq_Mnt(ULONG target){

	if(BMIC_Info.ulBatPackErrLog != 0u){
		target = 0u;
	}
	BMIC_Drv.ulBMIC_Cell_Balancetgt_Mnt = target;
}

/****************************************************************************
FUNCTION	: ulBMIC_GetCellbalanceReqPack
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		: 2020/12/01
DATE		: 2019/11/01
*****************************************************************************/
ULONG ulBMIC_GetCellbalanceReqPack(void){
	ULONG ret = 0u;
	ULONG cellbal;
	UCHAR i = 0u;
	ULONG chk_bit = 0u;
	CHAR skip_shift = 0u;

	if(BMIC_Info.ucMaintenanceMode == 1u){
		cellbal = BMIC_Drv.ulBMIC_Cell_Balancetgt_Mnt;
	} else{
		cellbal = BMIC_Drv.ulBMIC_Cell_Balancetgt;
	}

	for(i=0; i<BMICCellConf.ucSeriesCount; i++) {
		chk_bit = (ULONG)((ULONG)0x1u << BMIC_Drv.ucCellList[i]);
		skip_shift = (CHAR)(BMIC_Drv.ucCellList[i] - i);
		if(skip_shift < 0) {
			skip_shift = 0u;
		}
		if((chk_bit & cellbal) != 0u) {
			ret |= (chk_bit >> (UCHAR)skip_shift);
		}
	}
	return ret;
}

/****************************************************************************
FUNCTION	: ulBMIC_GetCellbalanceReq
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
ULONG ulBMIC_GetCellbalanceReq(void){
	ULONG ret = 0u;

	if(BMIC_Info.ucMaintenanceMode == 1u){
		ret = BMIC_Drv.ulBMIC_Cell_Balancetgt_Mnt;
	} else{
		ret = BMIC_Drv.ulBMIC_Cell_Balancetgt;
	}
	return ret;
}

/****************************************************************************
FUNCTION	: bBMIC_ChkCellbalance
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
bool bBMIC_ChkCellbalance(void){
	bool ret = false;

	if(BMIC_Drv.ucBMIC_Cell_Balancer != NON_CELL_BALANCE){
		ret = true;
	}
	return ret;
}

/****************************************************************************
FUNCTION	: vBMIC_SetMntMode
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_SetMntMode(UCHAR mode){
	BMIC_Info.ucMaintenanceMode = mode;
	BMIC_Drv.ulBMIC_Cell_Balancetgt = 0u;
	BMIC_Drv.ulBMIC_Cell_Balancetgt_Mnt = 0u;
}

/****************************************************************************
FUNCTION	: bBMIC_GetMntMode
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
bool bBMIC_GetMntMode(void)
{
	bool ret = false;
	if(	BMIC_Info.ucMaintenanceMode == 1u){
		ret = true;
	}
	return ret;
}

/****************************************************************************
FUNCTION	: vBMIC_Thermistor_readReq
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_Thermistor_readReq(UCHAR req)
{
	BMIC_Drv.ucTmoni_read_req = req;
}

/****************************************************************************
FUNCTION	: vBMIC_Discharge_FET_Out
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_Discharge_FET_Out(UCHAR output)
{
	USHORT snd_data = FDRV_DISCHARGE_FET_OFF;
	UCHAR fet_alarm_clr_req = 0u;
	USHORT data = usBMICReferRegData(STAT1_ADDR);

	if(output == 1u){
		snd_data = FDRV_DISCHARGE_FET_ON;
		if((data & FDRV_DIS_ST_MASK) == 0u){
			fet_alarm_clr_req = 1u;
		}
	}
	ucBMIC_Send_Req(FDRV_CTRL_ADDR, snd_data, FDRV_DISCHARGE_FET_MASK, BMIC_DATA_INPUT_NEED);
	if(fet_alarm_clr_req == 1u){
		ucBMIC_Send_Req(FDRV_CTRL_ADDR, FDRV_ALM_CLR, FDRV_ALM_CLR, BMIC_DATA_INPUT_NONE);
		ucBMIC_Send_Req(FDRV_CTRL_ADDR, (USHORT)~FDRV_ALM_CLR, FDRV_ALM_CLR, BMIC_DATA_INPUT_NEED);
	}

}


/****************************************************************************
FUNCTION	: vBMIC_Charge_FET_Out
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		: 2020/10/20
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_Charge_FET_Out(UCHAR output)
{
	USHORT snd_data = FDRV_CHARGE_FET_OFF;
	UCHAR fet_alarm_clr_req = 0u;
	USHORT data = usBMICReferRegData(STAT1_ADDR);

	if(output == 1u){
		snd_data = FDRV_CHARGE_FET_ON;
		if((data & FDRV_CHG_ST_MASK) == 0u){
			fet_alarm_clr_req = 1u;
		}
	}
	ucBMIC_Send_Req(FDRV_CTRL_ADDR, snd_data, FDRV_CHARGE_FET_MASK, BMIC_DATA_INPUT_NEED);
	if(fet_alarm_clr_req == 1u){
		ucBMIC_Send_Req(FDRV_CTRL_ADDR, FDRV_ALM_CLR, FDRV_ALM_CLR, BMIC_DATA_INPUT_NONE);
		ucBMIC_Send_Req(FDRV_CTRL_ADDR, (USHORT)~FDRV_ALM_CLR, FDRV_ALM_CLR, BMIC_DATA_INPUT_NEED);
	}

}


/****************************************************************************
FUNCTION	: vBMIC_PreDischarge_FET_Out
DESCRIPTION	:
INPUT		: UCHAR output (1: output ON, 0: output OFF (Hi-Z, default)
INPUT		: UCHAR fet_setting (1: FET control setting of GPOHx - FET control use, 0: FET control not in use (default)
OUTPUT		:
UPDATE		:
DATE		: 2020/10/20
*****************************************************************************/
void vBMIC_PreDischarge_FET_Out(UCHAR output, UCHAR fet_setting)
{
	USHORT snd_data = GPOH2_HIZ;
	UCHAR fet_alarm_clr_req = 0u;
	USHORT data = usBMICReferRegData(STAT1_ADDR);

	if(output == 1u){
		snd_data = GPOH2_LOW;
		if((data & GPOH2_ST_MASK) == 0u){
			fet_alarm_clr_req = 1u;
		}
	}

	if(fet_setting == USE_GPOH_FET){
		snd_data |= GPOH_FET_FET;
	}

	ucBMIC_Send_Req(GPOH_CTL_ADDR, snd_data, GPOH_FET_MASK | GPOH2_MASK, BMIC_DATA_INPUT_NEED);

	if(fet_alarm_clr_req == 1u){
		ucBMIC_Send_Req(FDRV_CTRL_ADDR, FDRV_ALM_CLR, FDRV_ALM_CLR, BMIC_DATA_INPUT_NONE);
		ucBMIC_Send_Req(FDRV_CTRL_ADDR, (USHORT)~FDRV_ALM_CLR, FDRV_ALM_CLR, BMIC_DATA_INPUT_NEED);
	}
}

/****************************************************************************
FUNCTION	: vBMIC_PreCharge_FET_Out
DESCRIPTION	:
INPUT		: UCHAR output (1: output ON, 0: output OFF (Hi-Z, default)
INPUT		: UCHAR fet_setting (1: FET control setting of GPOHx - FET control use, 0: FET control not in use (default)
OUTPUT		:
UPDATE		:
DATE		: 2020/10/20
*****************************************************************************/
void vBMIC_PreCharge_FET_Out(UCHAR output, UCHAR fet_setting)
{
	USHORT snd_data = GPOH1_HIZ;
	UCHAR fet_alarm_clr_req = 0u;
	USHORT data = usBMICReferRegData(STAT1_ADDR);

	if(output == 1u){
		snd_data = GPOH1_LOW;
		if((data & GPOH1_ST_MASK) == 0u){
			fet_alarm_clr_req = 1u;
		}
	}
	if(fet_setting == USE_GPOH_FET){
		snd_data |= GPOH_FET_FET;
	}

	ucBMIC_Send_Req(GPOH_CTL_ADDR, snd_data, GPOH_FET_MASK | GPOH1_MASK, BMIC_DATA_INPUT_NEED);

	if(fet_alarm_clr_req == 1u){
		ucBMIC_Send_Req(FDRV_CTRL_ADDR, FDRV_ALM_CLR, FDRV_ALM_CLR, BMIC_DATA_INPUT_NONE);
		ucBMIC_Send_Req(FDRV_CTRL_ADDR, (USHORT)~FDRV_ALM_CLR, FDRV_ALM_CLR, BMIC_DATA_INPUT_NEED);
	}
}


/****************************************************************************
FUNCTION	: vBMIC_SHDN_Out
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_SHDN_Out(UCHAR output)
{
	UCHAR state = GPIO_PIN_RESET;
	if(output == HIGH){
		state = GPIO_PIN_SET;
	}
	HAL_GPIO_WritePin(BMIC_SHDN_GPIO_Port, BMIC_SHDN_Pin, state);
}

/****************************************************************************
FUNCTION	: vBMIC_FETOFF_Out
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_FETOFF_Out(UCHAR output)
{
	UCHAR state = GPIO_PIN_RESET;
	if(output == HIGH){						/* High : Force swith off	*/
		state = GPIO_PIN_SET;
	}
	HAL_GPIO_WritePin(BMIC_FETOFF_GPIO_Port, BMIC_FETOFF_Pin, state);
}


/****************************************************************************
FUNCTION	: ucGetBMIC_ALARM1_In
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
UCHAR ucGetBMIC_ALARM1_In(void)
{

	UCHAR in = LOW;
	if(HAL_GPIO_ReadPin(BMIC_ALARM1_GPIO_Port, BMIC_ALARM1_Pin) == GPIO_PIN_SET){
		in = HIGH;
	}
	return in;
}


/****************************************************************************
FUNCTION	: ucGetBMIC_SDO_In
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
UCHAR ucGetBMIC_SDO_In(void)
{
	UCHAR in = LOW;
	if(HAL_GPIO_ReadPin(BMIC_SPI_SDO_GPIO_Port, BMIC_SPI_SDO_Pin) == GPIO_PIN_SET){
			in = HIGH;
	}
	return in;
}


/****************************************************************************
FUNCTION	: ucGetBMIC_ADIRQ2_In
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2020/10/14
*****************************************************************************/
UCHAR ucGetBMIC_ADIRQ2_In(void)
{
	UCHAR in = LOW;
	if(HAL_GPIO_ReadPin(BMIC_GPIO3_GPIO_Port, BMIC_GPIO3_Pin) == GPIO_PIN_SET){
		in = HIGH;
	}
	return in;
}

/****************************************************************************
FUNCTION	: ucGetBMICcrc
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
UCHAR ucGetBMICcrc(UCHAR *buf, UCHAR size)
{
	static const UCHAR ucCRC8_tCalc[] = {
		0x00u,0xd5u,0x7fu,0xaau,0xfeu,0x2bu,0x81u,0x54u,0x29u,0xfcu,0x56u,0x83u,0xd7u,0x02u,0xa8u,0x7du,
		0x52u,0x87u,0x2du,0xf8u,0xacu,0x79u,0xd3u,0x06u,0x7bu,0xaeu,0x04u,0xd1u,0x85u,0x50u,0xfau,0x2fu,
		0xa4u,0x71u,0xdbu,0x0eu,0x5au,0x8fu,0x25u,0xf0u,0x8du,0x58u,0xf2u,0x27u,0x73u,0xa6u,0x0cu,0xd9u,
		0xf6u,0x23u,0x89u,0x5cu,0x08u,0xddu,0x77u,0xa2u,0xdfu,0x0au,0xa0u,0x75u,0x21u,0xf4u,0x5eu,0x8bu,
		0x9du,0x48u,0xe2u,0x37u,0x63u,0xb6u,0x1cu,0xc9u,0xb4u,0x61u,0xcbu,0x1eu,0x4au,0x9fu,0x35u,0xe0u,
		0xcfu,0x1au,0xb0u,0x65u,0x31u,0xe4u,0x4eu,0x9bu,0xe6u,0x33u,0x99u,0x4cu,0x18u,0xcdu,0x67u,0xb2u,
		0x39u,0xecu,0x46u,0x93u,0xc7u,0x12u,0xb8u,0x6du,0x10u,0xc5u,0x6fu,0xbau,0xeeu,0x3bu,0x91u,0x44u,
		0x6bu,0xbeu,0x14u,0xc1u,0x95u,0x40u,0xeau,0x3fu,0x42u,0x97u,0x3du,0xe8u,0xbcu,0x69u,0xc3u,0x16u,
		0xefu,0x3au,0x90u,0x45u,0x11u,0xc4u,0x6eu,0xbbu,0xc6u,0x13u,0xb9u,0x6cu,0x38u,0xedu,0x47u,0x92u,
		0xbdu,0x68u,0xc2u,0x17u,0x43u,0x96u,0x3cu,0xe9u,0x94u,0x41u,0xebu,0x3eu,0x6au,0xbfu,0x15u,0xc0u,
		0x4bu,0x9eu,0x34u,0xe1u,0xb5u,0x60u,0xcau,0x1fu,0x62u,0xb7u,0x1du,0xc8u,0x9cu,0x49u,0xe3u,0x36u,
		0x19u,0xccu,0x66u,0xb3u,0xe7u,0x32u,0x98u,0x4du,0x30u,0xe5u,0x4fu,0x9au,0xceu,0x1bu,0xb1u,0x64u,
		0x72u,0xa7u,0x0du,0xd8u,0x8cu,0x59u,0xf3u,0x26u,0x5bu,0x8eu,0x24u,0xf1u,0xa5u,0x70u,0xdau,0x0fu,
		0x20u,0xf5u,0x5fu,0x8au,0xdeu,0x0bu,0xa1u,0x74u,0x09u,0xdcu,0x76u,0xa3u,0xf7u,0x22u,0x88u,0x5du,
		0xd6u,0x03u,0xa9u,0x7cu,0x28u,0xfdu,0x57u,0x82u,0xffu,0x2au,0x80u,0x55u,0x01u,0xd4u,0x7eu,0xabu,
		0x84u,0x51u,0xfbu,0x2eu,0x7au,0xafu,0x05u,0xd0u,0xadu,0x78u,0xd2u,0x07u,0x53u,0x86u,0x2cu,0xf9u
	};

	UCHAR	uccrc,uci;

/*	uccrc = 0x00u;	*/
	uccrc = 0xD5u;							/* CRC initial: 0xD5*/

	for( uci = 0u; uci < size; uci++) {
		uccrc = ucCRC8_tCalc[uccrc ^ buf[uci]];
	}

	return uccrc;

}

/****************************************************************************
FUNCTION	: vBMICWriteReg
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMICWriteReg(UCHAR addr, USHORT data)
{
	if(HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_READY){
	//if(bBMIC_SpiTake(BMIC_SPIWAIT_TIMEOUT)){
		BMIC_Drv.ucTxBuf[0u] = 0xe0u;
		BMIC_Drv.ucTxBuf[1u] = (UCHAR)(addr << 1u);
		BMIC_Drv.ucTxBuf[2u] = (UCHAR)(data >> 8u);
		BMIC_Drv.ucTxBuf[3u] = (UCHAR)(data & 0xFFu);
		BMIC_Drv.ucTxBuf[4u] = ucGetBMICcrc(BMIC_Drv.ucTxBuf, 4u);

		HAL_GPIO_WritePin(BMIC_SPI_SEN_GPIO_Port, BMIC_SPI_SEN_Pin, GPIO_PIN_SET);

		bSPI_BMIC_Tx(BMIC_Drv.ucTxBuf, 5);

		HAL_GPIO_WritePin(BMIC_SPI_SEN_GPIO_Port, BMIC_SPI_SEN_Pin, GPIO_PIN_RESET);

		vBMIC_Wait(BMIC_WAIT_1US);	/* 1uS Wait for next request */

		BMIC_Drv.ucBMIC_SPI_status = ucBMIC_Check_SPI_status();
		//BMIC_SpiRelease();
	}
}

/****************************************************************************
FUNCTION	: vBMICWriteReg_Mask
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMICWriteReg_Mask(UCHAR addr, USHORT usdata, USHORT usMask, UCHAR ucdata)
{
	USHORT us_tmp;
	bool spi_result;

	us_tmp = usBMICReferRegData(addr);
	us_tmp = us_tmp & (usMask ^ 0xFFFFu);
	us_tmp = us_tmp | (usdata & usMask);

	vBMICWriteReg(addr, us_tmp);
	spi_result = bBMICCheckSpiResult();

	if ((ucdata == BMIC_DATA_INPUT_NEED) && (spi_result == true)){
		vBMICInputRecvData(addr, us_tmp);
	} else {
		;
	}
}

/****************************************************************************
FUNCTION	: usBMICReadReg
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
USHORT usBMICReadReg(UCHAR addr)
{
	USHORT	ret = usBMICReferRegData(addr);
	UCHAR	crc;
	UCHAR crc_err = 0u;
	UCHAR spi_miso = 0u;

	if(bBMIC_SpiTake(BMIC_SPIWAIT_TIMEOUT)){
		BMIC_Drv.ucTxBuf[0u] = 0xe1u;
		BMIC_Drv.ucTxBuf[1u] = (UCHAR)(addr << 1u);
		BMIC_Drv.ucTxBuf[2u] = ucGetBMICcrc(BMIC_Drv.ucTxBuf, 2u);
		BMIC_Drv.ucTxBuf[3u] = 0xFFu;
		BMIC_Drv.ucTxBuf[4u] = 0xFFu;
		BMIC_Drv.ucTxBuf[5u] = 0xFFu;

		HAL_GPIO_WritePin(BMIC_SPI_SEN_GPIO_Port, BMIC_SPI_SEN_Pin, GPIO_PIN_SET);

		bSPI_BMIC_Rx(BMIC_Drv.ucTxBuf, BMIC_Drv.ucRxBuf, 6u);	/* 5byte=3byte(send)+3byte(receive)	*/

		HAL_GPIO_WritePin(BMIC_SPI_SEN_GPIO_Port, BMIC_SPI_SEN_Pin, GPIO_PIN_RESET);

		vBMIC_Wait(BMIC_WAIT_1US);	/* 1uS Wait for next request */
		crc = ucGetBMICcrc(&BMIC_Drv.ucRxBuf[3u], 2u);
		spi_miso = ucGetBMIC_SDO_In();
		if ((crc == BMIC_Drv.ucRxBuf[5u]) && (spi_miso == HIGH)) {
			ret = (USHORT)BMIC_Drv.ucRxBuf[3u] << 8u;
			ret += (USHORT)BMIC_Drv.ucRxBuf[4u];
			vBMICInputRecvData(addr, ret);
		} else {
			crc_err = 1u;
		}
		BMIC_Drv.ucBMIC_SPI_status = ucBMIC_Check_SPI_status_crc(crc_err);
		BMIC_SpiRelease();
	}
	return ret;
}

/****************************************************************************
FUNCTION	: vBMICReadReg_Multi
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMICReadReg_Multi(UCHAR addr, UCHAR num)
{
	UCHAR byte_num = (UCHAR)(num << 1u);
	UCHAR crc_error = 0u;
	UCHAR crc = 0u;
	UCHAR spi_miso = 0u;
	UCHAR i = 0u;

	if(bBMIC_SpiTake(BMIC_SPIWAIT_TIMEOUT)){
		BMIC_Drv.ucTxBuf[0u] = 0xe1u;
		BMIC_Drv.ucTxBuf[1u] = ((UCHAR)(addr << 1u) | 0x01u);
		BMIC_Drv.ucTxBuf[2u] = num-(UCHAR)1;
		BMIC_Drv.ucTxBuf[3u] = ucGetBMICcrc(&BMIC_Drv.ucTxBuf[0], 3u);
		for(i=4u; i< BMIC_PACKET_BUF_SIZE; i++){
			BMIC_Drv.ucTxBuf[i] = 0xFFu;
		}

		HAL_GPIO_WritePin(BMIC_SPI_SEN_GPIO_Port, BMIC_SPI_SEN_Pin, GPIO_PIN_SET);

		bSPI_BMIC_Rx(BMIC_Drv.ucTxBuf, BMIC_Drv.ucRxBuf, byte_num+4u+1u);	/* 4byte(send)+byte_num(receive)+1byte CRC(receive)	*/

		HAL_GPIO_WritePin(BMIC_SPI_SEN_GPIO_Port, BMIC_SPI_SEN_Pin, GPIO_PIN_RESET);

		vBMIC_Wait(BMIC_WAIT_1US);	/* 1uS Wait for next request */
		crc = ucGetBMICcrc(&BMIC_Drv.ucRxBuf[4], byte_num);
		spi_miso = ucGetBMIC_SDO_In();
		if ((crc == BMIC_Drv.ucRxBuf[byte_num+4u]) && (spi_miso == HIGH)) {
			BMIC_Drv.usSPI_ComAtLeastOnceFlg |= COMM_MULTI_READ;
			for(i = 0; i < num; i++){
				UCHAR addr_index = i + addr;
				UCHAR buff_index = (UCHAR)(i << 1) + 4u;
				USHORT ret = (USHORT)BMIC_Drv.ucRxBuf[buff_index] << 8u;
				ret += (USHORT)BMIC_Drv.ucRxBuf[buff_index + 1u];
				vBMICInputRecvData(addr_index, ret);
			}
		} else {
			crc_error = 1u;
		}
		BMIC_Drv.ucBMIC_SPI_status = ucBMIC_Check_SPI_status_crc(crc_error);
		BMIC_SpiRelease();
	}
}

/****************************************************************************
FUNCTION	: usBMIC_ReadReg
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
USHORT usBMIC_ReadReg(UCHAR addr)
{
	return usBMICReadReg(addr);
}

/****************************************************************************
FUNCTION	: ucBMIC_Send_Req
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
UCHAR ucBMIC_Send_Req(UCHAR addr, USHORT data, USHORT mask, UCHAR mask_req)
{
	UCHAR ret = (UCHAR)false;
	volatile bool spi_result;
	UCHAR send_stat = OK;

	if((addr <= BMIC_REG_MAX_ADDR) && (addr != 0u)){
		vBMICWriteReg_Mask(addr, data, mask, mask_req);
		spi_result = bBMICCheckSpiResult();
		send_stat &= (UCHAR)spi_result;
	} else {
		send_stat &= 0u;
	}
	if(send_stat == OK){
		ret = (UCHAR)true;
	}
	return ret;
}


/****************************************************************************
FUNCTION	: usBMICReferRegData
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
USHORT usBMICReferRegData(UCHAR addr)
{
	return	BMIC_Drv.usRegData[addr];
}

/****************************************************************************
FUNCTION	: vBMICInputRecvData
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMICInputRecvData(UCHAR addr, USHORT usdata)
{
	BMIC_Drv.usRegData[addr] = usdata;
}

/****************************************************************************
FUNCTION	: ucBMIC_Check_SPI_status
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
UCHAR ucBMIC_Check_SPI_status(void)
{
	UCHAR spi_miso = ucGetBMIC_SDO_In();
	UCHAR ret = 0u;

	if(spi_miso == LOW){
		BMIC_Drv.ucBMIC_SPI_err_count++;
		BMIC_Drv.ucBMIC_SPI_err_detail |= 0x01u;				/* bp0: write error - SDO low */
	} else {
		BMIC_Drv.ucBMIC_SPI_err_count = 0;
		BMIC_Drv.ucBMIC_SPI_err_detail &= 0xFEu;				/* bp0: write error clear */
	}

	if(BMIC_Drv.ucBMIC_SPI_err_count >= 5u){
		BMIC_Drv.ucBMIC_SPI_err_count = 5u;
		ret = 1u;
	}
	return ret;
}

/****************************************************************************
FUNCTION	: ucBMIC_Check_SPI_status_crc
DESCRIPTION	:
INPUT		: UCHAR crc_chk
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
UCHAR ucBMIC_Check_SPI_status_crc(UCHAR crc_chk)
{
	UCHAR spi_miso = ucGetBMIC_SDO_In();
	UCHAR ret = 0u;
	if((spi_miso == LOW) || (crc_chk == 1u)){
		BMIC_Drv.ucBMIC_SPI_err_count++;

		BMIC_Drv.ucBMIC_SPI_err_detail |= 0x02u;				/* bp1: read error - SDO low or CRC */
	} else {
		BMIC_Drv.ucBMIC_SPI_err_count = 0;
		BMIC_Drv.ucBMIC_SPI_err_detail &= 0xFDu;			/* bp1: read erro clear */
	}

	if(BMIC_Drv.ucBMIC_SPI_err_count >= 5u){
		BMIC_Drv.ucBMIC_SPI_err_count = 5u;
		ret = 1u;
	}
	return ret;
}

/****************************************************************************
FUNCTION	: bBMICCheckSpiResult
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2020/01/15
*****************************************************************************/
bool bBMICCheckSpiResult(void)
{
	bool ret = true;
	if(BMIC_Drv.ucBMIC_SPI_err_count != 0u){
		ret = false;
	}
	return ret;
}


/****************************************************************************
FUNCTION	: ucGetBMIC_SpiErrDetail
DESCRIPTION	:
INPUT		:
OUTPUT		: UCHAR
UPDATE		:
DATE		: 2020/11/09
*****************************************************************************/
UCHAR ucGetBMIC_SpiErrDetail(void)
{
	return(BMIC_Drv.ucBMIC_SPI_err_detail);
}


/****************************************************************************
FUNCTION	: BMIC_Clear_Spi_Err_counter
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2020/01/15
*****************************************************************************/
void BMIC_Clear_Spi_Err_counter(void)
{
	BMIC_Drv.ucBMIC_SPI_err_count = 0u;
}


/****************************************************************************
FUNCTION	: vBMICRegInitRead
DESCRIPTION	: Read all registers
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMICRegInitRead(void)
{
	vBMICReadReg_Multi(PWR_CTRL_ADDR, 89);
}

/****************************************************************************
FUNCTION	: ucBMICRegInitChk
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
UCHAR	ucBMICRegInitChk(void)
{
	UCHAR ucjdg = OK;
	UCHAR ucret = ucBMICChk_CtrlReg();

	ucret = (ucret & ucBMICChk_StatusReg());

	if(ucret != OK){
		ucjdg = NG;
	}
	return ucjdg;
}

/****************************************************************************
FUNCTION	: ucBMICChk_StatusReg
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
UCHAR	ucBMICChk_StatusReg(void)
{
	UCHAR   ucjdg = OK;
	USHORT usregdata;
	UCHAR addr = 0u;

	/* Status Register initial check : 0x1C STAT1 b4~0 operation mode flags */
	usregdata = usBMICReferRegData(STAT1_ADDR);
	if ( (usregdata & MODE_ST_MASK) != MODE_ST_ACT ) {
		ucjdg = NG;
	}

	/* Status Register initial check : 0x21�`0x26 OTHSTAT, OVSTAT1&2, UVSTAT1&2, BIASSTAT */
	for (addr=OTHSTAT_ADDR; addr < (BIASSTAT_ADDR + 1u); addr++) {
		usregdata = usBMICReferRegData(addr);
		if (addr == OTHSTAT_ADDR){
			if ((usregdata & ~(ULONG)OTHSTAT_SCD_OCD_OCC_MASK) == 0u) {
				usregdata = 0u;
			} else {
				usregdata = 1u;
			}
		}
		#if(0)	/* not checked OV/UV flags when startup init */
		if ( usregdata != 0u ) {
			ucjdg = NG;
			break;
		}
		#endif
	}

	#if(0)		/* not checked OV/UV stat when startup init */
	/* Status Register initial check : 0x4D�`0x52 OVL_STAT1&2, UVL_STAT1&2, CBSTAT1&2 */
	for (addr=OVL_STAT1_ADDR; addr < (CBSTAT2_ADDR + 1u); addr++) {
		usregdata = usBMICReferRegData(addr);
		if ( usregdata != 0u ) {
			ucjdg = NG;
			break;
		}
	}
	#endif

	return ucjdg;

}

/****************************************************************************
FUNCTION	: ucBMICChk_CtrlReg
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
UCHAR	ucBMICChk_CtrlReg(void)
{
	UCHAR   ucjdg = OK;
	UCHAR addr = 0u;
	USHORT usregdata = 0u;
	USHORT usexpectdata = 0u;

	for (addr=PWR_CTRL_ADDR; addr < (INR_CTL_DIAG_EN_ADDR + 1u); addr++) {
		usregdata = usBMICReferRegData(stCtrlReg_Init_BMIC[addr-(UCHAR)1u].m_ucreg);
		usexpectdata = stCtrlReg_Init_BMIC[addr-(UCHAR)1u].m_usdata;
		if ( usregdata != usexpectdata ) {
			ucjdg = NG;
			break;
		}
	}

	return ucjdg;
}

/****************************************************************************
FUNCTION	: ucBMICReadFuse
DESCRIPTION	: Read BMIC Internal pull-up resistance register and get pull-up resistance value
INPUT		: None
OUTPUT		:
UPDATE		: 2020/10/13
DATE		: 2019/11/01
*****************************************************************************/
UCHAR	ucBMICReadFuse(void)
{
	SHORT sfuse[MAX_THERMISTOR_NUM];
	USHORT usdata;
	CHAR data;
	volatile bool spi_result;
	UCHAR read_fuse_stat = (UCHAR)true;
	UCHAR i = 0u;

	usdata = usBMICReadReg(TMONI1_ADDR);
	spi_result = bBMICCheckSpiResult();
	read_fuse_stat &= (UCHAR)spi_result;
	usdata = usdata & (USHORT)0x03FF;						/* b9~0: TMONI1 pin pull-up resistance value */
	if((usdata & (USHORT)0x0200u) != (USHORT)0){
		usdata = ((USHORT)0xFE00 | usdata);
	}
	sfuse[0] = (SHORT)usdata;

	usdata = usBMICReadReg(TMONI23_ADDR);
	spi_result = bBMICCheckSpiResult();
	read_fuse_stat &= (UCHAR)spi_result;
	data = (CHAR)(usdata & 0x00FFu);				/*b7~0: TMONI2 pin pull-up resistance difference value */
	sfuse[1] = data + sfuse[0];
	data = (CHAR)((usdata & 0xFF00u) >> 8u);		/*b15~8: TMONI3 pin pull-up resistance difference value */
	sfuse[2] = data + sfuse[0];

	usdata = usBMICReadReg(TMONI45_ADDR);
	spi_result = bBMICCheckSpiResult();
	read_fuse_stat &= (UCHAR)spi_result;
	data = (CHAR)(usdata & 0x00FFu);				/*b7~0: TMONI4 pin pull-up resistance difference value */
	sfuse[3] = data + sfuse[0];
	data = (CHAR)((usdata & 0xFF00u) >> 8u);	/*b15~8: TMONI5 pin pull-up resistance difference value */
	sfuse[4] = data + sfuse[0];

	if(read_fuse_stat == (UCHAR)true){
		for(i=0u; i < MAX_THERMISTOR_NUM; i++){
			BMIC_Drv.ulRfuse[i] = ulBMICCalcRfuse(sfuse[i]);
		}
	}
	return read_fuse_stat;
}

/****************************************************************************
FUNCTION	: ulBMICCalcRfuse
DESCRIPTION	: Calculate TMONIn internal pull-up resistance value
INPUT		: SHORT sfuse
OUTPUT		: ULONG rfuse
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
ULONG ulBMICCalcRfuse(SHORT sfuse)
{
	ULONG rfuse;
	if(sfuse > 511){
		sfuse = 511;
	} else if(sfuse < -512){
		sfuse = -512;
	} else {;}

	rfuse = (ULONG)((((6000 * sfuse) << 8) / 1024 + (10000 << 8)) >> 8);	/* usfuse(register) * 6K[ohm]/2^10 + 10K[ohm]	*/
	return rfuse;
}

/****************************************************************************
FUNCTION	: sCalcTemperature_01deg
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		: 2020/09/15
DATE		: 2019/11/01
*****************************************************************************/
SHORT sCalcTemperature_01deg(USHORT vtm_ad, UCHAR num)
{
	ULONG vdd_mV = ((ULONG)BMIC_Drv.usVddAveAD * 7500u) / 16384u;		/* VDD55 max 7.5V	*/
	ULONG vtm_mV = ((ULONG)vtm_ad * 5000u) / 16384u;									/* VTOMIn max 5V	*/

	SHORT ret;
	ULONG rth;

	if(vdd_mV == vtm_mV){
		ret = 0x7FFF;
	} else {
		rth = (vtm_mV * BMIC_Drv.ulRfuse[num]) / (vdd_mV - vtm_mV); 							/*Rth = (Vtm*Rfuse)/(Vdd-Vtm) */
		ret = iLookupTemp_01deg(rth);
	}

	return (ret);
}

/****************************************************************************
FUNCTION	: iLookupTemp_01deg
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
SHORT iLookupTemp_01deg(ULONG rth)
{
#define	TC_TABLE_NUM (141u)
	typedef struct {
		SHORT		temp;
		ULONG	resistance;
	}TTemp_Characteristic;

	static const TTemp_Characteristic TC[TC_TABLE_NUM]= {
#if 0
		/*QE103F3380FB3EC050X20*/
		{ -40,	197390 },	{ -39,	186540 },	{ -38,	176350 },	{ -37,	166800 },	{ -36,	157820 },
		{ -35,	149390 },	{ -34,	141510 },	{ -33,	134090 },	{ -32,	127110 },	{ -31,	120530 },
		{ -30,	114340 },	{ -29,	108530 },	{ -28,	103040 },	{ -27,	97870 },	{ -26,	92989 },
		{ -25,	88381 },	{ -24,	84036 },	{ -23,	79931 },	{ -22,	76052 },	{ -21,	72384 },
		{ -20,	68915 },	{ -19,	65634 },	{ -18,	62529 },	{ -17,	59589 },	{ -16,	56804 },
		{ -15,	54166 },	{ -14,	51665 },	{ -13,	49294 },	{ -12,	47046 },	{ -11,	44913 },
		{ -10,	42889 },	{ -9,	40967 },	{ -8,	39142 },	{ -7,	37408 },	{ -6,	35761 },
		{ -5,	34196 },	{ -4,	32707 },	{ -3,	31291 },	{ -2,	29945 },	{ -1,	28664 },
		{ 0,	27445 },	{ 1,	26283 },	{ 2,	25177 },	{ 3,	24124 },	{ 4,	23121 },
		{ 5,	22165 },	{ 6,	21253 },	{ 7,	20384 },	{ 8,	19555 },	{ 9,	18764 },
		{ 10,	18010 },	{ 11,	17290 },	{ 12,	16602 },	{ 13,	15946 },	{ 14,	15319 },
		{ 15,	14720 },	{ 16,	14148 },	{ 17,	13601 },	{ 18,	13078 },	{ 19,	12578 },
		{ 20,	12099 },	{ 21,	11642 },	{ 22,	11204 },	{ 23,	10785 },	{ 24,	10384 },
		{ 25,	10000 },	{ 26,	9632 },		{ 27,	9280 },		{ 28,	8943 },		{ 29,	8619 },
		{ 30,	8309 },		{ 31,	8012 },		{ 32,	7727 },		{ 33,	7453 },		{ 34,	7191 },
		{ 35,	6939 },		{ 36,	6698 },		{ 37,	6466 },		{ 38,	6243 },		{ 39,	6029 },
		{ 40,	5824 },		{ 41,	5627 },		{ 42,	5437 },		{ 43,	5255 },		{ 44,	5080 },
		{ 45,	4911 },		{ 46,	4749 },		{ 47,	4593 },		{ 48,	4443 },		{ 49,	4299 },
		{ 50,	4160 },		{ 51,	4027 },		{ 52,	3898 },		{ 53,	3774 },		{ 54,	3654 },
		{ 55,	3539 },		{ 56,	3429 },		{ 57,	3322 },		{ 58,	3219 },		{ 59,	3119 },
		{ 60,	3024 },		{ 61,	2931 },		{ 62,	2842 },		{ 63,	2756 },		{ 64,	2673 },
		{ 65,	2593 },		{ 66,	2516 },		{ 67,	2441 },		{ 68,	2369 },		{ 69,	2300 },
		{ 70,	2233 },		{ 71,	2168 },		{ 72,	2105 },		{ 73,	2044 },		{ 74,	1986 },
		{ 75,	1929 },		{ 76,	1874 },		{ 77,	1821 },		{ 78,	1770 },		{ 79,	1720 },
		{ 80,	1673 },		{ 81,	1626 },		{ 82,	1581 },		{ 83,	1538 },		{ 84,	1496 },
		{ 85,	1455 },		{ 86,	1416 },		{ 87,	1377 },		{ 88,	1340 },		{ 89,	1304 },
		{ 90,	1270 },		{ 91,	1236 },		{ 92,	1204 },		{ 93,	1172 },		{ 94,	1141 },
		{ 95,	1112 },		{ 96,	1083 },		{ 97,	1055 },		{ 98,	1028 },		{ 99,	1002 },
		{ 100,	976 }
#else
		/*NCP15XH103F03RC*/
		{ -40,	195652 },	{ -39,	184917 },	{ -38,	174845 },	{ -37,	165391 },	{ -36,	156512 },
		{ -35,	148171 },	{ -34,	140330 },	{ -33,	132957 },	{ -32,	126021 },	{ -31,	119493 },
		{ -30,	113347 },	{ -29,	107564 },	{ -28,	102115 },	{ -27,	96977 },	{ -26,	92131 },
		{ -25,	87558 },	{ -24,	83242 },	{ -23,	79166 },	{ -22,	75315 },	{ -21,	71676 },
		{ -20,	68236 },	{ -19,	64990 },	{ -18,	61919 },	{ -17,	59011 },	{ -16,	56257 },
		{ -15,	53649 },	{ -14,	51177 },	{ -13,	48834 },	{ -12,	46613 },	{ -11,	44505 },
		{ -10,	42506 },	{ -9,	40599 },	{ -8,	38790 },	{ -7,	37072 },	{ -6,	35441 },
		{ -5,	33892 },	{ -4,	32419 },	{ -3,	31020 },	{ -2,	29689 },	{ -1,	28423 },
		{ 0,	27218 },	{ 1,	26076 },	{ 2,	24987 },	{ 3,	23950 },	{ 4,	22962 },
		{ 5,	22021 },	{ 6,	21123 },	{ 7,	20266 },	{ 8,	19449 },	{ 9,	18669 },
		{ 10,	17925 },	{ 11,	17213 },	{ 12,	16534 },	{ 13,	15885 },	{ 14,	15265 },
		{ 15,	14673 },	{ 16,	14107 },	{ 17,	13566 },	{ 18,	13048 },	{ 19,	12554 },
		{ 20,	12080 },	{ 21,	11628 },	{ 22,	11194 },	{ 23,	10779 },	{ 24,	10381 },
		{ 25,	10000 },	{ 26,	9634 },		{ 27,	9283 },		{ 28,	8947 },		{ 29,	8624 },
		{ 30,	8314 },		{ 31,	8018 },		{ 32,	7733 },		{ 33,	7460 },		{ 34,	7199 },
		{ 35,	6947 },		{ 36,	6706 },		{ 37,	6475 },		{ 38,	6252 },		{ 39,	6039 },
		{ 40,	5833 },		{ 41,	5635 },		{ 42,	5445 },		{ 43,	5262 },		{ 44,	5086 },
		{ 45,	4916 },		{ 46,	4753 },		{ 47,	4597 },		{ 48,	4446 },		{ 49,	4300 },
		{ 50,	4160 },		{ 51,	4026 },		{ 52,	3896 },		{ 53,	3771 },		{ 54,	3651 },
		{ 55,	3535 },		{ 56,	3423 },		{ 57,	3315 },		{ 58,	3211 },		{ 59,	3111 },
		{ 60,	3014 },		{ 61,	2922 },		{ 62,	2833 },		{ 63,	2748 },		{ 64,	2665 },
		{ 65,	2586 },		{ 66,	2509 },		{ 67,	2435 },		{ 68,	2363 },		{ 69,	2294 },
		{ 70,	2227 },		{ 71,	2162 },		{ 72,	2100 },		{ 73,	2039 },		{ 74,	1981 },
		{ 75,	1924 },		{ 76,	1869 },		{ 77,	1817 },		{ 78,	1765 },		{ 79,	1716 },
		{ 80,	1668 },		{ 81,	1622 },		{ 82,	1577 },		{ 83,	1534 },		{ 84,	1492 },
		{ 85,	1452 },		{ 86,	1412 },		{ 87,	1374 },		{ 88,	1338 },		{ 89,	1302 },
		{ 90,	1268 },		{ 91,	1234 },		{ 92,	1201 },		{ 93,	1170 },		{ 94,	1139 },
		{ 95,	1109 },		{ 96,	1080 },		{ 97,	1052 },		{ 98,	1025 },		{ 99,	999 },
		{ 100,	973 }
#endif
	};

	USHORT uiLeft;
	USHORT uiRight;
	float frac;
	UCHAR size = TC_TABLE_NUM - 1u;
	float temp;

	if(rth > TC[0].resistance){
		uiLeft = 0u;
		frac = 0.0;
	} else if(rth > TC[size].resistance) {
		USHORT index = (USHORT)size >> 1u;
		uiLeft = 0u;
		uiRight = size;
		while(uiRight - uiLeft > 1u){
			if(rth > TC[index].resistance){
				uiRight = index;
			} else {
				uiLeft = index;
			}
			index = (uiRight + uiLeft) >> 1u;
		}
		frac = (float)(TC[index].resistance - rth) / (float)(TC[index].resistance - TC[index+(USHORT)1].resistance);
	} else {
		uiLeft = size;
		frac = 0.0;
	}

	if(uiLeft == size){
		temp = (float)TC[uiLeft].temp;
	} else {
		temp = (TC[uiLeft+(USHORT)1].temp - TC[uiLeft].temp) * frac + (float)TC[uiLeft].temp;
	}
#if 0
	if(temp > 0.0){
		temp += 0.5;
	} else {
		temp = ((0.0 - temp) + 0.5);
		temp *= -1.0;
	}
#else
	temp *= (float)10.0;
#endif
	SHORT ret = (SHORT)temp;
	return (ret);
}


/****************************************************************************
FUNCTION	: vBMIC_Wait
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
void vBMIC_Wait(USHORT usdata)
{
	/* 500ns * 50MHz / 1(temp) need to check step size = 25 loop */
	volatile USHORT usdummy;
	for (usdummy = 0u; usdummy < usdata; usdummy++){	/* temp */
		;
	}
}


/****************************************************************************
FUNCTION	: ucBMIC_GetPackTemp_deg_40
DESCRIPTION	: Get Pack Temperature with offset 40
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
UCHAR ucBMIC_GetPackTemp_deg_40(void)
{
	UCHAR ret = 0u;
	SHORT maxdata = -1000;
	USHORT cnt = 0u;
	USHORT i = 0u;
	for (i=0; i<THERMISTOR_NUM; i++) {
		if ((BMICCellConf.ulCellTempPos & ((ULONG)1 << i)) != 0u) {
			if (maxdata < BMIC_Info.sTemp_01Cdeg[i]) {
				maxdata = BMIC_Info.sTemp_01Cdeg[i];
			}
			cnt++;
			if (cnt >= BMICCellConf.ucCellTempCount) {
				break;
			}
		}
	}
	if (maxdata != -1000) {
		maxdata += 400;
		maxdata /= 10;
		ret = (UCHAR)maxdata;
	}
	return ret;
}

/****************************************************************************
FUNCTION	: sBMIC_GetPackTemp_01deg
DESCRIPTION	: Get Pack Temperature
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
SHORT sBMIC_GetPackTemp_01deg(void)
{
	SHORT maxdata = -1000;
	USHORT cnt = 0u;
	USHORT i = 0u;
	for (i=0; i<THERMISTOR_NUM; i++) {
		if ((BMICCellConf.ulCellTempPos & ((ULONG)1 << i)) != 0u) {
			if (maxdata < BMIC_Info.sTemp_01Cdeg[i]) {
				maxdata = BMIC_Info.sTemp_01Cdeg[i];
			}
			cnt++;
			if (cnt >= BMICCellConf.ucCellTempCount) {
				break;
			}
		}
	}
	if (maxdata == -1000) {
		maxdata = 0;
	}
	return maxdata;
}

/****************************************************************************
FUNCTION	: ulBMIC_GetBatPackChgSum_01mAh
DESCRIPTION	: (ChgSum_AD * 360 / 2^16 / SHUNT_mohm)[A] * 100ms
			 => (ChgSum_AD * 360 / 2^16 / SHUNT_mohm)[A] / 36000 [100ms -> h]
			 => (ChgSum_AD * 360 / 2^16 / SHUNT_mohm) * 10000 [A->0.1mA] / 36000 [100ms -> h]
			 => (ChgSum_AD * 100 / 2^16 / SHUNT_mohm) [0.1mAh]
			(If cycle time not 100ms, the formula need change !!!)
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
ULONG ulBMIC_GetBatPackChgSum_01mAh(void)
{
	ULLONG ullval = (ULLONG)BMIC_Info.ullChgSum_AD;
	ULONG ret;
/*	ullval = (ullval * 100u) / (65536u * SHUNT_mohm);*/
	ullval = (ullval * 100u) / (65536u * (ULLONG)SHUNT_uohm / 1000u);
		/* Pack current[10mA] = 100 x AD(signed) * 360mV/2^16 / 3mohm)	*/
		/* [0.1mAh] = Pack current[10mA*100ms] / 36000[100msec/h] / 100[10mA/A] * 10000 [A->0.1mA]	*/
	ret = (ULONG)ullval;
	return ret;
}

/****************************************************************************
FUNCTION	: ulBMIC_GetBatPackDchgSum_01mAh
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/01
*****************************************************************************/
ULONG ulBMIC_GetBatPackDchgSum_01mAh(void)
{
	ULLONG ullval = (ULLONG)BMIC_Info.ullDisChgSum_AD;
	ULONG ret;
/*	ullval = (ullval * 100u) / (65536u * SHUNT_mohm);*/
	ullval = (ullval * 100u) / (65536u * (ULLONG)SHUNT_uohm / 1000u);
			/* Pack current[10mA] = 100 x AD(signed) * 360mV/2^16 / 3mohm)	*/
			/* [0.1mAh] = Pack current[10mA*100ms] / 36000[100msec/h] / 100[10mA/A] * 10000 [A->0.1mA]	*/
	ret = (ULONG)ullval;
	return ret;
}

/****************************************************************************
FUNCTION	: vBMIC_SetBatPackChgSum_01mAh
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/13
*****************************************************************************/
void vBMIC_SetBatPackChgSum_01mAh(ULONG sum_01mAh)
{
	ULLONG ullval = (ULLONG)sum_01mAh;
/*	ullval *= SHUNT_mohm * 65536u;*/
	ullval *= (ULLONG)SHUNT_uohm * 65536u / 1000u;
	ullval /= 100u;
	BMIC_Info.ullChgSum_AD = ullval;
}

/****************************************************************************
FUNCTION	: vBMIC_SetBatPackDchgSum_01mAh
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/13
*****************************************************************************/
void vBMIC_SetBatPackDchgSum_01mAh(ULONG sum_01mAh)
{
	ULLONG ullval = (ULLONG)sum_01mAh;
/*	ullval *= SHUNT_mohm * 65536u;*/
	ullval *= (ULLONG)SHUNT_uohm * 65536u / 1000u;
	ullval /= 100u;
	BMIC_Info.ullDisChgSum_AD = ullval;
}

/****************************************************************************
FUNCTION	: vBMIC_StartCurCalibration
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/20
*****************************************************************************/
void vBMIC_StartCurCalibration(USHORT ref_10mA_300A)
{
	LONG ad_ref = 0u;
	ucBMIC_CurClbFin = 0u;
	usBMIC_CurClbCount = CALIB_AD_COUNT;		/* 8 * 100msec = 800msec	*/
	lBMIC_CurClbSum = 0u;
/*	ad_ref = (ref_10mA_300A-30000)*10*(LONG)SHUNT_mohm*65536;*/
/*	ad_ref = (ref_10mA_300A-30000)*10*(LONG)SHUNT_uohm*65536/1000u;*/
	ad_ref = (LONG)(ref_10mA_300A-(USHORT)30000)*(LONG)SHUNT_uohm*65536/(LONG)100u;
	lBMIC_CurClbReference_1000 = ad_ref / 360;
}

/****************************************************************************
FUNCTION	: vBMIC_ExeCurCalibration
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/20
*****************************************************************************/
static void vBMIC_ExeCurCalibration(void)
{
	if(usBMIC_CurClbCount != (USHORT)0){
		SHORT data = (SHORT)usBMICReadReg(CVIL_AD_ADDR);
		bool spi_result = bBMICCheckSpiResult();
		LONG clb_ave_AD_1000 = 0u;
		float fgain = 0.0;
		if(spi_result == false){
			usBMIC_CurClbCount = 0u;
			ucBMIC_CurClbFin = 1u;
		} else {
			data *= -1;
			lBMIC_CurClbSum += data;
			usBMIC_CurClbCount--;
			if(usBMIC_CurClbCount == (USHORT)0){
				ucBMIC_CurClbFin = 1u;
				clb_ave_AD_1000 = (lBMIC_CurClbSum * 1000) / (LONG)CALIB_AD_COUNT;
				fgain = (float)lBMIC_CurClbReference_1000 / (float)clb_ave_AD_1000;
				if(fgain < 0.0){
					fgain *= (float)(-1.0);
				}
				if(fgain > 2.0){
					fgain = 2.0;
				} else if(fgain < 0.5){
					fgain = 0.5;
				} else{;}
				fBMIC_CurClbGain = fgain;
			}
		}
	}
}

/****************************************************************************
FUNCTION	: ucBMIC_ChkCalibration
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/20
*****************************************************************************/
UCHAR ucBMIC_ChkCalibration(void)
{
	UCHAR ret = (UCHAR)false;
	if(ucBMIC_CurClbFin == 1u){
		ret = (UCHAR)true;
	}
	return ret;
}

/****************************************************************************
FUNCTION	: fBMIC_GetCalibration
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/20
*****************************************************************************/
float fBMIC_GetCalibration(void)
{
	return fBMIC_CurClbGain;
}

/****************************************************************************
FUNCTION	: vBMIC_SetCalibration
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/11/20
*****************************************************************************/
void vBMIC_SetCalibration(float calib)
{
	fBMIC_CurClbGain = calib;
}


/****************************************************************************
FUNCTION	: bBMIC_ChkSpiMiso
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2020/1/21
*****************************************************************************/
bool c(void)
{
	bool ret = false;
	if(BMIC_Drv.bBMIC_NgSpiMiso == true) {
		ret = true;
	}

	return ret;
}

/****************************************************************************
FUNCTION	: vBMIC_UvReset
DESCRIPTION	:
INPUT		: ucUvResetState 1: UVTH = UVTH - UV_HYS - 2,  0: UVTH = initial value
OUTPUT		:
UPDATE		:
DATE		: 2019/12/27

 UV���o�d�����q�X�e���V�X���������ċ����I��UV��Ԃ���������B
 UV���o��UV���o�d���ȏ�ŏ[�d�J�n���Ɏg�p����B

*****************************************************************************/
void vBMIC_UvReset(USHORT usUvSetVol)
{
	BMICSetParam.usUVInfo_vol = usUvSetVol;
	vBMIC_Send_OUVCTL1();
}


/****************************************************************************
FUNCTION	: vBMIC_Send_PDREG55en
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2020/1/21
*****************************************************************************/
void vBMIC_Send_PDREG55en(UCHAR pdreg55en)
{
	USHORT data = PD_REG55_OFF;
	if(pdreg55en == 1u) {
		data = PD_REG55_ON;
	}
	ucBMIC_Send_Req(OTHCTL_ADDR, data, PD_REG55_MASK, BMIC_DATA_INPUT_NEED);
}

/****************************************************************************
FUNCTION	: bBMICGetRegChkState
DESCRIPTION	:
INPUT		:
OUTPUT		: true : Register check enable, false : Register check disable
UPDATE		:
DATE		: 2020/2/28
*****************************************************************************/
bool bBMICGetRegChkState(void)
{
	return bRegChkEnable;
}

/****************************************************************************
FUNCTION	: vBMICSetRegChkState
DESCRIPTION	:
INPUT		: true : Register check enable, false : Register check disable
OUTPUT		:
UPDATE		:
DATE		: 2020/2/28
*****************************************************************************/
void vBMICSetRegChkState(bool bChkEnable)
{
	bRegChkEnable = bChkEnable;
}


/****************************************************************************
FUNCTION	: vBMICRegChkRelCheck
DESCRIPTION	: If external IF change control register setting, release the register check
INPUT		: UCHAR regaddr
OUTPUT		:
UPDATE		:
DATE		: 2020/11/10
*****************************************************************************/
void vBMICRegChkRelease(UCHAR regaddr)
{
	if((regaddr >= PWR_CTRL_ADDR)
	&& (regaddr <= INR_CTL_DIAG_EN_ADDR)
	){
		vBMICSetRegChkState(false);
	}
}


/****************************************************************************
FUNCTION	: wait100us
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		:2020/10/12
*****************************************************************************/
void	wait100us(void)
{
	USHORT usCnt;
	for(usCnt =0; usCnt < (USHORT)800; usCnt++){
		;
	}
}


/****************************************************************************
FUNCTION	: Wait_msec
DESCRIPTION	:
INPUT		: USHORT time_inms
OUTPUT		:
UPDATE		:
DATE		:2020/10/12
*****************************************************************************/
void Wait_msec(USHORT time_inms)
{
	USHORT delay = time_inms * MS_100US;
	USHORT usCnt;
	for(usCnt =0; usCnt < delay; usCnt++){
		wait100us();
	}
}

/****************************************************************************
FUNCTION	: BMIC_SpiRelease
DESCRIPTION	: clear BMIC SPI busy flag
INPUT		:
OUTPUT		:
UPDATE		:
DATE		:2020/10/12
*****************************************************************************/
void BMIC_SpiRelease(void)
{
	BMIC_SpiBusy = 0;
}

/****************************************************************************
FUNCTION	: bBMIC_SpiTake
DESCRIPTION	: Check/wait BMIC SPI free & set BMIC SPI busy flag
INPUT		: USHORT timeout
OUTPUT		: true: BMIC SPI not in use, false: BMIC SPIN in use
UPDATE		:
DATE		:2020/10/12
*****************************************************************************/
bool bBMIC_SpiTake(USHORT timeout)
{
	bool bRet = false;
	USHORT wait_t = 0;

	for(; wait_t < timeout; wait_t++){
		if(!BMIC_SpiBusy){
			BMIC_SpiBusy = 1;
			bRet = true;
			break;
		}
	}
	return bRet;
}
