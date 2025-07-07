#include "fatfs_sd.h"

#define SD_SPI_TIMEOUT_MS 1000

static void SD_Select();
static void SD_Deselect();
static void SD_PowerOn();

static SD_Response    SD_Send_Command(SD_Command_Type cmd, uint32_t arg);
static SD_Information SD_SPI_ReceiveInformation();
static void           SD_SPI_Send(BYTE data);

static SD_Version_Type   sd_version;
extern volatile uint32_t Timer1, Timer2;

/**
 * SPI를 사용한 초기화 과정
 * 
 * https://www.dejazzer.com/ee379/lecture_notes/lec12_sd_card.pdf
 * https://elm-chan.org/docs/mmc/mmc_e.html#spiinit
 * https://onlinedocs.microchip.com/oxy/GUID-F9FE1ABC-D4DD-4988-87CE-2AFD74DEA334-en-US-3/GUID-48879CB2-9C60-4279-8B98-E17C499B12AF.html
 */
sd_status_t SD_Initialize(BYTE pdrv) {
    SD_Response    res;
    SD_Information info;

    hspi1.Init.BaudRatePrescaler =
        SPI_BAUDRATEPRESCALER_256; /* 예: 24 MHz /256 ≈ 94 kHz */
    HAL_SPI_Init(&hspi1);

    SD_PowerOn();

    sd_version = SD_TYPE_UNKNOWN;

    res = SD_Send_Command(CMD0, 0);
    if (SD_IS_ERROR_RESPONSE(res)) {
        return SD_ERR_NO_INIT;
    }

    // CMD8 실행
    res = SD_Send_Command(CMD8 | 0x40, 0x1AA);

    if (res == 1) {
        // Check Voltage
        info = SD_SPI_ReceiveInformation();
        if (info & 0x1AA) {
            Timer1 = 1000;
            do {
                // CMD55 for Leading ACMD
                res = SD_Send_Command(CMD55, 0);
                if (SD_IS_ERROR_RESPONSE(res)) {
                    return SD_ERR_NO_INIT;
                }
                // APP Init
                res = SD_Send_Command(ACMD41, 1 << 30);
            } while (Timer1 > 0 && SD_IS_ERROR_RESPONSE(res));
            if (Timer1 <= 0) {
                return SD_ERR_TIMEOUT;
            }

            // Read OCR
            res = SD_Send_Command(CMD58, 0);
            if (SD_IS_ERROR_RESPONSE(res)) {
                info = SD_SPI_ReceiveInformation();
                // Check High capacity
                if (info & 0x40) {
                    sd_version = SD_TYPE_V2_BLOCK_ADDRESS;
                } else {
                    sd_version = SD_TYPE_V2_BYTE_ADDRESS;
                }
            } else {
                sd_version = SD_TYPE_V2_BYTE_ADDRESS;
            }
        } else {
            sd_version = SD_TYPE_UNKNOWN;
        }
    } else {
        // todo: 가지고 있는 SD카드가 하나라 이 분기 하위 코드들을
        //  테스트 해볼 수 없었음.
        
        // CMD55 for Leading ACMD
        res = SD_Send_Command(CMD55, 0);
        if (SD_IS_ERROR_RESPONSE(res)) {
            return SD_ERR_NO_INIT;
        }
        res = SD_Send_Command(ACMD41, 0);

        if (res & SD_RESPONSE_ILLEGAL_COMMAND) {
            Timer1 = 1000;
            do {
                res = SD_Send_Command(CMD1, 0);
            } while (Timer1 > 0 && SD_IS_ERROR_RESPONSE(res));
            if (Timer1 <= 0) {
                sd_version = SD_TYPE_UNKNOWN;
            } else if (res == 0) {
                sd_version = SD_TYPE_MMC_V3;
            }
        } else {
            sd_version = SD_TYPE_V1;
        }
    }

    if (sd_version == SD_TYPE_V2_BYTE_ADDRESS || sd_version == SD_TYPE_V1 ||
        sd_version == SD_TYPE_MMC_V3) {
        res = SD_Send_Command(CMD16, 512);
        if (res == 0) {
            // increate SPI Clock Speed...?
            hspi1.Init.BaudRatePrescaler =
                SPI_BAUDRATEPRESCALER_4; /* 예: 24 MHz /256 ≈ 94 kHz */
            HAL_SPI_Init(&hspi1);
        } else {
            sd_version = SD_TYPE_UNKNOWN;
        }
    }

    return SD_OK;
}

