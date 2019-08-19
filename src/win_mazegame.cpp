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

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <stdexcept>
#include <vector>
#include <fstream>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

#include "win_VulkanDebugStrings.cpp"
#include "win_WinSocketDebugStrings.cpp"
#include "win_ErrorStrings.hpp"

#include "win_Mazegame.hpp"
#include "win_Audio.cpp"
#include "win_Network.cpp"

constexpr int32 MAX_MODEL_COUNT = 100;
constexpr int32 MAX_FRAMES_IN_FLIGHT = 2;

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

// Note(Leo): make unity build
#include "win_VulkanCommandBuffers.cpp"
#include "win_Vulkan.cpp"

constexpr char texture_path [] = "textures/chalet.jpg";

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
UpdateControllerInput(GameInput * input)
{
    local_persist bool32 xButtonWasDown = false;


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

        bool32 xButtonIsDown = (pressedButtons & XINPUT_GAMEPAD_A) != 0;

        input->jump = xButtonIsDown + 2 * xButtonWasDown;
        xButtonWasDown = xButtonIsDown;
    }
    else 
    {

    }
}

internal FILETIME
WinApiGetFileLastWriteTime(const char * fileName)
{
    WIN32_FILE_ATTRIBUTE_DATA fileInfo = {};
    if (GetFileAttributesExA(fileName, GetFileExInfoStandard, &fileInfo))
    {
    }
    else
    {
        // Todo(Leo): Now what???
    }    
    FILETIME result = fileInfo.ftLastWriteTime;
    return result;
}

internal VkSurfaceKHR
WinApiCreateVulkanSurface(VkInstance vulkanInstance, GLFWwindow * window)
{
    // TOdo(Leo): Get rid of glfw
    
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(vulkanInstance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface");
    }
    return surface;
}


class MazegameApplication
{
public:
    
    class_member void
    GamePushMeshes(void * graphicsContext, int32 meshCount, Mesh * meshArray, MeshHandle * resultMeshHandleArray)
    {
        MazegameApplication * platform = reinterpret_cast<MazegameApplication *>(graphicsContext);
        platform->PushMeshesToBuffer(   &platform->staticMeshPool, &platform->loadedModels,
                                        meshCount, meshArray, resultMeshHandleArray);
    }   

