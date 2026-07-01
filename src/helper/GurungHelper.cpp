#include <Windows.h>
#include <shellapi.h>

namespace {

constexpr UINT kTrayMessage = WM_APP + 41;
constexpr UINT kExitCommand = 1001;
constexpr wchar_t kWindowClass[] = L"GurungScientificImeHelperWindow";
constexpr wchar_t kTooltip[] = L"Gurung Scientific IME";

NOTIFYICONDATAW g_icon = {};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            g_icon.cbSize = sizeof(g_icon);
            g_icon.hWnd = hwnd;
            g_icon.uID = 1;
            g_icon.uFlags = NIF_MESSAGE | NIF_TIP | NIF_ICON;
            g_icon.uCallbackMessage = kTrayMessage;
            g_icon.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
            lstrcpynW(g_icon.szTip, kTooltip, ARRAYSIZE(g_icon.szTip));
            Shell_NotifyIconW(NIM_ADD, &g_icon);
            return 0;
        }
        case kTrayMessage:
            if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
                POINT pt = {};
                GetCursorPos(&pt);
                HMENU menu = CreatePopupMenu();
                AppendMenuW(menu, MF_STRING | MF_GRAYED, 0, L"Gurung Scientific IME");
                AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(menu, MF_STRING, kExitCommand, L"Exit helper");
                SetForegroundWindow(hwnd);
                TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
                DestroyMenu(menu);
            }
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == kExitCommand) {
                DestroyWindow(hwnd);
            }
            return 0;
        case WM_DESTROY:
            Shell_NotifyIconW(NIM_DELETE, &g_icon);
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kWindowClass;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, kWindowClass, L"Gurung Scientific IME Helper", 0,
                                0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, nullptr);
    if (hwnd == nullptr) {
        return 1;
    }

    MSG message = {};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return 0;
}
