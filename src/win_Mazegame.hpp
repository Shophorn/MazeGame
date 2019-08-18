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
