/****************************************************************************
#ifdef DOC
File Name		:drv_spi.c
Description		:
Remark			:
Date			:2020/10/28
Copyright		:Nuvoton Technology Singapore
#endif
*****************************************************************************/
#include "main.h"
#include <stdbool.h>
#include "sys_type.h"
#include "drv_spi.h"


/****************************************************************************
 define macro
*****************************************************************************/
extern SPI_HandleTypeDef hspi1;
/****************************************************************************
 define type (STRUCTURE, UNION)
*****************************************************************************/

/****************************************************************************
 define type (Enumerated type ENUM)
*****************************************************************************/

/****************************************************************************
 Variable declaration
*****************************************************************************/


/****************************************************************************
 Prototype declaration
*****************************************************************************/

/****************************************************************************
 define data table
*****************************************************************************/


/****************************************************************************
FUNCTION	: bSPI_BMIC_Rx
DESCRIPTION	: BMIC SPI Receive
INPUT		: UCHAR *txBuf, UCHAR *rxBuf, UCHAR sendSize
OUTPUT		: bool
UPDATE		:
DATE		: 2020/10/12
*****************************************************************************/
bool bSPI_BMIC_Rx(UCHAR *txBuf, UCHAR *rxBuf, UCHAR sendSize)
{
	bool ret = false;
	//UCHAR cnt=0;

	//SPI_ClearRxFIFO(SPI1);									/* Clear Rx FIFO before Rx start	*/
	HAL_SPI_TransmitReceive(&hspi1, txBuf, rxBuf, sendSize, 1000);
//	for(cnt=0;cnt < sendSize; cnt++){
//
//		SPI_WRITE_TX(SPI1, (*(txBuf+cnt) & (ULONG)0x000000FF));
//		while(SPI_IS_BUSY(SPI1)){
//			;
//		}
//		*(rxBuf+cnt) = (UCHAR)SPI_READ_RX(SPI1);
//
//	}

	ret = true;

	return ret;
}


/****************************************************************************
FUNCTION	: bSPI_BMIC_Tx
DESCRIPTION	: BMIC SPI transmission
INPUT		: UCHAR *txBuf, UCHAR sendSize
OUTPUT		: bool
UPDATE		:
DATE		: 2020/10/12
*****************************************************************************/
bool bSPI_BMIC_Tx(UCHAR *txBuf, UCHAR sendSize)
{
	bool ret = false;
//	UCHAR cnt = 0;
//
//	for(cnt=0; cnt < sendSize; cnt++){
//
//		SPI_WRITE_TX(SPI1, (*(txBuf+cnt) & (ULONG)0x000000FF));
//		while(SPI_IS_BUSY(SPI1)){
//			;
//		}
//
//	}
	HAL_SPI_Transmit(&hspi1, txBuf, sendSize, 1000);
	ret = true;

	return ret;
}

