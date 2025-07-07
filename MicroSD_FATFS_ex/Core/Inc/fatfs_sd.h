#ifndef _FATFS_SD_H_
#define _FATFS_SD_H_

#include "ff.h"

/**
 * https://elm-chan.org/docs/mmc/mmc_e.html
 */

#define CMD0_CRC 0x95
#define CMD8_CRC 0x87
#define CMD58_CRC 0x75

typedef uint8_t SD_Command_Type;

#define CMD0 (0x40 + 0)
#define CMD1 (0x40 + 1)
#define ACMD41 (0x40 + 41)
#define CMD8 (0x40 + 8)
#define CMD12 (0x40 + 12)
#define CMD16 (0x40 + 16)
#define CMD17 (0x40 + 17)
#define CMD18 (0x40 + 18)
#define CMD24 (0x40 + 24)
#define CMD25 (0x40 + 25)
#define CMD55 (0x40 + 55)
#define CMD58 (0x40 + 58)

typedef enum {
    SD_RESPONSE_SUCCESS              = 0x00,
    SD_RESPONSE_IN_IDLE_STATE        = 0x01,
    SD_RESPONSE_ERASE_RESET          = 0x02,
    SD_RESPONSE_ILLEGAL_COMMAND      = 0x04,
    SD_RESPONSE_COMMAND_CRC_ERROR    = 0x08,
    SD_RESPONSE_ERASE_SEQUENCE_ERROR = 0x10,
    SD_RESPONSE_ADDRESS_ERROR        = 0x20,
    SD_RESPONSE_PARAMETER_ERROR      = 0x40
} SD_Response_Error_Type;

#define SD_IS_ERROR_RESPONSE(resp)                                             \
    (!(resp == SD_RESPONSE_IN_IDLE_STATE || resp == SD_RESPONSE_IN_IDLE_STATE))

typedef uint8_t  SD_Response;
typedef uint32_t SD_Information;

typedef enum {
    SD_TYPE_V2_BLOCK_ADDRESS,
    SD_TYPE_V2_BYTE_ADDRESS,
    SD_TYPE_V1,
    SD_TYPE_MMC_V3,
    SD_TYPE_UNKNOWN
} SD_Version_Type;

typedef enum {
    SD_OK = 0,
    SD_ERR_NO_INIT,
    SD_ERR_TIMEOUT,
    SD_ERR_OCR_FAILED
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