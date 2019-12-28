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

#include <stdexcept>
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

// Todo(Leo): hack to get two controller on single pc and still use 1st controller when online
global_variable int globalXinputControllerIndex;

#include "winapi_WindowMessages.cpp"
#include "winapi_VulkanDebugStrings.cpp"
#include "winapi_WinSocketDebugStrings.cpp"
#include "winapi_ErrorStrings.hpp"
#include "winapi_Mazegame.hpp"

constexpr int32 WINDOW_WIDTH = 960;
constexpr int32 WINDOW_HEIGHT = 540;

// Todo(Leo): Vulkan implementation depends on this, not cool
using BinaryAsset = std::vector<uint8>;
BinaryAsset
ReadBinaryFile (const char * fileName)
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

constexpr static int32
    DESCRIPTOR_SET_LAYOUT_SCENE_UNIFORM = 0,
    DESCRIPTOR_SET_LAYOUT_MATERIAL      = 1,
    DESCRIPTOR_SET_LAYOUT_MODEL_UNIFORM = 2;


// Note(Leo): make unity build
#include "winapi_VulkanCommandBuffers.cpp"
#include "winapi_Vulkan.cpp"
#include "winapi_Audio.cpp"
#include "winapi_Network.cpp"

// XInput things.
using XInputGetStateFunc = decltype(XInputGetState);
internal XInputGetStateFunc * XInputGetState_;
DWORD WINAPI XInputGetStateStub (DWORD, XINPUT_STATE *)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

/*Todo(Leo): see if we should get rid of #define below by creating
a proper named struct/namespace to hold this pointer */
#define XInputGetState XInputGetState_

using XInputSetStateFunc = decltype(XInputSetState);
internal XInputSetStateFunc * XInputSetState_;
DWORD WINAPI XInputSetStateStub (DWORD, XINPUT_VIBRATION *)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

/*Todo(Leo): see if we should get rid of #define below by creating
a proper named struct/namespace to hold this pointer */
#define XInputSetState XInputSetState_

internal void
LoadXInput()
{
    // TODO(Leo): check other xinput versions and pick most proper
    HMODULE xinputModule = LoadLibraryA("xinput1_3.dll");

    if (xinputModule != nullptr)
    {
        XInputGetState_ = reinterpret_cast<XInputGetStateFunc *> (GetProcAddress(xinputModule, "XInputGetState"));
        XInputSetState_ = reinterpret_cast<XInputSetStateFunc *> (GetProcAddress(xinputModule, "XInputSetState"));
    }
    else
    {
        XInputGetState_ = XInputGetStateStub;
        XInputSetState_ = XInputSetStateStub;
    }
}

internal float
ReadXInputJoystickValue(int16 value)
{
    float deadZone = 0.2f;
    float result = static_cast<float>(value) / MaxValue<int16>;

    float sign = Sign(result);
    result *= sign; // cheap abs()
    result -= deadZone;
    result = Max(0.0f, result);
    result /= (1.0f - deadZone);
    result *= sign;

    return result;
}



