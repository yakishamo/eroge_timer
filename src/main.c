#include <windows.h>
#include <shellapi.h>
#include <wchar.h>

#define TIMER_ID 1
#define TIMER_INTERVAL_MS 200
#define TRANSPARENT_COLOR RGB(1, 2, 3)
#define TRAY_ICON_ID 1
#define TRAY_CALLBACK_MESSAGE (WM_APP + 1)
#define MENU_TOGGLE_VISIBILITY 1001
#define MENU_EXIT 1002
#define MENU_SETTINGS 1003
#define CONTROL_SIZE_SMALL 2001
#define CONTROL_SIZE_MEDIUM 2002
#define CONTROL_SIZE_LARGE 2003
#define CONTROL_POSITION_TOP_LEFT 2011
#define CONTROL_POSITION_TOP_RIGHT 2012
#define CONTROL_POSITION_BOTTOM_LEFT 2013
#define CONTROL_POSITION_BOTTOM_RIGHT 2014
#define CONTROL_FONT 2021

static const wchar_t WINDOW_CLASS_NAME[] = L"ErogeTimerOverlay";
static const wchar_t SETTINGS_CLASS_NAME[] = L"ErogeTimerSettings";
static UINT taskbar_created_message;
static NOTIFYICONDATAW tray_icon;
static HWND settings_window;

typedef enum {
    CLOCK_SIZE_SMALL,
    CLOCK_SIZE_MEDIUM,
    CLOCK_SIZE_LARGE
} ClockSize;

typedef enum {
    CLOCK_POSITION_TOP_LEFT,
    CLOCK_POSITION_TOP_RIGHT,
    CLOCK_POSITION_BOTTOM_LEFT,
    CLOCK_POSITION_BOTTOM_RIGHT
} ClockPosition;

static ClockSize clock_size = CLOCK_SIZE_MEDIUM;
static ClockPosition clock_position = CLOCK_POSITION_TOP_LEFT;
static wchar_t clock_font[LF_FACESIZE] = L"Segoe UI";

static void apply_clock_settings(HWND hwnd)
{
    static const int widths[] = {190, 260, 340};
    static const int heights[] = {54, 72, 94};
    const int margin = 32;
    int width = widths[clock_size];
    int height = heights[clock_size];

    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitor_info = {0};
    monitor_info.cbSize = sizeof(monitor_info);
    GetMonitorInfoW(monitor, &monitor_info);

    RECT area = monitor_info.rcMonitor;
    int x = area.left + margin;
    int y = area.top + margin;

    if (clock_position == CLOCK_POSITION_TOP_RIGHT ||
        clock_position == CLOCK_POSITION_BOTTOM_RIGHT) {
        x = area.right - width - margin;
    }
    if (clock_position == CLOCK_POSITION_BOTTOM_LEFT ||
        clock_position == CLOCK_POSITION_BOTTOM_RIGHT) {
        y = area.bottom - height - margin;
    }

    SetWindowPos(hwnd, HWND_TOPMOST, x, y, width, height,
                 SWP_NOACTIVATE | (IsWindowVisible(hwnd) ? SWP_SHOWWINDOW : 0));
    InvalidateRect(hwnd, NULL, TRUE);
}

static void set_control_font(HWND control)
{
    SendMessageW(control, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
}

static HWND create_control(HWND parent, const wchar_t *class_name, const wchar_t *text,
                           DWORD style, int x, int y, int width, int height, int id)
{
    HWND control = CreateWindowExW(
        0, class_name, text, WS_CHILD | WS_VISIBLE | style,
        x, y, width, height, parent, (HMENU)(INT_PTR)id,
        (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE), NULL);
    if (control != NULL) {
        set_control_font(control);
    }
    return control;
}

static int CALLBACK enumerate_font(const LOGFONTW *log_font,
                                   const TEXTMETRICW *text_metric,
                                   DWORD font_type, LPARAM parameter)
{
    (void)text_metric;
    (void)font_type;

    HWND combo = (HWND)parameter;
    const wchar_t *face_name = log_font->lfFaceName;
    if (face_name[0] == L'@') {
        return 1;
    }

    if (SendMessageW(combo, CB_FINDSTRINGEXACT, (WPARAM)-1,
                     (LPARAM)face_name) == CB_ERR) {
        SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)face_name);
    }
    return 1;
}

