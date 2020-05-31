/*=============================================================================
Leo Tamminen

Windows platform layer for mazegame
=============================================================================*/

#include <WinSock2.h>
#include <Windows.h>
#include <xinput.h>

#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>
#include <fstream>
#include <chrono>

// TODO(Leo): Make sure that arrays for getting extensions ana layers are large enough
// TOdo(Leo): Combine to fewer functions and remove throws, instead returning specific enum value

/* TODO(Leo) extra: Use separate queuefamily thing for transfering between vertex
staging buffer and actual vertex buffer. https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer */

/* STUDY(Leo):
    http://asawicki.info/news_1698_vulkan_sparse_binding_-_a_quick_overview.html
*/

#include "MazegamePlatform.hpp"

using TimePoint = platform::TimePoint;

LARGE_INTEGER get_performance_frequency()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    
    logConsole(0) << "performance frequency = " << frequency.QuadPart;

    return frequency;
}

TimePoint current_time () 
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return {t.QuadPart};
}

f64 elapsed_seconds(TimePoint start, TimePoint end)
{
    local_persist s64 frequency = get_performance_frequency().QuadPart;

    f64 seconds = (f64)(end.value - start.value) / frequency;
    return seconds;
}


// Todo(Leo): hack to get two controller on single pc and use only 1st controller when online
global_variable int globalXinputControllerIndex;

#include "winapi_VulkanDebugStrings.cpp"
#include "winapi_WinSocketDebugStrings.cpp"
#include "winapi_ErrorStrings.hpp"
#include "winapi_Mazegame.hpp"

// Todo(Leo): Vulkan implementation depends on this, not cool
using BinaryAsset = std::vector<u8>;
BinaryAsset
read_binary_file (const char * fileName)
{
    std::ifstream file (fileName, std::ios::ate | std::ios::binary);

    if (file.is_open() == false)
    {
        throw std::runtime_error("failed to open file");
    }

    size_t fileSize = file.tellg();
    BinaryAsset result (fileSize);

    file.seekg(0);
    file.read(reinterpret_cast<char*>(result.data()), fileSize);

    file.close();
    return result;    
}

// Note(Leo): make unity build
#include "winapi_Input.cpp"
#include "winapi_Window.cpp"
#include "winapi_Vulkan.cpp"
#include "winapi_Audio.cpp"
#include "winapi_Network.cpp"