    void Run()
    {

        // ---------- INITIALIZE PLATFORM ------------
        InitializeWindow();
        VulkanInitialize();

        LoadXInput();

        HWND thisWinApiWindow = glfwGetWin32Window(window);

        // ------- MEMORY ---------------------------
        GameMemory gameMemory = {};
        {
            // TODO [MEMORY] (Leo): Properly measure required amount
            gameMemory.persistentMemorySize = Megabytes(64);
            gameMemory.transientMemorySize = Gigabytes(2);
            uint64 totalMemorySize = gameMemory.persistentMemorySize + gameMemory.transientMemorySize;
       
            // TODO [MEMORY] (Leo): Check support for large pages
            // TODO [MEMORY] (Leo): specify base address for development builds
            void * memoryBlock = VirtualAlloc(nullptr, totalMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            
            gameMemory.persistentMemory = memoryBlock;
            gameMemory.transientMemory = (uint8 *)memoryBlock + gameMemory.persistentMemorySize;

            gameMemory.PushMeshesImpl = GamePushMeshes;
            gameMemory.graphicsContext = this;
       }

       // -------- GPU MEMORY ---------------------- 
       {
            // TODO [MEMORY] (Leo): Properly measure required amount
            uint64 staticMeshPoolSize       = Gigabytes(1);
            uint64 stagingBufferPoolSize    = Megabytes(100);
            uint64 modelUniformBufferSize   = Megabytes(100);
            uint64 sceneUniformBufferSize   = Megabytes(100);
            
            // TODO[MEMORY] (Leo): This will need guarding against multithreads once we get there
            // Static mesh pool
            Vulkan::CreateBufferResource(   logicalDevice, physicalDevice, staticMeshPoolSize,
                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                            &staticMeshPool);
            // Staging buffer
            Vulkan::CreateBufferResource(   logicalDevice, physicalDevice, stagingBufferPoolSize,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                            &stagingBufferPool);

            // Uniform buffer for model matrices
            Vulkan::CreateBufferResource(   logicalDevice, physicalDevice, modelUniformBufferSize,
                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                            &modelUniformBuffer);

            // Uniform buffer for scene data
            Vulkan::CreateBufferResource(   logicalDevice, physicalDevice, sceneUniformBufferSize,
                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                            &sceneUniformBuffer);
        }

        // -------- DRAWING ---------
        int32 currentLoopingFrameIndex = 0;
        {
            CreateImageTexture();
            CreateTextureImageView();
            CreateTextureSampler();

            // Note(Leo): After buffer creations!!
            // Note(Leo): Also for now also after mockup texture creation!!
            descriptorSets = CreateDescriptorSets(  logicalDevice, descriptorPool, descriptorSetLayout,
                                                    &swapchainItems, &sceneUniformBuffer, &modelUniformBuffer,
                                                    textureImageView, textureSampler, &context); 
            CreateCommandBuffers();
        }

        // -------- INITIALIZE NETWORK ---------
        bool32 networkIsRuined = false;
        WinApiNetwork network = WinApi::CreateNetwork();
        GameNetwork gameNetwork = {};
        
        /// --------- INITIALIZE AUDIO ----------------
        WinApiAudio audio = WinApi::CreateAudio();
        WinApi::StartPlaying(&audio);                

        /// --------- TIMING ---------------------------
        auto startTimeMark = std::chrono::high_resolution_clock::now();
        real64 lastTime = 0;

        real64 networkSendDelay = 1.0 / 20;
        real64 networkNextSendTime = 0;

        /// ---------- LOAD GAME CODE ----------------------
        // Todo(Leo): Only when in development
        WinApiGame game = {};
        FILETIME dllWriteTime;
        {
            CopyFileA(GAMECODE_DLL_FILE_NAME, GAMECODE_DLL_FILE_NAME_TEMP, false);
            game.dllHandle = LoadLibraryA(GAMECODE_DLL_FILE_NAME_TEMP);
            if (game.dllHandle != nullptr)
            {
                game.Update = reinterpret_cast<WinApiGame::UpdateFunc *> (GetProcAddress(game.dllHandle, GAMECODE_UPDATE_FUNC_NAME));
                dllWriteTime = WinApiGetFileLastWriteTime(GAMECODE_DLL_FILE_NAME);
            }
        }

        /// ------- MAIN LOOP -------
        while (glfwWindowShouldClose(window) == false)
        {
            // Note(Leo): just quit now if some unhandled network error occured
            if (networkIsRuined)
            {
                auto error = WSAGetLastError();
                std::cout << "NETWORK failed: " << WinSocketErrorString(error) << " (" << error << ")\n"; 
                break;
            }

            /// -------- RELOAD GAME CODE --------
            FILETIME dllLatestWriteTime = WinApiGetFileLastWriteTime(GAMECODE_DLL_FILE_NAME);
            if (CompareFileTime(&dllLatestWriteTime, &dllWriteTime) > 0)
            {
                FreeLibrary(game.dllHandle);
                game.dllHandle =  nullptr;

                CopyFileA(GAMECODE_DLL_FILE_NAME, GAMECODE_DLL_FILE_NAME_TEMP, false);
                game.dllHandle = LoadLibraryA(GAMECODE_DLL_FILE_NAME_TEMP);
                if(game.IsLoaded())
                {
                    game.Update = reinterpret_cast<WinApiGame::UpdateFunc *>(GetProcAddress(game.dllHandle, GAMECODE_UPDATE_FUNC_NAME));
                    dllWriteTime = dllLatestWriteTime;
                }
            }


            /// --------- TIME -----------------
            real64 time;
            real64 deltaTime;
            {
                auto currentTimeMark = std::chrono::high_resolution_clock::now();
                time = std::chrono::duration<real64, std::chrono::seconds::period>(currentTimeMark - startTimeMark).count();
                deltaTime = time - lastTime;
                lastTime = time;
            }

            /// --------- HANDLE INPUT -----------
            GameInput gameInput = {};
            {
                // Note(Leo): this is not input only...
                glfwPollEvents();

                HWND foregroundWindow = GetForegroundWindow();
                bool32 windowIsActive = thisWinApiWindow == foregroundWindow;

                UpdateControllerInput(&gameInput);

                gameInput.timeDelta = deltaTime;
            }

            /// ------ PRE-UPDATE NETWORK PASS ---------
            if (network.isListening)
            {
                WinApi::NetworkListen(&network);
            }
            else if (network.isConnected)
            {
                WinApi::NetworkReceive(&network, &gameNetwork.inPackage);
            }

            /// --------- UPDATE GAME -------------
            GameRenderInfo gameRenderInfo = {};
            {
                // Note(Leo): Just recreate gamePlatformInfo each frame for now
                GamePlatformInfo gamePlatformInfo = {};
                gamePlatformInfo.screenWidth = swapchainItems.extent.width;
                gamePlatformInfo.screenHeight = swapchainItems.extent.height;

                Matrix44 modelMatrixArray [MAX_MODEL_COUNT];

                gameRenderInfo.modelMatrixArray = modelMatrixArray;
                gameRenderInfo.modelMatrixCount = loadedModels.size();

                gameNetwork.isConnected = network.isConnected;


                GameSoundOutput gameSoundOutput = {};
                WinApi::GetAudioBuffer(&audio, &gameSoundOutput.sampleCount, &gameSoundOutput.samples);


                if(game.IsLoaded())
                {
                    game.Update(&gameInput, &gameMemory, &gamePlatformInfo,
                                &gameNetwork, &gameSoundOutput, &gameRenderInfo);
                }


                WinApi::ReleaseAudioBuffer(&audio, gameSoundOutput.sampleCount);
            }


            // ----- POST-UPDATW NETWORK PASS ------
            if (network.isConnected && time > networkNextSendTime)
            {
                networkNextSendTime = time + networkSendDelay;
                WinApi::NetworkSend(&network, &gameNetwork.outPackage);
            }

            // ---- DRAW -----    
            {
                vkWaitForFences(logicalDevice, 1, &syncObjects.inFlightFences[currentLoopingFrameIndex],
                                VK_TRUE, VULKAN_NO_TIME_OUT);

                uint32 imageIndex;
                VkResult result = vkAcquireNextImageKHR(logicalDevice, swapchainItems.swapchain, MaxValue<uint64>,
                                                    syncObjects.imageAvailableSemaphores[currentLoopingFrameIndex],
                                                    VK_NULL_HANDLE, &imageIndex);

                switch (result)
                {
                    case VK_SUCCESS:
                    case VK_SUBOPTIMAL_KHR:
                        UpdateUniformBuffer(imageIndex, &gameRenderInfo);
                        DrawFrame(imageIndex, currentLoopingFrameIndex);
                        break;

                    case VK_ERROR_OUT_OF_DATE_KHR:
                        RecreateSwapchain();
                        break;

                    default:
                        throw std::runtime_error("Failed to acquire swap chain image");
                }

                currentLoopingFrameIndex = (currentLoopingFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
            }
        }

        /// -------- CLEANUP ---------
        WinApi::StopPlaying(&audio);
        WinApi::ReleaseAudio(&audio);

        WinApi::CloseNetwork(&network);

        /// ------- CLEANUP VULKAN -----
        {
            // Note(Leo): All draw frame operations are asynchronous, must wait for them to finish
            vkDeviceWaitIdle(logicalDevice);
            Cleanup();
            std::cout << "[VULKAN]: shut down\n";
        }
    }

private:
    GLFWwindow * window;
    VkInstance vulkanInstance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkDevice logicalDevice;

    VulkanContext context;

    VkSurfaceKHR surface;  // This is where we draw?
    /* Note: We need both graphics and present queue, but they might be on
    separate devices (correct???), so we may need to end up with multiple queues */
    VkQueue graphicsQueue;
    VkQueue presentQueue;


    /* Note(Leo): these images dont need explicit memory, it is managed by GPU
    We receive these from gpu for presenting. */
    std::vector<VkFramebuffer>  frameBuffers;

    VulkanSwapchainItems swapchainItems;

    VkRenderPass            renderPass;
    VulkanPipelineItems     pipelineItems;

    VkDescriptorSetLayout           descriptorSetLayout;
    VkDescriptorPool                descriptorPool;
    std::vector<VkDescriptorSet>    descriptorSets;

    VkCommandPool                   commandPool;
    std::vector<VkCommandBuffer>    commandBuffers;
    
    VulkanSyncObjects               syncObjects;

    bool32 framebufferResized = false;

    VulkanBufferResource            staticMeshPool;
    std::vector<VulkanLoadedModel>  loadedModels;
    VulkanBufferResource            stagingBufferPool;

    VulkanBufferResource modelUniformBuffer;
    VulkanBufferResource sceneUniformBuffer;

    // mockup IMAGE TEXTURE
    uint32          textureMipLevels = 1;
    VkImage         textureImage;
    VkDeviceMemory  textureImageMemory;
    VkImageView     textureImageView;

    /* Note(Leo): image and sampler are now (as in Vulkan) unrelated, and same
    sampler can be used with multiple images, noice */
    VkSampler textureSampler;
    
    // MULTISAMPLING
    VkSampleCountFlagBits msaaSamples;
    VkSampleCountFlagBits msaaMaxSamples = VK_SAMPLE_COUNT_2_BIT;

    VulkanDrawingResources drawingResources;

    void InitializeWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, true);

        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Maze Game", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);

        auto FramebufferResizeCallback = [] (GLFWwindow * window, int width, int height)
        {
            auto application = reinterpret_cast<MazegameApplication *>(glfwGetWindowUserPointer(window));
            application->framebufferResized = true;
        };

        glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
    }

