#ifndef DRV_BMIC_H
#define DRV_BMIC_H
/****************************************************************************
#ifdef DOC
File Name		:drv_bmic.h
Description		:
Remark			:
Date			:2020/10/28
Copyright		:Nuvoton Technology Singapore
#endif
*****************************************************************************/

/****************************************************************************
 define macro
*****************************************************************************/
#define	BMIC_49522			(0u)		/* Default 0: 49517A used */
/*#define	BMIC_49522		(1u)*/	/* Enable 1: 49522A used */

/* BMIC DD Ver: 1.00 */
#if BMIC_49522
#define	BMIC_DD_VER_MAJOR		(01u | 0x80u)		/* VER_MAJOR: bp0~bp6 (00~99), bp7 fixed 1 for BMIC_49522 */
#else	/* BMIC_49517 */
#define	BMIC_DD_VER_MAJOR		(01u & 0x7Fu)		/* VER_MAJOR: bp0~bp6 (00~99), bp7 fixed 0 for BMIC_49517 */
#endif	/* BMIC_49522 */
#define	BMIC_DD_VER_MINOR		(01u)				/* VER_MINOR: bp0~bp7 (00~99) */


#if BMIC_49522
#define	MAX_CELL_NUM			((UCHAR)22u)
#else	/* BMIC_49517 */
#define	MAX_CELL_NUM			((UCHAR)17u)
#endif	/* BMIC_49522 */
#define MIN_CELL_NUM			((UCHAR)4u)

#define	MAX_THERMISTOR_NUM	((UCHAR)5u)				/* BMIC max TMONI: 5 */
#define	THERMISTOR_NUM		MAX_THERMISTOR_NUM			/* Range: 1 ~ MAX_THERMISTOR_NUM */


/*#define 									(0x0001u)*/		/* bp0 not used										*/
/*#define 									(0x0002u)*/		/* bp1 not used                                     */
#define ERR_IC_SHUTDOWN						(0x0004u)		/* bp2 bit position depends on BMIC(MODE STAT)      */
/*#define 									(0x0008u)*/		/* bp3 not used									   	*/
#define ERR_OVER_CURRENT_CHARGE				(0x0010u)		/* bp4 bit position depends on BMIC(STAT)           */
#define ERR_OVER_CURRENT_DISCHARGE			(0x0020u)		/* bp5 bit position depends on BMIC(STAT)           */
#define ERR_SHORT_CIRCUIT_DET				(0x0040u)		/* bp6 bit position depends on BMIC(STAT)           */
#define ERR_CELL_CONNECT_NG					(0x0080u)		/* bp7                                              */
#define ERR_UNDER_VOLTAGE					(0x0100u)		/* bp8 bit position depends on BMIC(STAT)           */
#define ERR_OVER_VOLTAGE					(0x0200u)		/* bp9 bit position depends on BMIC(STAT)           */
#define ERR_OVER_TEMP_BATTERY_CHARGE		(0x0400u)		/* bp10                                             */
#define ERR_LOW_TEMP_BATTERY_CHARGE			(0x0800u)		/* bp11                                             */
#define ERR_OVER_TEMP_BATTERY_DISCHARGE		(0x1000u)		/* bp12                                             */
#define ERR_LOW_TEMP_BATTERY_DISCHARGE		(0x2000u)		/* bp13                                             */
/*#define 									(0x4000u)*/		/* bp14 not used									*/
/*#define BMIC_NG_SPI_MISO					(0x8000u)*/		/* bp15	This flag not used							*/
#define MASK_TEMPARATURE_ERR				(0x3C00u)		/* bp10~13: temperature related errors				*/
#define MASK_CURRENT_ERR					(0x0070u)		/* bp4~6: current related errors					*/


#define	BMIC_DATA_INPUT_NONE			(0u)
#define	BMIC_DATA_INPUT_NEED			(1u)

#define USE_GPOH_FET					(1u)
#define NOT_USE_GPOH_FET				(0u)

