#ifndef _12864_H_
#define _12864_H_

#include <stdbool.h>

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
    LCD_CMD_SET_DDRAM_ADDRESS = 0x80,
    LCD_CMD_WRITE = 0x00,
    LCD_CMD_EX_SCROLL_OR_RAM_ADDRESS_SELECT = 0x02,
    LCD_CMD_EX_REVERSE = 0x04,
    LCD_CMD_EX_FUNCTION_SET = 0x20,
    LCD_CMD_EX_SET_GDRAM_ADDRESS = 0x80
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
    LCD_BASIC_INSTRUCTION = 0x0,
    LCD_EXTENDED_INSTRUCTION = 0x4,
    LCD_EIGHT_BIT_INTERFACE = 0x10,
    LCD_FOUR_BIT_INTERFACE = 0x00,
} LCD_FunctionSetTypeDef;

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

typedef enum
{
    LCD_ENABLE_CGRAM_ADDRESS = 0x0,
    LCD_ENABLE_VERTICAL_SCROLL_POSITION = 0x1,
} LCD_ScrollOrRamAddressSelectTypedef;

void LCD_Init();
void LCD_SPI_Send(uint8_t* msg);
void LCD_SendCmd(LCD_CommandTypedef cmd);
void LCD_SendData(uint8_t data);

void LCD_SwitchToBasicInstructionMode();
void LCD_SwitchToExtendedInstructionMode(bool enableGraphicDisplay);

void LCD_GDRAM_Clear();
#endif /* _12864_H_*/