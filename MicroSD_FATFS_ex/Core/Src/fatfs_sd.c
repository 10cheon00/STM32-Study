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

    /**
     * 100khz ~ 400khz로 클럭 낮추기
     * -----------------------------------------------------------------------
     * To ensure the proper operation of the SD card, the SD CLK signal should
     * have a frequency in the range of 100 to 400 kHz.
     */
    hspi1.Init.BaudRatePrescaler =
        SPI_BAUDRATEPRESCALER_256; /* 예: 24 MHz /256 ≈ 94 kHz */
    HAL_SPI_Init(&hspi1);

    SD_PowerOn();

    /**
     * 이 이후로는 SD카드의 버전 탐색 및 초기화 로직 수행
     */
    sd_version = SD_TYPE_UNKNOWN;

    /**
     * SPI 모드로 전환한 후에는 리셋을 수행해야함(GO_IDLE_STATE 명령어 전송).
     * 일반적인 SPI 통신처럼 CS를 Low로 만들고 명령어 전송.
     * 8비트 응답에 에러가 포함되어 있다면 초기화 로직 수행 불가.
     * ------------------------------------------------------------------------
     * After the 74 cycles (or more) have occurred, your program should set the
     * CS line to 0 and send the command CMD0: 01 000000 00000000 00000000
     * 00000000 00000000 1001010 1 This is the reset command, which puts the SD
     * card into the SPI mode if executed when the CS line is low. The SD card
     * will respond to the reset command by sending a basic 8-bit response on
     * the MISO line.
     */
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

SD_Version_Type SD_GetVersion() { return sd_version; }

static void SD_Select() {
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
}

static void SD_Deselect() {
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

static void SD_PowerOn() {
    /**
     * SD 카드를 SPI모드로 전환하기 위해 CS핀을 High로, MOSI라인도 High로
     * 설정하고 74번 전송하기 정확히 74번 보내기 어려우므로 단순하게 80번
     * 전송(=8비트를 10번 쓰기).
     * -------------------------------------------------------------------------
     * To communicate with the SD card, your program has to place the SD card
     * into the SPI mode. To do this, set the MOSI and CS lines to logic value 1
     * and toggle SD CLK for at least 74 cycles. After the 74 cycles (or more)
     * have occurred, your program should set the CS line to 0 and send the
     * command CMD0: 01 000000 00000000 00000000 00000000 00000000 1001010 1
     */

    SD_Deselect();
    for (int i = 0; i < 10; i++) {
        SD_SPI_Send(0xFF);
    }
}

static SD_Response SD_Send_Command(SD_Command_Type cmd, uint32_t arg) {
    uint8_t crc = 0x01;

    SD_Select();
    /**
     * CMD0, CMD8, CMD58의 경우 고정된 CRC값을 포함해야함. 나머지 명령어의 경우
     * 신경쓰지 않는다.
     */
    if (cmd == CMD0) {
        crc = CMD0_CRC;
    } else if (cmd == CMD8) {
        crc = CMD8_CRC;
    } else if (cmd == CMD58) {
        crc = CMD58_CRC;
    }
    /**
     * start bit(2) + cmd(6) + arg(32) + CRC(7) + stop bit(1)
     *  = 48 bits -> send 6 times
     */
    SD_SPI_Send(cmd);
    SD_SPI_Send((BYTE)(arg >> 24));
    SD_SPI_Send((BYTE)(arg >> 16));
    SD_SPI_Send((BYTE)(arg >> 8));
    SD_SPI_Send((BYTE)(arg));
    SD_SPI_Send(crc);

    /**
     * 일반적인 SPI 수신 절차에 따라 8번 클럭을 토글하기 위해 MOSI를 high로 둔
     * 더미데이터 전송.
     * SD카드의 경우 이전에 보냈던 명령에 따라 8비트 응답만 오거나, 32비트 추가
     * 응답이 온다. 추가 정보는 SD_SPI_ReceiveInformation 함수에서 얻도록
     * 처리한다.
     * ---------------------------------------------------------------------
     * Once the SD card receives a command it will begin processing it. To
     * respond to a command, the SD card requires the SD CLK signal to
     * toggle for at least 8 cycles. Your program will have to toggle the SD
     * CLK signal and maintain the MOSI line high while waiting for a
     * response. The length of a response message varies depending on the
     * command. Most of the commands get a response mostly in the form of
     * 8-bit messages, with two exceptions where the response consists of 40
     * bits.
     */
    uint8_t     dummy = 0xFF;
    uint32_t    n     = 10;
    SD_Response res;
    do {
        /**
         * 16클럭 내로 응답이 오지 않을 경우 리셋 명령어를 다시 전송해야함.
         * 16클럭이면 8비트*2이므로 2번만 읽어도 되지만, 안전성을 위해 10번
         * 읽어보도록 설정함.
         * --------------------------------------------------------------------
         * Note that the response to each command is sent by the card a few SD
         * CLK cycles later. If the expected response is not received within 16
         * clock cycles after sending the reset command, the reset command has
         * to be sent again.
         */
        HAL_SPI_TransmitReceive(&hspi1, &dummy, &res, 1, SD_SPI_TIMEOUT_MS);
    } while ((res & 0x80) && --n);

    SD_Deselect();

    /**
     * MMC와 SDC가 각각 응답을 처리하는 타이밍이 달라서, 강제로 8비트 정도
     * 출력을 해야 정상적으로 동작한다.
     * (이 내용은 https://elm-chan.org/docs/mmc/mmc_e.html#spibus 여기서
     * 찾았다.)
     * -----------------------------------------------------------------------
     * Right waveforms show the MISO line drive/release timing of the MMC/SDC
     * (the DO signal is pulled to 1/2 vcc to see the bus state). Therefore to
     * make MMC/SDC release the MISO line, the master device needs to send a
     * byte after the CS signal is deasserted.
     */
    SD_SPI_Send(0xFF);

    return res;
}

static SD_Information SD_SPI_ReceiveInformation() {
    /**
     * CMD8과 CMD55의 경우 58비트 응답이 오므로, R1 응답을 제외한 32비트 응답을
     * 받도록 처리하는 함수
     */
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