/****************************************************************************
 define type (STRUCTURE, UNION)
*****************************************************************************/
typedef struct {
	bool	BMICMeasured;
	LONG	lBatPackCur_100uA;					/*�p�b�N�d��						*/
	LONG	lBatPackFastCur_100uA;				/*�p�b�N�d��            */
	ULONG	ulBlkVol_uV[MAX_CELL_NUM];			/*�u���b�N�d��          */
	SHORT	sTemp_01Cdeg[THERMISTOR_NUM];		/*���x                  */
	ULONG	ulVpackV_uV;						/*�p�b�N�d��            */
	ULONG	ulVdd55V_uV;						/*VDD55 voltage in uV   */
	ULONG	ulVdd18V_uV;						/*VDD18 voltage in uV   */
	ULONG	ulVRegextV_uV;						/*REGEXT voltage in uV  */
	ULONG	ulVref2V_uV;						/*VREF2 voltage in uV   */
	#if(0)	/* VREF1 not used */
	ULONG	ulVref1V_uV;						/*VREF1 voltage in uV   */
	#endif
	ULONG	ulVpackSumV_uV;						/*�p�b�N�d��						*/

	USHORT	usBatPackCur_10mA_300A;				/*�p�b�N�d��						*/
	ULONG	ulBatPackMinBlkVol_uV;				/*�ŏ��u���b�N�d���i���n��j			*/
	ULONG	ulBatPackMaxBlkVol_uV;				/*�ő�u���b�N�d���i���n��j			*/

	ULLONG	ullChgSum_AD;
	ULLONG	ullDisChgSum_AD;
	ULONG	ulBatPackErr;						/*�G���[�t���O	*/
	ULONG	ulBatPackErrLog;
	USHORT	usBatPackErrNum;					/*�G���[�ԍ� */
	UCHAR	ucMaintenanceMode;
}TBMIC_Info;

/*--------------------------------------------*/
typedef struct {
	ULONG	ulCellPos;						/* Cell connection position																				*/
	UCHAR	ucSeriesCount;					/* Number of cell in series                                       */
	UCHAR	ucCellTempCount;				/* Number of thermistors for cell temperature detection           */
	ULONG	ulCellTempPos;					/* Cell temperature thermistor connection position                */
}TBMICCellConf;

/*--------------------------------------------*/
typedef struct {
	USHORT	usOCInfo_cur;						/* OC threshold for current abnormality before fuse cut		*/
	USHORT	usOVInfo_vol;						/* �ߓd��(OV)臒l	�d��(mV)                                */
	USHORT	usOVInfo_delay;						/* �ߓd��(OV)臒l	�x������(msec)                          */
	USHORT	usOVInfo_hys;						/* �ߓd��(OV)臒l	�q�X�e���V�X��(mV)                      */
	USHORT	usUVInfo_vol;						/* �ߕ��d(UV)臒l	�d��(mV)                                */
	USHORT	usUVInfo_delay;						/* �ߕ��d(UV)臒l	�x������(msec)                          */
	USHORT	usUVInfo_hys;						/* �ߕ��d(UV)臒l	�q�X�e���V�X��(mV)                      */
	USHORT	usOCCInfo_cur;						/* �[�d�ߓd��(OCC)臒l	�d��(A)                           */
	USHORT	usOCCInfo_delay;					/* �[�d�ߓd��(OCC)臒l	�x������(msec)                    */
	USHORT	usOCDInfo_cur;						/* ���d�ߓd��(OCD)臒l	�d��(A)                           */
	USHORT	usOCDInfo_delay;					/* ���d�ߓd��(OCD)臒l	�x������(msec)                    */
	USHORT	usSCDInfo_cur;						/* �Z���d��(SCD)臒l	�d��(A)                             */
	ULONG	ulSCDInfo_delay;					/* �Z���d��(SCD))臒l	�x������(nsec)                      */
	USHORT	usOTBatInfo_temp;					/* ���d���ߔM�i�o�b�e���j臒l	���x(C)(Offset 40C)         */
	USHORT	usOTBatInfo_temp_recover;			/* ���d���ߔM�i�o�b�e���j臒l	���A���x(C)(Offset 40C)     */
	USHORT	usLTBatInfo_temp;					/* ���d���ቷ�i�o�b�e���j臒l	���x(C)(Offset 40C)         */
	USHORT	usLTBatInfo_temp_recover;			/* ���d���ቷ�i�o�b�e���j臒l	���A���x(C)(Offset 40C)     */
	USHORT	usOTBatChrgInfo_temp;				/* �[�d���ߔM�i�o�b�e���j臒l	���x(C)(Offset 40C)         */
	USHORT	usOTBatChrgInfo_temp_recover;		/* �[�d���ߔM�i�o�b�e���j臒l	���A���x(C)(Offset 40C)*/
	USHORT	usLTBatChrgInfo_temp;				/* �[�d���ቷ�i�o�b�e���j臒l	���x(C)(Offset 40C)    */
	USHORT	usLTBatChrgInfo_temp_recover;		/* �[�d���ቷ�i�o�b�e���j臒l	���A���x(C)(Offset 40C)*/
}TBMICSetParam;