static void fill_font_list(HWND combo)
{
    HDC dc = GetDC(NULL);
    if (dc == NULL) {
        return;
    }

    LOGFONTW query = {0};
    query.lfCharSet = DEFAULT_CHARSET;
    EnumFontFamiliesExW(dc, &query, enumerate_font, (LPARAM)combo, 0);
    ReleaseDC(NULL, dc);

    LRESULT selected = SendMessageW(combo, CB_FINDSTRINGEXACT, (WPARAM)-1,
                                    (LPARAM)clock_font);
    if (selected == CB_ERR) {
        selected = SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)clock_font);
    }
    SendMessageW(combo, CB_SETCURSEL, (WPARAM)selected, 0);
}

static LRESULT CALLBACK settings_window_proc(HWND hwnd, UINT message,
                                             WPARAM w_param, LPARAM l_param)
{
    (void)l_param;

    switch (message) {
    case WM_CREATE:
        create_control(hwnd, L"BUTTON", L"サイズ", BS_GROUPBOX,
                       16, 12, 282, 82, 0);
        create_control(hwnd, L"BUTTON", L"小", BS_AUTORADIOBUTTON | WS_GROUP,
                       32, 42, 60, 24, CONTROL_SIZE_SMALL);
        create_control(hwnd, L"BUTTON", L"中", BS_AUTORADIOBUTTON,
                       118, 42, 60, 24, CONTROL_SIZE_MEDIUM);
        create_control(hwnd, L"BUTTON", L"大", BS_AUTORADIOBUTTON,
                       204, 42, 60, 24, CONTROL_SIZE_LARGE);

        create_control(hwnd, L"BUTTON", L"表示位置", BS_GROUPBOX,
                       16, 106, 282, 112, 0);
        create_control(hwnd, L"BUTTON", L"左上", BS_AUTORADIOBUTTON | WS_GROUP,
                       32, 134, 100, 24, CONTROL_POSITION_TOP_LEFT);
        create_control(hwnd, L"BUTTON", L"右上", BS_AUTORADIOBUTTON,
                       166, 134, 100, 24, CONTROL_POSITION_TOP_RIGHT);
        create_control(hwnd, L"BUTTON", L"左下", BS_AUTORADIOBUTTON,
                       32, 174, 100, 24, CONTROL_POSITION_BOTTOM_LEFT);
        create_control(hwnd, L"BUTTON", L"右下", BS_AUTORADIOBUTTON,
                       166, 174, 100, 24, CONTROL_POSITION_BOTTOM_RIGHT);

        create_control(hwnd, L"BUTTON", L"フォント", BS_GROUPBOX,
                       16, 230, 282, 70, 0);
        HWND font_combo = create_control(
            hwnd, L"COMBOBOX", L"",
            CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_TABSTOP,
            32, 254, 250, 200, CONTROL_FONT);
        if (font_combo != NULL) {
            fill_font_list(font_combo);
        }

        create_control(hwnd, L"BUTTON", L"OK", BS_DEFPUSHBUTTON,
                       132, 316, 78, 28, IDOK);
        create_control(hwnd, L"BUTTON", L"キャンセル", BS_PUSHBUTTON,
                       220, 316, 78, 28, IDCANCEL);

        CheckRadioButton(hwnd, CONTROL_SIZE_SMALL, CONTROL_SIZE_LARGE,
                         CONTROL_SIZE_SMALL + clock_size);
        CheckRadioButton(hwnd, CONTROL_POSITION_TOP_LEFT, CONTROL_POSITION_BOTTOM_RIGHT,
                         CONTROL_POSITION_TOP_LEFT + clock_position);
        return 0;

    case WM_COMMAND:
        if (LOWORD(w_param) == IDOK) {
            HWND owner = GetWindow(hwnd, GW_OWNER);

            if (IsDlgButtonChecked(hwnd, CONTROL_SIZE_SMALL) == BST_CHECKED) {
                clock_size = CLOCK_SIZE_SMALL;
            } else if (IsDlgButtonChecked(hwnd, CONTROL_SIZE_LARGE) == BST_CHECKED) {
                clock_size = CLOCK_SIZE_LARGE;
            } else {
                clock_size = CLOCK_SIZE_MEDIUM;
            }

            if (IsDlgButtonChecked(hwnd, CONTROL_POSITION_TOP_RIGHT) == BST_CHECKED) {
                clock_position = CLOCK_POSITION_TOP_RIGHT;
            } else if (IsDlgButtonChecked(hwnd, CONTROL_POSITION_BOTTOM_LEFT) == BST_CHECKED) {
                clock_position = CLOCK_POSITION_BOTTOM_LEFT;
            } else if (IsDlgButtonChecked(hwnd, CONTROL_POSITION_BOTTOM_RIGHT) == BST_CHECKED) {
                clock_position = CLOCK_POSITION_BOTTOM_RIGHT;
            } else {
                clock_position = CLOCK_POSITION_TOP_LEFT;
            }

            HWND font_combo = GetDlgItem(hwnd, CONTROL_FONT);
            LRESULT selected_font = SendMessageW(font_combo, CB_GETCURSEL, 0, 0);
            if (selected_font != CB_ERR) {
                SendMessageW(font_combo, CB_GETLBTEXT, (WPARAM)selected_font,
                             (LPARAM)clock_font);
                clock_font[LF_FACESIZE - 1] = L'\0';
            }

            apply_clock_settings(owner);
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(w_param) == IDCANCEL) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        settings_window = NULL;
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd, message, w_param, l_param);
}

