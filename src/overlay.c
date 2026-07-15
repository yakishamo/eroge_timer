#include "overlay.h"

#include <stdlib.h>

#include "clock_renderer.h"
#include "play_time_dialog.h"
#include "resource.h"
#include "settings_dialog.h"
#include "tray.h"

#define TIMER_ID 1
#define TIMER_INTERVAL_MS 200
#define SCREEN_MARGIN 32
#define OVERLAY_CONTEXT_MENU_MESSAGE (WM_APP + 2)

typedef struct {
    AppSettings *settings;
    PlayTimeTracker *play_time_tracker;
    ClockRenderer *renderer;
} OverlayContext;

static const wchar_t WINDOW_CLASS_NAME[] = L"ErogeTimerOverlay";
static UINT taskbar_created_message;
static HWND overlay_window;
static HHOOK mouse_hook;
static BOOL suppress_right_button_until_up;
static LRESULT CALLBACK mouse_hook_proc(int code, WPARAM w_param,
                                        LPARAM l_param);

void overlay_suspend_mouse_hook(void)
{
    if (mouse_hook != NULL) {
        UnhookWindowsHookEx(mouse_hook);
        mouse_hook = NULL;
    }
    suppress_right_button_until_up = FALSE;
}

void overlay_resume_mouse_hook(void)
{
    if (mouse_hook == NULL && overlay_window != NULL) {
        mouse_hook = SetWindowsHookExW(
            WH_MOUSE_LL, mouse_hook_proc, NULL, 0);
    }
}

static OverlayContext *get_context(HWND hwnd)
{
    return (OverlayContext *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
}

static LRESULT CALLBACK mouse_hook_proc(int code, WPARAM w_param, LPARAM l_param)
{
    if (code == HC_ACTION && overlay_window != NULL &&
        IsWindowVisible(overlay_window) &&
        (w_param == WM_RBUTTONDOWN || w_param == WM_RBUTTONUP)) {
        const MSLLHOOKSTRUCT *mouse = (const MSLLHOOKSTRUCT *)l_param;
        RECT clock_rect;
        GetWindowRect(overlay_window, &clock_rect);
        if (PtInRect(&clock_rect, mouse->pt)) {
            if (suppress_right_button_until_up) {
                if (w_param == WM_RBUTTONUP) suppress_right_button_until_up = FALSE;
                return 1;
            }
            if (tray_is_menu_open()) {
                if (w_param == WM_RBUTTONDOWN) {
                    suppress_right_button_until_up = TRUE;
                    tray_cancel_menu();
                }
                return 1;
            }
            if (w_param == WM_RBUTTONUP) {
                PostMessageW(overlay_window, OVERLAY_CONTEXT_MENU_MESSAGE, 0, 0);
            }
            return 1;
        }
    }
    return CallNextHookEx(mouse_hook, code, w_param, l_param);
}

void overlay_apply_settings(HWND hwnd, const AppSettings *settings)
{
    OverlayContext *context = get_context(hwnd);
    SIZE size;
    clock_renderer_measure(context->renderer, settings, &size);
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitor_info = {0};
    monitor_info.cbSize = sizeof(monitor_info);
    GetMonitorInfoW(monitor, &monitor_info);

    RECT area = monitor_info.rcMonitor;
    int x = area.left + SCREEN_MARGIN;
    int y = area.top + SCREEN_MARGIN;
    if (settings->position == CLOCK_POSITION_TOP_RIGHT ||
        settings->position == CLOCK_POSITION_BOTTOM_RIGHT) {
        x = area.right - size.cx - SCREEN_MARGIN;
    }
    if (settings->position == CLOCK_POSITION_BOTTOM_LEFT ||
        settings->position == CLOCK_POSITION_BOTTOM_RIGHT) {
        y = area.bottom - size.cy - SCREEN_MARGIN;
    }
    SetWindowPos(hwnd, HWND_TOPMOST, x, y, size.cx, size.cy,
                 SWP_NOACTIVATE | (IsWindowVisible(hwnd) ? SWP_SHOWWINDOW : 0));
    clock_renderer_render(context->renderer, hwnd, settings);
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
    OverlayContext *context = get_context(hwnd);

    switch (message) {
    case WM_CREATE:
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
    case OVERLAY_CONTEXT_MENU_MESSAGE:
        tray_show_menu(hwnd, IsWindowVisible(hwnd));
        return 0;
    case WM_COMMAND:
        switch (LOWORD(w_param)) {
        case TRAY_COMMAND_TOGGLE_VISIBILITY:
            toggle_visibility(hwnd);
            return 0;
        case TRAY_COMMAND_SETTINGS:
            settings_dialog_show(hwnd, context->settings);
            return 0;
        case TRAY_COMMAND_PLAY_TIME:
            play_time_dialog_show(hwnd, context->play_time_tracker);
            return 0;
        case TRAY_COMMAND_EXIT:
            DestroyWindow(hwnd);
            return 0;
        default:
            return 0;
        }
    case WM_TIMER:
        if (w_param == TIMER_ID) {
            play_time_tracker_update(context->play_time_tracker);
            clock_renderer_render(context->renderer, hwnd, context->settings);
        }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT paint;
        BeginPaint(hwnd, &paint);
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        if (mouse_hook != NULL) {
            UnhookWindowsHookEx(mouse_hook);
            mouse_hook = NULL;
        }
        overlay_window = NULL;
        tray_remove();
        clock_renderer_destroy(context->renderer);
        free(context);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
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
    window_class.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    window_class.hIconSm = window_class.hIcon;
    window_class.lpszClassName = WINDOW_CLASS_NAME;
    return RegisterClassExW(&window_class) != 0;
}

HWND overlay_create(HINSTANCE instance, AppSettings *settings,
                    PlayTimeTracker *play_time_tracker)
{
    OverlayContext *context = (OverlayContext *)calloc(1, sizeof(*context));
    if (context == NULL) return NULL;
    context->settings = settings;
    context->play_time_tracker = play_time_tracker;
    context->renderer = clock_renderer_create();
    if (context->renderer == NULL) {
        free(context);
        return NULL;
    }

    DWORD extended_style = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT |
                           WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
    HWND hwnd = CreateWindowExW(
        extended_style, WINDOW_CLASS_NAME, L"Eroge Timer", WS_POPUP,
        32, 32, 1, 1, NULL, NULL, instance, context);
    if (hwnd == NULL) {
        clock_renderer_destroy(context->renderer);
        free(context);
        return NULL;
    }

    overlay_window = hwnd;
    mouse_hook = SetWindowsHookExW(WH_MOUSE_LL, mouse_hook_proc, NULL, 0);
    overlay_apply_settings(hwnd, settings);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    return hwnd;
}
