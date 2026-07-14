#include "tray.h"

#include <shellapi.h>

#define TRAY_ICON_ID 1

static NOTIFYICONDATAW tray_icon;
static BOOL menu_open;
static HWND menu_host;

BOOL tray_is_menu_open(void)
{
    return menu_open;
}

void tray_cancel_menu(void)
{
    if (menu_open) {
        EndMenu();
    }
}

BOOL tray_add(HWND owner)
{
    if (menu_host == NULL) {
        menu_host = CreateWindowExW(
            WS_EX_TOOLWINDOW, L"STATIC", L"Eroge Timer Menu Host",
            WS_POPUP, -32000, -32000, 1, 1,
            NULL, NULL,
            (HINSTANCE)GetWindowLongPtrW(owner, GWLP_HINSTANCE), NULL);
    }

    ZeroMemory(&tray_icon, sizeof(tray_icon));
    tray_icon.cbSize = sizeof(tray_icon);
    tray_icon.hWnd = owner;
    tray_icon.uID = TRAY_ICON_ID;
    tray_icon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    tray_icon.uCallbackMessage = TRAY_CALLBACK_MESSAGE;
    tray_icon.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    lstrcpynW(tray_icon.szTip, L"Eroge Timer", ARRAYSIZE(tray_icon.szTip));
    return Shell_NotifyIconW(NIM_ADD, &tray_icon);
}

void tray_remove(void)
{
    Shell_NotifyIconW(NIM_DELETE, &tray_icon);
    if (menu_host != NULL) {
        DestroyWindow(menu_host);
        menu_host = NULL;
    }
}

void tray_show_menu(HWND owner, BOOL clock_visible)
{
    HMENU menu = CreatePopupMenu();
    if (menu == NULL) {
        return;
    }

    AppendMenuW(menu, MF_STRING | (clock_visible ? MF_CHECKED : MF_UNCHECKED),
                TRAY_COMMAND_TOGGLE_VISIBILITY, L"時計を表示");
    AppendMenuW(menu, MF_STRING, TRAY_COMMAND_SETTINGS, L"設定...");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, TRAY_COMMAND_EXIT, L"終了");

    POINT cursor;
    GetCursorPos(&cursor);

    HWND previous_foreground = GetForegroundWindow();
    HWND popup_owner = menu_host != NULL ? menu_host : owner;
    if (menu_host != NULL) {
        ShowWindow(menu_host, SW_SHOW);
    }
    SetForegroundWindow(popup_owner);
    menu_open = TRUE;
    UINT command = TrackPopupMenu(
        menu, TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
        cursor.x, cursor.y, 0, popup_owner, NULL);
    menu_open = FALSE;
    if (menu_host != NULL) {
        ShowWindow(menu_host, SW_HIDE);
    }
    if (GetForegroundWindow() == popup_owner &&
        previous_foreground != NULL && IsWindow(previous_foreground)) {
        SetForegroundWindow(previous_foreground);
    }
    if (command != 0) {
        PostMessageW(owner, WM_COMMAND, MAKEWPARAM(command, 0), 0);
    }
    PostMessageW(owner, WM_NULL, 0, 0);
    DestroyMenu(menu);
}
