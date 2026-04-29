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
#include "task_waveform_class.h"
#include "ili9341.h"
#include "board_config.h"
#include "usbd_cdc_if.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define DISP_LOG(s) CDC_Transmit_FS((uint8_t *)(s), sizeof(s) - 1)

static void render_status(void);
static void render_divider(void);

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
    {0x00,0x00,0x00,0x00,0x00}, /* '`' */
    {0x20,0x54,0x54,0x54,0x78}, /* 'a' */
    {0x7F,0x48,0x44,0x44,0x38}, /* 'b' */
    {0x38,0x44,0x44,0x44,0x20}, /* 'c' */
    {0x38,0x44,0x44,0x48,0x7F}, /* 'd' */
    {0x38,0x54,0x54,0x54,0x18}, /* 'e' */
    {0x08,0x7E,0x09,0x01,0x02}, /* 'f' */
    {0x08,0x54,0x54,0x54,0x3C}, /* 'g' */
    {0x7F,0x08,0x04,0x04,0x78}, /* 'h' */
    {0x00,0x44,0x7D,0x40,0x00}, /* 'i' */
    {0x20,0x40,0x44,0x3D,0x00}, /* 'j' */
    {0x7F,0x10,0x28,0x44,0x00}, /* 'k' */
    {0x00,0x41,0x7F,0x40,0x00}, /* 'l' */
    {0x7C,0x04,0x18,0x04,0x78}, /* 'm' */
    {0x7C,0x08,0x04,0x04,0x78}, /* 'n' */
    {0x38,0x44,0x44,0x44,0x38}, /* 'o' */
    {0x7C,0x14,0x14,0x14,0x08}, /* 'p' */
    {0x08,0x14,0x14,0x18,0x7C}, /* 'q' */
    {0x7C,0x08,0x04,0x04,0x08}, /* 'r' */
    {0x48,0x54,0x54,0x54,0x20}, /* 's' */
    {0x04,0x3F,0x44,0x40,0x20}, /* 't' */
    {0x3C,0x40,0x40,0x20,0x7C}, /* 'u' */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 'v' */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 'w' */
    {0x44,0x28,0x10,0x28,0x44}, /* 'x' */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 'y' */
    {0x44,0x64,0x54,0x4C,0x44}, /* 'z' */
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

