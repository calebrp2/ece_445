/**
 * @file    task_display.c
 * @brief   ILI9341 display rendering task.
 *
 * Screen layout (320 × 240, landscape):
 *   Y   0– 15 : Status bar     (16 px)
 *   Y  16–201 : Waveform area  (186 px)
 *   Y 202–209 : Divider        ( 8 px)
 *   Y 210–239 : FFT spectrum   (30 px)
 */

#include "task_display.h"
#include "ili9341.h"
#include "board_config.h"
#include "usbd_cdc_if.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define DISP_LOG(s) CDC_Transmit_FS((uint8_t *)(s), sizeof(s) - 1)

static void disp_printf(const char *fmt, ...)
{
    char buf[80];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0)
        CDC_Transmit_FS((uint8_t *)buf, (uint16_t)len);
}

/* =========================================================================
 * Layout constants
 * ========================================================================= */
#define DISP_W      ILI9341_WIDTH
#define DISP_H      ILI9341_HEIGHT

#define STATUS_Y    0
#define STATUS_H    16
#define WAVE_Y      16
#define WAVE_H      186
#define DIVIDER_Y   202
#define DIVIDER_H   8
#define FFT_Y       210
#define FFT_H       30

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
 * Voltage scaling — converts raw ADC count to actual terminal voltage (mV).
 * Formula: actual_V = 13.20 * adc_V - 19.54  (x in volts, y in volts)
 * ========================================================================= */
#define VSCALE_SLOPE_PPM   13200L   /* 13.200 expressed as parts-per-1000 */
#define VSCALE_OFFSET_MV   -19540L    /* -19.54 V offset in mV               */

static inline uint32_t count_to_actual_mv(uint16_t count)
{
    uint32_t adc_mv = (uint32_t)count * ADC_VREF_MV / ADC_MAX_COUNT;
    return adc_mv * VSCALE_SLOPE_PPM / 1000L + VSCALE_OFFSET_MV;
}

/* For AC amplitude: offset is DC bias, so only slope applies */
static inline uint32_t adc_mv_to_actual_amp_mv(uint32_t adc_mv)
{
    return adc_mv * VSCALE_SLOPE_PPM / 1000L;
}

/* =========================================================================
 * Auto-scaling state — updated each render_waveform pass
 * ========================================================================= */
static uint16_t s_y_bot_count = 0;
static uint16_t s_y_top_count = ADC_MAX_COUNT;

/* Differential trace state — avoids clearing the full waveform area each frame */
static int16_t  s_trace_y[DISP_W];       /* rendered cur_y per column, last frame  */
static int16_t  s_new_y[DISP_W];         /* cur_y per column, current frame        */
static bool     s_trace_valid = false;   /* false until first waveform is drawn    */
static uint16_t s_last_bot    = 0;
static uint16_t s_last_top    = 0;       /* detect Y-scale changes → force redraw  */

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

static void draw_axis_labels(void)
{
    uint32_t span = s_y_top_count - s_y_bot_count;

    /* Y-axis: voltage at each of the 5 horizontal grid lines (top → bottom) */
    for (int i = 0; i <= 4; i++) {
        int16_t  gy    = (int16_t)(WAVE_Y + i * (WAVE_H / 4));
        /* interpolate count linearly from top to bottom */
        uint32_t count = s_y_top_count - (uint32_t)i * span / 4U;
        uint32_t mv    = count_to_actual_mv((uint16_t)count);
        char buf[12];
        snprintf(buf, sizeof(buf), "%ld.%ldV", mv / 1000, (mv % 1000) / 100);
        /* text below gridline; bottom label goes above its line */
        int16_t ty = (i == 4) ? (int16_t)(gy - FONT_H - 1) : (int16_t)(gy + 1);
        draw_string(2, ty, buf, COL_STATUS_FG, COL_BG);
    }

    /* X-axis: absolute timestamps at x = 80, 160, 240.
     * Right edge (x = DISP_W) = HAL_GetTick(); earlier columns are proportionally older. */
    uint32_t now_ms      = HAL_GetTick();
    uint32_t total_ms    = (uint32_t)ADC_BUFFER_SIZE * 1000U / ADC_SAMPLE_RATE_HZ;
    int16_t  ty          = (int16_t)(WAVE_Y + WAVE_H - FONT_H - 2);
    for (int j = 1; j <= 3; j++) {
        int16_t  gx        = (int16_t)(j * DISP_W / 4);
        uint32_t offset_ms = (uint32_t)(DISP_W - gx) * total_ms / DISP_W;
        uint32_t abs_ms    = now_ms - offset_ms;
        char buf[14];
        snprintf(buf, sizeof(buf), "%lu.%02luS",
                 abs_ms / 1000U, (abs_ms % 1000U) / 10U);
        int16_t tx = gx - (int16_t)(strlen(buf) * (FONT_W + 1) / 2);
        draw_string(tx, ty, buf, COL_STATUS_FG, COL_BG);
    }
}

