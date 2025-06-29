#include "lcd.h"

void LCD_Init() {
    __LCD_CHANGE_TO_4BIT_OPERATION();
    __LCD_SET_CURSOR_MOVE_RIGHT_AND_NO_DISPLAY_SHIFT();
    __LCD_SHOW_DISPLAY_AND_HIDE_CURSOR_AND_BLINK();

    uint8_t initMsg[2][17] = {"Initializing....", "       Completed"};
    LCD_Print(initMsg);
    HAL_Delay(1000);
}

void LCD_Send(uint8_t cmd, uint8_t RS, uint8_t RW, uint8_t Backlight) {
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
    HAL_Delay(1);
}

void LCD_Print(uint8_t str[2][17]) {
    // LCD_Send(LCD_INSTRUCTION_CLEAR_DISPLAY, 0, 0, 1);
    // HAL_Delay(1);
    LCD_Send(LCD_INSTRUCTION_RETURN_HOME, 0, 0, 1);
    for (int i = 0; i < 16; i++) {
        LCD_Send(str[0][i], 1, 0, 1);
    }
    LCD_Send(0xAA, 0, 0, 1);
    for (int i = 0; i < 16; i++) {
        LCD_Send(str[1][i], 1, 0, 1);
    }
}

void LCD_PrintChar(uint8_t ch) { LCD_Send(ch, 1, 0, 1); }

void LCD_SetCursorPos(uint8_t line, uint8_t length) {
    line *= 0x40;
    uint8_t cmd = 0x80 + line + length;
    LCD_Send(cmd, 0, 0, 1);
}
