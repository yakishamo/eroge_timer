#include "clock_renderer.h"

#include <stdlib.h>
#include <wchar.h>

struct ClockRenderer {
    int reserved;
};

static HFONT create_clock_font(const AppSettings *settings)
{
    return CreateFontW(
        app_settings_font_height(settings), 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, settings->font);
}

static BYTE smooth_mask_maximum(const BYTE *mask, int width, int height,
                                int center_x, int center_y, int radius)
{
    BYTE maximum = 0;
    int outer_radius = radius + 1;
    int inner_squared = radius * radius;
    int outer_squared = outer_radius * outer_radius;
    int feather_range = outer_squared - inner_squared;

    for (int y = center_y - outer_radius; y <= center_y + outer_radius; ++y) {
        if (y < 0 || y >= height) continue;
        for (int x = center_x - outer_radius; x <= center_x + outer_radius; ++x) {
            if (x < 0 || x >= width) continue;
            int offset_x = x - center_x;
            int offset_y = y - center_y;
            int distance_squared = offset_x * offset_x + offset_y * offset_y;
            if (distance_squared >= outer_squared) continue;

            unsigned coverage = 255;
            if (distance_squared > inner_squared) {
                coverage = (unsigned)(outer_squared - distance_squared) * 255u /
                           (unsigned)feather_range;
            }
            BYTE value = mask[(size_t)y * (size_t)width + (size_t)x];
            value = (BYTE)((unsigned)value * coverage / 255u);
            if (value > maximum) maximum = value;
        }
    }
    return maximum;
}

static void composite_pixel(DWORD *destination, BYTE red, BYTE green,
                            BYTE blue, BYTE alpha)
{
    BYTE destination_blue = (BYTE)(*destination & 0xff);
    BYTE destination_green = (BYTE)((*destination >> 8) & 0xff);
    BYTE destination_red = (BYTE)((*destination >> 16) & 0xff);
    BYTE destination_alpha = (BYTE)((*destination >> 24) & 0xff);
    unsigned inverse_alpha = 255u - alpha;

    BYTE output_blue = (BYTE)(((unsigned)blue * alpha +
                               (unsigned)destination_blue * inverse_alpha) / 255u);
    BYTE output_green = (BYTE)(((unsigned)green * alpha +
                                (unsigned)destination_green * inverse_alpha) / 255u);
    BYTE output_red = (BYTE)(((unsigned)red * alpha +
                              (unsigned)destination_red * inverse_alpha) / 255u);
    BYTE output_alpha = (BYTE)(alpha +
        (unsigned)destination_alpha * inverse_alpha / 255u);
    *destination = ((DWORD)output_alpha << 24) |
                   ((DWORD)output_red << 16) |
                   ((DWORD)output_green << 8) | output_blue;
}

static void compose_text_effects(DWORD *pixels, const BYTE *mask,
                                 int width, int height,
                                 const AppSettings *settings)
{
    int outline_radius = settings->size == CLOCK_SIZE_SMALL ? 1 :
                         settings->size == CLOCK_SIZE_MEDIUM ? 2 : 3;
    int shadow_offset = outline_radius + 1;

    ZeroMemory(pixels, (size_t)width * (size_t)height * sizeof(*pixels));
    if (settings->shadow) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                BYTE alpha = smooth_mask_maximum(
                    mask, width, height, x - shadow_offset, y - shadow_offset, 2);
                alpha = (BYTE)((unsigned)alpha * 120u / 255u);
                if (alpha != 0) {
                    composite_pixel(&pixels[(size_t)y * width + x],
                                    0, 0, 0, alpha);
                }
            }
        }
    }
    if (settings->outline) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                BYTE alpha = smooth_mask_maximum(
                    mask, width, height, x, y, outline_radius);
                if (alpha != 0) {
                    composite_pixel(&pixels[(size_t)y * width + x],
                                    16, 16, 16, alpha);
                }
            }
        }
    }
    for (size_t i = 0, count = (size_t)width * height; i < count; ++i) {
        if (mask[i] != 0) {
            composite_pixel(&pixels[i], 255, 255, 255, mask[i]);
        }
    }
}

ClockRenderer *clock_renderer_create(void)
{
    return (ClockRenderer *)calloc(1, sizeof(ClockRenderer));
}

void clock_renderer_destroy(ClockRenderer *renderer)
{
    free(renderer);
}