internal void
UpdateControllerInput(game::Input * input)
{
    /* TODO[Input](Leo): Obviously not to stay static for too long, but when we
    remove the qualifier, we need to also take care of controllers being connected
    and disconnected and possibly ending at different index. 

    TODO [Input](Leo): Test controller and index behaviour when being connected and
    disconnected */
    local_persist bool32
        aButtonWasDown          = false,
        bButtonWasDown          = false,

        startButtonWasDown      = false,
        backButtonWasDown       = false,
        zoomInButtonWasDown     = false,
        zoomOutButtonWasDown    = false,

        leftButtonWasDown       = false,
        rightButtonWasDown      = false,
        downButtonWasDown       = false,
        upButtonWasDown         = false;


    /* Note(Leo): Only get input from a single controller, locally this is a single
    player game. Use global controller index depending on network status to test
    multiplayering */
    XINPUT_STATE controllerState;
    DWORD result = XInputGetState(globalXinputControllerIndex, &controllerState);

    if (result == ERROR_SUCCESS)
    {
        input->move = { 
            ReadXInputJoystickValue(controllerState.Gamepad.sThumbLX),
            ReadXInputJoystickValue(controllerState.Gamepad.sThumbLY)};
        input->move = ClampLength(input->move, 1.0f);

        input->look = {
            ReadXInputJoystickValue(controllerState.Gamepad.sThumbRX),
            ReadXInputJoystickValue(controllerState.Gamepad.sThumbRY)};
        input->look = ClampLength(input->look, 1.0f);

        uint16 pressedButtons = controllerState.Gamepad.wButtons;
        auto ProcessButton = [pressedButtons](uint32 button, bool32 * wasDown) -> game::InputButtonState
        {
            bool32 isDown = (pressedButtons & button) != 0;
            auto result = game::InputButtonState::FromCurrentAndPrevious(7 * isDown, 18 * (*wasDown));
            *wasDown = isDown;
            return result;
        };

        input->jump     = ProcessButton(XINPUT_GAMEPAD_A, &aButtonWasDown);
        input->confirm  = ProcessButton(XINPUT_GAMEPAD_B, &bButtonWasDown);
        input->interact = input->confirm;

        input->start    = ProcessButton(XINPUT_GAMEPAD_START, &startButtonWasDown);
        input->select   = ProcessButton(XINPUT_GAMEPAD_BACK, &backButtonWasDown);
        input->zoomIn   = ProcessButton(XINPUT_GAMEPAD_LEFT_SHOULDER, &zoomInButtonWasDown);
        input->zoomOut  = ProcessButton(XINPUT_GAMEPAD_RIGHT_SHOULDER, &zoomInButtonWasDown);

        input->left     = ProcessButton(XINPUT_GAMEPAD_DPAD_LEFT, &leftButtonWasDown);
        input->right    = ProcessButton(XINPUT_GAMEPAD_DPAD_RIGHT, &rightButtonWasDown);
        input->down     = ProcessButton(XINPUT_GAMEPAD_DPAD_DOWN, &downButtonWasDown);
        input->up       = ProcessButton(XINPUT_GAMEPAD_DPAD_UP, &upButtonWasDown);
    }
    else 
    {
        // No controller?????
    }
}



