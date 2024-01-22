/****************************************************************************
#ifdef DOC
File Name		:tsk_bmic.c
Description		:
Remark			:
Date			:2020/10/28
Copyright		:Nuvoton Technology Singapore
#endif
*****************************************************************************/
#include <stdbool.h>
#include "sys_type.h"
#include "main.h"
#include "drv_bmic.h"
#include "tsk_bmic.h"

/****************************************************************************
 define macro
*****************************************************************************/


/****************************************************************************
 define type (STRUCTURE, UNION)
*****************************************************************************/
typedef struct {
	USHORT	usMinCellVolLife_mV;				/*�ŏ��u���b�N�d���i���U�j				*/
	USHORT	usMaxCellVolLife_mV;				/*�ő�u���b�N�d���i���U�j        */
	USHORT	usMinPackCurLife_10mA_300A;			/*�ŏ��p�b�N�d���i���U�j          */
	USHORT	usMaxPackCurLife_10mA_300A;			/*�ő�p�b�N�d���i���U�j          */
	UCHAR	ucMinCellTempLife_deg_40;			/*�ŏ��v���[�u���x�i���U�j        */
	UCHAR	ucMaxCellTempLife_deg_40;			/*�ő�v���[�u���x�i���U�j        */
	ULONG	ulChgCount;							/* BatPackChgCnt �[�d�񐔁i���U�j */
}TLifeBatInfo;

/****************************************************************************
 define type (Enumerated type ENUM)
*****************************************************************************/

/****************************************************************************
 Variable declaration
*****************************************************************************/
static TLifeBatInfo 	LifeBatInfo;
#if(BMS_STATE_CTRL)
#else
static ULONG 		ulBMIC_RtcFrc;
#endif	/* BMS_STATE_CTRL */


/****************************************************************************
 Prototype declaration
*****************************************************************************/
void vTsk_bmic(void);



/****************************************************************************
FUNCTION	: UpdateLifeBatInfo
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/10/29
*****************************************************************************/
static void UpdateLifeBatInfo(void)
{
	if (BMIC_Info.BMICMeasured) {
		ULONG ultemp = BMIC_Info.ulBatPackMinBlkVol_uV;
		UCHAR ucdata_deg_40;
		ultemp /= 1000u;
		if (ultemp < LifeBatInfo.usMinCellVolLife_mV) {
			LifeBatInfo.usMinCellVolLife_mV = (USHORT)ultemp;
		}
		ultemp = BMIC_Info.ulBatPackMaxBlkVol_uV;
		ultemp /= 1000u;
		if (ultemp > LifeBatInfo.usMaxCellVolLife_mV) {
			LifeBatInfo.usMaxCellVolLife_mV = (USHORT)ultemp;
		}
		if (BMIC_Info.usBatPackCur_10mA_300A < LifeBatInfo.usMinPackCurLife_10mA_300A) {
			LifeBatInfo.usMinPackCurLife_10mA_300A = BMIC_Info.usBatPackCur_10mA_300A;
		}
		if (BMIC_Info.usBatPackCur_10mA_300A > LifeBatInfo.usMaxPackCurLife_10mA_300A) {
			LifeBatInfo.usMaxPackCurLife_10mA_300A = BMIC_Info.usBatPackCur_10mA_300A;
		}
		ucdata_deg_40 = ucBMIC_GetPackTemp_deg_40();
		if (ucdata_deg_40 < LifeBatInfo.ucMinCellTempLife_deg_40) {
			LifeBatInfo.ucMinCellTempLife_deg_40 = ucdata_deg_40;
		}
		if (ucdata_deg_40 > LifeBatInfo.ucMaxCellTempLife_deg_40) {
			LifeBatInfo.ucMaxCellTempLife_deg_40 = ucdata_deg_40;
		}
	}
}

#if(BMS_STATE_CTRL)
#else
/****************************************************************************
FUNCTION	: Check_1sec
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		: 2019/12/19
*****************************************************************************/
static void Check_1sec(void)
{
/*	ULONG ulRtcFrc = RtcDrv.ulGetRtcFrc();	*/
	ULONG ulRtcFrc = ulGetMainSecCnt();

	if(ulRtcFrc >= (1 + ulBMIC_RtcFrc)) {
		vBMIC_Thermistor_readReq(1u);
	}
	ulBMIC_RtcFrc = ulRtcFrc;

}
#endif	/* BMS_STATE_CTRL */

/****************************************************************************
FUNCTION	: vTskInit_bmic
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		:
*****************************************************************************/
void vTskInit_bmic(void)
{
	LifeBatInfo.usMinCellVolLife_mV = 0xFFFFu;
	LifeBatInfo.usMaxCellVolLife_mV = 0u;
	LifeBatInfo.usMinPackCurLife_10mA_300A = 0xFFFFu;
	LifeBatInfo.usMaxPackCurLife_10mA_300A = 0u;
	LifeBatInfo.ucMinCellTempLife_deg_40 = 0xFFu;
	LifeBatInfo.ucMaxCellTempLife_deg_40 = 0u;
	LifeBatInfo.ulChgCount = 0u;

	vBMIC_InitParams();
}



/****************************************************************************
FUNCTION	: vTsk_bmic
DESCRIPTION	:
INPUT		:
OUTPUT		:
UPDATE		:
DATE		:
*****************************************************************************/
void vTsk_bmic(void)
{
	UCHAR status;

	BMIC_Clear_Spi_Err_counter();
	status = getBMIC_Ctrl_Status();
	switch (status) {
		case (UCHAR)BMIC_STARTUP:
			vBMIC_Ctrl_Startup();
			break;
		case (UCHAR)BMIC_NORMAL:
#if(BMS_STATE_CTRL)								/* Read temperature data every loop */
			vBMIC_Thermistor_readReq(1u);
#else
			Check_1sec();
#endif	/* BMS_STATE_CTRL */
			vBMIC_Ctrl_Normal();
			vBMIC_Thermistor_readReq(0u);
			break;
		case (UCHAR)BMIC_SPIERR:
			vBMIC_Ctrl_SpiErr();
			break;
		case (UCHAR)BMIC_RESTART:
			vBMIC_Ctrl_Restart();
			break;
		case (UCHAR)BMIC_SHUTDOWN:
			vBMIC_Ctrl_Shutdown();
			break;
	}
	UpdateLifeBatInfo();

}

