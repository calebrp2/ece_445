/**
 * @file    ili9341.c
 * @brief   ILI9341 320x240 16-bit RGB565 driver over SPI2.
 */

#include "ili9341.h"
#include "cmsis_os.h"

SPI_HandleTypeDef hdisp_spi;

static osSemaphoreId_t s_tx_sem = NULL;
static uint8_t s_pixel_buf[ILI9341_WIDTH * 2]; /* one row, 640 bytes */

/* -------------------------------------------------------------------------
 * ILI9341 command bytes
 * ------------------------------------------------------------------------- */
#define CMD_SWRESET     0x01u
#define CMD_SLPOUT      0x11u
#define CMD_COLMOD      0x3Au
#define CMD_MADCTL      0x36u
#define CMD_CASET       0x2Au
#define CMD_RASET       0x2Bu
#define CMD_RAMWR       0x2Cu
#define CMD_DISPON      0x29u
#define CMD_PWCTR1      0xC0u
#define CMD_PWCTR2      0xC1u
#define CMD_VMCTR1      0xC5u
#define CMD_VMCTR2      0xC7u
#define CMD_FRMCTR1     0xB1u
#define CMD_DFUNCTR     0xB6u
#define CMD_GAMMASET    0x26u
#define CMD_GMCTRP1     0xE0u
#define CMD_GMCTRN1     0xE1u
#define CMD_PWCTRA      0xCBu
#define CMD_PWCTRB      0xCFu
#define CMD_TIMCTRA     0xE8u
#define CMD_TIMCTRB     0xEAu
#define CMD_PWSEQCTR    0xEDu
#define CMD_PUMPRATIO   0xF7u
#define CMD_EN3G        0xF2u

/* MADCTL landscape, BGR */
#define MADCTL_VAL      0x28u

/* -------------------------------------------------------------------------
 * Low-level helpers
 * ------------------------------------------------------------------------- */
static inline void cs_low(void)
{
    HAL_GPIO_WritePin(DISP_CS_GPIO, DISP_CS_PIN, GPIO_PIN_RESET);
}

static inline void cs_high(void)
{
    HAL_GPIO_WritePin(DISP_CS_GPIO, DISP_CS_PIN, GPIO_PIN_SET);
}

static inline void dc_cmd(void)
{
    HAL_GPIO_WritePin(DISP_DC_GPIO, DISP_DC_PIN, GPIO_PIN_RESET);
}

static inline void dc_data(void)
{
    HAL_GPIO_WritePin(DISP_DC_GPIO, DISP_DC_PIN, GPIO_PIN_SET);
}

static void write_cmd(uint8_t cmd)
{
    SPI_1LINE_TX(&hdisp_spi);
    dc_cmd();
    cs_low();
    HAL_SPI_Transmit(&hdisp_spi, &cmd, 1, HAL_MAX_DELAY);
    cs_high();
}

static void write_data(const uint8_t *data, uint16_t len)
{
    SPI_1LINE_TX(&hdisp_spi);
    dc_data();
    cs_low();
    HAL_SPI_Transmit(&hdisp_spi, (uint8_t *)data, len, HAL_MAX_DELAY);
    cs_high();
}

static void write_byte(uint8_t b)
{
    write_data(&b, 1);
}

/* -------------------------------------------------------------------------
 * Address window
 * ------------------------------------------------------------------------- */
static void set_window(int16_t x, int16_t y, int16_t x2, int16_t y2)
{
    uint8_t buf[4];

    write_cmd(CMD_CASET);
    buf[0] = (uint8_t)(x  >> 8); buf[1] = (uint8_t)(x);
    buf[2] = (uint8_t)(x2 >> 8); buf[3] = (uint8_t)(x2);
    write_data(buf, 4);

    write_cmd(CMD_RASET);
    buf[0] = (uint8_t)(y  >> 8); buf[1] = (uint8_t)(y);
    buf[2] = (uint8_t)(y2 >> 8); buf[3] = (uint8_t)(y2);
    write_data(buf, 4);

    write_cmd(CMD_RAMWR);
}

