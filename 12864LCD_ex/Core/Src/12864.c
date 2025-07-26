#include "12864.h"

void LCD_Init()
{
    HAL_GPIO_WritePin(PSB_GPIO_Port, PSB_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(XRST_GPIO_Port, XRST_Pin, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(XRST_GPIO_Port, XRST_Pin, GPIO_PIN_SET);
    HAL_Delay(1);   
    LCD_SPI_Send(LCD_CMD_FUNCTION_SET, RS_INSTRUCTION, RW_WRITE, 0);
    HAL_Delay(1);   
    LCD_SPI_Send(LCD_CMD_FUNCTION_SET, RS_INSTRUCTION, RW_WRITE, 0);
    HAL_Delay(1);   
    LCD_SPI_Send(LCD_CMD_DISPLAY_CTRL, RS_INSTRUCTION, RW_WRITE, LCD_DISPLAY_ON | LCD_DISPLAY_CURSOR_ON | LCD_DISPLAY_BLANK_ON);
    HAL_Delay(1);
    LCD_SPI_Send(LCD_CMD_CURSOR_DISPLAY_SHIFT_CTRL, RS_INSTRUCTION, RW_WRITE, LCD_CURSOR_MOVE_RIGHT);
    HAL_Delay(1);
    LCD_SPI_Send(LCD_CMD_DISPLAY_CLEAR, RS_INSTRUCTION, RW_WRITE, 0);
    HAL_Delay(50);
    LCD_SPI_Send(LCD_CMD_ENTRY_MODE_SET, RS_INSTRUCTION, RW_WRITE, LCD_ENTRY_MODE_CURSOR_INCREASE | LCD_ENTRY_MODE_DISPLAY_SHIFT_RIGHT);
    HAL_Delay(1);
}

void LCD_SPI_Send(LCD_CommandTypedef cmd, LCD_RSTypedef rs, LCD_RWTypedef rw, uint8_t data)
{
    uint8_t sync_rw_rs = 0xF8 | (rw << 2) | (rs << 1);
    data |= cmd;
    uint8_t high = data & 0xF0, low = (data & 0x0F) << 4; 
    uint8_t msg[3] = {
        sync_rw_rs,
        high,
        low
    };
    HAL_GPIO_WritePin(RS_GPIO_Port, RS_Pin, GPIO_PIN_SET);

    HAL_SPI_Transmit(&hspi1, msg, 3, 0xFF);

    HAL_GPIO_WritePin(RS_GPIO_Port, RS_Pin, GPIO_PIN_RESET);
}
