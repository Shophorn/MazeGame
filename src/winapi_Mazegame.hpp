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


struct platform::Platform
{
    struct {
        int32   width;
        int32   height;
        bool32  isFullscreen;
        bool32  isMinimized;
    } window;  
};

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
        game::PlatformFunctions platformFunctions = {};

        platform::Platform platform;




        bool32 isRunning; 

        bool32 windowIsFullscreen;
        WINDOWPLACEMENT windowedWindowPosition;        

        bool32 keyboardInputIsUsed;

        DWORD xinputLastPacketNumber;
        bool32 xinputIsUsed;

        KeyboardInput keyboardInput;

        bool32 windowIsDrawable()
        {
            bool32 isDrawable = (platform.window.isMinimized == false)
                                && (platform.window.width > 0)
                                && (platform.window.height > 0);
            return isDrawable;
        }

        // Todo(Leo): frame buffer size things do not belong here
        VkExtent2D GetFrameBufferSize ()
        {
            VkExtent2D result = {
                static_cast<uint32>(platform.window.width),
                static_cast<uint32>(platform.window.height)
            };
            return result;
        }
        bool32 framebufferResized = false;

    };  
}