/* For AC amplitude: offset is DC bias, so only slope applies */
static inline int32_t adc_mv_to_actual_amp_mv(int32_t adc_mv)
{
    return adc_mv * (int32_t)VSCALE_SLOPE_PPM / 1000L;
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
static int32_t  s_last_pan_x  = -1;     /* detect X-pan changes → force redraw    */
static uint8_t  s_last_sense  = 0;       /* detect mode switches → force full redraw */

/* Current-mode auto-scale + timestamp state */
#define CURR_HIST_LEN  32U
static uint16_t s_curr_hist[CURR_HIST_LEN];
static uint8_t  s_curr_hist_idx  = 0;
static bool     s_curr_hist_full = false;
static uint16_t s_curr_bot_count = 0;
static uint16_t s_curr_top_count = ADC_MAX_COUNT;
static uint16_t s_curr_last_bot  = 0xFFFFU;
static uint16_t s_curr_last_top  = 0;
static uint32_t s_curr_ticks[3]  = {0, 0, 0};

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
        int32_t  mv    = count_to_actual_mv((uint16_t)count);
        char buf[12];
        fmt_mv(buf, sizeof(buf), mv);
        /* text below gridline; bottom label goes above its line */
        int16_t ty = (i == 4) ? (int16_t)(gy - FONT_H - 1) : (int16_t)(gy + 1);
        draw_string(2, ty, buf, COL_STATUS_FG, COL_BG);
    }

    /* X-axis: wall-clock time (HAL_GetTick) of each visible sample.
     * The last sample in the buffer (index ADC_BUFFER_SIZE-1) was captured at
     * g_adc_capture_tick ms. Earlier samples are offset back by sample_index * us_per_samp. */
    {
        int32_t  cur_pan     = g_pan_x_samples;
        uint32_t us_per_samp = 1000000UL / ADC_SAMPLE_RATE_HZ;
        uint32_t cap_tick    = g_adc_capture_tick;
        int16_t  ty_x        = (int16_t)(WAVE_Y + WAVE_H - FONT_H - 2);
        for (int j = 1; j <= 3; j++) {
            int16_t  gx        = (int16_t)(j * DISP_W / 4);
            int32_t  samp_idx  = cur_pan + (int32_t)gx;
            uint32_t offset_ms = (uint32_t)((int32_t)(ADC_BUFFER_SIZE - 1) - samp_idx)
                                 * us_per_samp / 1000U;
            uint32_t abs_ms    = cap_tick - offset_ms;
            uint32_t t_sec     = abs_ms / 1000U;
            uint32_t t_tenth   = (abs_ms % 1000U) / 100U;
            char buf[14];
            snprintf(buf, sizeof(buf), "%lu.%luS", t_sec, t_tenth);
            int16_t tx = gx - (int16_t)(strlen(buf) * (FONT_W + 1) / 2);
            draw_string(tx, ty_x, buf, COL_STATUS_FG, COL_BG);
        }
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

    /* Apply zoom: shrink or expand span around signal center */
    int32_t center    = ((int32_t)mn + (int32_t)mx) / 2;
    int32_t half_span = ((int32_t)mx - (int32_t)mn) / 2;
    int8_t  zl        = g_zoom_level;
    if (zl > 0)
        half_span >>= zl;   /* zoom in: narrow */
    else if (zl < 0)
        half_span <<= (-zl); /* zoom out: widen */
    if (half_span < 1) half_span = 1;
    int32_t new_mn = center - half_span + g_pan_y_counts;
    int32_t new_mx = center + half_span + g_pan_y_counts;
    mn = (uint16_t)(new_mn < 0             ? 0             : new_mn > ADC_MAX_COUNT ? ADC_MAX_COUNT : new_mn);
    mx = (uint16_t)(new_mx < 0             ? 0             : new_mx > ADC_MAX_COUNT ? ADC_MAX_COUNT : new_mx);

    s_y_bot_count = mn;
    s_y_top_count = mx;

    int32_t bot_mv = count_to_actual_mv(mn);
    int32_t top_mv = count_to_actual_mv(mx);
    disp_printf("SCALE: bot=%ld cnt (%ld.%ldV) top=%ld cnt (%ld.%ldV)\r\n",
                (long)mn, (long)(bot_mv / 1000), (long)((bot_mv < 0 ? -bot_mv : bot_mv) % 1000 / 100),
                (long)mx, (long)(top_mv / 1000), (long)((top_mv < 0 ? -top_mv : top_mv) % 1000 / 100));
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
    /* Discard stale waveform commands that arrived before a mode switch */
    if (g_sensing_mode != 2) return;

    DISP_LOG("DISP: render_waveform\r\n");

    /* If mode just switched from current, force a full redraw */
    if (s_last_sense != 2) {
        s_trace_valid = false;
        s_last_sense  = 2;
    }

    /* Read ADC buffer and compute new y positions under the mutex, then release
     * before any SPI writes so the ADC task isn't blocked during rendering. */
    uint16_t last_count = 0;
    osMutexAcquire(xADCBufMutex, osWaitForever);
    compute_yscale();
    /* Show DISP_W consecutive samples starting at pan_x (one sample per pixel) */
    int32_t pan_x   = g_pan_x_samples;
    int32_t buf_len = (int32_t)g_adc_buf_len;
    if (buf_len > 0) last_count = g_voltage_buf[(uint32_t)(buf_len - 1)];
    for (int16_t px = 0; px < DISP_W; px++) {
        int32_t raw = pan_x + (int32_t)px;
        if (raw < 0)        raw = 0;
        if (raw >= buf_len) raw = buf_len - 1;
        s_new_y[px] = adc_to_y(g_voltage_buf[(uint32_t)raw]);
    }

    /* Print 8 evenly-spaced raw ADC samples for inspection */
    disp_printf("SMPL:");
    for (int k = 0; k < 8; k++) {
        int32_t i  = pan_x + (int32_t)k * DISP_W / 8;
        if (i < 0) i = 0;
        if (i >= buf_len) i = buf_len - 1;
        int32_t mv = count_to_actual_mv(g_voltage_buf[(uint32_t)i]);
        disp_printf(" [%ld]=%ld.%ldV", (long)i, (long)(mv / 1000), (long)((mv < 0 ? -mv : mv) % 1000 / 100));
    }
    disp_printf("\r\n");
    osMutexRelease(xADCBufMutex);

    bool full_redraw = !s_trace_valid
                    || s_y_bot_count != s_last_bot
                    || s_y_top_count != s_last_top
                    || pan_x         != s_last_pan_x;

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
        s_last_bot   = s_y_bot_count;
        s_last_top   = s_y_top_count;
        s_last_pan_x = pan_x;
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

    /* Most recent voltage reading — same position as current graph's mA readout */
    int32_t last_mv = count_to_actual_mv(last_count);
    char    vbuf[14];
    fmt_mv(vbuf, sizeof(vbuf), last_mv);
    ILI9341_FillRect(160, WAVE_Y + 4, 90, FONT_H, COL_BG);
    draw_string(160, WAVE_Y + 4, vbuf, COL_WAVE_V, COL_BG);
}