internal void
Run(HINSTANCE hInstance)
{
    SYSTEMTIME time_;
    GetLocalTime(&time_);

    SmallString timestamp   = make_timestamp(time_.wHour, time_.wMinute, time_.wSecond);
    auto logFileName        = SmallString("logs/log_").append(timestamp).append(".log");

    auto logFile = std::ofstream(logFileName.data());

    logDebug.output     = &logFile;
    logAnim.output      = &logFile;
    logVulkan.output    = &logFile;
    logWindow.output    = &logFile;
    logSystem.output    = &logFile;
    logNetwork.output   = &logFile;
    logAudio.output     = &logFile;


    logSystem(0) << "\n"
                << "\t----- FriendSimulator -----\n"
                << "\tBuild time: " << BUILD_DATE_TIME << "\n";

    // ----------------------------------------------------------------------------------

    winapi::State state = {};
    load_xinput();

    // ---------- INITIALIZE PLATFORM ------------
    WinAPIWindow window         = winapi::make_window(hInstance, 960, 540);


    /// --------- TIMING ---------------------------
    auto startTimeMark          = std::chrono::high_resolution_clock::now();
    f64 lastFrameStartTime   = 0;

    f64 targetFrameTime;
    u32 deviceMinSchedulerGranularity;
    {
        TIMECAPS timeCaps;
        timeGetDevCaps(&timeCaps, sizeof(timeCaps));
        deviceMinSchedulerGranularity = timeCaps.wPeriodMin;

        // TODO(LEO): Make proper settings :)
        /* NOTE(Leo): Change to lower framerate if on battery. My development
        laptop at least changes to slower processing speed when not on power,
        though this probably depends on power settings. These are just some
        arbitrary values that happen to work on development laptop. */
        SYSTEM_POWER_STATUS powerStatus;
        GetSystemPowerStatus (&powerStatus);

        f64 targetFrameTimeYesPower = 60;
        f64 targetFrameTimeNoPower = 30;

        constexpr BYTE AC_ONLINE = 1;
        f64 targetFrameTimeThreshold = (powerStatus.ACLineStatus == AC_ONLINE) ? targetFrameTimeYesPower : targetFrameTimeNoPower;

        if (powerStatus.ACLineStatus == AC_ONLINE)
        {
            logSystem() << "Power detected";
        }
        else
        {
            logSystem() << "No power detected";
        }

        s32 gameUpdateRate = targetFrameTimeThreshold;
        {
            HDC deviceContext = GetDC(window.hwnd);
            int monitorRefreshRate = GetDeviceCaps(deviceContext, VREFRESH);
            ReleaseDC(window.hwnd, deviceContext);

            if (monitorRefreshRate > 1)
            {  
                gameUpdateRate = monitorRefreshRate;

                while((gameUpdateRate / 2.0f) > targetFrameTimeThreshold)
                {
                    gameUpdateRate /= 2.0f;
                }
            }
        }
        targetFrameTime = 1.0f / gameUpdateRate;
        
        logSystem() << "Target frame time = " << targetFrameTime << ", (fps = " << 1.0 / targetFrameTime << ")";
    }
    
    VulkanContext vulkanContext = winapi::create_vulkan_context(&window);
    HWNDUserPointer userPointer = 
    {
        .window = &window,
        .state = &state,
    };
    SetWindowLongPtrW(window.hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&userPointer));
   
    // SETTING FUNCTION POINTERS
    {
        state.platformFunctions =
        {
            // WINDOW FUNCTIONS
            .get_window_width        = [](platform::Window const * window) { return window->width; },
            .get_window_height       = [](platform::Window const * window) { return window->height; },
            .is_window_fullscreen    = [](platform::Window const * window) { return window->isFullscreen; },
            .set_window_fullscreen   = winapi::set_window_fullscreen,
        };
        platform::set_functions(&vulkanContext, &state.platformFunctions);

        state.platformFunctions.current_time = current_time;
        state.platformFunctions.elapsed_seconds = elapsed_seconds;

        Assert(all_functions_set(&state.platformFunctions));
    }

    // ------- MEMORY ---------------------------
    platform::Memory gameMemory = {};
    {
        // TODO [MEMORY] (Leo): Properly measure required amount
        // TODO [MEMORY] (Leo): Think of alignment
        gameMemory.size = gigabytes(2);

        // TODO [MEMORY] (Leo): Check support for large pages
        #if MAZEGAME_DEVELOPMENT
        
        void * baseAddress = (void*)terabytes(2);
        gameMemory.memory = VirtualAlloc(baseAddress, gameMemory.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        
        #else
        
        gameMemory.memory = VirtualAlloc(nullptr, gameMemory.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        
        #endif
        
        Assert(gameMemory.memory != nullptr);
    }

    // -------- INITIALIZE NETWORK ---------
    bool32 networkIsRuined = false;
    WinApiNetwork network = winapi::CreateNetwork();
    game::Network gameNetwork = {};
    f64 networkSendDelay     = 1.0 / 20;
    f64 networkNextSendTime  = 0;
    
    /// --------- INITIALIZE AUDIO ----------------
    WinApiAudio audio = winapi::CreateAudio();
    winapi::StartPlaying(&audio);                

    /// ---------- LOAD GAME CODE ----------------------
    // Note(Leo): Only load game dynamically in development.
    winapi::Game game = {};
    winapi::load_game(&game);
    logSystem() << "Game dll loaded";

      ////////////////////////////////////////////////////
     ///             MAIN LOOP                        ///
    ////////////////////////////////////////////////////

    bool gameIsRunning = true;
    game::Input gameInput = {};

    TimePoint frameFlipTime;
    f64 lastFrameElapsedSeconds;

    f64 approxAvgFrameTime      = targetFrameTime;
    f64 approxAvgFrameTimeAlpha = 0.5;

    while(gameIsRunning)
    {
        /// ----- START TIME -----
        auto currentTimeMark = std::chrono::high_resolution_clock::now();
        f64 frameStartTime = std::chrono::duration<f64, std::chrono::seconds::period>(currentTimeMark - startTimeMark).count();

        /// ----- RELOAD GAME CODE -----
        FILETIME dllLatestWriteTime = get_file_write_time(GAMECODE_DLL_FILE_NAME);
        if (CompareFileTime(&dllLatestWriteTime, &game.dllWriteTime) > 0)
        {
            logSystem(0) << "Attempting to reload game";

            winapi::unload_game(&game);
            winapi::load_game(&game);

            logSystem(0) << "Reloaded game";
        }

        /// ----- HANDLE INPUT -----
        {
            // Note(Leo): this is not input only...
            winapi::ProcessPendingMessages(&state, window.hwnd);

            HWND foregroundWindow = GetForegroundWindow();
            bool32 windowIsActive = window.hwnd == foregroundWindow;

            /* Note(Leo): Only get input from a single controller, locally this is a single
            player game. Use global controller index depending on network status to test
            multiplayering */
           /* TODO [Input](Leo): Test controller and index behaviour when being connected and
            disconnected */

            XINPUT_STATE xinputState;
            bool32 xinputReceived = XInputGetState(globalXinputControllerIndex, &xinputState) == ERROR_SUCCESS;
            bool32 xinputUsed = xinputReceived && xinput_is_used(&state, &xinputState);

            if (windowIsActive == false)
            {
                update_unused_input(&gameInput);
            }
            else if (xinputUsed)
            {
                update_controller_input(&gameInput, &xinputState);
            }
            else if (state.keyboardInputIsUsed)
            {
                update_keyboard_input(&gameInput, &state.keyboardInput);
                state.keyboardInputIsUsed = false;
            }
            else
            {
                update_unused_input(&gameInput);
            }

            gameInput.elapsedTime = lastFrameElapsedSeconds;
        }


        /// ----- PRE-UPDATE NETWORK PASS -----
        {
            // Todo(Leo): just quit now if some unhandled network error occured
            if (networkIsRuined)
            {
                auto error = WSAGetLastError();
                logNetwork() << "failed: " << WinSocketErrorString(error) << " (" << error << ")"; 
                break;
            }

            if (network.isListening)
            {
                winapi::NetworkListen(&network);
            }
            else if (network.isConnected)
            {
                winapi::NetworkReceive(&network, &gameNetwork.inPackage);
            }
            gameNetwork.isConnected = network.isConnected;
        }


        /// --------- UPDATE GAME -------------
        // Note(Leo): Game is not updated when window is not drawable.
        if (winapi::is_window_drawable(&window))
        {
            platform::SoundOutput gameSoundOutput = {};
            winapi::GetAudioBuffer(&audio, &gameSoundOutput.sampleCount, &gameSoundOutput.samples);
    
            switch(platform::prepare_frame(&vulkanContext))
            {
                case platform::FRAME_OK:
                    gameIsRunning = game.Update(&gameInput, &gameMemory,
                                                &gameNetwork, &gameSoundOutput,
                                                
                                                &vulkanContext,
                                                &window,
                                                &state.platformFunctions,
                                                &logFile);
                    break;

                case platform::FRAME_RECREATE:
                    // Todo(Leo): this is last function we actually call specifically from 'vulkan'
                    // and not platform. Do something about that.
                    // Or not, preferences have shifted
                    vulkan::recreate_drawing_resources(&vulkanContext, window.width, window.height);
                    break;

                case platform::FRAME_BAD_PROBLEM:
                    DEBUG_ASSERT(false, "We should not be here, please investigate");


            }


            winapi::ReleaseAudioBuffer(&audio, gameSoundOutput.sampleCount);
            // Note(Leo): It doesn't so much matter where this is checked.
            if (window.shouldClose)
            {
                gameIsRunning = false;
            }

        }


        // ----- POST-UPDATE NETWORK PASS ------
        // TODO[network](Leo): Now that we have fixed frame rate, we should count frames passed instead of time
        if (network.isConnected && frameStartTime > networkNextSendTime)
        {
            networkNextSendTime = frameStartTime + networkSendDelay;
            winapi::NetworkSend(&network, &gameNetwork.outPackage);
        }


        /// ----- WAIT FOR TARGET FRAMETIME -----
        // {
        //     auto currentTimeMark = std::chrono::high_resolution_clock::now();
        //     f64 currentTimeSeconds = std::chrono::duration<f64, std::chrono::seconds::period>(currentTimeMark - startTimeMark).count();

        //     f64 elapsedSeconds = currentTimeSeconds - frameStartTime;
        //     f64 timeToSleepSeconds = targetFrameTime - elapsedSeconds;

        //     // logConsole(0) << "Frametime " << elapsedSeconds;

        //      TODO[time](Leo): It seems okay to sleep 0 milliseconds in case the time to sleep ends up being
        //     less than 1 millisecond on floating point representation. Also we may want to do a busy loop over
        //     remainder time after sleep. This is due to windows scheduler granularity, that is at best on 
        //     one millisecond scale. 
        //     if (timeToSleepSeconds > 0)
        //     {
        //         DWORD timeToSleepMilliSeconds = static_cast<DWORD>(1000 * timeToSleepSeconds);
        //         logDebug() << "Sleep for " << timeToSleepMilliSeconds << " ms\n";
        //         Sleep(timeToSleepMilliSeconds);

        //     }
        // }
        // ----- MEASURE ELAPSED TIME ----- 
        {
            TimePoint now           = current_time();
            lastFrameElapsedSeconds = elapsed_seconds(frameFlipTime, now);
            frameFlipTime           = now;

            approxAvgFrameTime = interpolate(approxAvgFrameTime, lastFrameElapsedSeconds, approxAvgFrameTimeAlpha);
            // logConsole(0) << approxAvgFrameTime;
        }
    }
    ///////////////////////////////////////
    ///         END OF MAIN LOOP        ///
    ///////////////////////////////////////


    /// -------- CLEANUP ---------
    winapi::StopPlaying(&audio);
    winapi::ReleaseAudio(&audio);
    winapi::CloseNetwork(&network);
#if 0
    ImGui_ImplVulkan_Shutdown();
#endif
    winapi::destroy_vulkan_context(&vulkanContext);

    /// ----- Cleanup Windows
    {
        // Note(Leo): Windows will mostly clean up after us once process exits :)


        /* TODO(Leo): If there are multiple of these it means we had crash previously
        and want to reset this. This should be handled betterly though.... :( */
        timeEndPeriod(deviceMinSchedulerGranularity);
        logWindow(0) << "shut down\n";
    }

}


// int CALLBACK
// WinMain(
//     HINSTANCE   hInstance,
//     HINSTANCE   previousWinInstance,
//     LPSTR       cmdLine,
//     int         showCommand)
// {



int main()
{
    /* Todo(Leo): we should make a decision about how we handle errors etc.
    Currently there are exceptions (which lead here) and asserts mixed ;)

    Maybe, put a goto on asserts, so they would come here too? */
    try {

        HMODULE module = GetModuleHandle(nullptr);
        Run(module);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}