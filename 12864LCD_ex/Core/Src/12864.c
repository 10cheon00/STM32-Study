#include "12864.h"

void LCD_Init()
{
  // Run basic instruction
  HAL_GPIO_WritePin(XRST_GPIO_Port, XRST_Pin, GPIO_PIN_RESET);
  HAL_Delay(50);
  HAL_GPIO_WritePin(XRST_GPIO_Port, XRST_Pin, GPIO_PIN_SET);
  HAL_Delay(1);

  LCD_SendCmd(LCD_CMD_FUNCTION_SET |
              LCD_FOUR_BIT_INTERFACE);
  HAL_Delay(1);
  LCD_SendCmd(LCD_CMD_FUNCTION_SET |
              LCD_FOUR_BIT_INTERFACE |
              LCD_BASIC_INSTRUCTION);
  HAL_Delay(1);
  LCD_SendCmd(LCD_CMD_DISPLAY_CTRL |
              LCD_DISPLAY_OFF |
              LCD_DISPLAY_CURSOR_OFF |
              LCD_DISPLAY_BLANK_OFF);
  HAL_Delay(1);
  LCD_SendCmd(LCD_CMD_DISPLAY_CLEAR);
  HAL_Delay(50);

  LCD_SendCmd(LCD_CMD_ENTRY_MODE_SET |
              LCD_ENTRY_MODE_CURSOR_INCREASE |
              LCD_ENTRY_MODE_DISPLAY_SHIFT_RIGHT);
  HAL_Delay(1);
  LCD_SendCmd(LCD_CMD_DISPLAY_CTRL |
              LCD_DISPLAY_ON |
              LCD_DISPLAY_CURSOR_OFF |
              LCD_DISPLAY_BLANK_OFF);
  HAL_Delay(1);
  LCD_SendCmd(LCD_CMD_RETURN_HOME);
  HAL_Delay(1);
}

void LCD_SPI_Send(uint8_t *msg)
{
  HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);

  HAL_SPI_Transmit(&hspi1, msg, 3, 0xFF);

  HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
}

void LCD_SendCmd(LCD_CommandTypedef cmd)
{
  uint8_t sync = 0xF8;
  uint8_t high = cmd & 0xF0, low = (cmd << 4) & 0xF0;
  uint8_t msg[3] = {sync, high, low};

  LCD_SPI_Send(msg);
}

void LCD_SendData(uint8_t data)
{
  uint8_t sync = 0xFA;
  uint8_t high = data & 0xF0, low = (data << 4) & 0xF0;
  uint8_t msg[3] = {sync, high, low};

  LCD_SPI_Send(msg);
}

void LCD_SwitchToBasicInstructionMode()
{
  LCD_SendCmd(LCD_CMD_FUNCTION_SET |
              LCD_FOUR_BIT_INTERFACE |
              LCD_BASIC_INSTRUCTION);
  HAL_Delay(1);
}

void LCD_SwitchToExtendedInstructionMode(bool enableGraphicDisplay)
{
  LCD_SendCmd(LCD_CMD_FUNCTION_SET |
              LCD_FOUR_BIT_INTERFACE |
              LCD_EXTENDED_INSTRUCTION);
  HAL_Delay(1);
  if (enableGraphicDisplay)
  {
    LCD_SendCmd(LCD_CMD_FUNCTION_SET |
                LCD_FOUR_BIT_INTERFACE |
                LCD_EXTENDED_INSTRUCTION |
                LCD_GRAPHIC_DISPLAY_ON);
    HAL_Delay(1);
  }
}

void LCD_GDRAM_Clear()
{
  int x, y;
  for (y = 0; y < 32; y++)
  {
    LCD_SendCmd(0x80 | (y & 0x3F));
    LCD_SendCmd(0x80 | (x & 0x0F));
    for (x = 0; x < 16; x++)
    {
      LCD_SendData(0x00);
      LCD_SendData(0x00);
    }
  }
}

void LCD_GDRAM_SetCoord(uint8_t x, uint8_t y)
{
  LCD_SendCmd(0x80 | (y & 0x3F));
  LCD_SendCmd(0x80 | (x & 0x0F));
}

/// @brief
/// 128x64크기의 이미지를 한 번에 LCD 디스플레이에 그립니다.
/// 이전에 그렸던 내용을 지우지 않고 그 위에 덮어씁니다.
/// @param bitmap
void LCD_GDRAM_DrawBitmap(uint8_t **bitmap)
{
  int x, y;
  static int i = 0;
  for (y = 0; y < 32; y++)
  {
    LCD_SendCmd(0x80 | (y & 0x3F));
    LCD_SendCmd(0x80);
    for (x = 0; x < 16; x++)
    {
      LCD_SendData(i);
      LCD_SendData(i);
    }
    i++;
  }
}
