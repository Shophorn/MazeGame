struct Win32WindowUserData
{
	Win32PlatformWindow * window;
	Win32PlatformInput  * input;
};  

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


// Note(Leo): CALLBACK specifies calling convention for 32-bit applications
// https://stackoverflow.com/questions/15126454/what-does-the-callback-keyword-mean-in-a-win-32-c-application
static LRESULT CALLBACK win32_window_callback (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{


	if (message == WM_CREATE)
	{
	    CREATESTRUCTW * pCreate 		= reinterpret_cast<CREATESTRUCTW *>(lParam);
   	 	Win32WindowUserData * userData 	= reinterpret_cast<Win32WindowUserData *>(pCreate->lpCreateParams);
    	SetWindowLongPtrW (hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(userData));

    	return 0;	
	}


	if(ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam) != 0)
	{
		return 0;
	}

	Win32WindowUserData * userData = reinterpret_cast<Win32WindowUserData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	LRESULT result = 0;

	switch (message)
	{
		case WM_LBUTTONDOWN:
		{
			Win32PlatformInput & input 			= *userData->input;
			input.buttons[InputButton_mouse_0]	= InputButtonState_went_down;
			input.mouseAndKeyboardInputUsed		= true;
		} break;

		case WM_LBUTTONUP:
		{
			Win32PlatformInput & input 			= *userData->input;
			input.buttons[InputButton_mouse_0]  = InputButtonState_went_up;
			input.mouseAndKeyboardInputUsed		= true;
		} break;

		case WM_RBUTTONDOWN:
		{           
			Win32PlatformInput & input 			= *userData->input;
			input.buttons[InputButton_mouse_1]  = InputButtonState_went_down;
			input.mouseAndKeyboardInputUsed		= true;
		} break;

		case WM_RBUTTONUP:
		{
			Win32PlatformInput & input 			= *userData->input;
			input.buttons[InputButton_mouse_1]  = InputButtonState_went_up;
			input.mouseAndKeyboardInputUsed		= true;
		} break;

		case WM_MOUSEWHEEL:
		{
			// Note(Leo): this may come multiple times per frame, if player scrolls furiously
			// so we add value to current value here, and value must be reset before input each frame. 
			Win32PlatformInput & input 			= *userData->input;
			input.axes[InputAxis_mouse_scroll] 	+= static_cast<f32>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<f32>(WHEEL_DELTA);

		} break;

		case WM_KEYDOWN:
		{
			Win32PlatformInput & input 						= *userData->input;
			input.buttons[win32_input_button_map(wParam)] 	= InputButtonState_went_down;
			input.mouseAndKeyboardInputUsed		        	= true;
		} break;

		case WM_KEYUP:
		{
			Win32PlatformInput & input = *userData->input;
			// Note(Leo): InputButton_invalid has trash value by definition, so we can carelessly write to it
			input.buttons[win32_input_button_map(wParam)] = InputButtonState_went_up;
			input.mouseAndKeyboardInputUsed		        = true;
		} break;

		case WM_SIZE:
		{
			/* Todo(Leo): we should recreate vulkan swapchain
			again ourselves instead of waiting for vulkan to ask. */

			if (userData->window == nullptr)
			{
				break;
			}

			log_debug(FILE_ADDRESS, "WM_SIZE");

			Win32PlatformWindow & window = *userData->window;
			window.width   = LOWORD(lParam);
			window.height  = HIWORD(lParam);

			switch (wParam)
			{
				case SIZE_RESTORED:
				case SIZE_MAXIMIZED:
					window.isMinimized = false;
					break;

				case SIZE_MINIMIZED:
					window.isMinimized = true;
					break;
			}

			window.events.resized = true;

		} break;

		case WM_CLOSE:
		{
			userData->window->shouldClose = true;
		} break;

		case WindowMessage_set_fullscreen:
		{
			bool32 setFullscreen = static_cast<bool32>(wParam);

			// Study(Leo): https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
			DWORD style = GetWindowLongPtr(userData->window->hwnd, GWL_STYLE);
			bool32 isFullscreen = !(style & WS_OVERLAPPEDWINDOW);

			if (isFullscreen == false && setFullscreen)
			{
				// Todo(Leo): Actually use this value to check for potential errors
				bool32 success;

				success = GetWindowPlacement(userData->window->hwnd, &userData->window->windowedWindowPosition); 

				HMONITOR monitor        = MonitorFromWindow(userData->window->hwnd, MONITOR_DEFAULTTOPRIMARY);
				MONITORINFO monitorInfo = {sizeof(MONITORINFO)};
				success                 = GetMonitorInfoW(monitor, &monitorInfo);

				DWORD fullScreenStyle = (style & ~WS_OVERLAPPEDWINDOW);
				SetWindowLongPtrW(userData->window->hwnd, GWL_STYLE, fullScreenStyle);

				// Note(Leo): Remember that height value starts from top and grows towards bottom
				LONG left = monitorInfo.rcMonitor.left;
				LONG top = monitorInfo.rcMonitor.top;
				LONG width = monitorInfo.rcMonitor.right - left;
				LONG heigth = monitorInfo.rcMonitor.bottom - top;
				SetWindowPos(userData->window->hwnd, HWND_TOP, left, top, width, heigth, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

				userData->window->isFullscreen 		= true;
				userData->window->events.resized 	= true;
			}

			else if (isFullscreen && setFullscreen == false)
			{
				SetWindowLongPtr(userData->window->hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
				SetWindowPlacement(userData->window->hwnd, &userData->window->windowedWindowPosition);
				SetWindowPos(   userData->window->hwnd, NULL, 0, 0, 0, 0,
								SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

				userData->window->isFullscreen 		= false;
				userData->window->events.resized 	= true;
			}
		} break;
		// case platform_window_set_cursor_visible:
		// {
		// } break;

		default:
			result = DefWindowProc(hwnd, message, wParam, lParam);
	}
	return result;
}