    void 
    VulkanInitialize()
    {
        vulkanInstance = CreateInstance();
        // TODO(Leo): (if necessary, but at this point) Setup debug callbacks, look vulkan-tutorial.com
        surface = WinApiCreateVulkanSurface(vulkanInstance, window);

        physicalDevice = PickPhysicalDevice(vulkanInstance, surface);
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        msaaSamples = GetMaxUsableMsaaSampleCount(physicalDevice);
        std::cout << "Sample count: " << VulkanSampleCountFlagBitsString(msaaSamples) << "\n";

        context.physicalDevice = physicalDevice;
        context.physicalDeviceProperties = physicalDeviceProperties;

        logicalDevice = CreateLogicalDevice(physicalDevice, surface);

        VulkanQueueFamilyIndices queueFamilyIndices = Vulkan::FindQueueFamilies(physicalDevice, surface);
        vkGetDeviceQueue(logicalDevice, queueFamilyIndices.graphics, 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, queueFamilyIndices.present, 0, &presentQueue);

        /// ---- Create Command Pool ----
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphics;
        poolInfo.flags = 0;

        if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create command pool");
        }

        /// ....
        syncObjects = CreateSyncObjects(logicalDevice);
        
        /* 
        Above is device
        -----------------------------------------------------------------------
        Below is content
        */ 

