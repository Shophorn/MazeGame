/*
Leo Tamminen

Dynamically loaded game code dll
*/

// Todo(Leo): Change to wide strings???
constexpr internal char const * GAMECODE_DLL_FILE_NAME        = "friendsimulator.dll";
constexpr internal char const * GAMECODE_DLL_FILE_NAME_TEMP   = "friendsimulator_temp.dll";
constexpr internal char const * GAMECODE_UPDATE_FUNC_NAME     = "update_game";

struct Win32Game
{
    // Note(Leo): these are for hot reloading game code during development
    // Todo(Leo): Remove functionality from final game
	HMODULE dllHandle;
    FILETIME dllWriteTime;
    bool32 shouldReInitializeGlobalVariables;

	UpdateGameFunc * update;	
};

internal bool32 fswin32_game_is_loaded(Win32Game * game)
{
    bool32 loaded   = game->dllHandle != nullptr
                    && game->update != nullptr;
    return loaded;
}

internal void fswin32_game_load_dll(Win32Game * game)
{
    AssertMsg(fswin32_game_is_loaded(game) == false, "Game code is already loaded, unload before reloading");

    CopyFileA(GAMECODE_DLL_FILE_NAME, GAMECODE_DLL_FILE_NAME_TEMP, false);
    game->dllHandle = LoadLibraryA(GAMECODE_DLL_FILE_NAME_TEMP);
    if(game->dllHandle != nullptr)
    {
        FARPROC procAddress = GetProcAddress(game->dllHandle, GAMECODE_UPDATE_FUNC_NAME);
        game->update        = reinterpret_cast<UpdateGameFunc*>(procAddress);
        game->dllWriteTime  = fswin32_file_get_write_time(GAMECODE_DLL_FILE_NAME);
    }

    Assert(fswin32_game_is_loaded(game) && "Game not loaded");
}

internal void fswin32_game_unload_dll(Win32Game * game)
{
    FreeLibrary(game->dllHandle);
    game->dllHandle =  nullptr;

    DeleteFileA(GAMECODE_DLL_FILE_NAME_TEMP);
}

internal void fswin32_game_reload(Win32Game & game)
{
    #if !defined FRIENDSIMULATOR_FULL_GAME
        FILETIME dllLatestWriteTime = fswin32_file_get_write_time(GAMECODE_DLL_FILE_NAME);
        if (CompareFileTime(&dllLatestWriteTime, &game.dllWriteTime) > 0)
        {
            log_application(0, "Attempting to reload game");

            fswin32_game_unload_dll(&game);
            fswin32_game_load_dll(&game);

            log_application(0, "Reloaded game");

            game.shouldReInitializeGlobalVariables = true;
        }
    #endif
}