/****************************************************************************
 define type (Enumerated type ENUM)
*****************************************************************************/
enum {
	BMIC_ALARM1 = (UCHAR)0u,
	BMIC_ALARM2,
	BMIC_ADIRQ1,
	BMIC_ADIRQ2
}; /* BMIC 49517/22 port */

enum {
	BMIC_STARTUP = (UCHAR)0u,
	BMIC_NORMAL,
	BMIC_SPIERR,
	BMIC_RESTART,
	BMIC_SHUTDOWN
}; /* static ucBMIC_Ctrl_Status */



/****************************************************************************
 Variable declaration
*****************************************************************************/
extern TBMICCellConf		BMICCellConf;
extern TBMIC_Info			BMIC_Info;
extern TBMICSetParam 		BMICSetParam;

/****************************************************************************
 Prototype declaration
*****************************************************************************/
extern void vBMIC_InitParams(void);
extern void vBMIC_Send_OUVCTL1(void);
extern void vBMIC_Send_OUVCTL2(void);
extern void vBMIC_Send_ALARM_CTL2(void);
extern void vBMIC_Send_ALARM_CTL3(void);
extern UCHAR getBMIC_Ctrl_Status(void);
extern void vBMIC_Ctrl_Startup(void);
extern void vBMIC_Ctrl_Normal(void);
extern void vBMIC_Ctrl_SpiErr(void);
extern void vBMIC_Ctrl_Restart(void);
extern void vBMIC_Ctrl_Shutdown(void);
extern void vBMIC_ShutdownRequest(void);
extern ULONG ulBMIC_GetBatPackChgSum_01mAh(void);
extern ULONG ulBMIC_GetBatPackDchgSum_01mAh(void);
extern void vBMIC_SetBatPackChgSum_01mAh(ULONG sum_01Ah);
extern void vBMIC_SetBatPackDchgSum_01mAh(ULONG sum_01Ah);
extern SHORT sBMIC_GetPackTemp_01deg(void);
extern UCHAR ucBMIC_Send_Req(UCHAR addr, USHORT data, USHORT mask, UCHAR mask_req);
extern void vBMIC_SetCellbalanceReq(ULONG target);
extern void vBMIC_SetCellbalanceReq_Mnt(ULONG target);
extern ULONG ulBMIC_GetCellbalanceReqPack(void);
extern ULONG ulBMIC_GetCellbalanceReq(void);
extern bool bBMIC_ChkCellbalance(void);
extern void vBMIC_SetMntMode(UCHAR mode);
extern bool bBMIC_GetMntMode(void);
extern void vBMIC_Thermistor_readReq(UCHAR req);

extern void vBMIC_Discharge_FET_Out(UCHAR output);
extern void vBMIC_Charge_FET_Out(UCHAR output);
extern void vBMIC_PreDischarge_FET_Out(UCHAR output, UCHAR fet_setting);
extern void vBMIC_PreCharge_FET_Out(UCHAR output, UCHAR fet_setting);
extern void vBMIC_SHDN_Out(UCHAR output);
extern void vBMIC_FETOFF_Out(UCHAR output);
/*extern UCHAR ucGetBMIC_ALARM1_In(void);	*/
extern USHORT usBMIC_ReadReg(UCHAR addr);
extern USHORT usBMICReferRegData(UCHAR addr);
extern void vBMIC_StartCurCalibration(USHORT ref_mA);
extern UCHAR ucBMIC_ChkCalibration(void);
extern float fBMIC_GetCalibration(void);
extern void vBMIC_SetCalibration(float calib);
extern bool bBMICCheckSpiResult(void);
extern UCHAR ucGetBMIC_SpiErrDetail(void);
extern void BMIC_Clear_Spi_Err_counter(void);
extern bool bBMIC_ChkSpiMiso(void);
extern void vBMIC_UvReset(USHORT usUvSetVol);
extern void vBMIC_Send_PDREG55en(UCHAR pdreg55en);
extern void vBMICSetRegChkState(bool bChkEnable);
extern void vBMICRegChkRelease(UCHAR regaddr);
extern UCHAR ucBMIC_GetPackTemp_deg_40(void);
#endif /* DRV_BMIC_H */
