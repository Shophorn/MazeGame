#include "fswin32_window_message_string.cpp"

using Win32Window = PlatformWindow;

struct PlatformWindow
{
    u32 width;
    u32 height;
    bool32 isFullscreen;
    bool32 isMinimized;

    bool32 isCursorVisible = true;

    // Lol(Leo): maybe this is acceptable name.
    HWND            hwnd;
    HINSTANCE       hInstance;

    // Note(Leo): This stores previous window size before fullscreen
    WINDOWPLACEMENT windowedWindowPosition;        
};



// ---------- Platform API functions --------------------------


internal void platform_window_set_fullscreen(Win32Window * window, bool32 setFullscreen)
{
    // Study(Leo): https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
    DWORD style = GetWindowLongPtr(window->hwnd, GWL_STYLE);
    bool32 isFullscreen = !(style & WS_OVERLAPPEDWINDOW);

    if (isFullscreen == false && setFullscreen)
    {
        // Todo(Leo): Actually use this value to check for potential errors
        bool32 success;

        success = GetWindowPlacement(window->hwnd, &window->windowedWindowPosition); 

        HMONITOR monitor        = MonitorFromWindow(window->hwnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO monitorInfo = {sizeof(MONITORINFO)};
        success                 = GetMonitorInfoW(monitor, &monitorInfo);

        DWORD fullScreenStyle = (style & ~WS_OVERLAPPEDWINDOW);
        SetWindowLongPtrW(window->hwnd, GWL_STYLE, fullScreenStyle);

        // Note(Leo): Remember that height value starts from top and grows towards bottom
        LONG left = monitorInfo.rcMonitor.left;
        LONG top = monitorInfo.rcMonitor.top;
        LONG width = monitorInfo.rcMonitor.right - left;
        LONG heigth = monitorInfo.rcMonitor.bottom - top;
        SetWindowPos(window->hwnd, HWND_TOP, left, top, width, heigth, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        window->isFullscreen = true;
    }

    else if (isFullscreen && setFullscreen == false)
    {
        SetWindowLongPtr(window->hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window->hwnd, &window->windowedWindowPosition);
        SetWindowPos(   window->hwnd, NULL, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        window->isFullscreen = false;

    }
}

internal void platform_window_set_cursor_visible(PlatformWindow * window, bool32 visible)
{
    if (window->isCursorVisible != visible)
    {
        window->isCursorVisible = visible;
        ShowCursor(visible);
    }
}

internal u32 platform_window_get_width(PlatformWindow const * window)
{
    return window->width;
}

internal u32 platform_window_get_height(PlatformWindow const * window)
{
    return window->height;
}

internal bool32 platform_window_is_fullscreen(PlatformWindow const * window)
{
    return window->isFullscreen;
}


// --------- Not platform api functions --------

internal bool32 fswin32_is_window_drawable(Win32Window const * window)
{
    bool32 isDrawable = (window->isMinimized == false)
                        && (window->width > 0)
                        && (window->height > 0);
    return isDrawable;
}

struct Win32ApplicationState
{
    bool32 shouldClose;

    // Input
    bool32 keyboardInputIsUsed;
    bool32 xinputIsUsed;

    Win32XInput         gamepadInput;
    Win32KeyboardInput  keyboardInput;

    Win32Window *       window;
};  



internal Win32ApplicationState * fswin32_get_user_pointer(HWND hwnd)
{
    LONG_PTR value = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    return reinterpret_cast<Win32ApplicationState*>(value);
}

// Note(Leo): CALLBACK specifies calling convention for 32-bit applications
// https://stackoverflow.com/questions/15126454/what-does-the-callback-keyword-mean-in-a-win-32-c-application
internal LRESULT CALLBACK fswin32_window_callback (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_MOUSEMOVE:
        {
            Win32ApplicationState & state = *fswin32_get_user_pointer(hwnd);
            state.keyboardInput.mousePosition.x = GET_X_LPARAM(lParam);
            state.keyboardInput.mousePosition.y = GET_Y_LPARAM(lParam);
        } break;

        case WM_LBUTTONDOWN:
        {
            Win32ApplicationState & state   = *fswin32_get_user_pointer(hwnd);
            state.keyboardInput.mouse0      = true;
            state.keyboardInputIsUsed       = true;

            // log_debug(FILE_ADDRESS, "WM_LBUTTONDOWN: ", state.keyboardInput.mouse0);
        } break;

        case WM_LBUTTONUP:
        {
            Win32ApplicationState & state   = *fswin32_get_user_pointer(hwnd);
            state.keyboardInput.mouse0      = false;
            state.keyboardInputIsUsed       = true;
        } break;

        case WM_RBUTTONDOWN:
        {           
            Win32ApplicationState & state   = *fswin32_get_user_pointer(hwnd);
            state.keyboardInput.mouse1      = true;
            state.keyboardInputIsUsed       = true;
        } break;

        case WM_RBUTTONUP:
        {
            Win32ApplicationState & state   = *fswin32_get_user_pointer(hwnd);
            state.keyboardInput.mouse1      = false;
            state.keyboardInputIsUsed       = true;
        } break;


        case WM_MOUSEWHEEL:
        {
            constexpr f32 windowsScrollConstant = 120;
            
            Win32ApplicationState & state   = *fswin32_get_user_pointer(hwnd);
            state.keyboardInput.mouseScroll       += GET_WHEEL_DELTA_WPARAM(wParam) / windowsScrollConstant;
        } break;

        case WM_KEYDOWN:
        {
            Win32ApplicationState * state = fswin32_get_user_pointer(hwnd);
            process_keyboard_input(state->keyboardInput, wParam, true);
            state->keyboardInputIsUsed = true;
        } break;

        case WM_KEYUP:
        {
            Win32ApplicationState * state = fswin32_get_user_pointer(hwnd);
            process_keyboard_input(state->keyboardInput, wParam, false);
            state->keyboardInputIsUsed = true;
        } break;

        // case WM_CREATE:
        // {
        //     // CREATESTRUCTW * pCreate = reinterpret_cast<CREATESTRUCTW *>(lParam);
        //     // HWNDUserPointer * state = reinterpret_cast<HWNDUserPointer*>(pCreate->lpCreateParams); 
        //     // // Win32ApplicationState * state = reinterpret_cast<Win32ApplicationState *>(pCreate->lpCreateParams);
        //     // SetWindowLongPtrW (hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        // } break;

        case WM_SIZE:
        {
            auto * state = fswin32_get_user_pointer(hwnd);
            if (state == nullptr)
            {
                log_application(1, "Skipping first WM_SIZE message due to no user pointer set yet.");
                result = DefWindowProcW(hwnd, message, wParam, lParam);
                break;
            }

            /* Todo(Leo): we should recreate vulkan swapchain
            again ourselves instead of waiting for vulkan to ask. */

            auto * window   = state->window;
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
            fswin32_get_user_pointer(hwnd)->shouldClose = true;
        } break;

        // case platform_window_set_cursor_visible:
        // {
        // } break;

        default:

// --------- Not platform api functions --------
            result = DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return result;
}

internal void fswin32_process_pending_messages(Win32ApplicationState * state, HWND winWindow)
{
    // Note(Leo): This must be reseted manually
    state->keyboardInputIsUsed = false;

    MSG message;
    while (PeekMessageW(&message, winWindow, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}

internal Win32Window fswin32_make_window(HINSTANCE winInstance, u32 width, u32 height)
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

    Win32Window window = {};


    window.width        = windowRect.right - windowRect.left;
    window.height       = windowRect.bottom - windowRect.top;
    window.hwnd         = hwnd;
    window.hInstance    = winInstance;

    return window;
}
