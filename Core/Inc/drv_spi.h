#ifndef DRV_SPI_H
#define DRV_SPI_H
/****************************************************************************
#ifdef DOC
File Name		:drv_spi.h
Description		:
Remark			:
Date			:2020/10/28
Copyright		:Nuvoton Technology Singapore
#endif
*****************************************************************************/

/****************************************************************************
 Prototype declaration
*****************************************************************************/
extern bool bSPI_BMIC_Rx(UCHAR *txBuf, UCHAR *rxBuf, UCHAR sendSize);
extern bool bSPI_BMIC_Tx(UCHAR *txBuf, UCHAR sendSize);

#endif /* DRV_SPI_H */