/* Call while holding xADCBufMutex. Finds min/max in buffer and adds 12% headroom. */
static void compute_yscale(void)
{
    if (g_adc_buf_len == 0) return;

    uint16_t mn = ADC_MAX_COUNT, mx = 0;
    for (uint32_t i = 0; i < g_adc_buf_len; i++) {
        if (g_voltage_buf[i] < mn) mn = g_voltage_buf[i];
        if (g_voltage_buf[i] > mx) mx = g_voltage_buf[i];
    }

    /* Map actual signal min/max directly to screen edges so labels match the signal.
     * For near-DC (flat signal), enforce a minimum span to avoid division by zero. */
    if (mx <= mn + 10U) {
        uint32_t mid = mn;
        mn = (mid > 124U)                    ? (uint16_t)(mid - 124U) : 0U;
        mx = (mid + 124U < ADC_MAX_COUNT)    ? (uint16_t)(mid + 124U) : ADC_MAX_COUNT;
    }

    s_y_bot_count = mn;
    s_y_top_count = mx;

    uint32_t bot_mv = count_to_actual_mv(mn);
    uint32_t top_mv = count_to_actual_mv(mx);
    disp_printf("SCALE: bot=%ld cnt (%ld.%ldV) top=%ld cnt (%ld.%ldV)\r\n",
                (uint32_t)mn, bot_mv / 1000, (bot_mv % 1000) / 100,
                (uint32_t)mx, top_mv / 1000, (top_mv % 1000) / 100);
}

static inline int16_t adc_to_y(uint16_t count)
{
    if (count <= s_y_bot_count) return WAVE_Y + WAVE_H;
    if (count >= s_y_top_count) return WAVE_Y;
    uint32_t span = s_y_top_count - s_y_bot_count;
    return (int16_t)(WAVE_Y + WAVE_H)
         - (int16_t)((uint32_t)(count - s_y_bot_count) * WAVE_H / span);
}

