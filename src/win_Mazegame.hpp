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

struct WinApiGame
{
	HMODULE dllHandle;
	bool32 IsLoaded() { return dllHandle != nullptr; }

	using UpdateFunc = decltype(GameUpdate);
	UpdateFunc * Update;	
};

namespace winapi
{
    struct State
    {
        bool32 isRunning; 

        int32 windowWidth;
        int32 windowHeight;
    };  
}