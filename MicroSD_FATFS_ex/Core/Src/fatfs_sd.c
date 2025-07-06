#include "fatfs_sd.h"

#define SD_SPI_TIMEOUT_MS 1000

static void SD_Select();
static void SD_Deselect();
static void SD_PowerOn();

static SD_Response    SD_Send_Command(SD_Command_Type cmd, DWORD arg);
static SD_Information SD_SPI_ReceiveInformation();
static void           SD_SPI_Send(BYTE data);

static SD_Version_Type   sd_version;
extern volatile uint32_t Timer1, Timer2;

sd_status_t SD_Initialize(BYTE pdrv) {
    SD_Response    res;
    SD_Information info;

    SD_PowerOn();

    hspi1.Init.BaudRatePrescaler =
        SPI_BAUDRATEPRESCALER_256; /* 예: 24 MHz /256 ≈ 94 kHz */
    HAL_SPI_Init(&hspi1);

    SD_Select();
    HAL_Delay(1);
    // CMD0 실행
    SD_SPI_Send(CMD0);
    SD_SPI_Send(0);
    SD_SPI_Send(0);
    SD_SPI_Send(0);
    SD_SPI_Send(0);
    SD_SPI_Send(CMD0_CRC);

    uint8_t  dummy = 0xFF;
    uint32_t n     = 0x1FFF; // 왜??

    res = 0;
    while (res != 1 && n) {
        HAL_SPI_TransmitReceive(&hspi1, &dummy, &res, 1, SD_SPI_TIMEOUT_MS);
        n--;
    }
    if (res != 1 || n == 0) {
        return SD_ERROR;
    }
    SD_Deselect();

    // res = SD_Send_Command(CMD0, 0);
    // if (SD_IS_ERROR_RESPONSE(res)) {
    //     return SD_ERROR;
    // }

    // CMD8 실행
    res = SD_Send_Command(CMD8, 0x1AA);

    if (res == 1) {
        // Check Voltage
        info = SD_SPI_ReceiveInformation();
        if (info & 0x1AA) {
            // APP Init
            Timer1 = 1000;
            do {
                res = SD_Send_Command(CMD41, 1 << 30);
            } while (Timer1 > 0 && res > 0);
            if (Timer1 <= 0) {
                // todo: SD_ERROR만 반환하고 끝나는게 맞는가?
                return SD_ERROR;
            }

            // Read OCR
            res = SD_Send_Command(CMD58, 0);
            if (SD_IS_ERROR_RESPONSE(res)) {
                // todo: SD_ERROR만 반환하고 끝나는게 맞는가?
                return SD_ERROR;
            }

            if (res == 0) {
                info = SD_SPI_ReceiveInformation();
                // Check High capacity
                if (info & 0x40) {
                    sd_version = SD_V2_BLOCK_ADDRESS;
                } else {
                    sd_version = SD_V2_BYTE_ADDRESS;
                }
            } else {
                sd_version = SD_V2_BYTE_ADDRESS;
            }
        }
    } else {
        do {
            res = SD_Send_Command(CMD41, 0);
        } while (res > 0);

        if (res == 0) {
            sd_version = SD_V1;
        } else {
            Timer1 = 1000;
            do {
                res = SD_Send_Command(CMD1, 0);
            } while (Timer1 > 0 && res > 0);
            if (Timer1 <= 0) {
                sd_version = SD_UNKNOWN;
            } else if (res == 0) {
                sd_version = SD_MMC_V3;
            }
        }
    }

    if (sd_version == SD_V2_BYTE_ADDRESS || sd_version == SD_V1 ||
        sd_version == SD_MMC_V3) {
        res = SD_Send_Command(CMD16, 512);
        if (res == 0) {
            // increate SPI Clock Speed...?
            hspi1.Init.BaudRatePrescaler =
                SPI_BAUDRATEPRESCALER_4; /* 예: 24 MHz /256 ≈ 94 kHz */
            HAL_SPI_Init(&hspi1);
        } else {
            sd_version = SD_UNKNOWN;
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

static SD_Response SD_Send_Command(SD_Command_Type cmd, DWORD arg) {
    uint8_t crc = 0x01;

    SD_Select();

    // start bit(2) + cmd(6) + arg(32) + CRC(7) + stop bit(1)
    // = 48 bits -> send 6 times
    if (cmd == CMD0) {
        crc = CMD0_CRC;
    } else if (cmd == CMD8) {
        crc = CMD8_CRC;
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

    return res;
}

static SD_Information SD_SPI_ReceiveInformation() {
    uint8_t        dummy = 0xFF;
    SD_Response    res;
    SD_Information info = 0;

    for (int i = 0; i < 4; i++) {
        HAL_SPI_TransmitReceive(&hspi1, &dummy, &res, 1, SD_SPI_TIMEOUT_MS);
        info = (info << 8) | res;
    }

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
