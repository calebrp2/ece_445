/**
 * @file    task_display.c
 * @brief   ILI9341 display rendering task.
 *
 * Screen layout (320 × 240, landscape):
 *   Y  0– 19 : Status bar     (20 px)
 *   Y 20–179 : Waveform area  (160 px)
 *   Y 180–199: Divider        (20 px)
 *   Y 200–239: FFT spectrum   (40 px)
 */

#include "task_display.h"
#include "ili9341.h"
#include "board_config.h"
#include "usbd_cdc_if.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define DISP_LOG(s) CDC_Transmit_FS((uint8_t *)(s), sizeof(s) - 1)

/* =========================================================================
 * Layout constants
 * ========================================================================= */
#define DISP_W      ILI9341_WIDTH
#define DISP_H      ILI9341_HEIGHT

#define STATUS_Y    0
#define STATUS_H    20
#define WAVE_Y      20
#define WAVE_H      160
#define DIVIDER_Y   180
#define DIVIDER_H   20
#define FFT_Y       200
#define FFT_H       40

#define COL_BG          ILI9341_BLACK
#define COL_WAVE_V      ILI9341_GREEN
#define COL_WAVE_I      ILI9341_CYAN
#define COL_FFT_BAR     ILI9341_YELLOW
#define COL_FIT_LINE    ILI9341_MAGENTA
#define COL_STATUS_BG   ILI9341_COLOR(0,0,80)
#define COL_STATUS_FG   ILI9341_WHITE
#define COL_DIVIDER     ILI9341_COLOR(40,40,40)
#define COL_GRID        ILI9341_COLOR(20,20,20)

/* =========================================================================
 * Minimal 5×7 bitmap font (printable ASCII 0x20–0x5F)
 * ========================================================================= */
#define FONT_W    5
#define FONT_H    7
#define FONT_BASE 0x20

static const uint8_t s_font[][FONT_W] = {
    {0x00,0x00,0x00,0x00,0x00}, /* ' ' */
    {0x00,0x00,0x5F,0x00,0x00}, /* '!' */
    {0x00,0x07,0x00,0x07,0x00}, /* '"' */
    {0x14,0x7F,0x14,0x7F,0x14}, /* '#' */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* '$' */
    {0x23,0x13,0x08,0x64,0x62}, /* '%' */
    {0x36,0x49,0x55,0x22,0x50}, /* '&' */
    {0x00,0x05,0x03,0x00,0x00}, /* '\'' */
    {0x00,0x1C,0x22,0x41,0x00}, /* '(' */
    {0x00,0x41,0x22,0x1C,0x00}, /* ')' */
    {0x14,0x08,0x3E,0x08,0x14}, /* '*' */
    {0x08,0x08,0x3E,0x08,0x08}, /* '+' */
    {0x00,0x50,0x30,0x00,0x00}, /* ',' */
    {0x08,0x08,0x08,0x08,0x08}, /* '-' */
    {0x00,0x60,0x60,0x00,0x00}, /* '.' */
    {0x20,0x10,0x08,0x04,0x02}, /* '/' */
    {0x3E,0x51,0x49,0x45,0x3E}, /* '0' */
    {0x00,0x42,0x7F,0x40,0x00}, /* '1' */
    {0x42,0x61,0x51,0x49,0x46}, /* '2' */
    {0x21,0x41,0x45,0x4B,0x31}, /* '3' */
    {0x18,0x14,0x12,0x7F,0x10}, /* '4' */
    {0x27,0x45,0x45,0x45,0x39}, /* '5' */
    {0x3C,0x4A,0x49,0x49,0x30}, /* '6' */
    {0x01,0x71,0x09,0x05,0x03}, /* '7' */
    {0x36,0x49,0x49,0x49,0x36}, /* '8' */
    {0x06,0x49,0x49,0x29,0x1E}, /* '9' */
    {0x00,0x36,0x36,0x00,0x00}, /* ':' */
    {0x00,0x56,0x36,0x00,0x00}, /* ';' */
    {0x08,0x14,0x22,0x41,0x00}, /* '<' */
    {0x14,0x14,0x14,0x14,0x14}, /* '=' */
    {0x00,0x41,0x22,0x14,0x08}, /* '>' */
    {0x02,0x01,0x51,0x09,0x06}, /* '?' */
    {0x32,0x49,0x79,0x41,0x3E}, /* '@' */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 'A' */
    {0x7F,0x49,0x49,0x49,0x36}, /* 'B' */
    {0x3E,0x41,0x41,0x41,0x22}, /* 'C' */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 'D' */
    {0x7F,0x49,0x49,0x49,0x41}, /* 'E' */
    {0x7F,0x09,0x09,0x09,0x01}, /* 'F' */
    {0x3E,0x41,0x49,0x49,0x7A}, /* 'G' */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 'H' */
    {0x00,0x41,0x7F,0x41,0x00}, /* 'I' */
    {0x20,0x40,0x41,0x3F,0x01}, /* 'J' */
    {0x7F,0x08,0x14,0x22,0x41}, /* 'K' */
    {0x7F,0x40,0x40,0x40,0x40}, /* 'L' */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* 'M' */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 'N' */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 'O' */
    {0x7F,0x09,0x09,0x09,0x06}, /* 'P' */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 'Q' */
    {0x7F,0x09,0x19,0x29,0x46}, /* 'R' */
    {0x46,0x49,0x49,0x49,0x31}, /* 'S' */
    {0x01,0x01,0x7F,0x01,0x01}, /* 'T' */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 'U' */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 'V' */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 'W' */
    {0x63,0x14,0x08,0x14,0x63}, /* 'X' */
    {0x07,0x08,0x70,0x08,0x07}, /* 'Y' */
    {0x61,0x51,0x49,0x45,0x43}, /* 'Z' */
    {0x00,0x7F,0x41,0x41,0x00}, /* '[' */
    {0x02,0x04,0x08,0x10,0x20}, /* '\\' */
    {0x00,0x41,0x41,0x7F,0x00}, /* ']' */
    {0x04,0x02,0x01,0x02,0x04}, /* '^' */
    {0x40,0x40,0x40,0x40,0x40}, /* '_' */
};