/* -------------------------------------------------------------------------
 * Hardware + peripheral init
 * ------------------------------------------------------------------------- */
void ILI9341_Init(void)
{
    /* GPIO clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    /* SPI2 AF pins: PB13=SCK, PB15=MOSI */
    gpio.Pin       = DISP_SPI_SCK_PIN | DISP_SPI_MOSI_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = DISP_SPI_AF;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* Control outputs on GPIOB: CS=PB12, BL=PB9 */
    gpio.Pin       = DISP_CS_PIN | DISP_BL_PIN;
    gpio.Mode      = GPIO_MODE_OUTPUT_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = 0;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* DC=PC13 */
    gpio.Pin = DISP_DC_PIN;
    HAL_GPIO_Init(GPIOC, &gpio);

    /* RST=PA7 */
    gpio.Pin = DISP_RST_PIN;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* Deselect, backlight on */
    cs_high();
    HAL_GPIO_WritePin(DISP_BL_GPIO, DISP_BL_PIN, GPIO_PIN_SET);

    /* SPI2: MODE0, 8-bit, 8 MHz (APB1/2) */
    hdisp_spi.Instance               = DISP_SPI_INSTANCE;
    hdisp_spi.Init.Mode              = SPI_MODE_MASTER;
    hdisp_spi.Init.Direction         = SPI_DIRECTION_1LINE;
    hdisp_spi.Init.DataSize          = SPI_DATASIZE_8BIT;
    hdisp_spi.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hdisp_spi.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hdisp_spi.Init.NSS               = SPI_NSS_SOFT;
    hdisp_spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hdisp_spi.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hdisp_spi.Init.TIMode            = SPI_TIMODE_DISABLED;
    hdisp_spi.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLED;
    HAL_SPI_Init(&hdisp_spi);

    /* Hardware reset */
    HAL_GPIO_WritePin(DISP_RST_GPIO, DISP_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(DISP_RST_GPIO, DISP_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(120);

    /* Init sequence */
    write_cmd(CMD_SWRESET); HAL_Delay(5);

    write_cmd(CMD_PWCTRA);
    { uint8_t d[] = {0x39,0x2C,0x00,0x34,0x02}; write_data(d, 5); }

    write_cmd(CMD_PWCTRB);
    { uint8_t d[] = {0x00,0xC1,0x30}; write_data(d, 3); }

    write_cmd(CMD_TIMCTRA);
    { uint8_t d[] = {0x85,0x00,0x78}; write_data(d, 3); }

    write_cmd(CMD_TIMCTRB);
    { uint8_t d[] = {0x00,0x00}; write_data(d, 2); }

    write_cmd(CMD_PWSEQCTR);
    { uint8_t d[] = {0x64,0x03,0x12,0x81}; write_data(d, 4); }

    write_cmd(CMD_PUMPRATIO); write_byte(0x20);

    write_cmd(CMD_PWCTR1);   write_byte(0x23);
    write_cmd(CMD_PWCTR2);   write_byte(0x10);

    write_cmd(CMD_VMCTR1);
    { uint8_t d[] = {0x3E,0x28}; write_data(d, 2); }
    write_cmd(CMD_VMCTR2);   write_byte(0x86);

    write_cmd(CMD_MADCTL);   write_byte(MADCTL_VAL);
    write_cmd(CMD_COLMOD);   write_byte(0x55);   /* 16-bit RGB565 */

    write_cmd(CMD_FRMCTR1);
    { uint8_t d[] = {0x00,0x18}; write_data(d, 2); }

    write_cmd(CMD_DFUNCTR);
    { uint8_t d[] = {0x08,0x82,0x27}; write_data(d, 3); }

    write_cmd(CMD_EN3G);     write_byte(0x00);
    write_cmd(CMD_GAMMASET); write_byte(0x01);

    write_cmd(CMD_GMCTRP1);
    { uint8_t d[] = {0x0F,0x31,0x2B,0x0C,0x0E,0x08,
                     0x4E,0xF1,0x37,0x07,0x10,0x03,
                     0x0E,0x09,0x00}; write_data(d, 15); }

    write_cmd(CMD_GMCTRN1);
    { uint8_t d[] = {0x00,0x0E,0x14,0x03,0x11,0x07,
                     0x31,0xC1,0x48,0x08,0x0F,0x0C,
                     0x31,0x36,0x0F}; write_data(d, 15); }

    write_cmd(CMD_SLPOUT); HAL_Delay(120);
    write_cmd(CMD_DISPON);
}

/* -------------------------------------------------------------------------
 * DMA init — call once after osKernelStart()
 * ------------------------------------------------------------------------- */
void ILI9341_DMAInit(void)
{
    s_tx_sem = osSemaphoreNew(1, 0, NULL);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == DISP_SPI_INSTANCE && s_tx_sem != NULL)
        osSemaphoreRelease(s_tx_sem);
}

/* -------------------------------------------------------------------------
 * Drawing primitives
 * ------------------------------------------------------------------------- */
void ILI9341_DrawPixel(int16_t x, int16_t y, uint16_t color)
{
    if ((uint16_t)x >= ILI9341_WIDTH || (uint16_t)y >= ILI9341_HEIGHT) return;
    set_window(x, y, x, y);
    uint8_t buf[2] = { (uint8_t)(color >> 8), (uint8_t)(color) };
    write_data(buf, 2);
}

void ILI9341_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (w <= 0 || h <= 0) return;
    set_window(x, y, (int16_t)(x + w - 1), (int16_t)(y + h - 1));

    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color);

    SPI_1LINE_TX(&hdisp_spi);
    dc_data();
    cs_low();

    if (s_tx_sem != NULL) {
        if (w == 1) {
            /* Column fast path — pack all h pixels and send in one DMA transfer.
             * s_pixel_buf is ILI9341_WIDTH*2 = 640 bytes; max h = 240 = 480 bytes. */
            for (int16_t i = 0; i < h; i++) {
                s_pixel_buf[i * 2]     = hi;
                s_pixel_buf[i * 2 + 1] = lo;
            }
            HAL_SPI_Transmit_DMA(&hdisp_spi, s_pixel_buf, (uint16_t)(h * 2));
            osSemaphoreAcquire(s_tx_sem, osWaitForever);
        } else {
            /* Row fill — send one row at a time */
            for (int16_t i = 0; i < w; i++) {
                s_pixel_buf[i * 2]     = hi;
                s_pixel_buf[i * 2 + 1] = lo;
            }
            for (int16_t row = 0; row < h; row++) {
                HAL_SPI_Transmit_DMA(&hdisp_spi, s_pixel_buf, (uint16_t)(w * 2));
                osSemaphoreAcquire(s_tx_sem, osWaitForever);
            }
        }
    } else {
        /* Blocking fallback before DMAInit is called */
        for (int32_t n = (int32_t)w * h; n > 0; n--) {
            HAL_SPI_Transmit(&hdisp_spi, &hi, 1, HAL_MAX_DELAY);
            HAL_SPI_Transmit(&hdisp_spi, &lo, 1, HAL_MAX_DELAY);
        }
    }

    cs_high();
}

void ILI9341_FillScreen(uint16_t color)
{
    ILI9341_FillRect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}

void ILI9341_DrawHLine(int16_t x, int16_t y, int16_t len, uint16_t color)
{
    ILI9341_FillRect(x, y, len, 1, color);
}

void ILI9341_DrawVLine(int16_t x, int16_t y, int16_t len, uint16_t color)
{
    ILI9341_FillRect(x, y, 1, len, color);
}

void ILI9341_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t dx =   (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = -((y1 > y0) ? (y1 - y0) : (y0 - y1));
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;

    for (;;) {
        ILI9341_DrawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = (int16_t)(2 * err);
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
