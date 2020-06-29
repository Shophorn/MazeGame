#include "winapi_WindowMessages.cpp"

using WinAPIWindow = PlatformWindow;

struct PlatformWindow
{
    u32 width;
    u32 height;
    bool32 isFullscreen;
    bool32 isMinimized;

    bool32 shouldClose;

    // Lol(Leo): maybe this is acceptable name.
    HWND            hwnd;
    HINSTANCE       hInstance;

    // Note(Leo): This stores previous window size before fullscreen
    WINDOWPLACEMENT windowedWindowPosition;        
};


struct HWNDUserPointer
{
    WinAPIWindow * window;

    // Only for now
    winapi::State * state;

    // Todo(Leo): set to Input struct like Window and Graphics are now
    // KeyboardInput * input;
};


internal bool32 fswin32_is_window_drawable(WinAPIWindow const * window)
{
    bool32 isDrawable = (window->isMinimized == false)
                        && (window->width > 0)
                        && (window->height > 0);
    return isDrawable;
}

internal void fswin32_set_window_fullscreen(WinAPIWindow * window, bool32 setFullscreen)
{
    // Study(Leo): https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
    DWORD style = GetWindowLongPtr(window->hwnd, GWL_STYLE);
    bool32 isFullscreen = !(style & WS_OVERLAPPEDWINDOW);

    if (isFullscreen == false && setFullscreen)
    {
        // Todo(Leo): Actually use this value to check for potential errors
        bool32 success;

        success = GetWindowPlacement(window->hwnd, &window->windowedWindowPosition); 

        HMONITOR monitor = MonitorFromWindow(window->hwnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
        success = GetMonitorInfoW(monitor, &monitorInfo);

        DWORD fullScreenStyle = (style & ~WS_OVERLAPPEDWINDOW);
        SetWindowLongPtrW(window->hwnd, GWL_STYLE, fullScreenStyle);

        // Note(Leo): Remember that height value starts from top and grows towards bottom
        LONG left = monitorInfo.rcMonitor.left;
        LONG top = monitorInfo.rcMonitor.top;
        LONG width = monitorInfo.rcMonitor.right - left;
        LONG heigth = monitorInfo.rcMonitor.bottom - top;
        SetWindowPos(window->hwnd, HWND_TOP, left, top, width, heigth, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        ShowCursor(false);

        window->isFullscreen = true;
    }

    else if (isFullscreen && setFullscreen == false)
    {
        SetWindowLongPtr(window->hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window->hwnd, &window->windowedWindowPosition);
        SetWindowPos(   window->hwnd, NULL, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        ShowCursor(true);

        window->isFullscreen = false;

    }
}

internal HWNDUserPointer * fswin32_get_user_pointer(HWND hwnd)
{
    LONG_PTR value = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    return reinterpret_cast<HWNDUserPointer*>(value);
}

// Note(Leo): CALLBACK specifies calling convention for 32-bit applications
// https://stackoverflow.com/questions/15126454/what-does-the-callback-keyword-mean-in-a-win-32-c-application
internal LRESULT CALLBACK fswin32_window_callback (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    using namespace winapi;

    switch (message)
    {
        case WM_KEYDOWN:
            process_keyboard_input(fswin32_get_user_pointer(hwnd)->state, wParam, true);
            break;

        case WM_KEYUP:
            process_keyboard_input(fswin32_get_user_pointer(hwnd)->state, wParam, false);
            break;

        // case WM_CREATE:
        // {
        //     // CREATESTRUCTW * pCreate = reinterpret_cast<CREATESTRUCTW *>(lParam);
        //     // HWNDUserPointer * userPointer = reinterpret_cast<HWNDUserPointer*>(pCreate->lpCreateParams); 
        //     // // winapi::State * state = reinterpret_cast<winapi::State *>(pCreate->lpCreateParams);
        //     // SetWindowLongPtrW (hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(userPointer));
        // } break;

        case WM_SIZE:
        {
            auto * userPointer = fswin32_get_user_pointer(hwnd);
            if (userPointer == nullptr)
            {
                logSystem() << "Skipping first WM_SIZE message due to no user pointer set. Yet.";
                result = DefWindowProcW(hwnd, message, wParam, lParam);
                break;
            }

            /* Todo(Leo): we should recreate vulkan swapchain
            again ourselves instead of waiting for vulkan to ask. */

            auto * window   = userPointer->window;
            window->width   = LOWORD(lParam);
            window->height  = HIWORD(lParam);

            switch (wParam)
            {
                case SIZE_RESTORED:
                case SIZE_MAXIMIZED:
                    window->isMinimized = false;
                    break;

                case SIZE_MINIMIZED:
                    window->isMinimized = true;
                    break;
            }

        } break;

        case WM_CLOSE:
        {
            fswin32_get_user_pointer(hwnd)->window->shouldClose = true;
        } break;

        // case WM_EXITSIZEMOVE:
        // {
        // } break;

        default:
            result = DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return result;
}

internal void fswin32_process_pending_messages(winapi::State * state, HWND winWindow)
{
    // Note(Leo): Apparently Windows requires us to do this.

    MSG message;
    while (PeekMessageW(&message, winWindow, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}

internal WinAPIWindow fswin32_make_window(HINSTANCE winInstance, u32 width, u32 height)
{
    wchar windowClassName [] = L"FriendSimulatorWindowClass";
    wchar windowTitle [] = L"FriendSimulator";

    WNDCLASSW windowClass = {};
    windowClass.style           = CS_VREDRAW | CS_HREDRAW;
    windowClass.lpfnWndProc     = fswin32_window_callback;
    windowClass.hInstance       = winInstance;
    windowClass.lpszClassName   = windowClassName;

    // Study: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-makeintresourcew
    auto defaultArrowCursor = MAKEINTRESOURCEW(32512);
    windowClass.hCursor = LoadCursorW(nullptr, defaultArrowCursor);

    if (RegisterClassW(&windowClass) == 0)
    {
        AssertRelease(false, "Failed to create window class");
    }

    HWND hwnd = CreateWindowExW (
        0, windowClassName, windowTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, winInstance, nullptr);

    AssertRelease(hwnd != nullptr, "Failed to create window");

    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);

    WinAPIWindow window = {};


    window.width        = windowRect.right - windowRect.left;
    window.height       = windowRect.bottom - windowRect.top;
    window.hwnd         = hwnd;
    window.hInstance    = winInstance;

    return window;
}