namespace winapi
{
    internal void
    SetWindowedOrFullscreen(HWND winWindow, winapi::State * state, bool32 setWindowed)
    {
        // Study(Leo): https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
        DWORD style = GetWindowLongPtr(winWindow, GWL_STYLE);
        bool32 isWindowed = (style & WS_OVERLAPPEDWINDOW);

        if (isWindowed && setWindowed == false)
        {
            // Todo(Leo): Actually use this value to check for potential errors
            bool32 success;

            success = GetWindowPlacement(winWindow, &state->windowedWindowPosition); 

            HMONITOR monitor = MonitorFromWindow(winWindow, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
            success = GetMonitorInfoW(monitor, &monitorInfo);

            DWORD fullScreenStyle = (style & ~WS_OVERLAPPEDWINDOW);
            SetWindowLongPtrW(winWindow, GWL_STYLE, fullScreenStyle);

            // Note(Leo): Remember that height value starts from top and grows towards bottom
            LONG left = monitorInfo.rcMonitor.left;
            LONG top = monitorInfo.rcMonitor.top;
            LONG width = monitorInfo.rcMonitor.right - left;
            LONG heigth = monitorInfo.rcMonitor.bottom - top;
            SetWindowPos(winWindow, HWND_TOP, left, top, width, heigth, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

            state->windowIsFullscreen = true;
        }

        else if (isWindowed == false && setWindowed)
        {
            SetWindowLongPtr(winWindow, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
            SetWindowPlacement(winWindow, &state->windowedWindowPosition);
            SetWindowPos(   winWindow, NULL, 0, 0, 0, 0,
                            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

            state->windowIsFullscreen = false;
        }
    }

    internal FILETIME
    GetFileLastWriteTime(const char * fileName)
    {
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

    internal VkSurfaceKHR
    CreateVulkanSurface(VkInstance vulkanInstance, HINSTANCE winInstance, HWND winWindow)
    {
        VkSurfaceKHR surface;

        // Todo(Leo): check that extensions are enabled
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        surfaceCreateInfo.hinstance = winInstance;
        surfaceCreateInfo.hwnd = winWindow;

        if (vkCreateWin32SurfaceKHR (vulkanInstance, &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface");
        }

        return surface;
    }

    // Remove. Or fix. This is merely a remnant of time when we used glfw
    internal VkExtent2D
    GetFrameBufferSize(void * window)
    {
        return {WINDOW_WIDTH, WINDOW_HEIGHT};
    }


    internal VulkanContext
    VulkanInitialize(HINSTANCE winInstance, HWND winWindow)
    {
        VulkanContext resultContext = {};

        resultContext.instance = CreateInstance();
        // TODO(Leo): (if necessary, but at this point) Setup debug callbacks, look vulkan-tutorial.com
        
        resultContext.surface = winapi::CreateVulkanSurface(resultContext.instance, winInstance, winWindow);


        resultContext.physicalDevice = PickPhysicalDevice(resultContext.instance, resultContext.surface);
        vkGetPhysicalDeviceProperties(resultContext.physicalDevice, &resultContext.physicalDeviceProperties);
        resultContext.msaaSamples = GetMaxUsableMsaaSampleCount(resultContext.physicalDevice);
        std::cout << "Sample count: " << vulkan::EnumToString(resultContext.msaaSamples) << "\n";

        resultContext.physicalDevice = resultContext.physicalDevice;
        resultContext.physicalDeviceProperties = resultContext.physicalDeviceProperties;

        resultContext.device = CreateLogicalDevice(resultContext.physicalDevice, resultContext.surface);

        VulkanQueueFamilyIndices queueFamilyIndices = vulkan::FindQueueFamilies(resultContext.physicalDevice, resultContext.surface);
        vkGetDeviceQueue(resultContext.device, queueFamilyIndices.graphics, 0, &resultContext.graphicsQueue);
        vkGetDeviceQueue(resultContext.device, queueFamilyIndices.present, 0, &resultContext.presentQueue);

        /// ---- Create Command Pool ----
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphics;
        poolInfo.flags = 0;

        if (vkCreateCommandPool(resultContext.device, &poolInfo, nullptr, &resultContext.commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create command pool");
        }

        /// ...?
        resultContext.syncObjects = CreateSyncObjects(resultContext.device);
        
        /* 
        Above is device
        -----------------------------------------------------------------------
        Below is content
        */ 


        /* Todo(Leo): now that everything is in VulkanContext, it does not make so much sense
        to explicitly specify each component as parameter */
        resultContext.swapchainItems      = CreateSwapchainAndImages(&resultContext, winapi::GetFrameBufferSize(nullptr));
        resultContext.renderPass          = CreateRenderPass(&resultContext, &resultContext.swapchainItems, resultContext.msaaSamples);
        
        resultContext.descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_SCENE_UNIFORM]   = CreateSceneDescriptorSetLayout(resultContext.device);
        resultContext.descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MATERIAL]       = CreateMaterialDescriptorSetLayout(resultContext.device);
        resultContext.descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MODEL_UNIFORM]   = CreateModelDescriptorSetLayout(resultContext.device);

        resultContext.guiDescriptorSetLayout = CreateModelDescriptorSetLayout(resultContext.device);

        // Todo(Leo): This seems rather stupid way to organize creation of these...
        resultContext.pipelineItems             = CreateGraphicsPipeline(&resultContext, 3);
        resultContext.guiPipelineItems          = CreateGuiPipeline(&resultContext, 3);
        resultContext.drawingResources          = CreateDrawingResources(&resultContext);
        resultContext.frameBuffers              = CreateFrameBuffers(&resultContext);
        resultContext.uniformDescriptorPool     = CreateDescriptorPool(resultContext.device, resultContext.swapchainItems.images.size());
        resultContext.materialDescriptorPool    = CreateMaterialDescriptorPool(resultContext.device);

        std::cout << "\nVulkan Initialized succesfully\n\n";

        return resultContext;
    }

    internal State * 
    GetStateFromWindow(HWND winWindow)
    {
        State * state = reinterpret_cast<State *> (GetWindowLongPtr(winWindow, GWLP_USERDATA));
        return state;
    }

    // Note(Leo): CALLBACK specifies calling convention for 32-bit applications
    // https://stackoverflow.com/questions/15126454/what-does-the-callback-keyword-mean-in-a-win-32-c-application
    internal LRESULT CALLBACK
    MainWindowCallback (HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        LRESULT result = 0;

        switch (message)
        {
            case WM_CREATE:
            {
                CREATESTRUCTW * pCreate = reinterpret_cast<CREATESTRUCTW *>(lParam);
                winapi::State * pState = reinterpret_cast<winapi::State *>(pCreate->lpCreateParams);
                SetWindowLongPtrW (window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pState));
            } break;

            case WM_SIZE:
            {
                winapi::State * state = winapi::GetStateFromWindow(window);
                state->windowWidth = LOWORD(lParam);
                state->windowHeight = HIWORD(lParam);

                switch (wParam)
                {
                    case SIZE_RESTORED:
                    case SIZE_MAXIMIZED:
                        state->windowIsMinimized = false;
                        break;

                    case SIZE_MINIMIZED:
                        state->windowIsMinimized = true;
                        break;
                }

            } break;

            case WM_CLOSE:
            {
                winapi::State * state = winapi::GetStateFromWindow(window);
                state->isRunning = false;
                std::cout << "window callback: WM_CLOSE\n";
            } break;

            case WM_EXITSIZEMOVE:
            {
                winapi::State * state = winapi::GetStateFromWindow(window);
                std::cout << "Window exit sizemove, width: " << state->windowWidth << ", height: " << state->windowHeight << "\n";
            } break;

            default:
                result = DefWindowProcW(window, message, wParam, lParam);
        }
        return result;
    }

    internal void
    ProcessPendingMessages(winapi::State * state, HWND winWindow)
    {
        // Note(Leo): Apparently Windows requires us to do this.

        MSG message;
        while (PeekMessageW(&message, winWindow, 0, 0, PM_REMOVE))
        {
            switch (message.message)
            {
                default:
                    TranslateMessage(&message);
                    DispatchMessage(&message);
            }
        }
    }
}

internal void
Run(HINSTANCE winInstance)
{
    winapi::State state = {};
    LoadXInput();

    // ---------- INITIALIZE PLATFORM ------------
    HWND winWindow;
    VulkanContext context;
    game::PlatformInfo gamePlatformInfo = {};
    {
        wchar windowClassName [] = L"MazegameWindowClass";
        wchar windowTitle [] = L"Mazegame";

        WNDCLASSW windowClass = {};
        windowClass.style           = CS_VREDRAW | CS_HREDRAW;
        windowClass.lpfnWndProc     = winapi::MainWindowCallback;
        windowClass.hInstance       = winInstance;
        windowClass.lpszClassName   = windowClassName;

        // Study: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-makeintresourcew
        auto defaultArrowCursor = MAKEINTRESOURCEW(32512);
        windowClass.hCursor = LoadCursorW(nullptr, defaultArrowCursor);

        if (RegisterClassW(&windowClass) == 0)
        {
            // Todo(Leo): Logging and backup plan for class invalidness
            std::cout << "Failed to register window class\n";
            return;
        }

        winWindow = CreateWindowExW (
            0, windowClassName, windowTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
            nullptr, nullptr, winInstance, &state);

        if (winWindow == nullptr)
        {
            // Todo[Logging] (Leo): Log this and make backup plan
            std::cout << "Failed to create window\n";
            return;
        }

        context = winapi::VulkanInitialize(winInstance, winWindow);
        gamePlatformInfo.graphicsContext = &context;
    }

    // ------- MEMORY ---------------------------
    game::Memory gameMemory = {};
    MemoryArena platformTransientMemoryArena;
    {
        // TODO [MEMORY] (Leo): Properly measure required amount
        // TODO [MEMORY] (Leo): Think of alignment
        gameMemory.persistentMemorySize = Gigabytes(1);
        gameMemory.transientMemorySize  = Gigabytes(1);

        uint64 totalGameMemorySize = gameMemory.persistentMemorySize + gameMemory.transientMemorySize;

        uint64 platformTransientMemorySize = Megabytes(1);
        uint64 totalMemorySize = totalGameMemorySize + platformTransientMemorySize;
   
        // TODO [MEMORY] (Leo): Check support for large pages
        // TODO [MEMORY] (Leo): specify base address for development builds
    #if MAZEGAME_DEVELOPMENT
        void * memoryBlock = VirtualAlloc((void*)Terabytes(2), totalMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    #else
        void * memoryBlock = VirtualAlloc(nullptr, totalMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    #endif

        gameMemory.persistentMemory     = memoryBlock;
        gameMemory.transientMemory      = (byte *)memoryBlock + gameMemory.persistentMemorySize;

        platformTransientMemoryArena = make_memory_arena((byte *)memoryBlock + totalGameMemorySize, platformTransientMemorySize);

    }

   // -------- GPU MEMORY ---------------------- 
   {
        // TODO [MEMORY] (Leo): Properly measure required amount
        // TODO[memory] (Leo): Log usage
        uint64 staticMeshPoolSize       = Megabytes(500);
        uint64 stagingBufferPoolSize    = Megabytes(100);
        uint64 modelUniformBufferSize   = Megabytes(100);
        uint64 sceneUniformBufferSize   = Megabytes(100);
        uint64 guiUniformBufferSize     = Megabytes(100);

        // TODO[MEMORY] (Leo): This will need guarding against multithreads once we get there
        // Static mesh pool
        vulkan::CreateBufferResource(   &context, staticMeshPoolSize,
                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                        &context.staticMeshPool);
        // Staging buffer
        vulkan::CreateBufferResource(   &context, stagingBufferPoolSize,
                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                        &context.stagingBufferPool);

        // Uniform buffer for model matrices. This means every model including scenery.
        vulkan::CreateBufferResource(   &context, modelUniformBufferSize,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                        &context.modelUniformBuffer);

        // Uniform buffer for scene data, ie. camera, lights etc.
        vulkan::CreateBufferResource(   &context, sceneUniformBufferSize,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                        &context.sceneUniformBuffer);

        // Uniform buffer for gui elements
        vulkan::CreateBufferResource(   &context, guiUniformBufferSize,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                        &context.guiUniformBuffer);
    }

    // -------- DRAWING ---------
    {
        // Note(Leo): After buffer creations!!
        context.descriptorSets      = CreateModelDescriptorSets(&context);
        context.guiDescriptorSets   = CreateGuiDescriptorSets(&context);
        context.sceneDescriptorSets = CreateSceneDescriptorSets(&context);
        context.textureSampler      = CreateTextureSampler(&context);
        CreateCommandBuffers(&context);
    }
    int32 currentLoopingFrameIndex = 0;

    game::RenderInfo gameRenderInfo = {};
    vulkan::RenderInfo platformRenderInfo = {};
    {
        /* Todo(Leo): We are currently pushing for worst case. It is ok because
        push_array is super cheap and memory is available anyway. However we may
        want to be more exact */
        platformRenderInfo.renderedObjects = push_array<Matrix44, RenderedObjectHandle>(
                                            &platformTransientMemoryArena, VULKAN_MAX_MODEL_COUNT);
        platformRenderInfo.guiObjects = push_array<Matrix44, GuiHandle>(
                                        &platformTransientMemoryArena, VULKAN_MAX_MODEL_COUNT);

        // Todo(Leo): define these somewhere else as proper functions and not lamdas??
        gameRenderInfo.render = [&platformRenderInfo](RenderedObjectHandle handle, Matrix44 transform)
        {
            platformRenderInfo.renderedObjects[handle] = transform;
        };
        gameRenderInfo.render_gui = [&platformRenderInfo](GuiHandle handle, Matrix44 transform)
        {
            platformRenderInfo.guiObjects[handle] = transform;
        };
        gameRenderInfo.set_camera = [&platformRenderInfo](Matrix44 view, Matrix44 perspective)
        {
            platformRenderInfo.cameraView = view;
            platformRenderInfo.cameraPerspective = perspective;
        };

    }

    // -------- INITIALIZE NETWORK ---------
    bool32 networkIsRuined = false;
    WinApiNetwork network = winapi::CreateNetwork();
    game::Network gameNetwork = {};
    real64 networkSendDelay     = 1.0 / 20;
    real64 networkNextSendTime  = 0;
    
    /// --------- INITIALIZE AUDIO ----------------
    WinApiAudio audio = winapi::CreateAudio();
    winapi::StartPlaying(&audio);                

    /// --------- TIMING ---------------------------
    auto startTimeMark          = std::chrono::high_resolution_clock::now();
    real64 lastFrameStartTime   = 0;

    MAZEGAME_NO_INIT real64 targetFrameTime;
    MAZEGAME_NO_INIT uint32 deviceMinSchedulerGranularity;
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

        real64 targetFrameTimeYesPower = 60;
        real64 targetFrameTimeNoPower = 30;

        constexpr BYTE AC_ONLINE = 1;
        real64 targetFrameTimeThreshold = (powerStatus.ACLineStatus == AC_ONLINE) ? targetFrameTimeYesPower : targetFrameTimeNoPower;

        if (powerStatus.ACLineStatus == AC_ONLINE)
        {
            std::cout << "Power detected\n";
        }
        else
        {
            std::cout << "No power detected\n";
        }

        int32 gameUpdateRate = targetFrameTimeThreshold;
        {
            HDC deviceContext = GetDC(winWindow);
            int monitorRefreshRate = GetDeviceCaps(deviceContext, VREFRESH);
            ReleaseDC(winWindow, deviceContext);

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
        
        std::cout << "Target frame time = " << targetFrameTime << ", (fps = " << 1.0 / targetFrameTime << ")\n";
    }

    /// ---------- LOAD GAME CODE ----------------------
    // Note(Leo): Only load game dynamically in development.
    winapi::Game game = {};
    FILETIME dllWriteTime;
    {
        CopyFileA(GAMECODE_DLL_FILE_NAME, GAMECODE_DLL_FILE_NAME_TEMP, false);
        game.dllHandle = LoadLibraryA(GAMECODE_DLL_FILE_NAME_TEMP);
        if (game.dllHandle != nullptr)
        {
            game.Update = reinterpret_cast<winapi::Game::UpdateFunc *> (GetProcAddress(game.dllHandle, GAMECODE_UPDATE_FUNC_NAME));
            dllWriteTime = winapi::GetFileLastWriteTime(GAMECODE_DLL_FILE_NAME);
        }
    }

    ////////////////////////////////////////////////////
    ///             MAIN LOOP                        ///
    ////////////////////////////////////////////////////
    state.isRunning = true;
    while(state.isRunning)
    {

        /// ----- START TIME -----
        auto currentTimeMark = std::chrono::high_resolution_clock::now();
        real64 frameStartTime = std::chrono::duration<real64, std::chrono::seconds::period>(currentTimeMark - startTimeMark).count();

        /// ----- RELOAD GAME CODE -----
        FILETIME dllLatestWriteTime = winapi::GetFileLastWriteTime(GAMECODE_DLL_FILE_NAME);
        if (CompareFileTime(&dllLatestWriteTime, &dllWriteTime) > 0)
        {
            FreeLibrary(game.dllHandle);
            game.dllHandle =  nullptr;

            CopyFileA(GAMECODE_DLL_FILE_NAME, GAMECODE_DLL_FILE_NAME_TEMP, false);
            game.dllHandle = LoadLibraryA(GAMECODE_DLL_FILE_NAME_TEMP);
            if(game.IsLoaded())
            {
                game.Update = reinterpret_cast<winapi::Game::UpdateFunc *>(GetProcAddress(game.dllHandle, GAMECODE_UPDATE_FUNC_NAME));
                dllWriteTime = dllLatestWriteTime;
            }

        }

        /// ----- HANDLE INPUT -----
        game::Input gameInput = {};
        {
            // Note(Leo): this is not input only...
            winapi::ProcessPendingMessages(&state, winWindow);

            HWND foregroundWindow = GetForegroundWindow();
            bool32 windowIsActive = winWindow == foregroundWindow;

            UpdateControllerInput(&gameInput);
            gameInput.elapsedTime = targetFrameTime;
        }

        /// ----- PRE-UPDATE NETWORK PASS -----
        {
            // Todo(Leo): just quit now if some unhandled network error occured
            if (networkIsRuined)
            {
                auto error = WSAGetLastError();
                std::cout << "NETWORK failed: " << WinSocketErrorString(error) << " (" << error << ")\n"; 
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
        }

        /// --------- UPDATE GAME -------------
        {
            gamePlatformInfo.windowWidth = context.swapchainItems.extent.width;
            gamePlatformInfo.windowHeight = context.swapchainItems.extent.height;
            gamePlatformInfo.windowIsFullscreen = state.windowIsFullscreen;

            gameNetwork.isConnected = network.isConnected;

            game::SoundOutput gameSoundOutput = {};
            winapi::GetAudioBuffer(&audio, &gameSoundOutput.sampleCount, &gameSoundOutput.samples);

            if(game.IsLoaded())
            {
                game::UpdateResult updateResult = game.Update(  &gameInput, &gameMemory,
                                                                &gamePlatformInfo, &gameNetwork, 
                                                                &gameSoundOutput, &gameRenderInfo);

                if (updateResult.exit)
                {
                    state.isRunning = false;
                }

                switch (updateResult.setWindow)
                {
                    case game::UpdateResult::SET_WINDOW_FULLSCREEN:
                        winapi::SetWindowedOrFullscreen(winWindow, &state, false);
                        break;

                    case game::UpdateResult::SET_WINDOW_WINDOWED:
                        winapi::SetWindowedOrFullscreen(winWindow, &state, true);
                        break;

                    case game::UpdateResult::SET_WINDOW_NONE:
                        break;
                }

            }

            winapi::ReleaseAudioBuffer(&audio, gameSoundOutput.sampleCount);
        }


        // ----- POST-UPDATE NETWORK PASS ------
        // TODO[network](Leo): Now that we have fixed frame rate, we should count frames passed instead of time
        if (network.isConnected && frameStartTime > networkNextSendTime)
        {
            networkNextSendTime = frameStartTime + networkSendDelay;
            winapi::NetworkSend(&network, &gameNetwork.outPackage);
        }

        /// ---- DRAW -----    
        /*
        Note(Leo): Only draw image if we have window that is not minimized. Vulkan on windows MUST not
        have framebuffer size 0, which is what minimized window is.

        'currentLoopingFrameIndex' does not need to be incremented if we do not draw since it refers to
        next available swapchain frame/image, and we do not use one if we do not draw.
        */
        if (state.isRunning && state.windowIsDrawable())
        {
            // Todo(Leo): Study fences
            vkWaitForFences(context.device, 1, &context.syncObjects.inFlightFences[currentLoopingFrameIndex],
                            VK_TRUE, VULKAN_NO_TIME_OUT);

            uint32 imageIndex;
            VkResult result = vkAcquireNextImageKHR(context.device, context.swapchainItems.swapchain, MaxValue<uint64>,
                                                    context.syncObjects.imageAvailableSemaphores[currentLoopingFrameIndex],
                                                    VK_NULL_HANDLE, &imageIndex);

            switch (result)
            {
                case VK_SUCCESS:
                    vulkan::UpdateUniformBuffer(&context, imageIndex, &platformRenderInfo);
                    vulkan::DrawFrame(&context, imageIndex, currentLoopingFrameIndex);
                    break;

                case VK_SUBOPTIMAL_KHR:
                case VK_ERROR_OUT_OF_DATE_KHR:

                    vulkan::RecreateSwapchain(&context, state.GetFrameBufferSize());
                    break;

                default:
                    throw std::runtime_error("Failed to acquire swap chain image");
            }

            // Todo(Leo): should this be in switch case where we draw succesfully???
            currentLoopingFrameIndex = (currentLoopingFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        /// ----- CLEAR MEMORY ------
        flush_memory_arena(&platformTransientMemoryArena);

        /// ----- WAIT FOR TARGET FRAMETIME -----
        {
            auto currentTimeMark = std::chrono::high_resolution_clock::now();
            real64 currentTimeSeconds = std::chrono::duration<real64, std::chrono::seconds::period>(currentTimeMark - startTimeMark).count();

            real64 elapsedSeconds = currentTimeSeconds - frameStartTime;
            real64 timeToSleepSeconds = targetFrameTime - elapsedSeconds;

            /* TODO[time](Leo): It seems okay to sleep 0 milliseconds in case the time to sleep ends up being
            less than 1 millisecond on floating point representation. Also we may want to do a busy loop over
            remainder time after sleep. This is due to windows scheduler granularity, that is at best on 
            one millisecond scale. */
            if (timeToSleepSeconds > 0)
            {
                DWORD timeToSleepMilliSeconds = static_cast<DWORD>(1000 * timeToSleepSeconds);
                // std::cout << "Sleep for " << timeToSleepMilliSeconds << " ms\n";
                Sleep(timeToSleepMilliSeconds);

            }
        }
    }
    ///////////////////////////////////////
    ///         END OF MAIN LOOP        ///
    ///////////////////////////////////////


    /// -------- CLEANUP ---------
    winapi::StopPlaying(&audio);
    winapi::ReleaseAudio(&audio);
    winapi::CloseNetwork(&network);

    /// ------- CLEANUP VULKAN -----
    {
        // Note(Leo): All draw frame operations are asynchronous, must wait for them to finish
        vkDeviceWaitIdle(context.device);
        Cleanup(&context);

        std::cout << "[VULKAN]: shut down\n";
    }
    /// ----- Cleanup Windows
    {
        // Note(Leo): Windows will mostly clean up after us once process exits :)


        /* TODO(Leo): If there are multiple of these it means we had crash previously
        and want to reset this. This should be handled betterly though.... :( */
        timeEndPeriod(deviceMinSchedulerGranularity);
        std::cout << "[WINDOWS]: shut down\n";
    }
}


int CALLBACK
WinMain(
    HINSTANCE   winInstance,
    HINSTANCE   previousWinInstance,
    LPSTR       cmdLine,
    int         showCommand)
{
    try {
        Run(winInstance);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}