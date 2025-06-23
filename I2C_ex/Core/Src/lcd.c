#include "lcd.h"

void LCD_Init() {
    __LCD_CHANGE_TO_4BIT_OPERATION();
    __LCD_SET_CURSOR_MOVE_RIGHT_AND_NO_DISPLAY_SHIFT();
    __LCD_SHOW_DISPLAY_AND_HIDE_CURSOR_AND_BLINK();
}

void LCD_SendMessage(uint8_t cmd, uint8_t RS, uint8_t RW, uint8_t Backlight) {
    uint8_t high_nibble = cmd & 0xF0;       // 0x00
    uint8_t low_nibble = (cmd << 4) & 0xF0; // 0x10
    uint8_t control = ((Backlight & 1) << 3) | ((RW & 1) << 1) | (RS & 1);
    uint8_t enable = 1 << 2;
    uint8_t hb_en = high_nibble | control | enable;
    uint8_t lb_en = low_nibble | control | enable;
    uint8_t hb_no = hb_en & ~enable;
    uint8_t lb_no = lb_en & ~enable;

    // send upper 4 bit and lower 4bit with falling edge.(enable 1 to 0)
    uint8_t seq[4] = {hb_en, hb_no, lb_en, lb_no};
    HAL_I2C_Master_Transmit(&hi2c1, WRITE_ADDR, seq, 4, 0xFF);
}

void LCD_WriteChar(uint8_t cmd) { LCD_SendMessage(cmd, 1, 0, 1); }

void LCD_Write(uint8_t str[2][17]) {
    LCD_SendMessage(LCD_INSTRUCTION_CLEAR_DISPLAY, 0, 0, 1);
    HAL_Delay(1);
    LCD_SendMessage(LCD_INSTRUCTION_RETURN_HOME, 0, 0, 1);
    HAL_Delay(1);
    for (int i = 0; i < 15; i++) {
        LCD_SendMessage(str[0][i], 1, 0, 1);
        HAL_Delay(1);
    }
    LCD_SendMessage(0xAA, 0, 0, 1);
    HAL_Delay(1);
    for (int i = 0; i < 15; i++) {
        LCD_SendMessage(str[1][i], 1, 0, 1);
        HAL_Delay(1);
    }
}