static void render_waveform(void)
{
    DISP_LOG("DISP: render_waveform\r\n");

    /* Read ADC buffer and compute new y positions under the mutex, then release
     * before any SPI writes so the ADC task isn't blocked during rendering. */
    osMutexAcquire(xADCBufMutex, osWaitForever);
    compute_yscale();
    s_new_y[0] = adc_to_y(g_voltage_buf[0]);
    for (int16_t px = 1; px < DISP_W; px++) {
        uint32_t idx = (uint32_t)px * g_adc_buf_len / DISP_W;
        if (idx >= g_adc_buf_len) idx = g_adc_buf_len - 1U;
        s_new_y[px] = adc_to_y(g_voltage_buf[idx]);
    }

    /* Print 8 evenly-spaced raw ADC samples for inspection */
    disp_printf("SMPL:");
    for (int k = 0; k < 8; k++) {
        uint32_t i  = (uint32_t)k * g_adc_buf_len / 8U;
        uint32_t mv = count_to_actual_mv(g_voltage_buf[i]);
        disp_printf(" [%ld]=%ld.%ldV", i, mv / 1000, (mv % 1000) / 100);
    }
    disp_printf("\r\n");
    osMutexRelease(xADCBufMutex);

    bool full_redraw = !s_trace_valid
                    || s_y_bot_count != s_last_bot
                    || s_y_top_count != s_last_top;

    if (full_redraw) {
        ILI9341_FillRect(0, WAVE_Y, DISP_W, WAVE_H, COL_BG);
        draw_grid();
        int16_t prev = s_new_y[0];
        for (int16_t px = 1; px < DISP_W; px++) {
            int16_t cur = s_new_y[px];
            int16_t y0 = (prev <= cur) ? prev : cur;
            int16_t h  = (prev <= cur) ? cur - prev + 1 : prev - cur + 1;
            ILI9341_DrawVLine(px, y0, h, COL_WAVE_V);
            prev = cur;
        }
        draw_axis_labels();
        s_last_bot = s_y_bot_count;
        s_last_top = s_y_top_count;
    } else {
        /* Differential update: erase old VLine, draw new VLine per column.
         * Grid lines are not redrawn — they persist except where overwritten. */
        int16_t old_prev = s_trace_y[0];
        int16_t new_prev = s_new_y[0];
        for (int16_t px = 1; px < DISP_W; px++) {
            int16_t old_cur = s_trace_y[px];
            int16_t new_cur = s_new_y[px];

            int16_t oy0 = (old_prev <= old_cur) ? old_prev : old_cur;
            int16_t oh  = (old_prev <= old_cur) ? old_cur - old_prev + 1 : old_prev - old_cur + 1;
            ILI9341_DrawVLine(px, oy0, oh, COL_BG);

            int16_t ny0 = (new_prev <= new_cur) ? new_prev : new_cur;
            int16_t nh  = (new_prev <= new_cur) ? new_cur - new_prev + 1 : new_prev - new_cur + 1;
            ILI9341_DrawVLine(px, ny0, nh, COL_WAVE_V);

            old_prev = old_cur;
            new_prev = new_cur;
        }
    }

    memcpy(s_trace_y, s_new_y, DISP_W * sizeof(int16_t));
    s_trace_valid = true;
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

    uint32_t bot_mv   = count_to_actual_mv(s_y_bot_count);
    uint32_t top_mv   = count_to_actual_mv(s_y_top_count);
    uint32_t freq_int = (uint32_t)g_fft_peak_freq_hz;

    osMutexAcquire(xFFTBufMutex, osWaitForever);
    float amp_v  = g_fit_result.valid ? g_fit_result.amplitude : 0.0f;
    bool  fit_ok = g_fit_result.valid;
    osMutexRelease(xFFTBufMutex);

    uint32_t amp_mv  = adc_mv_to_actual_amp_mv((uint32_t)(amp_v * 1000.0f));
    uint32_t tick    = HAL_GetTick();
    uint32_t t_sec   = tick / 1000U;
    uint32_t t_tenth = (tick % 1000U) / 100U;

    char buf[64];
    snprintf(buf, sizeof(buf), "%lu.%lu-%lu.%luV %s F:%luHZ A:%luMV %s T:%lu.%luS",
             bot_mv / 1000, (bot_mv % 1000) / 100,
             top_mv / 1000, (top_mv % 1000) / 100,
             mode_str, freq_int, amp_mv, fit_ok ? "OK" : "--",
             t_sec, t_tenth);
    draw_string(2, STATUS_Y + 5, buf, COL_STATUS_FG, COL_STATUS_BG);
}

static void render_divider(void)
{
    ILI9341_FillRect(0, DIVIDER_Y, DISP_W, DIVIDER_H, COL_DIVIDER);
    draw_string(2, DIVIDER_Y + 1, "FFT", COL_STATUS_FG, COL_DIVIDER);
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
    draw_axis_labels();
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