/* =========================================================================
 * Font helpers
 * ========================================================================= */
static void draw_char(int16_t x, int16_t y, char c, uint16_t fg, uint16_t bg)
{
    if (c < FONT_BASE || c >= (char)(FONT_BASE + sizeof(s_font) / FONT_W))
        c = '?';
    const uint8_t *glyph = s_font[(uint8_t)c - FONT_BASE];
    for (int16_t col = 0; col < FONT_W; col++) {
        uint8_t bits = glyph[col];
        for (int16_t row = 0; row < FONT_H; row++) {
            uint16_t color = (bits & (1U << row)) ? fg : bg;
            ILI9341_DrawPixel(x + col, y + row, color);
        }
    }
}

static void draw_string(int16_t x, int16_t y, const char *str,
                        uint16_t fg, uint16_t bg)
{
    while (*str) {
        draw_char(x, y, *str, fg, bg);
        x += FONT_W + 1;
        str++;
    }
}

/* =========================================================================
 * Rendering helpers
 * ========================================================================= */
static void draw_grid(void)
{
    for (int16_t gy = WAVE_Y; gy <= WAVE_Y + WAVE_H; gy += WAVE_H / 4)
        ILI9341_DrawHLine(0, gy, DISP_W, COL_GRID);
    for (int16_t gx = 0; gx <= DISP_W; gx += DISP_W / 8)
        ILI9341_DrawVLine(gx, WAVE_Y, WAVE_H, COL_GRID);
}

static inline int16_t adc_to_y(uint16_t count)
{
    int16_t y = (int16_t)WAVE_Y + (int16_t)WAVE_H
              - (int16_t)((uint32_t)count * WAVE_H / ADC_MAX_COUNT);
    if (y < WAVE_Y)          y = WAVE_Y;
    if (y > WAVE_Y + WAVE_H) y = WAVE_Y + WAVE_H;
    return y;
}

static void render_waveform(void)
{
    DISP_LOG("DISP: render_waveform\r\n");
    ILI9341_FillRect(0, WAVE_Y, DISP_W, WAVE_H, COL_BG);
    draw_grid();

    osMutexAcquire(xADCBufMutex, osWaitForever);
    int16_t prev_y = adc_to_y(g_voltage_buf[0]);
    for (int16_t px = 1; px < DISP_W; px++) {
        uint32_t idx = (uint32_t)px * g_adc_buf_len / DISP_W;
        if (idx >= g_adc_buf_len) idx = g_adc_buf_len - 1U;
        int16_t cur_y = adc_to_y(g_voltage_buf[idx]);
        ILI9341_DrawLine(px - 1, prev_y, px, cur_y, COL_WAVE_V);
        prev_y = cur_y;
    }
    osMutexRelease(xADCBufMutex);
}

static void render_fft(void)
{
    ILI9341_FillRect(0, FFT_Y, DISP_W, FFT_H, COL_BG);

    osMutexAcquire(xFFTBufMutex, osWaitForever);
    float max_mag = 0.001f;
    for (uint32_t k = 0; k < ADC_BUFFER_SIZE / 2U; k++)
        if (g_fft_mag[k] > max_mag) max_mag = g_fft_mag[k];

    uint32_t bins = ADC_BUFFER_SIZE / 2U;
    for (int16_t px = 0; px < DISP_W; px++) {
        uint32_t k = (uint32_t)px * bins / DISP_W;
        if (k >= bins) k = bins - 1U;
        int16_t bar_h = (int16_t)(g_fft_mag[k] / max_mag * (float)FFT_H);
        if (bar_h > (int16_t)FFT_H) bar_h = (int16_t)FFT_H;
        if (bar_h > 0)
            ILI9341_DrawVLine(px, (int16_t)(FFT_Y + FFT_H) - bar_h, bar_h, COL_FFT_BAR);
    }
    osMutexRelease(xFFTBufMutex);
}

