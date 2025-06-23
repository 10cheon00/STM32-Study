#ifndef __LCD_H
#define __LCD_H
#include "stm32f1xx_hal.h"

#define DEVICE_ADDR 0x27
#define WRITE_ADDR (0x27 << 1) | 1

/* Initialization Macros */

#define LCD_INSTRUCTION_CLEAR_DISPLAY 0x01
#define LCD_INSTRUCTION_RETURN_HOME 0x02
#define LCD_INSTRUCTION_ENTRY_MODE_SET 0x04
#define LCD_INSTRUCTION_DISPLAY_CONTROL 0x08
#define LCD_INSTRUCTION_CURSOR_DISPLAY_SHIFT 0x10
#define LCD_INSTRUCTION_FUNCTION_SET 0x20
#define LCD_INSTRUCTION_SET_DDRAM_ADDRESS 0x80

/* Entry mode set flags */
#define LCD_INSTRUCTION_FLAG_INCREMENT 0x02
#define LCD_INSTRUCTION_FLAG_DECREMENT 0x00
#define LCD_INSTRUCTION_FLAG_SHIFT 0x01
#define LCD_INSTRUCTION_FLAG_NO_SHIFT 0x00
/* Display on/off control flags */
#define LCD_INSTRUCTION_FLAG_DISPLAY_ON 0x04
#define LCD_INSTRUCTION_FLAG_DISPLAY_OFF 0x00
#define LCD_INSTRUCTION_FLAG_CURSOR_ON 0x02
#define LCD_INSTRUCTION_FLAG_CURSOR_OFF 0x00
#define LCD_INSTRUCTION_FLAG_BLINK_ON 0x01
#define LCD_INSTRUCTION_FLAG_BLINK_OFF 0x00
/* Cursor or display shift flags */
#define LCD_INSTRUCTION_FLAG_DISPLAY_SHIFT 0x08
#define LCD_INSTRUCTION_FLAG_CURSOR_SHIFT 0x00
#define LCD_INSTRUCTION_FLAG_SHIFT_TO_RIGHT 0x04
#define LCD_INSTRUCTION_FLAG_SHIFT_TO_LEFT 0x00
/* Function set flags */
#define LCD_INSTRUCTION_FLAG_DATA_LENGTH_8BIT 0x10
#define LCD_INSTRUCTION_FLAG_DATA_LENGTH_4BIT 0x00
#define LCD_INSTRUCTION_FLAG_2LINE 0x80
#define LCD_INSTRUCTION_FLAG_1LINE 0x00
#define LCD_INSTRUCTION_FLAG_5X10_DOTS 0x04
#define LCD_INSTRUCTION_FLAG_5X8_DOTS 0x00
/* Reads busy flag */
#define LCD_INSTRUCTION_FLAG_BUSY 0x80

/**
 * Refer to "4-bit operation, 8-digit ´ 1-line display with internal reset"
 * section, page 39, HD44780U datasheet.
 * Send 0b00000011 thrice, and send 0b00000010 once. That change operation from
 * 8 bit to 4 bit.
 */
#define __LCD_CHANGE_TO_4BIT_OPERATION()                                       \
    LCD_SendMessage(0x03, 0, 0, 1);                                            \
    HAL_Delay(5);                                                              \
    LCD_SendMessage(0x03, 0, 0, 1);                                            \
    HAL_Delay(1);                                                              \
    LCD_SendMessage(0x03, 0, 0, 1);                                            \
    HAL_Delay(1);                                                              \
    LCD_SendMessage(0x02, 0, 0, 1);                                            \
    HAL_Delay(1);

#define __LCD_SET_CURSOR_MOVE_RIGHT_AND_NO_DISPLAY_SHIFT()                     \
    LCD_SendMessage(LCD_INSTRUCTION_ENTRY_MODE_SET |                           \
                        LCD_INSTRUCTION_FLAG_INCREMENT |                       \
                        LCD_INSTRUCTION_FLAG_NO_SHIFT,                         \
                    0, 0, 1);                                                  \
    HAL_Delay(1);

#define __LCD_SHOW_DISPLAY_AND_HIDE_CURSOR_AND_BLINK()                         \
    LCD_SendMessage(                                                           \
        LCD_INSTRUCTION_DISPLAY_CONTROL | LCD_INSTRUCTION_FLAG_CURSOR_ON |     \
            LCD_INSTRUCTION_FLAG_DISPLAY_ON | LCD_INSTRUCTION_FLAG_BLINK_ON,   \
        0, 0, 1);                                                              \
    HAL_Delay(1);

/* Print Macros*/

extern I2C_HandleTypeDef hi2c1;

void LCD_Init();

void LCD_SendMessage(uint8_t cmd, uint8_t RS, uint8_t RW, uint8_t Backlight);

void LCD_WriteChar(uint8_t cmd);

void LCD_Write(uint8_t str[2][17]);

#endif /* __LCD_H */