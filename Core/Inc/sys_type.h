#ifndef SYS_TYPE_H
#define SYS_TYPE_H
/****************************************************************************
#ifdef DOC
File Name		:sys_type.h
Description		:
Remark			:
Date			:2020/10/28
Copyright		:Nuvoton Technology Singapore
#endif
*****************************************************************************/
#include "main.h"
/****************************************************************************
 Type declaration
*****************************************************************************/
typedef	uint8_t		UCHAR;							/* unsigned 8bit type define */
typedef	int8_t		CHAR;							/* signed 8bit type define */
typedef	uint16_t	USHORT;							/* unsigned 16bit type define */
typedef	int16_t		SHORT;							/* signed 16bit type define */
typedef	uint32_t	ULONG;							/* unsigned 32bit type define */
typedef	int32_t		LONG;							/* signed 32bit type define */
typedef	uint64_t	ULLONG;							/* unsigned 64bit type define */


/****************************************************************************
 define macro
*****************************************************************************/
#define	OK		((UCHAR)1u)
#define NG		((UCHAR)0u)

#define	HIGH	((UCHAR)1u)
#define LOW		((UCHAR)0u)

#endif /* SYS_TYPE_H */
