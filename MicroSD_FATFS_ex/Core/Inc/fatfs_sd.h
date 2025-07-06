#ifndef _FATFS_SD_H_
#define _FATFS_SD_H_

#include "ff.h"

/**
 * https://elm-chan.org/docs/mmc/mmc_e.html
 */

#define CMD0_CRC 0x95
#define CMD8_CRC 0x87

typedef enum {
    CMD0  = 0x40 + 0,
    CMD1  = 0x40 + 1,
    CMD41 = 0x40 + 41,
    CMD8  = 0x40 + 8,
    CMD12 = 0x40 + 12,
    CMD16 = 0x40 + 16,
    CMD17 = 0x40 + 17,
    CMD18 = 0x40 + 18,
    CMD24 = 0x40 + 24,
    CMD25 = 0x40 + 25,
    CMD55 = 0x40 + 55,
    CMD58 = 0x40 + 58
} SD_Command_Type;

typedef enum {
    IN_IDLE_STATE        = 0x01,
    ERASE_RESET          = 0x02,
    ILLEGAL_COMMAND      = 0x04,
    COMMAND_CRC_ERROR    = 0x08,
    ERASE_SEQUENCE_ERROR = 0x10,
    ADDRESS_ERROR        = 0x20,
    PARAMETER_ERROR      = 0x40
} SD_Response_Error_Type;

#define SD_IS_ERROR_RESPONSE(resp) (!(resp == IN_IDLE_STATE))

typedef uint8_t  SD_Response;
typedef uint32_t SD_Information;

typedef enum {
    SD_V2_BLOCK_ADDRESS,
    SD_V2_BYTE_ADDRESS,
    SD_V1,
    SD_MMC_V3,
    SD_UNKNOWN
} SD_Version_Type;

typedef enum {
    SD_OK = 0,
    SD_ERROR,
    SD_ERR_TIMEOUT,
} sd_status_t;

sd_status_t SD_Initialize(BYTE pdrv);

void SD_Read(BYTE  pdrv,   /* Physical drive nmuber to identify the drive */
             BYTE *buff,   /* Data buffer to store read data */
             DWORD sector, /* Sector address in LBA */
             UINT  count);

void SD_Write(BYTE pdrv, /* Physical drive nmuber to identify the drive */
              const BYTE *buff,   /* Data to be written */
              DWORD       sector, /* Sector address in LBA */
              UINT        count);

extern SPI_HandleTypeDef hspi1;

#endif