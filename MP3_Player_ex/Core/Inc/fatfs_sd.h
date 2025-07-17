#ifndef _FATFS_SD_H_
#define _FATFS_SD_H_

#include <stdbool.h>

#include "diskio.h"
#include "ff.h"

/**
 * https://elm-chan.org/docs/mmc/mmc_e.html
 */

extern SPI_HandleTypeDef hspi1;

typedef enum {
    SD_CMD0   = (0x40 + 0),
    SD_CMD1   = (0x40 + 1),
    SD_ACMD41 = (0x40 + 41),
    SD_CMD8   = (0x40 + 8),
    SD_CMD9   = (0x40 + 9),
    SD_CMD12  = (0x40 + 12),
    SD_CMD16  = (0x40 + 16),
    SD_CMD17  = (0x40 + 17),
    SD_CMD18  = (0x40 + 18),
    SD_CMD24  = (0x40 + 24),
    SD_CMD25  = (0x40 + 25),
    SD_CMD55  = (0x40 + 55),
    SD_CMD58  = (0x40 + 58),
} SD_Command_Type;

typedef enum {
    SD_CMD0_CRC  = 0x95,
    SD_CMD8_CRC  = 0x87,
    SD_CMD58_CRC = 0x75
} SD_Command_CRC;

typedef enum {
    SD_RESPONSE_OK                   = 0x00,
    SD_RESPONSE_IN_IDLE_STATE        = 0x01,
    SD_RESPONSE_ERASE_RESET          = 0x02,
    SD_RESPONSE_ILLEGAL_COMMAND      = 0x04,
    SD_RESPONSE_COMMAND_CRC_ERROR    = 0x08,
    SD_RESPONSE_ERASE_SEQUENCE_ERROR = 0x10,
    SD_RESPONSE_ADDRESS_ERROR        = 0x20,
    SD_RESPONSE_PARAMETER_ERROR      = 0x40
} SD_Response_Error_Type;

#define SD_SPI_TIMEOUT_MS 500

typedef uint8_t SD_Response;
typedef uint8_t SD_Information[4];

typedef enum {
    SD_TYPE_UNKNOWN,
    SD_TYPE_V1,
    SD_TYPE_V2_BLOCK_ADDRESS,
    SD_TYPE_V2_BYTE_ADDRESS,
    SD_TYPE_MMC_V3
} SD_Version_Type;

DSTATUS SD_Initialize(BYTE pdrv);

DSTATUS SD_Status(BYTE pdrv);

DSTATUS SD_ioctl(BYTE pdrv, BYTE cmd, void *buff);

DSTATUS SD_Read(BYTE  pdrv,   /* Physical drive nmuber to identify the drive */
                BYTE *buff,   /* Data buffer to store read data */
                DWORD sector, /* Sector address in LBA */
                UINT  count);

DSTATUS SD_Write(BYTE pdrv, /* Physical drive nmuber to identify the drive */
                 const BYTE *buff,   /* Data to be written */
                 DWORD       sector, /* Sector address in LBA */
                 UINT        count);

SD_Version_Type SD_GetVersion();

#define SD_GET_CSD_STRUCTURE_VERSION(csd) ((csd & 0xC0000000) >> 7)
#define SD_CSD_VERSION_1 0
#define SD_CSD_VERSION_2 1
#define SD_GET_SECTOR_COUNT_ON_CSD_VERSION_2(csd96_64, csd63_32)               \
    (DWORD)(((csd96_64 & 0x3F) << 16) | ((csd63_32) & 0xFFFF0000) >> 16)

/*                         Defines for Read/Write                             */

typedef uint8_t  SD_DataToken;
typedef uint32_t SD_DataBlock[32];
typedef uint8_t  SD_DataCRC;
#define SD_DATA_TOKEN_CMD17_18_24 0xFE
#define SD_DATA_TOKEN_CMD25 0xFC
#define SD_STOP_DATA_TOKEN_CMD25 0xFD

typedef uint8_t SD_DataResponse;
#define SD_IS_DATA_ACCEPTED(data_res) (data_res & 0x05)
#define SD_IS_DATA_REJECTED_WITH_CRC_ERROR(data_res) (data_res == 0x0B)
#define SD_IS_DATA_REJECTED_WITH_WRITE_ERROR(data_res) (data_res == 0x0C)

#define SD_OK true
#define SD_ERROR false

#endif