static void fft_fmt_hz(char *buf, size_t n, uint32_t hz)
{
    if (hz >= 1000U)
        snprintf(buf, n, "%lu.%lukHz", hz / 1000U, (hz % 1000U) / 100U);
    else
        snprintf(buf, n, "%luHz", hz);
}

static void render_fft(void)
{
    /* Reserve the bottom FONT_H+1 rows for frequency labels */
    const int16_t BAR_H   = (int16_t)(FFT_H - FONT_H - 1);   /* 22 px of bars  */
    const int16_t LABEL_Y = (int16_t)(FFT_Y + FFT_H - FONT_H); /* 233           */

    ILI9341_FillRect(0, FFT_Y, DISP_W, FFT_H, COL_BG);

    osMutexAcquire(xFFTBufMutex, osWaitForever);

    const uint32_t bins = ADC_BUFFER_SIZE / 2U;

    /* Peak magnitude for normalisation (skip DC bin 0) */
    float max_mag = 0.001f;
    for (uint32_t k = 1; k < bins; k++)
        if (g_fft_mag[k] > max_mag) max_mag = g_fft_mag[k];

    /* Find the active frequency range: bins >= 5 % of peak */
    float    thresh = max_mag * 0.05f;
    uint32_t k_lo   = 1U, k_hi = bins - 1U;
    for (uint32_t k = 1;        k < bins;  k++) { if (g_fft_mag[k] >= thresh) { k_lo = k; break; } }
    for (uint32_t k = bins - 1U; k > k_lo; k--) { if (g_fft_mag[k] >= thresh) { k_hi = k; break; } }

    /* Add 20 % padding so peaks aren't cut off at the edges */
    uint32_t span = (k_hi > k_lo) ? k_hi - k_lo : 1U;
    uint32_t pad  = span / 5U;
    if (pad < 2U) pad = 2U;
    k_lo = (k_lo > pad)        ? k_lo - pad : 1U;
    k_hi = (k_hi + pad < bins) ? k_hi + pad : bins - 1U;
    span = k_hi - k_lo;
    if (span == 0U) span = 1U;

    /* Draw bars — each pixel maps to one bin in [k_lo, k_hi] */
    for (int16_t px = 0; px < (int16_t)DISP_W; px++) {
        uint32_t k = k_lo + (uint32_t)px * span / (uint32_t)DISP_W;
        if (k >= bins) k = bins - 1U;
        int16_t bar_h = (int16_t)(g_fft_mag[k] / max_mag * (float)BAR_H);
        if (bar_h > BAR_H) bar_h = BAR_H;
        if (bar_h > 0)
            ILI9341_DrawVLine(px, (int16_t)(FFT_Y + BAR_H) - bar_h, bar_h, COL_FFT_BAR);
    }

    /* Frequency axis labels: left edge, centre, right edge */
    float    freq_res = (float)ADC_SAMPLE_RATE_HZ / (float)ADC_BUFFER_SIZE;
    uint32_t f_lo  = (uint32_t)((float)k_lo             * freq_res);
    uint32_t f_mid = (uint32_t)((float)(k_lo + span / 2U) * freq_res);
    uint32_t f_hi  = (uint32_t)((float)k_hi             * freq_res);

    char lo_buf[10], mid_buf[10], hi_buf[10];
    fft_fmt_hz(lo_buf,  sizeof(lo_buf),  f_lo);
    fft_fmt_hz(mid_buf, sizeof(mid_buf), f_mid);
    fft_fmt_hz(hi_buf,  sizeof(hi_buf),  f_hi);

    draw_string(2, LABEL_Y, lo_buf, COL_STATUS_FG, COL_BG);

    int16_t mid_x = (int16_t)(DISP_W / 2) - (int16_t)(strlen(mid_buf) * (FONT_W + 1) / 2);
    if (mid_x < 0) mid_x = 0;
    draw_string(mid_x, LABEL_Y, mid_buf, COL_STATUS_FG, COL_BG);

    int16_t hi_x = (int16_t)DISP_W - (int16_t)(strlen(hi_buf) * (FONT_W + 1)) - 2;
    if (hi_x < 0) hi_x = 0;
    draw_string(hi_x, LABEL_Y, hi_buf, COL_STATUS_FG, COL_BG);

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

static void compute_current_yscale(uint16_t raw)
{
    s_curr_hist[s_curr_hist_idx] = raw;
    s_curr_hist_idx = (uint8_t)((s_curr_hist_idx + 1U) % CURR_HIST_LEN);
    if (!s_curr_hist_full && s_curr_hist_idx == 0) s_curr_hist_full = true;

    uint32_t len = s_curr_hist_full ? CURR_HIST_LEN : s_curr_hist_idx;
    if (len == 0) return;

    uint16_t mn = ADC_MAX_COUNT, mx = 0;
    for (uint32_t i = 0; i < len; i++) {
        if (s_curr_hist[i] < mn) mn = s_curr_hist[i];
        if (s_curr_hist[i] > mx) mx = s_curr_hist[i];
    }

    if (mx <= mn + 10U) {
        uint16_t mid = mn;
        mn = (mid > 124U) ? mid - 124U : 0U;
        mx = ((uint32_t)mid + 124U < ADC_MAX_COUNT) ? mid + 124U : (uint16_t)ADC_MAX_COUNT;
    }

    uint32_t span = mx - mn;
    uint32_t head = span / 8U;
    mn = (mn > head) ? (uint16_t)(mn - head) : 0U;
    mx = ((uint32_t)mx + head < ADC_MAX_COUNT) ? (uint16_t)(mx + head) : (uint16_t)ADC_MAX_COUNT;

    s_curr_bot_count = mn;
    s_curr_top_count = mx;
}

static inline int16_t current_count_to_y(uint16_t count)
{
    if (count <= s_curr_bot_count) return WAVE_Y + WAVE_H;
    if (count >= s_curr_top_count) return WAVE_Y;
    uint32_t span = s_curr_top_count - s_curr_bot_count;
    return (int16_t)(WAVE_Y + WAVE_H)
         - (int16_t)((uint32_t)(count - s_curr_bot_count) * WAVE_H / span);
}

static void render_current(void)
{
    uint16_t raw = g_current_raw;
    float    ma  = count_to_current_ma(raw);

    compute_current_yscale(raw);
    int16_t cy = current_count_to_y(raw);

    bool scale_changed = (s_curr_bot_count != s_curr_last_bot ||
                          s_curr_top_count != s_curr_last_top);

    if (s_last_sense != 1) {
        /* Full redraw on mode switch: clear all areas, reset history */
        ILI9341_FillRect(0, WAVE_Y, DISP_W, WAVE_H, COL_BG);
        ILI9341_FillRect(0, FFT_Y,  DISP_W, FFT_H,  COL_BG);
        render_divider();
        render_status();
        draw_grid();
        s_curr_hist_idx  = 0;
        s_curr_hist_full = false;
        s_curr_last_bot  = 0xFFFFU;
        s_curr_last_top  = 0;
        scale_changed    = true;
        s_last_sense     = 1;
        s_trace_valid    = false;
    }

    /* Redraw y-axis mA labels only when scale changes */
    if (scale_changed) {
        for (int i = 0; i <= 4; i++) {
            int16_t  gy  = (int16_t)(WAVE_Y + i * (WAVE_H / 4));
            uint32_t cnt = s_curr_top_count
                         - (uint32_t)i * (s_curr_top_count - s_curr_bot_count) / 4U;
            float    lma = count_to_current_ma((uint16_t)cnt);
            int      li  = (int)lma;
            int      lf  = (int)((lma - (float)li) * 10.0f);
            char lbuf[12];
            snprintf(lbuf, sizeof(lbuf), "%d.%dmA", li, lf);
            int16_t ty = (i == 4) ? (int16_t)(gy - FONT_H - 1) : (int16_t)(gy + 1);
            ILI9341_FillRect(2, ty, 62, FONT_H, COL_BG);
            draw_string(2, ty, lbuf, COL_STATUS_FG, COL_BG);
        }
        s_curr_last_bot = s_curr_bot_count;
        s_curr_last_top = s_curr_top_count;
        s_trace_valid   = false;  /* force line redraw after y-scale change */
    }

    /* Erase previous line and restore any grid intersections on that row */
    if (s_trace_valid) {
        int16_t old_y = s_trace_y[0];
        ILI9341_DrawHLine(0, old_y, DISP_W, COL_BG);
        for (int16_t gy = WAVE_Y; gy <= WAVE_Y + WAVE_H; gy += WAVE_H / 4)
            if (old_y == gy) { ILI9341_DrawHLine(0, gy, DISP_W, COL_GRID); break; }
        for (int16_t gx = 0; gx <= DISP_W; gx += DISP_W / 8)
            ILI9341_DrawPixel(gx, old_y, COL_GRID);
    }

    ILI9341_DrawHLine(0, cy, DISP_W, COL_WAVE_I);

    /* Current reading text — right of y-axis labels */
    int  ma_i = (int)ma;
    int  ma_f = (int)((ma - (float)ma_i) * 10.0f);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d.%dmA", ma_i, ma_f);
    ILI9341_FillRect(160, WAVE_Y + 4, 80, FONT_H, COL_BG);
    draw_string(160, WAVE_Y + 4, buf, COL_WAVE_I, COL_BG);

    /* 3 timestamps at bottom — store last 3 reading ticks, display at x = W/4, W/2, 3W/4 */
    s_curr_ticks[0] = s_curr_ticks[1];
    s_curr_ticks[1] = s_curr_ticks[2];
    s_curr_ticks[2] = HAL_GetTick();

    int16_t ty_ts = (int16_t)(WAVE_Y + WAVE_H - FONT_H - 2);
    ILI9341_FillRect(0, ty_ts, DISP_W, FONT_H, COL_BG);
    for (int j = 0; j < 3; j++) {
        uint32_t t      = s_curr_ticks[j];
        uint32_t t_sec  = t / 1000U;
        uint32_t t_frac = (t % 1000U) / 100U;
        char     tbuf[12];
        snprintf(tbuf, sizeof(tbuf), "%lu.%luS", t_sec, t_frac);
        int16_t  gx = (int16_t)((j + 1) * DISP_W / 4);
        int16_t  tx = gx - (int16_t)(strlen(tbuf) * (FONT_W + 1) / 2);
        draw_string(tx, ty_ts, tbuf, COL_STATUS_FG, COL_BG);
    }

    s_trace_y[0]  = cy;
    s_trace_valid = true;
}

static void render_status(void)
{
    ILI9341_FillRect(0, STATUS_Y, DISP_W, STATUS_H, COL_STATUS_BG);

    const char *mode_str = (g_sensing_mode == 1) ? "CURR"
                         : (g_sensing_mode == 2) ? "VOLT" : "DAC";

    int32_t  bot_mv   = count_to_actual_mv(s_y_bot_count);
    int32_t  top_mv   = count_to_actual_mv(s_y_top_count);
    uint32_t freq_int = (uint32_t)g_fft_peak_freq_hz;

    osMutexAcquire(xFFTBufMutex, osWaitForever);
    float amp_v  = g_fit_result.valid ? g_fit_result.amplitude : 0.0f;
    bool  fit_ok = g_fit_result.valid;
    osMutexRelease(xFFTBufMutex);

    int32_t  amp_mv  = adc_mv_to_actual_amp_mv((int32_t)(amp_v * 1000.0f));
    uint32_t tick    = HAL_GetTick();
    uint32_t t_sec   = tick / 1000U;
    uint32_t t_tenth = (tick % 1000U) / 100U;

    char bot_str[10], top_str[10];
    fmt_mv(bot_str, sizeof(bot_str), bot_mv);
    fmt_mv(top_str, sizeof(top_str), top_mv);

    static const char * const s_wave_str[] = { "???", "SIN", "SQR", "SAW" };
    const char *wave_str = s_wave_str[
        (g_waveform_type < 4U) ? (uint8_t)g_waveform_type : 0U];

    char buf[80];
    snprintf(buf, sizeof(buf), "%s-%s %s F:%luHZ A:%ldMV %s W:%s T:%lu.%luS",
             bot_str, top_str,
             mode_str, freq_int, (long)amp_mv, fit_ok ? "OK" : "--",
             wave_str, t_sec, t_tenth);
    draw_string(2, STATUS_Y + 5, buf, COL_STATUS_FG, COL_STATUS_BG);
}

static void render_divider(void)
{
    ILI9341_FillRect(0, DIVIDER_Y, DISP_W, DIVIDER_H, COL_DIVIDER);
    draw_string(2, DIVIDER_Y + 1, "FFT", COL_STATUS_FG, COL_DIVIDER);
}

static void render_dac(void)
{
    uint32_t freq = g_dac_freq_hz;
    uint32_t amp  = g_dac_amp_pct;

    if (s_last_sense != 3) {
        ILI9341_FillRect(0, WAVE_Y, DISP_W, WAVE_H, COL_BG);
        ILI9341_FillRect(0, FFT_Y,  DISP_W, FFT_H,  COL_BG);
        render_divider();
        draw_grid();
        s_trace_valid = false;
        s_last_sense  = 3;
    }

    /* Compute simulated sine trace: show 3 complete periods across the screen */
    int16_t cy_center = (int16_t)(WAVE_Y + WAVE_H / 2);
    int16_t half_h    = (int16_t)(WAVE_H / 2 - 2);
    float   amp_scale = (float)amp / 100.0f;
    /* phase_per_px spans 3 full cycles over DISP_W pixels */
    float   phase_per_px = 3.0f * 2.0f * (float)M_PI / (float)DISP_W;

    for (int16_t px = 0; px < (int16_t)DISP_W; px++) {
        float   v = amp_scale * sinf((float)px * phase_per_px);
        s_new_y[px] = cy_center - (int16_t)(v * (float)half_h);
    }

    /* Differential update — erase old trace, draw new */
    if (s_trace_valid) {
        int16_t old_prev = s_trace_y[0];
        int16_t new_prev = s_new_y[0];
        for (int16_t px = 1; px < (int16_t)DISP_W; px++) {
            int16_t oc = s_trace_y[px], nc = s_new_y[px];
            int16_t oy0 = (old_prev < oc) ? old_prev : oc;
            int16_t oh  = (old_prev < oc) ? oc - old_prev + 1 : old_prev - oc + 1;
            ILI9341_DrawVLine(px, oy0, oh, COL_BG);
            int16_t ny0 = (new_prev < nc) ? new_prev : nc;
            int16_t nh  = (new_prev < nc) ? nc - new_prev + 1 : new_prev - nc + 1;
            ILI9341_DrawVLine(px, ny0, nh, ILI9341_MAGENTA);
            old_prev = oc;
            new_prev = nc;
        }
    } else {
        ILI9341_FillRect(0, WAVE_Y, DISP_W, WAVE_H, COL_BG);
        draw_grid();
        int16_t prev = s_new_y[0];
        for (int16_t px = 1; px < (int16_t)DISP_W; px++) {
            int16_t cur = s_new_y[px];
            int16_t y0  = (prev < cur) ? prev : cur;
            int16_t h   = (prev < cur) ? cur - prev + 1 : prev - cur + 1;
            ILI9341_DrawVLine(px, y0, h, ILI9341_MAGENTA);
            prev = cur;
        }
    }

    memcpy(s_trace_y, s_new_y, DISP_W * sizeof(int16_t));
    s_trace_valid = true;

    /* Freq + amplitude label */
    char buf[24];
    snprintf(buf, sizeof(buf), "%luHz  %lu%%", freq, amp);
    ILI9341_FillRect(160, WAVE_Y + 4, 120, FONT_H, COL_BG);
    draw_string(160, WAVE_Y + 4, buf, ILI9341_MAGENTA, COL_BG);
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
                case DISP_CMD_CURRENT:
                    render_current();
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
                case DISP_CMD_DAC:
                    render_dac();
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