static void show_settings_window(HWND owner)
{
    if (settings_window != NULL) {
        ShowWindow(settings_window, SW_RESTORE);
        SetForegroundWindow(settings_window);
        return;
    }

    HINSTANCE instance = (HINSTANCE)GetWindowLongPtrW(owner, GWLP_HINSTANCE);
    settings_window = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        SETTINGS_CLASS_NAME,
        L"Eroge Timer 設定",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 330, 395,
        owner, NULL, instance, NULL);

    if (settings_window != NULL) {
        ShowWindow(settings_window, SW_SHOW);
        SetForegroundWindow(settings_window);
    }
}

static BOOL add_tray_icon(HWND hwnd)
{
    ZeroMemory(&tray_icon, sizeof(tray_icon));
    tray_icon.cbSize = sizeof(tray_icon);
    tray_icon.hWnd = hwnd;
    tray_icon.uID = TRAY_ICON_ID;
    tray_icon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    tray_icon.uCallbackMessage = TRAY_CALLBACK_MESSAGE;
    tray_icon.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    lstrcpynW(tray_icon.szTip, L"Eroge Timer", ARRAYSIZE(tray_icon.szTip));
    return Shell_NotifyIconW(NIM_ADD, &tray_icon);
}

static void remove_tray_icon(void)
{
    Shell_NotifyIconW(NIM_DELETE, &tray_icon);
}