BOOL clock_renderer_measure(ClockRenderer *renderer,
                            const AppSettings *settings, SIZE *size)
{
    (void)renderer;
    const int horizontal_padding = 32;
    const int vertical_padding = 16;
    size->cx = 260;
    size->cy = 72;

    HDC dc = GetDC(NULL);
    HFONT font = create_clock_font(settings);
    if (dc == NULL || font == NULL) {
        if (font != NULL) DeleteObject(font);
        if (dc != NULL) ReleaseDC(NULL, dc);
        return FALSE;
    }

    SYSTEMTIME time;
    wchar_t text[64];
    SIZE text_size;
    GetLocalTime(&time);
    app_settings_format_clock(settings, &time, text, ARRAYSIZE(text));
    HGDIOBJ old_font = SelectObject(dc, font);
    BOOL measured = GetTextExtentPoint32W(
        dc, text, (int)wcslen(text), &text_size);
    if (measured) {
        size->cx = text_size.cx + horizontal_padding;
        size->cy = text_size.cy + vertical_padding;
    }
    SelectObject(dc, old_font);
    DeleteObject(font);
    ReleaseDC(NULL, dc);
    return measured;
}

BOOL clock_renderer_render(ClockRenderer *renderer, HWND hwnd,
                           const AppSettings *settings)
{
    (void)renderer;
    RECT client_rect;
    RECT window_rect;
    GetClientRect(hwnd, &client_rect);
    GetWindowRect(hwnd, &window_rect);
    int width = client_rect.right - client_rect.left;
    int height = client_rect.bottom - client_rect.top;
    if (width <= 0 || height <= 0) return FALSE;

    HDC screen_dc = GetDC(NULL);
    HDC memory_dc = screen_dc != NULL ? CreateCompatibleDC(screen_dc) : NULL;
    BITMAPINFO bitmap_info = {0};
    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = width;
    bitmap_info.bmiHeader.biHeight = -height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    DWORD *pixels = NULL;
    HBITMAP bitmap = screen_dc != NULL ? CreateDIBSection(
        screen_dc, &bitmap_info, DIB_RGB_COLORS, (void **)&pixels, NULL, 0) : NULL;
    HFONT font = create_clock_font(settings);
    if (memory_dc == NULL || bitmap == NULL || pixels == NULL || font == NULL) {
        if (font != NULL) DeleteObject(font);
        if (bitmap != NULL) DeleteObject(bitmap);
        if (memory_dc != NULL) DeleteDC(memory_dc);
        if (screen_dc != NULL) ReleaseDC(NULL, screen_dc);
        return FALSE;
    }

    HGDIOBJ old_bitmap = SelectObject(memory_dc, bitmap);
    HGDIOBJ old_font = SelectObject(memory_dc, font);
    ZeroMemory(pixels, (size_t)width * (size_t)height * sizeof(*pixels));
    SYSTEMTIME time;
    wchar_t text[64];
    GetLocalTime(&time);
    app_settings_format_clock(settings, &time, text, ARRAYSIZE(text));
    SetBkMode(memory_dc, TRANSPARENT);
    SetTextColor(memory_dc, RGB(255, 255, 255));
    DrawTextW(memory_dc, text, -1, &client_rect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    size_t pixel_count = (size_t)width * (size_t)height;
    BYTE *text_mask = (BYTE *)malloc(pixel_count);
    BOOL mask_created = text_mask != NULL;
    if (text_mask != NULL) {
        for (size_t i = 0; i < pixel_count; ++i) {
            BYTE blue = (BYTE)(pixels[i] & 0xff);
            BYTE green = (BYTE)((pixels[i] >> 8) & 0xff);
            BYTE red = (BYTE)((pixels[i] >> 16) & 0xff);
            BYTE alpha = red > green ? red : green;
            if (blue > alpha) alpha = blue;
            text_mask[i] = alpha;
        }
        compose_text_effects(pixels, text_mask, width, height, settings);
        free(text_mask);
    }

    POINT destination = {window_rect.left, window_rect.top};
    POINT source = {0, 0};
    SIZE bitmap_size = {width, height};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    BOOL rendered = mask_created && UpdateLayeredWindow(
        hwnd, screen_dc, &destination, &bitmap_size,
        memory_dc, &source, 0, &blend, ULW_ALPHA);

    SelectObject(memory_dc, old_font);
    SelectObject(memory_dc, old_bitmap);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(memory_dc);
    ReleaseDC(NULL, screen_dc);
    return rendered;
}