        swapchainItems      = CreateSwapchainAndImages(physicalDevice, logicalDevice, surface, window);
        renderPass          = CreateRenderPass(logicalDevice, physicalDevice, &swapchainItems, msaaSamples);
        descriptorSetLayout = CreateDescriptorSetLayout(logicalDevice);
        pipelineItems       = CreateGraphicsPipeline(logicalDevice, descriptorSetLayout, renderPass, &swapchainItems, msaaSamples);
        drawingResources    = CreateDrawingResources(logicalDevice, physicalDevice, commandPool, graphicsQueue, &swapchainItems, msaaSamples);
        frameBuffers        = CreateFrameBuffers(logicalDevice, renderPass, &swapchainItems, &drawingResources);
        descriptorPool      = CreateDescriptorPool(logicalDevice, swapchainItems.images.size());

        std::cout << "\nVulkan Initialized succesfully\n\n";
    }

    void
    CleanupSwapchain()
    {
        vkDestroyImage (logicalDevice, drawingResources.colorImage, nullptr);
        vkDestroyImageView(logicalDevice, drawingResources.colorImageView, nullptr);
        vkDestroyImage (logicalDevice, drawingResources.depthImage, nullptr);
        vkDestroyImageView (logicalDevice, drawingResources.depthImageView, nullptr);
        vkFreeMemory(logicalDevice, drawingResources.memory, nullptr);

        vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);

        for (auto framebuffer : frameBuffers)
        {
            vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
        }

        vkDestroyPipeline(logicalDevice, pipelineItems.pipeline, nullptr);
        vkDestroyPipelineLayout(logicalDevice, pipelineItems.layout, nullptr);
        vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

        for (auto imageView : swapchainItems.imageViews)
        {
            vkDestroyImageView(logicalDevice, imageView, nullptr);
        }

        vkDestroySwapchainKHR(logicalDevice, swapchainItems.swapchain, nullptr);
    }



    void
    Cleanup()
    {
        CleanupSwapchain();

        vkDestroyImage(logicalDevice, textureImage, nullptr);
        vkFreeMemory(logicalDevice, textureImageMemory, nullptr);
        vkDestroyImageView(logicalDevice, textureImageView, nullptr);

        vkDestroySampler(logicalDevice, textureSampler, nullptr);

        vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);  

        Vulkan::DestroyBufferResource(logicalDevice, &staticMeshPool);
        Vulkan::DestroyBufferResource(logicalDevice, &stagingBufferPool);
        Vulkan::DestroyBufferResource(logicalDevice, &modelUniformBuffer);
        Vulkan::DestroyBufferResource(logicalDevice, &sceneUniformBuffer);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            vkDestroySemaphore(logicalDevice, syncObjects.renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(logicalDevice, syncObjects.imageAvailableSemaphores[i], nullptr);

            vkDestroyFence(logicalDevice, syncObjects.inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

        vkDestroyDevice(logicalDevice, nullptr);
        vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
        vkDestroyInstance(vulkanInstance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    internal std::vector<VkFramebuffer>
    CreateFrameBuffers(
        VkDevice                    device, 
        VkRenderPass                renderPass,
        VulkanSwapchainItems *      swapchainItems,
        VulkanDrawingResources *    drawingResources)
    {   
        /* Note(Leo): This is basially allocating right, there seems to be no
        need for VkDeviceMemory for swapchainimages??? */
        
        int imageCount = swapchainItems->imageViews.size();
        // frameBuffers.resize(imageCount);
        std::vector<VkFramebuffer> resultFrameBuffers (imageCount);

        for (int i = 0; i < imageCount; ++i)
        {
            constexpr int ATTACHMENT_COUNT = 3;
            VkImageView attachments[ATTACHMENT_COUNT] = {
                drawingResources->colorImageView,
                drawingResources->depthImageView,
                swapchainItems->imageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
            framebufferInfo.renderPass      = renderPass;
            framebufferInfo.attachmentCount = ATTACHMENT_COUNT;
            framebufferInfo.pAttachments    = &attachments[0];
            framebufferInfo.width           = swapchainItems->extent.width;
            framebufferInfo.height          = swapchainItems->extent.height;
            framebufferInfo.layers          = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &resultFrameBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer");
            }
        }

        return resultFrameBuffers;
    }


    // Note(Leo): expect image layout be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    void
    CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32 width, uint32 height)
    {
        VkCommandBuffer commandBuffer = Vulkan::BeginOneTimeCommandBuffer(logicalDevice, commandPool);

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage (commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        Vulkan::EndOneTimeCommandBuffer (logicalDevice, commandPool, graphicsQueue, commandBuffer);
    }

    uint32
    ComputeMipmapLevels(uint32 texWidth, uint32 texHeight)
    {
       uint32 result = std::floor(std::log2(std::max(texWidth, texHeight))) + 1;
       return result;
    }

    void
    GenerateMipMaps(VkImage image, VkFormat imageFormat, uint32 texWidth, uint32 texHeight, uint32 mipLevels)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == false)
        {
            throw std::runtime_error("Texture image format does not support blitting!");
        }


        VkCommandBuffer commandBuffer = Vulkan::BeginOneTimeCommandBuffer(logicalDevice, commandPool);

        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int mipWidth = texWidth;
        int mipHeight = texHeight;

        for (int i = 1; i <mipLevels; ++i)
        {
            int newMipWidth = mipWidth > 1 ? mipWidth / 2 : 1;
            int newMipHeight = mipHeight > 1 ? mipHeight / 2 : 1;

            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);


            VkImageBlit blit = {};
            
            blit.srcOffsets [0] = {0, 0, 0};
            blit.srcOffsets [1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1; 

            blit.dstOffsets [0] = {0, 0, 0};
            blit.dstOffsets [1] = {newMipWidth, newMipHeight, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier (commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                1, &barrier);

            mipWidth = newMipWidth;
            mipHeight = newMipHeight;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
            1, &barrier);

        Vulkan::EndOneTimeCommandBuffer (logicalDevice, commandPool, graphicsQueue, commandBuffer);
    }

    void
    CreateImageTexture()
    {
        int32 texWidth, texHeight, texChannels;
        stbi_uc * pixels = stbi_load(texture_path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        textureMipLevels = ComputeMipmapLevels(texWidth, texHeight);

        std::cout << "Mip levels: " << textureMipLevels << "\n";

        // Note(Leo): 4 applies for rgba images, duh
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if(pixels == nullptr)
        {
            throw std::runtime_error("Failed to load image");
        }

        void * data;
        vkMapMemory(logicalDevice, stagingBufferPool.memory, 0, imageSize, 0, &data);
        memcpy (data, pixels, imageSize);
        vkUnmapMemory(logicalDevice, stagingBufferPool.memory);
        stbi_image_free(pixels);

        CreateImageAndMemory(logicalDevice, physicalDevice,
                    texWidth, texHeight, textureMipLevels,
                    VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_TILING_OPTIMAL, 
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    VK_SAMPLE_COUNT_1_BIT,
                    &textureImage, &textureImageMemory);

        /* Note(Leo):
            1. change layout to copy optimal
            2. copy contents
            3. change layout to read optimal

            This last is good to use after generated textures, finally some 
            benefits from this verbose nonsense :D
        */
        // Todo(Leo): begin and end command buffers once only and then just add commands from inside these
        TransitionImageLayout(  logicalDevice, commandPool, graphicsQueue, textureImage, VK_FORMAT_R8G8B8A8_UNORM, textureMipLevels,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        CopyBufferToImage(stagingBufferPool.buffer, textureImage, texWidth, texHeight);

        /* Note(Leo): mipmap generator will be doing the transition top proper layout for now
        Eventually miplevels will be stored in file instead and we need to transition manually */
        // TransitionImageLayout(  textureImage, VK_FORMAT_R8G8B8A8_UNORM, textureMipLevels,
        //                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);        

        GenerateMipMaps(textureImage, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, textureMipLevels);
    }

    void
    CreateTextureImageView()
    {
        textureImageView = CreateImageView(logicalDevice, textureImage, textureMipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void
    CreateTextureSampler()
    {
        VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(textureMipLevels);
        samplerInfo.mipLodBias = 0.0f;

        if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create texture sampler!");
        }
    }

    void
    PushMeshesToBuffer (
        VulkanBufferResource *              destination, 
        std::vector<VulkanLoadedModel> *    loadedModelsArray, 
        int32                               meshCount, 
        Mesh *                              meshArray, 
        MeshHandle *                        outHandleArray
    ){
        for (int meshIndex = 0; meshIndex < meshCount; ++meshIndex)
        {
            Mesh * mesh = &meshArray[meshIndex];

            uint64 indexBufferSize = mesh->indices.size() * sizeof(mesh->indices[0]);
            uint64 vertexBufferSize = mesh->vertices.size() * sizeof(mesh->vertices[0]);
            uint64 totalBufferSize = indexBufferSize + vertexBufferSize;

            uint64 indexOffset = 0;
            uint64 vertexOffset = indexBufferSize;

            uint8 * data;
            vkMapMemory(logicalDevice, stagingBufferPool.memory, 0, totalBufferSize, 0, (void**)&data);
            memcpy(data, &mesh->indices[0], indexBufferSize);
            data += indexBufferSize;
            memcpy(data, &mesh->vertices[0], vertexBufferSize);
            vkUnmapMemory(logicalDevice, stagingBufferPool.memory);

            VkCommandBuffer commandBuffer = Vulkan::BeginOneTimeCommandBuffer(logicalDevice, commandPool);

            VkBufferCopy copyRegion = { 0, destination->used, totalBufferSize };
            vkCmdCopyBuffer(commandBuffer, stagingBufferPool.buffer, destination->buffer, 1, &copyRegion);

            Vulkan::EndOneTimeCommandBuffer(logicalDevice, commandPool, graphicsQueue, commandBuffer);

            VulkanLoadedModel model = {};

            model.buffer = destination->buffer;
            model.memory = destination->memory;
            model.vertexOffset = destination->used + vertexOffset;
            model.indexOffset = destination->used + indexOffset;
            
            destination->used += totalBufferSize;

            model.indexCount = mesh->indices.size();
            model.indexType = Vulkan::ConvertIndexType(mesh->indexType);

            uint32 modelIndex = loadedModelsArray->size();

            uint32 memorySizePerModelMatrix = AlignUpTo(
                physicalDeviceProperties.limits.minUniformBufferOffsetAlignment,
                sizeof(Matrix44));
            model.uniformBufferOffset = modelIndex * memorySizePerModelMatrix;

            loadedModelsArray->push_back(model);

            MeshHandle result = modelIndex;

            outHandleArray[meshIndex] = result;
        }

        /* Todo(Leo): Is this necessary/Appropriate???
        Note(Leo) Important: CommandBuffers refer to actual number of models,
        so we need to update it everytime any number of models is added */
        RefreshCommandBuffers();
    }



    void
    RefreshCommandBuffers()
    {
        vkFreeCommandBuffers(logicalDevice, commandPool, commandBuffers.size(), &commandBuffers[0]);        
        CreateCommandBuffers();
    }

    void
    CreateCommandBuffers()
    {
        int32 framebufferCount = frameBuffers.size();
        commandBuffers.resize(framebufferCount);

        VkCommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = commandBuffers.size();

        if (vkAllocateCommandBuffers(logicalDevice, &allocateInfo, &commandBuffers[0]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers");
        }

        std::cout << "Allocated command buffers\n";

        int32 modelCount = loadedModels.size();
        std::vector<VulkanRenderInfo> infos (modelCount);

        for (int modelIndex = 0; modelIndex < modelCount; ++modelIndex)
        {
            infos[modelIndex].meshBuffer             = loadedModels[modelIndex].buffer; 
            infos[modelIndex].vertexOffset           = loadedModels[modelIndex].vertexOffset;
            infos[modelIndex].indexOffset            = loadedModels[modelIndex].indexOffset;
            infos[modelIndex].indexCount             = loadedModels[modelIndex].indexCount;
            infos[modelIndex].indexType              = loadedModels[modelIndex].indexType;
            infos[modelIndex].uniformBufferOffset    = loadedModels[modelIndex].uniformBufferOffset;
        }

        for (int frameIndex = 0; frameIndex < commandBuffers.size(); ++frameIndex)
        {
            VkCommandBuffer commandBuffer = commandBuffers[frameIndex];

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr;

            if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to begin recording command buffer");
            }

            std::cout << "Started recording command buffers\n";
    
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = frameBuffers[frameIndex];
            
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapchainItems.extent;

            // Todo(Leo): These should also use constexpr ids or unscoped enums
            VkClearValue clearValues [2] = {};
            clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};

            renderPassInfo.clearValueCount = 2;
            renderPassInfo.pClearValues = &clearValues [0];

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineItems.pipeline);

            for (int i = 0; i < modelCount; ++i)
            {
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &infos[i].meshBuffer, &infos[i].vertexOffset);
                vkCmdBindIndexBuffer(commandBuffer, infos[i].meshBuffer, infos[i].indexOffset, infos[i].indexType);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineItems.layout, 0, 1,
                                        &descriptorSets[frameIndex], 1, &infos[i].uniformBufferOffset);
                vkCmdDrawIndexed(commandBuffer, infos[i].indexCount, 1, 0, 0, 0);
            }

            std::cout << "Recorded commandBuffers for " << modelCount << " models\n";

            // DONE

            vkCmdEndRenderPass(commandBuffer);

            if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to record command buffer");
            }
        }
    }

    void
    UpdateUniformBuffer(uint32 imageIndex, GameRenderInfo * renderInfo)
    {
        // Todo(Leo): Single mapping is really enough, offsets can be used here too
        Matrix44 * pModelMatrix;
        uint32 startUniformBufferOffset = GetModelUniformBufferOffsetForSwapchainImages(&context, imageIndex);

        int32 modelCount = loadedModels.size();
        for (int modelIndex = 0; modelIndex < modelCount; ++modelIndex)
        {
            vkMapMemory(logicalDevice, modelUniformBuffer.memory, 
                        startUniformBufferOffset + loadedModels[modelIndex].uniformBufferOffset,
                        sizeof(pModelMatrix), 0, (void**)&pModelMatrix);

            *pModelMatrix = renderInfo->modelMatrixArray[modelIndex];

            vkUnmapMemory(logicalDevice, modelUniformBuffer.memory);
        }

        // Note (Leo): map vulkan memory directly to right type so we can easily avoid one (small) memcpy per frame
        VulkanCameraUniformBufferObject * pUbo;
        vkMapMemory(logicalDevice, sceneUniformBuffer.memory,
                    GetSceneUniformBufferOffsetForSwapchainImages(&context, imageIndex),
                    sizeof(VulkanCameraUniformBufferObject), 0, (void**)&pUbo);

        pUbo->view          = renderInfo->cameraView;
        pUbo->perspective   = renderInfo->cameraPerspective;

        vkUnmapMemory(logicalDevice, sceneUniformBuffer.memory);
    }

    bool32
    IsSwapChainUpToDate ()
    {
        return true;
    }

    void
    DrawFrame(uint32 imageIndex, uint32 frameIndex)
    {
        VkSubmitInfo submitInfo[1] = {};
        submitInfo[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores [] = { syncObjects.imageAvailableSemaphores[frameIndex] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo[0].waitSemaphoreCount = 1;
        submitInfo[0].pWaitSemaphores = waitSemaphores;
        submitInfo[0].pWaitDstStageMask = waitStages;

        submitInfo[0].commandBufferCount = 1;
        submitInfo[0].pCommandBuffers = &commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = { syncObjects.renderFinishedSemaphores[frameIndex] };
        submitInfo[0].signalSemaphoreCount = ARRAY_COUNT(signalSemaphores);
        submitInfo[0].pSignalSemaphores = signalSemaphores;

        /* Note(Leo): reset here, so that if we have recreated swapchain above,
        our fences will be left in signaled state */
        vkResetFences(logicalDevice, 1, &syncObjects.inFlightFences[frameIndex]);

        if (vkQueueSubmit(graphicsQueue, 1, submitInfo, syncObjects.inFlightFences[frameIndex]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer");
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = ARRAY_COUNT(signalSemaphores);
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.pResults = nullptr;

        VkSwapchainKHR swapchains [] = { swapchainItems.swapchain };
        presentInfo.swapchainCount = ARRAY_COUNT(swapchains);
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex; // Todo(Leo): also an array, one for each swapchain


        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

        // Todo, Study(Leo): This may not be needed, 
        // Note(Leo): Do after presenting to not interrupt semaphores at whrong time
        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
        {
            framebufferResized = false;
            RecreateSwapchain();
        }
        else if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swapchain");
        }
    }

    void
    RecreateSwapchain()
    {
        int width = 0;
        int height = 0;
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(logicalDevice);

        CleanupSwapchain();

        swapchainItems      = CreateSwapchainAndImages(physicalDevice, logicalDevice, surface, window);
        renderPass          = CreateRenderPass(logicalDevice, physicalDevice, &swapchainItems, msaaSamples);
        pipelineItems       = CreateGraphicsPipeline(logicalDevice, descriptorSetLayout, renderPass, &swapchainItems, msaaSamples);
        drawingResources    = CreateDrawingResources(logicalDevice, physicalDevice, commandPool, graphicsQueue, &swapchainItems, msaaSamples);
        frameBuffers        = CreateFrameBuffers(logicalDevice, renderPass, &swapchainItems, &drawingResources);
        descriptorPool      = CreateDescriptorPool(logicalDevice, swapchainItems.images.size());
        descriptorSets      = CreateDescriptorSets(  logicalDevice, descriptorPool, descriptorSetLayout,
                                                    &swapchainItems, &sceneUniformBuffer, &modelUniformBuffer,
                                                    textureImageView, textureSampler, &context); 

        RefreshCommandBuffers();
    }
};


int main(int argc, char ** argv)
{
    if (argc == 3)
    {
        networkOwnIpAddress = argv[1];
        networkOtherIpAddress = argv[2];

        std::cout
            << "Set ip addresses:\n"
            << "\town   " << networkOwnIpAddress << "\n"
            << "\tother " << networkOtherIpAddress << "\n\n\n";
    }

    try {
        MazegameApplication app = {};
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}