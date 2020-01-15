/*=============================================================================
Leo Tamminen

Windows platform specific header for :MAZEGAME:
=============================================================================*/

// Todo(Leo): Proper logging and severity system. Severe prints always, and mild only on error
internal void
WinApiLog(const char * message, HRESULT result)
{
    #if MAZEGAME_DEVELOPMENT
    if (result != S_OK)
    {
    	std::cout << message << "(" << WinApiErrorString(result) << ")\n";
    }
    #endif
}

// Todo(Leo): Change to wide strings???
constexpr static char GAMECODE_DLL_FILE_NAME [] = "Mazegame.dll";
constexpr static char GAMECODE_DLL_FILE_NAME_TEMP [] = "Mazegame_temp.dll";
constexpr static char GAMECODE_UPDATE_FUNC_NAME [] = "GameUpdate";


namespace winapi
{
    struct Game
    {
    	HMODULE dllHandle;
    	bool32 IsLoaded() { return dllHandle != nullptr; }

    	using UpdateFunc = decltype(GameUpdate);
    	UpdateFunc * Update;	
    };

    struct KeyboardInput
    {
        bool32 left, right, up, down;
        bool32 space;
        bool32 enter, escape;
    };

    struct State
    {
        game::PlatformInfo gamePlatformInfo = {};


        bool32 isRunning; 
        bool32 windowIsMinimized;

        bool32 windowIsFullscreen;
        WINDOWPLACEMENT windowedWindowPosition;        

        bool32 keyboardInputIsUsed;

        DWORD xinputLastPacketNumber;
        bool32 xinputIsUsed;

        KeyboardInput keyboardInput;

        bool32 windowIsDrawable()
        {
            bool32 isDrawable = (windowIsMinimized == false)
                                && (gamePlatformInfo.windowWidth > 0)
                                && (gamePlatformInfo.windowHeight > 0);
            return isDrawable;
        }

        // Todo(Leo): frame buffer size things do not belong here
        VkExtent2D GetFrameBufferSize ()
        {
            VkExtent2D result = {
                static_cast<uint32>(gamePlatformInfo.windowWidth),
                static_cast<uint32>(gamePlatformInfo.windowHeight)
            };
            return result;
        }
        bool32 framebufferResized = false;

    };  
}