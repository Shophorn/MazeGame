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

internal void
LoadXInput()
{
    // TODO(Leo): check other xinput versions and pick most proper
    HMODULE xinputModule = LoadLibraryA("xinput1_3.dll");

    if (xinputModule != nullptr)
    {
        XInputGetState_ = reinterpret_cast<XInputGetStateFunc *> (GetProcAddress(xinputModule, "XInputGetState"));
    }
    else
    {
        XInputGetState_ = XInputGetStateStub;
    }
}

internal real32
ReadXInputJoystickValue(int16 value)
{
    real32 result = static_cast<real32>(value) / MaxValue<int16>;
    return result;
}


internal void
UpdateControllerInput(game::Input * input)
{
    local_persist bool32 aButtonWasDown = false;

    // We use pointer instead of return value, since we store other data to input too.
    // Note(Leo): Only get input from first controller, locally this is a single player game
    XINPUT_STATE controllerState;

    // Note(Leo): Use global controller index depending on network status to test multiplayering
    // DWORD result = XInputGetState(0, &controllerState);
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

        bool32 zoomIn = (pressedButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
        bool32 zoomOut = (pressedButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;

        input->zoomIn = zoomIn && !zoomOut;
        input->zoomOut = zoomOut && !zoomIn;

        bool32 aButtonIsDown = (pressedButtons & XINPUT_GAMEPAD_A) != 0;

        input->jump = static_cast<game::InputButtonState>(aButtonIsDown + 2 * aButtonWasDown);
        aButtonWasDown = aButtonIsDown;
    }
    else 
    {

    }
}



namespace winapi
{
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
        resultContext.descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MATERIAL]        = CreateMaterialDescriptorSetLayout(resultContext.device);
        resultContext.descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MODEL_UNIFORM]   = CreateModelDescriptorSetLayout(resultContext.device);

        resultContext.pipelineItems       = CreateGraphicsPipeline(resultContext.device, resultContext.descriptorSetLayouts, 3, resultContext.renderPass, &resultContext.swapchainItems, resultContext.msaaSamples);
        resultContext.drawingResources    = CreateDrawingResources(resultContext.device, resultContext.physicalDevice, resultContext.commandPool, resultContext.graphicsQueue, &resultContext.swapchainItems, resultContext.msaaSamples);
        resultContext.frameBuffers        = CreateFrameBuffers(resultContext.device, resultContext.renderPass, &resultContext.swapchainItems, &resultContext.drawingResources);
        resultContext.uniformDescriptorPool      = CreateDescriptorPool(resultContext.device, resultContext.swapchainItems.images.size(), resultContext.textureCount);
        resultContext.materialDescriptorPool      = CreateDescriptorPool(resultContext.device, resultContext.swapchainItems.images.size(), resultContext.textureCount);

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
    {
        // TODO [MEMORY] (Leo): Properly measure required amount
        gameMemory.persistentMemorySize = Megabytes(64);
        gameMemory.transientMemorySize  = Gigabytes(2);
        uint64 totalMemorySize = gameMemory.persistentMemorySize + gameMemory.transientMemorySize;
   
        // TODO [MEMORY] (Leo): Check support for large pages
        // TODO [MEMORY] (Leo): specify base address for development builds
        void * memoryBlock = VirtualAlloc(nullptr, totalMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        
        gameMemory.persistentMemory     = memoryBlock;
        gameMemory.transientMemory      = (uint8 *)memoryBlock + gameMemory.persistentMemorySize;
   }

   // -------- GPU MEMORY ---------------------- 
   {
        // TODO [MEMORY] (Leo): Properly measure required amount
        // TODO[memory] (Leo): Log usage
        uint64 staticMeshPoolSize       = Gigabytes(1);
        uint64 stagingBufferPoolSize    = Megabytes(100);
        uint64 modelUniformBufferSize   = Megabytes(100);
        uint64 sceneUniformBufferSize   = Megabytes(100);

        // TODO[MEMORY] (Leo): This will need guarding against multithreads once we get there
        // Static mesh pool
        vulkan::CreateBufferResource(   context.device, context.physicalDevice, staticMeshPoolSize,
                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                        &context.staticMeshPool);
        // Staging buffer
        vulkan::CreateBufferResource(   context.device, context.physicalDevice, stagingBufferPoolSize,
                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                        &context.stagingBufferPool);

        // Uniform buffer for model matrices
        vulkan::CreateBufferResource(   context.device, context.physicalDevice, modelUniformBufferSize,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                        &context.modelUniformBuffer);

        // Uniform buffer for scene data
        vulkan::CreateBufferResource(   context.device, context.physicalDevice, sceneUniformBufferSize,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                        &context.sceneUniformBuffer);
    }

    // -------- DRAWING ---------
    {
        // Note(Leo): After buffer creations!!
        context.descriptorSets = CreateModelDescriptorSets( &context, context.uniformDescriptorPool,
                                                    context.descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MODEL_UNIFORM],
                                                    &context.swapchainItems, &context.modelUniformBuffer); 

        context.sceneDescriptorSets = CreateSceneDescriptorSets(&context, context.uniformDescriptorPool,
                                                        context.descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_SCENE_UNIFORM],
                                                        &context.swapchainItems, &context.sceneUniformBuffer);
        
        context.textureSampler = CreateTextureSampler(&context);

        CreateCommandBuffers(&context);
    }
    int32 currentLoopingFrameIndex = 0;

    // -------- INITIALIZE NETWORK ---------
    bool32 networkIsRuined = false;
    WinApiNetwork network = winapi::CreateNetwork();
    game::Network gameNetwork = {};
    
    /// --------- INITIALIZE AUDIO ----------------
    WinApiAudio audio = winapi::CreateAudio();
    winapi::StartPlaying(&audio);                

    /// --------- TIMING ---------------------------
    auto startTimeMark = std::chrono::high_resolution_clock::now();
    real64 lastTime = 0;
    real64 frameTimeSeconds = 1.0 / 60.0;

    real64 networkSendDelay = 1.0 / 20;
    real64 networkNextSendTime = 0;

    /// ---------- LOAD GAME CODE ----------------------
    // Todo(Leo): Only when in development
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

    /// ------- MAIN LOOP -------
    state.isRunning = true;
    while(state.isRunning)
    {
        // Note(Leo): just quit now if some unhandled network error occured
        if (networkIsRuined)
        {
            auto error = WSAGetLastError();
            std::cout << "NETWORK failed: " << WinSocketErrorString(error) << " (" << error << ")\n"; 
            break;
        }

        /// --------- TIME -----------------
        real64 time;
        real64 deltaTime;
        real64 frameStartTime;
        {
            auto currentTimeMark = std::chrono::high_resolution_clock::now();
            time = std::chrono::duration<real64, std::chrono::seconds::period>(currentTimeMark - startTimeMark).count();
            frameStartTime = time;
            deltaTime = time - lastTime;
            lastTime = time;
        }

        /// -------- RELOAD GAME CODE --------
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


        /// --------- HANDLE INPUT -----------
        game::Input gameInput = {};
        {
            // Note(Leo): this is not input only...
            winapi::ProcessPendingMessages(&state, winWindow);

            HWND foregroundWindow = GetForegroundWindow();
            bool32 windowIsActive = winWindow == foregroundWindow;

            UpdateControllerInput(&gameInput);
            gameInput.timeDelta = deltaTime;
        }

        /// ------ PRE-UPDATE NETWORK PASS ---------
        if (network.isListening)
        {
            winapi::NetworkListen(&network);
        }
        else if (network.isConnected)
        {
            winapi::NetworkReceive(&network, &gameNetwork.inPackage);
        }

        /// --------- UPDATE GAME -------------
        game::RenderInfo gameRenderInfo = {};
        {
            gamePlatformInfo.windowWidth = context.swapchainItems.extent.width;
            gamePlatformInfo.windowHeight = context.swapchainItems.extent.height;

            Matrix44 modelMatrixArray [VULKAN_MAX_MODEL_COUNT];

            gameRenderInfo.modelMatrixCount = context.loadedModels.size();
            gameRenderInfo.modelMatrixArray = modelMatrixArray;

            gameNetwork.isConnected = network.isConnected;


            game::SoundOutput gameSoundOutput = {};
            winapi::GetAudioBuffer(&audio, &gameSoundOutput.sampleCount, &gameSoundOutput.samples);


            if(game.IsLoaded())
            {
                game.Update(&gameInput, &gameMemory, &gamePlatformInfo,
                            &gameNetwork, &gameSoundOutput, &gameRenderInfo);
            }

            winapi::ReleaseAudioBuffer(&audio, gameSoundOutput.sampleCount);
        }


        // ----- POST-UPDATE NETWORK PASS ------
        if (network.isConnected && time > networkNextSendTime)
        {
            networkNextSendTime = time + networkSendDelay;
            winapi::NetworkSend(&network, &gameNetwork.outPackage);
        }

        /// ---- DRAW -----    
        /*
        Note(Leo): Only draw image if we have window that is not minimized. Vulkan on windows MUST not
        have framebuffer size 0, which minimized window would result in.

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
                    vulkan::UpdateUniformBuffer(&context, imageIndex, &gameRenderInfo);
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

        {
        #if 0
            // Note(Leo): debug print time
            auto currentTimeMark = std::chrono::high_resolution_clock::now();
            real64 currentTimeSeconds = std::chrono::duration<real64, std::chrono::seconds::period>(currentTimeMark - startTimeMark).count();
            real64 frameTimeElapsed = currentTimeSeconds - frameStartTime;

            std::cout << frameTimeSeconds << "s\n";
        #endif
        }

    }

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
    /// ----- Cleanup Window
    // Note(Leo): Windows will clean up after us once process exits :)
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