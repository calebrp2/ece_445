/**
 * @file    ili9341.h
 * @brief   ILI9341 320x240 16-bit RGB565 driver over SPI2.
 */

#ifndef INC_ILI9341_H_
#define INC_ILI9341_H_

#include "board_config.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Colour helpers (RGB565)
 * ------------------------------------------------------------------------- */
#define ILI9341_COLOR(r,g,b) \
    ((uint16_t)(((r) & 0xF8u) << 8) | (((g) & 0xFCu) << 3) | ((b) >> 3))

#define ILI9341_BLACK    0x0000u
#define ILI9341_WHITE    0xFFFFu
#define ILI9341_RED      ILI9341_COLOR(255,   0,   0)
#define ILI9341_GREEN    ILI9341_COLOR(  0, 255,   0)
#define ILI9341_BLUE     ILI9341_COLOR(  0,   0, 255)
#define ILI9341_CYAN     ILI9341_COLOR(  0, 255, 255)
#define ILI9341_MAGENTA  ILI9341_COLOR(255,   0, 255)
#define ILI9341_YELLOW   ILI9341_COLOR(255, 255,   0)

/* -------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */
void     ILI9341_Init(void);
void     ILI9341_DMAInit(void);
void     ILI9341_FillScreen(uint16_t color);
void     ILI9341_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void     ILI9341_DrawPixel(int16_t x, int16_t y, uint16_t color);
void     ILI9341_DrawHLine(int16_t x, int16_t y, int16_t len, uint16_t color);
void     ILI9341_DrawVLine(int16_t x, int16_t y, int16_t len, uint16_t color);
void     ILI9341_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

extern SPI_HandleTypeDef hdisp_spi;

#ifdef __cplusplus
}
#endif

#endif /* INC_ILI9341_H_ */
