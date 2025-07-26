#ifndef _12864_H_
#define _12864_H_

#include "stm32f4xx.h"

#include "main.h"

extern SPI_HandleTypeDef hspi1;

typedef enum
{
    LCD_CMD_DISPLAY_CLEAR = 0x01,
    LCD_CMD_RETURN_HOME = 0x02,
    LCD_CMD_ENTRY_MODE_SET = 0x04,
    LCD_CMD_DISPLAY_CTRL = 0x08,
    LCD_CMD_CURSOR_DISPLAY_SHIFT_CTRL = 0x10,
    LCD_CMD_FUNCTION_SET = 0x20,
    LCD_CMD_SET_COORD = 0x80,
} LCD_CommandTypedef;

typedef enum
{
    RS_INSTRUCTION = 0,
    RS_DATA,
} LCD_RSTypedef;

typedef enum
{
    RW_WRITE = 0,
    RW_READ,
} LCD_RWTypedef;

typedef enum
{
    LCD_DISPLAY_BLANK_OFF = 0x0,
    LCD_DISPLAY_BLANK_ON = 0x1,
    LCD_DISPLAY_CURSOR_OFF = 0x0,
    LCD_DISPLAY_CURSOR_ON = 0x2,
    LCD_DISPLAY_OFF = 0x0,
    LCD_DISPLAY_ON = 0x4,
} LCD_DisplayCtrlTypeDef;

typedef enum
{
    LCD_CURSOR_MOVE_LEFT = 0x0,
    LCD_CURSOR_MOVE_RIGHT = 0x4,
    LCD_DISPLAY_SHIFT_LEFT = 0x8,
    LCD_DISPLAY_SHIFT_RIGHT = 0xC,
} LCD_CursorDisplayCtrlTypeDef;

typedef enum
{
    LCD_ENTRY_MODE_CURSOR_DECREASE = 0x0,
    LCD_ENTRY_MODE_CURSOR_INCREASE = 0x2,
    LCD_ENTRY_MODE_DISPLAY_SHIFT_RIGHT = 0x0,
    LCD_ENTRY_MODE_DISPLAY_SHIFT_LEFT = 0x1,
} LCD_EntryModeSetTypeDef;

void LCD_Init();
void LCD_SPI_Send(LCD_CommandTypedef cmd, LCD_RSTypedef rs, LCD_RWTypedef rw, uint8_t data);

#endif /* _12864_H_*/