static void render_curvefit(void)
{
    osMutexAcquire(xFFTBufMutex, osWaitForever);
    CurveFitResult_t fit = g_fit_result;
    osMutexRelease(xFFTBufMutex);

    if (!fit.valid) return;

    float omega = 2.0f * (float)M_PI * fit.frequency;
    float dt    = 1.0f / (float)ADC_SAMPLE_RATE_HZ;
    float scale = (float)ADC_MAX_COUNT / ((float)ADC_VREF_MV / 1000.0f);

    int16_t prev_y = 0;
    for (int16_t px = 0; px < DISP_W; px++) {
        float t     = (float)px * (float)g_adc_buf_len / (float)DISP_W * dt;
        float v_hat = fit.amplitude * sinf(omega * t + fit.phase_rad) + fit.dc_offset;
        uint16_t count = (uint16_t)(v_hat * scale);
        if (count > ADC_MAX_COUNT) count = ADC_MAX_COUNT;
        int16_t cur_y = adc_to_y(count);
        if (px > 0)
            ILI9341_DrawLine(px - 1, prev_y, px, cur_y, COL_FIT_LINE);
        prev_y = cur_y;
    }
}

static void render_status(void)
{
    ILI9341_FillRect(0, STATUS_Y, DISP_W, STATUS_H, COL_STATUS_BG);

    const char *mode_str;
    switch (g_meas_mode) {
        case MEAS_MODE_LV:   mode_str = "LV";  break;
        case MEAS_MODE_HV:   mode_str = "HV";  break;
        case MEAS_MODE_CURR: mode_str = "CUR"; break;
        default:             mode_str = "?";   break;
    }

    uint32_t freq_int  = (uint32_t)g_fft_peak_freq_hz;
    uint32_t freq_frac = (uint32_t)((g_fft_peak_freq_hz - (float)freq_int) * 10.0f);

    osMutexAcquire(xFFTBufMutex, osWaitForever);
    float amp_v = g_fit_result.valid ? g_fit_result.amplitude : 0.0f;
    bool  fit_ok = g_fit_result.valid;
    osMutexRelease(xFFTBufMutex);

    uint32_t amp_mv = (uint32_t)(amp_v * 1000.0f);

    char buf[48];
    snprintf(buf, sizeof(buf), "%s F:%luHz A:%lumV %s",
             mode_str, freq_int, amp_mv, fit_ok ? "OK" : "--");
    draw_string(2, STATUS_Y + 6, buf, COL_STATUS_FG, COL_STATUS_BG);

    (void)freq_frac;
}

static void render_divider(void)
{
    ILI9341_FillRect(0, DIVIDER_Y, DISP_W, DIVIDER_H, COL_DIVIDER);
    draw_string(2, DIVIDER_Y + 6, "FFT", COL_STATUS_FG, COL_DIVIDER);
}

/* =========================================================================
 * Task
 * ========================================================================= */
void Task_Display_Init(void)
{
    render_divider();
    ILI9341_FillRect(0, STATUS_Y, DISP_W, STATUS_H, COL_STATUS_BG);
    ILI9341_FillRect(0, WAVE_Y,   DISP_W, WAVE_H,   COL_BG);
    draw_grid();
    ILI9341_FillRect(0, FFT_Y,    DISP_W, FFT_H,    COL_BG);
    render_status();
}

void Task_Display_Run(void *argument)
{
    (void)argument;

    ILI9341_DMAInit();

    osDelay(1500);  /* wait for USB enumeration */
    DISP_LOG("DISP: task started\r\n");

    ILI9341_FillScreen(COL_BG);
    DISP_LOG("DISP: FillScreen done\r\n");

    Task_Display_Init();
    DISP_LOG("DISP: Init done\r\n");

    DisplayCmd_t cmd;

    for (;;) {
        if (osMessageQueueGet(xDisplayCmdQueue, &cmd, NULL, 500) == osOK) {
            switch (cmd.type) {
                case DISP_CMD_WAVEFORM:
                    render_waveform();
                    render_status();
                    break;
                case DISP_CMD_FFT:
                    render_fft();
                    break;
                case DISP_CMD_CURVEFIT:
                    render_curvefit();
                    render_status();
                    break;
                case DISP_CMD_STATUS:
                    render_status();
                    break;
                case DISP_CMD_CLEAR:
                    ILI9341_FillScreen(COL_BG);
                    Task_Display_Init();
                    break;
                default:
                    break;
            }
        } else {
            render_status();
        }
    }
}
