#ifndef _FATFS_SD_H_
#define _FATFS_SD_H_

#include "diskio.h"
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
#define CMD9 (0x40 + 9)
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

typedef uint8_t  SD_Response;
typedef uint32_t SD_Information;

typedef enum {
    SD_TYPE_UNKNOWN,
    SD_TYPE_V1,
    SD_TYPE_V2_BLOCK_ADDRESS,
    SD_TYPE_V2_BYTE_ADDRESS,
    SD_TYPE_MMC_V3
} SD_Version_Type;

DSTATUS SD_Initialize(BYTE pdrv);

DSTATUS SD_Status(BYTE pdrv);

SD_Version_Type SD_GetVersion();

#define SD_GET_CSD_STRUCTURE_VERSION(csd) ((csd & 0xC0000000) >> 7)
#define SD_CSD_VERSION_1 0
#define SD_CSD_VERSION_2 1
#define SD_GET_SECTOR_COUNT_ON_CSD_VERSION_2(csd96_64, csd63_32)               \
    (DWORD)(((csd96_64 & 0x3F) << 16) | ((csd63_32) & 0xFFFF0000) >> 16)
DSTATUS SD_ioctl(BYTE pdrv, BYTE cmd, void *buff);

typedef uint8_t  SD_DataToken;
#define SD_DATA_TOKEN_CMD17_18_24 0xFE
#define SD_DATA_TOKEN_CMD25 0xFC
#define SD_STOP_DATA_TOKEN_CMD25 0xFD
typedef uint32_t SD_DataBlock[32];
typedef uint8_t  SD_DataCRC;
typedef uint8_t SD_DataResponse;
#define SD_IS_DATA_ACCEPTED(data_res) (data_res & 0x05)
#define SD_IS_DATA_REJECTED_WITH_CRC_ERROR(data_res) (data_res == 0x0B)
#define SD_IS_DATA_REJECTED_WITH_WRITE_ERROR(data_res) (data_res == 0x0C)

DSTATUS SD_Read(BYTE  pdrv,   /* Physical drive nmuber to identify the drive */
                BYTE *buff,   /* Data buffer to store read data */
                DWORD sector, /* Sector address in LBA */
                UINT  count);

DSTATUS
SD_Write(BYTE        pdrv,   /* Physical drive nmuber to identify the drive */
         const BYTE *buff,   /* Data to be written */
         DWORD       sector, /* Sector address in LBA */
         UINT        count);

extern SPI_HandleTypeDef hspi1;

#endif