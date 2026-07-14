#include "overlay.h"

#include <wchar.h>

#include "settings_dialog.h"
#include "tray.h"

#define TIMER_ID 1
#define TIMER_INTERVAL_MS 200
#define TRANSPARENT_COLOR RGB(1, 2, 3)
#define SCREEN_MARGIN 32

static const wchar_t WINDOW_CLASS_NAME[] = L"ErogeTimerOverlay";
static UINT taskbar_created_message;

static AppSettings *get_settings(HWND hwnd)
{
    return (AppSettings *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
}

void overlay_apply_settings(HWND hwnd, const AppSettings *settings)
{
    int width = app_settings_clock_width(settings);
    int height = app_settings_clock_height(settings);
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitor_info = {0};
    monitor_info.cbSize = sizeof(monitor_info);
    GetMonitorInfoW(monitor, &monitor_info);

    RECT area = monitor_info.rcMonitor;
    int x = area.left + SCREEN_MARGIN;
    int y = area.top + SCREEN_MARGIN;
    if (settings->position == CLOCK_POSITION_TOP_RIGHT ||
        settings->position == CLOCK_POSITION_BOTTOM_RIGHT) {
        x = area.right - width - SCREEN_MARGIN;
    }
    if (settings->position == CLOCK_POSITION_BOTTOM_LEFT ||
        settings->position == CLOCK_POSITION_BOTTOM_RIGHT) {
        y = area.bottom - height - SCREEN_MARGIN;
    }

    SetWindowPos(hwnd, HWND_TOPMOST, x, y, width, height,
                 SWP_NOACTIVATE | (IsWindowVisible(hwnd) ? SWP_SHOWWINDOW : 0));
    InvalidateRect(hwnd, NULL, TRUE);
}

static void toggle_visibility(HWND hwnd)
{
    if (IsWindowVisible(hwnd)) {
        ShowWindow(hwnd, SW_HIDE);
    } else {
        ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

static void paint_clock(HWND hwnd)
{
    AppSettings *settings = get_settings(hwnd);
    PAINTSTRUCT paint;
    HDC dc = BeginPaint(hwnd, &paint);
    RECT rect;
    SYSTEMTIME time;
    wchar_t text[64];

    GetClientRect(hwnd, &rect);
    SetDCBrushColor(dc, TRANSPARENT_COLOR);
    FillRect(dc, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
    GetLocalTime(&time);
    app_settings_format_clock(settings, &time, text, ARRAYSIZE(text));

    HFONT font = CreateFontW(
        app_settings_font_height(settings), 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, settings->font);
    HGDIOBJ old_font = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    DrawTextW(dc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, old_font);
    DeleteObject(font);
    EndPaint(hwnd, &paint);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message,
                                    WPARAM w_param, LPARAM l_param)
{
    if (taskbar_created_message != 0 && message == taskbar_created_message) {
        tray_add(hwnd);
        return 0;
    }
    if (message == WM_NCCREATE) {
        CREATESTRUCTW *create = (CREATESTRUCTW *)l_param;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
    }

    switch (message) {
    case WM_CREATE:
        SetLayeredWindowAttributes(hwnd, TRANSPARENT_COLOR, 255, LWA_COLORKEY);
        SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL_MS, NULL);
        tray_add(hwnd);
        return 0;
    case TRAY_CALLBACK_MESSAGE:
        if ((UINT)l_param == WM_RBUTTONUP) {
            tray_show_menu(hwnd, IsWindowVisible(hwnd));
        } else if ((UINT)l_param == WM_LBUTTONDBLCLK) {
            toggle_visibility(hwnd);
        }
        return 0;
    case WM_COMMAND:
        switch (LOWORD(w_param)) {
        case TRAY_COMMAND_TOGGLE_VISIBILITY:
            toggle_visibility(hwnd);
            return 0;
        case TRAY_COMMAND_SETTINGS:
            settings_dialog_show(hwnd, get_settings(hwnd));
            return 0;
        case TRAY_COMMAND_EXIT:
            DestroyWindow(hwnd);
            return 0;
        default:
            return 0;
        }
    case WM_TIMER:
        if (w_param == TIMER_ID) {
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    case WM_ERASEBKGND: {
        RECT rect;
        GetClientRect(hwnd, &rect);
        SetDCBrushColor((HDC)w_param, TRANSPARENT_COLOR);
        FillRect((HDC)w_param, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
        return 1;
    }
    case WM_PAINT:
        paint_clock(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        tray_remove();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, message, w_param, l_param);
    }
}

BOOL overlay_register_class(HINSTANCE instance)
{
    taskbar_created_message = RegisterWindowMessageW(L"TaskbarCreated");
    WNDCLASSEXW window_class = {0};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
    window_class.lpszClassName = WINDOW_CLASS_NAME;
    return RegisterClassExW(&window_class) != 0;
}

HWND overlay_create(HINSTANCE instance, AppSettings *settings)
{
    DWORD extended_style = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT |
                           WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
    HWND hwnd = CreateWindowExW(
        extended_style, WINDOW_CLASS_NAME, L"Eroge Timer", WS_POPUP,
        32, 32, app_settings_clock_width(settings),
        app_settings_clock_height(settings),
        NULL, NULL, instance, settings);
    if (hwnd != NULL) {
        overlay_apply_settings(hwnd, settings);
        ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
    return hwnd;
}
