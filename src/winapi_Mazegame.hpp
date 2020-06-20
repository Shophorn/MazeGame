/*=============================================================================
Leo Tamminen

Windows platform specific header for :MAZEGAME:

Todo(Leo):
    - change all windows functions to W-versions, instead of A-versions
        (or change to macroed version, and set wideness from compiler).
=============================================================================*/

// Todo(Leo): Proper logging and severity system. Severe prints always, and mild only on error
internal void
WinApiLog(const char * message, HRESULT result)
{
    #if MAZEGAME_DEVELOPMENT
    if (result != S_OK)
    {
    	logDebug() << message << "(" << WinApiErrorString(result) << ")\n";
    }
    #endif
}

internal FILETIME
get_file_write_time(const char * fileName)
{
    // Todo(Leo): Make this function sensible
    WIN32_FILE_ATTRIBUTE_DATA fileInfo = {};
    if (GetFileAttributesExA(fileName, GetFileExInfoStandard, &fileInfo))
    {
    }
    else
    {
        // Todo(Leo): Now what??? Getting file time failed --> file does not exist??
    }    
    FILETIME result = fileInfo.ftLastWriteTime;
    return result;
}


// Todo(Leo): Change to wide strings???
constexpr static char GAMECODE_DLL_FILE_NAME [] = "Mazegame.dll";
constexpr static char GAMECODE_DLL_FILE_NAME_TEMP [] = "Mazegame_temp.dll";
constexpr static char GAMECODE_UPDATE_FUNC_NAME [] = "update_game";

namespace winapi
{
    struct Game
    {
        // Note(Leo): these are for hot reloading game code during development
        // Todo(Leo): Remove functionality from final game
    	HMODULE dllHandle;
        FILETIME dllWriteTime;

    	GameUpdateFunc * Update;	
    };

    bool32 is_loaded(Game * game)
    {
        bool32 loaded   = game->dllHandle != nullptr
                        && game->Update != nullptr;
        return loaded;
    }

    void
    load_game(Game * game)
    {
        AssertMsg(is_loaded(game) == false, "Game code is already loaded, unload before reloading");

        CopyFileA(GAMECODE_DLL_FILE_NAME, GAMECODE_DLL_FILE_NAME_TEMP, false);
        game->dllHandle = LoadLibraryA(GAMECODE_DLL_FILE_NAME_TEMP);
        if(game->dllHandle != nullptr)
        {
            FARPROC procAddress = GetProcAddress(game->dllHandle, GAMECODE_UPDATE_FUNC_NAME);
            game->Update        = reinterpret_cast<GameUpdateFunc*>(procAddress);
            game->dllWriteTime  = get_file_write_time(GAMECODE_DLL_FILE_NAME);
        }

        AssertMsg(is_loaded(game), "Game not loaded");
    }

    void
    unload_game(Game * game)
    {
        FreeLibrary(game->dllHandle);
        game->dllHandle =  nullptr;
    }

    struct KeyboardInput
    {
        bool32 left, right, up, down;
        bool32 space;
        bool32 enter, escape;
    };

    struct State
    {
        // Input
        bool32 keyboardInputIsUsed;
        DWORD xinputLastPacketNumber;
        bool32 xinputIsUsed;
        KeyboardInput keyboardInput;
    };  
}