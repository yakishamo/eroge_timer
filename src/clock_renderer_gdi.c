#include "clock_renderer.h"

#include <stdlib.h>
#include <wchar.h>

struct ClockRenderer {
    int reserved;
};

static HFONT create_clock_font(const AppSettings *settings, float scale)
{
    int font_height = (int)(app_settings_font_height(settings) * scale);
    if (font_height == 0) font_height = -1;
    return CreateFontW(
        font_height, 0, 0, 0, FW_BOLD,
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
    int base_radius = settings->size == CLOCK_SIZE_SMALL ? 1 :
                      settings->size == CLOCK_SIZE_MEDIUM ? 2 : 3;
    int outline_radius = base_radius;
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

static BYTE interpolate_channel(BYTE top_left, BYTE top_right,
                                BYTE bottom_left, BYTE bottom_right,
                                float fraction_x, float fraction_y)
{
    float top = top_left + (top_right - top_left) * fraction_x;
    float bottom = bottom_left + (bottom_right - bottom_left) * fraction_x;
    float value = top + (bottom - top) * fraction_y;
    if (value < 0.0f) value = 0.0f;
    if (value > 255.0f) value = 255.0f;
    return (BYTE)(value + 0.5f);
}

static void resample_bilinear(const DWORD *source, int source_width,
                              int source_height, DWORD *destination,
                              int destination_width, int destination_height)
{
    for (int y = 0; y < destination_height; ++y) {
        float source_y = ((y + 0.5f) * source_height /
                          destination_height) - 0.5f;
        int y0 = (int)source_y;
        float fraction_y = source_y - y0;
        if (y0 < 0) {
            y0 = 0;
            fraction_y = 0.0f;
        }
        int y1 = y0 + 1;
        if (y1 >= source_height) y1 = source_height - 1;

        for (int x = 0; x < destination_width; ++x) {
            float source_x = ((x + 0.5f) * source_width /
                              destination_width) - 0.5f;
            int x0 = (int)source_x;
            float fraction_x = source_x - x0;
            if (x0 < 0) {
                x0 = 0;
                fraction_x = 0.0f;
            }
            int x1 = x0 + 1;
            if (x1 >= source_width) x1 = source_width - 1;

            DWORD top_left = source[(size_t)y0 * source_width + x0];
            DWORD top_right = source[(size_t)y0 * source_width + x1];
            DWORD bottom_left = source[(size_t)y1 * source_width + x0];
            DWORD bottom_right = source[(size_t)y1 * source_width + x1];
            DWORD output = 0;
            for (int shift = 0; shift <= 24; shift += 8) {
                BYTE channel = interpolate_channel(
                    (BYTE)(top_left >> shift), (BYTE)(top_right >> shift),
                    (BYTE)(bottom_left >> shift), (BYTE)(bottom_right >> shift),
                    fraction_x, fraction_y);
                output |= (DWORD)channel << shift;
            }
            destination[(size_t)y * destination_width + x] = output;
        }
    }
}

static float get_render_scale(HWND hwnd, const AppSettings *settings)
{
    int target_width;
    int target_height;
    if (!app_settings_render_size(settings, &target_width, &target_height)) {
        return 1.0f;
    }

    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitor_info = {0};
    monitor_info.cbSize = sizeof(monitor_info);
    if (!GetMonitorInfoW(monitor, &monitor_info)) return 1.0f;
    int monitor_width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
    int monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
    if (monitor_width <= 0 || monitor_height <= 0) return 1.0f;

    float scale_x = (float)target_width / monitor_width;
    float scale_y = (float)target_height / monitor_height;
    float scale = scale_x > scale_y ? scale_x : scale_y;

    const float minimum_font_pixels = 12.0f;
    int font_height = app_settings_font_height(settings);
    if (font_height < 0) font_height = -font_height;
    if (font_height > 0) {
        float minimum_scale = minimum_font_pixels / font_height;
        if (scale < minimum_scale) scale = minimum_scale;
    }
    if (scale > 1.0f) scale = 1.0f;
    return scale < 0.995f ? scale : 1.0f;
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
    HFONT font = create_clock_font(settings, 1.0f);
    if (dc == NULL || font == NULL) {
        if (font != NULL) DeleteObject(font);
        if (dc != NULL) ReleaseDC(NULL, dc);
        return FALSE;
    }

    SYSTEMTIME time;
    wchar_t text[64];
    GetLocalTime(&time);
    app_settings_format_clock(settings, &time, text, ARRAYSIZE(text));
    HGDIOBJ old_font = SelectObject(dc, font);
    RECT text_rect = {0, 0, 0, 0};
    BOOL measured = DrawTextW(
        dc, text, -1, &text_rect, DT_CALCRECT | DT_CENTER | DT_NOPREFIX) != 0;
    if (measured) {
        size->cx = text_rect.right - text_rect.left + horizontal_padding;
        size->cy = text_rect.bottom - text_rect.top + vertical_padding;
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

    float render_scale = get_render_scale(hwnd, settings);
    int render_width = (int)(width * render_scale + 0.5f);
    int render_height = (int)(height * render_scale + 0.5f);
    if (render_width < 1) render_width = 1;
    if (render_height < 1) render_height = 1;

    BOOL rendered = FALSE;
    HDC screen_dc = GetDC(NULL);
    HDC output_dc = screen_dc != NULL ? CreateCompatibleDC(screen_dc) : NULL;
    HDC render_dc = screen_dc != NULL ? CreateCompatibleDC(screen_dc) : NULL;
    HBITMAP output_bitmap = NULL;
    HBITMAP render_bitmap = NULL;
    HFONT font = NULL;
    HGDIOBJ old_output_bitmap = NULL;
    HGDIOBJ old_render_bitmap = NULL;
    HGDIOBJ old_font = NULL;
    BYTE *text_mask = NULL;
    DWORD *output_pixels = NULL;
    DWORD *render_pixels = NULL;
    if (screen_dc == NULL || output_dc == NULL || render_dc == NULL) goto cleanup;

    BITMAPINFO output_info = {0};
    output_info.bmiHeader.biSize = sizeof(output_info.bmiHeader);
    output_info.bmiHeader.biWidth = width;
    output_info.bmiHeader.biHeight = -height;
    output_info.bmiHeader.biPlanes = 1;
    output_info.bmiHeader.biBitCount = 32;
    output_info.bmiHeader.biCompression = BI_RGB;
    output_bitmap = CreateDIBSection(
        screen_dc, &output_info, DIB_RGB_COLORS,
        (void **)&output_pixels, NULL, 0);

    BITMAPINFO render_info = output_info;
    render_info.bmiHeader.biWidth = render_width;
    render_info.bmiHeader.biHeight = -render_height;
    render_bitmap = CreateDIBSection(
        screen_dc, &render_info, DIB_RGB_COLORS,
        (void **)&render_pixels, NULL, 0);
    font = create_clock_font(settings, render_scale);
    if (output_bitmap == NULL || output_pixels == NULL ||
        render_bitmap == NULL || render_pixels == NULL || font == NULL) {
        goto cleanup;
    }

    old_output_bitmap = SelectObject(output_dc, output_bitmap);
    old_render_bitmap = SelectObject(render_dc, render_bitmap);
    old_font = SelectObject(render_dc, font);
    ZeroMemory(render_pixels,
               (size_t)render_width * render_height * sizeof(*render_pixels));
    SYSTEMTIME time;
    wchar_t text[64];
    GetLocalTime(&time);
    app_settings_format_clock(settings, &time, text, ARRAYSIZE(text));
    SetBkMode(render_dc, TRANSPARENT);
    SetTextColor(render_dc, RGB(255, 255, 255));
    RECT text_rect = {0, 0, 0, 0};
    UINT alignment = DT_CENTER;
    if (settings->text_alignment == TEXT_ALIGNMENT_LEFT) {
        alignment = DT_LEFT;
    } else if (settings->text_alignment == TEXT_ALIGNMENT_RIGHT) {
        alignment = DT_RIGHT;
    }
    DrawTextW(render_dc, text, -1, &text_rect,
              DT_CALCRECT | alignment | DT_NOPREFIX);
    int text_height = text_rect.bottom - text_rect.top;
    int text_top = (render_height - text_height) / 2;
    int horizontal_inset = (int)(16.0f * render_scale + 0.5f);
    if (horizontal_inset * 2 >= render_width) horizontal_inset = 0;
    RECT render_rect = {
        horizontal_inset, text_top,
        render_width - horizontal_inset, text_top + text_height
    };
    DrawTextW(render_dc, text, -1, &render_rect,
              alignment | DT_NOPREFIX);

    size_t render_pixel_count = (size_t)render_width * render_height;
    text_mask = (BYTE *)malloc(render_pixel_count);
    if (text_mask == NULL) goto cleanup;
    for (size_t i = 0; i < render_pixel_count; ++i) {
        BYTE blue = (BYTE)(render_pixels[i] & 0xff);
        BYTE green = (BYTE)((render_pixels[i] >> 8) & 0xff);
        BYTE red = (BYTE)((render_pixels[i] >> 16) & 0xff);
        BYTE alpha = red > green ? red : green;
        if (blue > alpha) alpha = blue;
        text_mask[i] = alpha;
    }
    for (size_t i = 0; i < render_pixel_count; ++i) {
        BYTE alpha = text_mask[i];
        render_pixels[i] = ((DWORD)alpha << 24) |
                           ((DWORD)alpha << 16) |
                           ((DWORD)alpha << 8) | alpha;
    }
    if (render_width == width && render_height == height) {
        CopyMemory(output_pixels, render_pixels,
                   (size_t)width * height * sizeof(*output_pixels));
    } else {
        resample_bilinear(render_pixels, render_width, render_height,
                          output_pixels, width, height);
    }

    free(text_mask);
    text_mask = (BYTE *)malloc((size_t)width * height);
    if (text_mask == NULL) goto cleanup;
    for (size_t i = 0, count = (size_t)width * height; i < count; ++i) {
        text_mask[i] = (BYTE)(output_pixels[i] >> 24);
    }
    compose_text_effects(output_pixels, text_mask, width, height, settings);

    POINT destination = {window_rect.left, window_rect.top};
    POINT source = {0, 0};
    SIZE bitmap_size = {width, height};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    rendered = UpdateLayeredWindow(
        hwnd, screen_dc, &destination, &bitmap_size,
        output_dc, &source, 0, &blend, ULW_ALPHA);

cleanup:
    free(text_mask);
    if (old_font != NULL) SelectObject(render_dc, old_font);
    if (old_render_bitmap != NULL) SelectObject(render_dc, old_render_bitmap);
    if (old_output_bitmap != NULL) SelectObject(output_dc, old_output_bitmap);
    if (font != NULL) DeleteObject(font);
    if (render_bitmap != NULL) DeleteObject(render_bitmap);
    if (output_bitmap != NULL) DeleteObject(output_bitmap);
    if (render_dc != NULL) DeleteDC(render_dc);
    if (output_dc != NULL) DeleteDC(output_dc);
    if (screen_dc != NULL) ReleaseDC(NULL, screen_dc);
    return rendered;
}