static void SD_Select() {
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
}

static void SD_Deselect() {
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

static void SD_PowerOn() {
    /**
     * To communicate with the SD card, your program has to place the SD
     * card into the SPI mode. To do this, set the MOSI and CS lines to logic
     * value 1 and toggle SD CLK for at least 74 cycles. After the 74 cycles (or
     * more) have occurred, your program should set the CS line to 0 and send
     * the command CMD0: 01 000000 00000000 00000000 00000000 00000000 1001010 1
     */

    SD_Deselect();
    // 최소 74클럭동안 1을 보내기
    // Q. 10번만 보내도 되는 이유?
    // A. 80번 보내게 되니까 최소 74번 보내는 것과 같음
    for (int i = 0; i < 10; i++) {
        SD_SPI_Send(0xFF);
    }
}

static SD_Response SD_Send_Command(SD_Command_Type cmd, uint32_t arg) {
    uint8_t crc = 0x01;

    SD_Select();

    // start bit(2) + cmd(6) + arg(32) + CRC(7) + stop bit(1)
    // = 48 bits -> send 6 times
    if (cmd == CMD0) {
        crc = CMD0_CRC;
    } else if (cmd == CMD8) {
        crc = CMD8_CRC;
    } else if (cmd == CMD58) {
        crc = CMD58_CRC;
    }
    SD_SPI_Send(cmd);
    SD_SPI_Send((BYTE)(arg >> 24));
    SD_SPI_Send((BYTE)(arg >> 16));
    SD_SPI_Send((BYTE)(arg >> 8));
    SD_SPI_Send((BYTE)(arg));
    SD_SPI_Send(crc);

    /**
     * Once the SD card receives a command it will begin processing it. To
     * respond to a command, the SD card requires the SD CLK signal to
     * toggle for at least 8 cycles. Your program will have to toggle the SD
     * CLK signal and maintain the MOSI line high while waiting for a
     * response. The length of a response message varies depending on the
     * command. Most of the commands get a response mostly in the form of
     * 8-bit messages, with two exceptions where the response consists of 40
     * bits.
     *
     * 일반적인 SPI 수신 절차. 8번 클럭을 토글하기 위해 MOSI를 high로 둔
     * 더미데이터 전송.
     * SD카드의 경우 이전에 보냈던 명령에 따라 8비트 응답이 오거나 40비트 응답이
     * 온다. 어떤 명령이든지 무조건 8비트 응답이 선행되고, 명령에 따라 32비트
     * 추가 정보가 전달되므로 추가 정보는 SD_SPI_ReceiveInformation 함수에서
     * 얻도록 처리한다.
     *
     * Note that the response to each command is sent by the card a few SD CLK
     * cycles later. If the expected response is not received within 16 clock
     * cycles after sending the reset command, the reset command has to be
     * sent again.
     *
     * 16클럭 내로 응답이 오지 않을 경우 리셋 명령어를 다시 전송해야함.
     * 16클럭이면 2번 전송(8비트*2)이므로, n을 10으로 잡아두었다면 재전송을 최대
     * 5번정도 하게 된다.
     */
    uint8_t     dummy = 0xFF;
    uint32_t    n     = 0xFF;
    SD_Response res;
    do {
        // Q: 왜 do while을 사용하지?
        // A: 명령 전송이므로 한 번은 꼭 실행해야함을 의미
        HAL_SPI_TransmitReceive(&hspi1, &dummy, &res, 1, SD_SPI_TIMEOUT_MS);
    } while ((res & 0x80) && --n);

    SD_Deselect();

    // NCS
    SD_SPI_Send(0xFF);

    return res;
}

static SD_Information SD_SPI_ReceiveInformation() {
    uint8_t        dummy = 0xFF;
    SD_Response    res;
    SD_Information info = 0;

    SD_Select();
    for (int i = 0; i < 4; i++) {
        while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY)
            ;
        HAL_SPI_TransmitReceive(&hspi1, &dummy, &res, 1, SD_SPI_TIMEOUT_MS);
        info = (info << 8) | res;
    }
    SD_Deselect();

    return info;
}

static void SD_SPI_Send(BYTE data) {
    while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY)
        ;
    HAL_SPI_Transmit(&hspi1, &data, 1, SD_SPI_TIMEOUT_MS);
}

void SD_Read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
    SD_Response res = SD_Send_Command(CMD18, 0);
}

void SD_Write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
    //
}