static void toggle_clock(HWND hwnd)
{
    if (IsWindowVisible(hwnd)) {
        ShowWindow(hwnd, SW_HIDE);
    } else {
        ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

static void show_tray_menu(HWND hwnd)
{
    HMENU menu = CreatePopupMenu();
    if (menu == NULL) {
        return;
    }

    AppendMenuW(menu, MF_STRING | (IsWindowVisible(hwnd) ? MF_CHECKED : MF_UNCHECKED),
                MENU_TOGGLE_VISIBILITY, L"時計を表示");
    AppendMenuW(menu, MF_STRING, MENU_SETTINGS, L"設定...");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, MENU_EXIT, L"終了");

    POINT cursor;
    GetCursorPos(&cursor);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hwnd, NULL);
    PostMessageW(hwnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param)
{
    if (message == taskbar_created_message) {
        add_tray_icon(hwnd);
        return 0;
    }

    switch (message) {
    case WM_CREATE:
        SetLayeredWindowAttributes(hwnd, TRANSPARENT_COLOR, 255, LWA_COLORKEY);
        SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL_MS, NULL);
        add_tray_icon(hwnd);
        return 0;

    case TRAY_CALLBACK_MESSAGE:
        if ((UINT)l_param == WM_RBUTTONUP) {
            show_tray_menu(hwnd);
        } else if ((UINT)l_param == WM_LBUTTONDBLCLK) {
            toggle_clock(hwnd);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(w_param)) {
        case MENU_TOGGLE_VISIBILITY:
            toggle_clock(hwnd);
            return 0;
        case MENU_SETTINGS:
            show_settings_window(hwnd);
            return 0;
        case MENU_EXIT:
            DestroyWindow(hwnd);
            return 0;
        default:
            break;
        }
        return 0;

    case WM_TIMER:
        if (w_param == TIMER_ID) {
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_ERASEBKGND: {
        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect((HDC)w_param, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(hwnd, &paint);
        RECT rect;
        SYSTEMTIME time;
        wchar_t text[16];

        GetClientRect(hwnd, &rect);
        SetDCBrushColor(dc, TRANSPARENT_COLOR);
        FillRect(dc, &rect, (HBRUSH)GetStockObject(DC_BRUSH));

        GetLocalTime(&time);
        swprintf(text, sizeof(text) / sizeof(text[0]), L"%02u:%02u:%02u",
                 time.wHour, time.wMinute, time.wSecond);

        static const int font_heights[] = {-30, -42, -56};
        HFONT font = CreateFontW(
            font_heights[clock_size], 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, clock_font);
        HGDIOBJ old_font = SelectObject(dc, font);

        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(255, 255, 255));
        DrawTextW(dc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(dc, old_font);
        DeleteObject(font);
        EndPaint(hwnd, &paint);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        remove_tray_icon();
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, message, w_param, l_param);
    }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous_instance,
                    PWSTR command_line, int show_command)
{
    (void)previous_instance;
    (void)command_line;
    (void)show_command;

    SetProcessDPIAware();
    taskbar_created_message = RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSEXW window_class = {0};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
    window_class.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassExW(&window_class)) {
        MessageBoxW(NULL, L"ウィンドウクラスを登録できませんでした。", L"Eroge Timer", MB_ICONERROR);
        return 1;
    }

    WNDCLASSEXW settings_class = {0};
    settings_class.cbSize = sizeof(settings_class);
    settings_class.lpfnWndProc = settings_window_proc;
    settings_class.hInstance = instance;
    settings_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
    settings_class.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    settings_class.lpszClassName = SETTINGS_CLASS_NAME;

    if (!RegisterClassExW(&settings_class)) {
        MessageBoxW(NULL, L"設定ウィンドウクラスを登録できませんでした。",
                    L"Eroge Timer", MB_ICONERROR);
        return 1;
    }

    DWORD extended_style = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT |
                           WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;

    HWND hwnd = CreateWindowExW(
        extended_style,
        WINDOW_CLASS_NAME,
        L"Eroge Timer",
        WS_POPUP,
        32, 32, 260, 72,
        NULL, NULL, instance, NULL);

    if (hwnd == NULL) {
        MessageBoxW(NULL, L"時計ウィンドウを作成できませんでした。", L"Eroge Timer", MB_ICONERROR);
        return 1;
    }

    apply_clock_settings(hwnd);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);

    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return (int)message.wParam;
}
