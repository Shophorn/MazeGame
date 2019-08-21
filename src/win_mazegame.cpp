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
        // Todo(Leo): Now what??? Getting file time failed --> file does not exist??
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


internal void
PushMeshesToBuffer (
    VulkanContext *                     context,
    VulkanBufferResource *              stagingBuffer,
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
        vkMapMemory(context->device, stagingBuffer->memory, 0, totalBufferSize, 0, (void**)&data);
        memcpy(data, &mesh->indices[0], indexBufferSize);
        data += indexBufferSize;
        memcpy(data, &mesh->vertices[0], vertexBufferSize);
        vkUnmapMemory(context->device, stagingBuffer->memory);

        VkCommandBuffer commandBuffer = Vulkan::BeginOneTimeCommandBuffer(context->device, context->commandPool);

        VkBufferCopy copyRegion = { 0, destination->used, totalBufferSize };
        vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, destination->buffer, 1, &copyRegion);

        Vulkan::EndOneTimeCommandBuffer(context->device, context->commandPool, context->graphicsQueue, commandBuffer);

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
            context->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment,
            sizeof(Matrix44));
        model.uniformBufferOffset = modelIndex * memorySizePerModelMatrix;

        loadedModelsArray->push_back(model);

        MeshHandle result = modelIndex;

        outHandleArray[meshIndex] = result;
    }

}

internal TextureAsset
LoadTextureAsset(const char * assetPath)
{
    TextureAsset resultTexture = {};
    stbi_uc * pixels = stbi_load(assetPath, &resultTexture.width, &resultTexture.height,
                                        &resultTexture.channels, STBI_rgb_alpha);

    if(pixels == nullptr)
    {
        // Todo[Error](Leo): Proper handling and logging
        throw std::runtime_error("Failed to load image");
    }

    // rgba channels
    resultTexture.channels = 4;

    int32 pixelCount = resultTexture.width * resultTexture.height;
    resultTexture.pixels.resize(pixelCount);

    uint64 imageMemorySize = resultTexture.width * resultTexture.height * resultTexture.channels;
    memcpy((uint8*)resultTexture.pixels.data(), pixels, imageMemorySize);

    stbi_image_free(pixels);
    return resultTexture;
}

constexpr static int32
    DESCRIPTOR_SET_LAYOUT_SCENE_UNIFORM = 0,
    DESCRIPTOR_SET_LAYOUT_MATERIAL = 1,
    DESCRIPTOR_SET_LAYOUT_MODEL_UNIFORM = 2;

class MazegameApplication
{
public:
    
    class_member void
    GamePushMeshes(void * graphicsContext, int32 meshCount, Mesh * meshArray, MeshHandle * resultMeshHandleArray)
    {
        MazegameApplication * platform = reinterpret_cast<MazegameApplication *>(graphicsContext);
        PushMeshesToBuffer(&platform->context,  &platform->stagingBufferPool, &platform->staticMeshPool, &platform->loadedModels,
                                        meshCount, meshArray, resultMeshHandleArray);
    
        /* Todo(Leo): Is this necessary/Appropriate???
        Note(Leo) Important: CommandBuffers has reference to actual current number of models,
        so we need to update it everytime any number of models is added */
        platform->RefreshCommandBuffers();
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
            // TODO[memory] (Leo): Log usage
            uint64 staticMeshPoolSize       = Gigabytes(1);
            uint64 stagingBufferPoolSize    = Megabytes(100);
            uint64 modelUniformBufferSize   = Megabytes(100);
            uint64 sceneUniformBufferSize   = Megabytes(100);

            // TODO[MEMORY] (Leo): This will need guarding against multithreads once we get there
            // Static mesh pool
            Vulkan::CreateBufferResource(   context.device, context.physicalDevice, staticMeshPoolSize,
                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                            &staticMeshPool);
            // Staging buffer
            Vulkan::CreateBufferResource(   context.device, context.physicalDevice, stagingBufferPoolSize,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                            &stagingBufferPool);

            // Uniform buffer for model matrices
            Vulkan::CreateBufferResource(   context.device, context.physicalDevice, modelUniformBufferSize,
                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                            &modelUniformBuffer);

            // Uniform buffer for scene data
            Vulkan::CreateBufferResource(   context.device, context.physicalDevice, sceneUniformBufferSize,
                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                            &sceneUniformBuffer);
        }

        // -------- DRAWING ---------
        int32 currentLoopingFrameIndex = 0;
        {
            TextureAsset textureAssets [] = {
                LoadTextureAsset("textures/chalet.jpg"),
                LoadTextureAsset("textures/lava.jpg"),
                LoadTextureAsset("textures/texture.jpg"),
            };

            texture = CreateImageTexture(&textureAssets[0], &context, &stagingBufferPool);
            texture2 = CreateImageTexture(&textureAssets[1], &context, &stagingBufferPool);
            texture3 = CreateImageTexture(&textureAssets[2], &context, &stagingBufferPool);


            textureSampler = CreateTextureSampler(&context);

            // Note(Leo): After buffer creations!!
            // Note(Leo): Also for now also after mockup texture creation!!
            descriptorSets = CreateModelDescriptorSets(  &context, descriptorPool, descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MODEL_UNIFORM],
                                                    &swapchainItems, &sceneUniformBuffer, &modelUniformBuffer,
                                                    texture.view, texture2.view, textureSampler); 

            sceneDescriptorSets 
                = CreateSceneDescriptorSets(&context, descriptorPool, descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_SCENE_UNIFORM],
                                            &swapchainItems, &sceneUniformBuffer);
            materialDescriptorSets
                = CreateMaterialDescriptorSets( &context, descriptorPool, descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MATERIAL],
                                                &swapchainItems, texture.view, texture2.view, textureSampler);

            materialDescriptorSets2
                = CreateMaterialDescriptorSets( &context, descriptorPool, descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MATERIAL],
                                                &swapchainItems, texture3.view, texture2.view, textureSampler); 

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

                Matrix44 modelMatrixArray [VULKAN_MAX_MODEL_COUNT];

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
                vkWaitForFences(context.device, 1, &syncObjects.inFlightFences[currentLoopingFrameIndex],
                                VK_TRUE, VULKAN_NO_TIME_OUT);

                uint32 imageIndex;
                VkResult result = vkAcquireNextImageKHR(context.device, swapchainItems.swapchain, MaxValue<uint64>,
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
            vkDeviceWaitIdle(context.device);
            Cleanup();
            std::cout << "[VULKAN]: shut down\n";
        }
    }

    GLFWwindow * window;
    VulkanContext context;

    // Per frame objects
    std::vector<VkCommandBuffer>    frameCommandBuffers;
    std::vector<VkFramebuffer>      frameBuffers;
    std::vector<VkDescriptorSet>    descriptorSets;
    std::vector<VkDescriptorSet>    sceneDescriptorSets;
    std::vector<VkDescriptorSet>    materialDescriptorSets;
    std::vector<VkDescriptorSet>    materialDescriptorSets2;

    VulkanSwapchainItems swapchainItems;

    VulkanPipelineItems     pipelineItems;
    VkRenderPass            renderPass;

    VkDescriptorSetLayout descriptorSetLayouts [3];

    VkDescriptorPool                descriptorPool;
    
    VulkanSyncObjects               syncObjects;

    bool32 framebufferResized = false;

    VulkanBufferResource            staticMeshPool;
    std::vector<VulkanLoadedModel>  loadedModels;
    VulkanBufferResource            stagingBufferPool;

    VulkanBufferResource modelUniformBuffer;
    VulkanBufferResource sceneUniformBuffer;

    // mockup IMAGE TEXTURE
    VulkanTexture texture;
    VulkanTexture texture2;
    VulkanTexture texture3;

    int32 textureCount = 3;

    /* Note(Leo): image and sampler are now (as in Vulkan) unrelated, and same
    sampler can be used with multiple images, noice */
    VkSampler textureSampler;
    
    // MULTISAMPLING
    VkSampleCountFlagBits msaaSamples;
    VkSampleCountFlagBits msaaMaxSamples = VK_SAMPLE_COUNT_2_BIT;

    /* Note(Leo): color and depth images for initial writing. These are
    afterwards resolved to actual framebufferimage */
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
        context.instance = CreateInstance();
        // TODO(Leo): (if necessary, but at this point) Setup debug callbacks, look vulkan-tutorial.com
        context.surface = WinApiCreateVulkanSurface(context.instance, window);

        context.physicalDevice = PickPhysicalDevice(context.instance, context.surface);
        vkGetPhysicalDeviceProperties(context.physicalDevice, &context.physicalDeviceProperties);
        msaaSamples = GetMaxUsableMsaaSampleCount(context.physicalDevice);
        std::cout << "Sample count: " << VulkanSampleCountFlagBitsString(msaaSamples) << "\n";

        context.physicalDevice = context.physicalDevice;
        context.physicalDeviceProperties = context.physicalDeviceProperties;

        context.device = CreateLogicalDevice(context.physicalDevice, context.surface);

        VulkanQueueFamilyIndices queueFamilyIndices = Vulkan::FindQueueFamilies(context.physicalDevice, context.surface);
        vkGetDeviceQueue(context.device, queueFamilyIndices.graphics, 0, &context.graphicsQueue);
        vkGetDeviceQueue(context.device, queueFamilyIndices.present, 0, &context.presentQueue);

        /// ---- Create Command Pool ----
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphics;
        poolInfo.flags = 0;

        if (vkCreateCommandPool(context.device, &poolInfo, nullptr, &context.commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create command pool");
        }

        /// ....
        syncObjects = CreateSyncObjects(context.device);
        
        /* 
        Above is device
        -----------------------------------------------------------------------
        Below is content
        */ 

        swapchainItems      = CreateSwapchainAndImages(&context, window);
        renderPass          = CreateRenderPass(&context, &swapchainItems, msaaSamples);
        
        descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_SCENE_UNIFORM]   = CreateSceneDescriptorSetLayout(context.device);
        descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MATERIAL]        = CreateMaterialDescriptorSetLayout(context.device);
        descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MODEL_UNIFORM]   = CreateModelDescriptorSetLayout(context.device);

        pipelineItems       = CreateGraphicsPipeline(   context.device, descriptorSetLayouts, 3, renderPass, &swapchainItems, msaaSamples);
        drawingResources    = CreateDrawingResources(context.device, context.physicalDevice, context.commandPool, context.graphicsQueue, &swapchainItems, msaaSamples);
        frameBuffers        = CreateFrameBuffers(context.device, renderPass, &swapchainItems, &drawingResources);
        descriptorPool      = CreateDescriptorPool(context.device, swapchainItems.images.size(), textureCount);

        std::cout << "\nVulkan Initialized succesfully\n\n";
    }

    void
    CleanupSwapchain()
    {
        vkDestroyImage (context.device, drawingResources.colorImage, nullptr);
        vkDestroyImageView(context.device, drawingResources.colorImageView, nullptr);
        vkDestroyImage (context.device, drawingResources.depthImage, nullptr);
        vkDestroyImageView (context.device, drawingResources.depthImageView, nullptr);
        vkFreeMemory(context.device, drawingResources.memory, nullptr);

        vkDestroyDescriptorPool(context.device, descriptorPool, nullptr);

        for (auto framebuffer : frameBuffers)
        {
            vkDestroyFramebuffer(context.device, framebuffer, nullptr);
        }

        vkDestroyPipeline(context.device, pipelineItems.pipeline, nullptr);
        vkDestroyPipelineLayout(context.device, pipelineItems.layout, nullptr);
        vkDestroyRenderPass(context.device, renderPass, nullptr);

        for (auto imageView : swapchainItems.imageViews)
        {
            vkDestroyImageView(context.device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(context.device, swapchainItems.swapchain, nullptr);
    }

    void
    Cleanup()
    {
        CleanupSwapchain();

        DestroyImageTexture(&context, &texture);
        DestroyImageTexture(&context, &texture2);
        DestroyImageTexture(&context, &texture3);

        vkDestroySampler(context.device, textureSampler, nullptr);

        vkDestroyDescriptorSetLayout(context.device, descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MODEL_UNIFORM], nullptr);  
        vkDestroyDescriptorSetLayout(context.device, descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_SCENE_UNIFORM], nullptr);  
        vkDestroyDescriptorSetLayout(context.device, descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MATERIAL], nullptr);  

        Vulkan::DestroyBufferResource(context.device, &staticMeshPool);
        Vulkan::DestroyBufferResource(context.device, &stagingBufferPool);
        Vulkan::DestroyBufferResource(context.device, &modelUniformBuffer);
        Vulkan::DestroyBufferResource(context.device, &sceneUniformBuffer);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            vkDestroySemaphore(context.device, syncObjects.renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(context.device, syncObjects.imageAvailableSemaphores[i], nullptr);

            vkDestroyFence(context.device, syncObjects.inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(context.device, context.commandPool, nullptr);

        vkDestroyDevice(context.device, nullptr);
        vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
        vkDestroyInstance(context.instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void
    RefreshCommandBuffers()
    {
        vkFreeCommandBuffers(context.device, context.commandPool, frameCommandBuffers.size(), &frameCommandBuffers[0]);        
        CreateCommandBuffers();
    }

    void
    CreateCommandBuffers()
    {
        int32 framebufferCount = frameBuffers.size();
        frameCommandBuffers.resize(framebufferCount);

        VkCommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = context.commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = frameCommandBuffers.size();

        if (vkAllocateCommandBuffers(context.device, &allocateInfo, &frameCommandBuffers[0]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers");
        }

        std::cout << "Allocated command buffers\n";

        int32 modelCount = loadedModels.size();
        std::vector<VulkanRenderInfo> renderInfos (modelCount);

        uint32 materialIndex = 0;

        for (int modelIndex = 0; modelIndex < modelCount; ++modelIndex)
        {
            renderInfos[modelIndex].meshBuffer             = loadedModels[modelIndex].buffer; 
            renderInfos[modelIndex].vertexOffset           = loadedModels[modelIndex].vertexOffset;
            renderInfos[modelIndex].indexOffset            = loadedModels[modelIndex].indexOffset;
            renderInfos[modelIndex].indexCount             = loadedModels[modelIndex].indexCount;
            renderInfos[modelIndex].indexType              = loadedModels[modelIndex].indexType;
            renderInfos[modelIndex].uniformBufferOffset    = loadedModels[modelIndex].uniformBufferOffset;
            renderInfos[modelIndex].materialIndex          = modelIndex % 2;
        }

        for (int frameIndex = 0; frameIndex < frameCommandBuffers.size(); ++frameIndex)
        {
            VkCommandBuffer commandBuffer = frameCommandBuffers[frameIndex];

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr;

            if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to begin recording command buffer");
            }

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


            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineItems.layout,
                                    0, 1, &sceneDescriptorSets[frameIndex], 0, nullptr);

            for (int modelIndex = 0; modelIndex < modelCount; ++modelIndex)
            {
                // Bind material
                if (renderInfos[modelIndex].materialIndex == 0)
                {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineItems.layout,
                                            1, 1, &materialDescriptorSets[frameIndex], 0,   nullptr);
                }
                else
                {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineItems.layout,
                                            1, 1, &materialDescriptorSets2[frameIndex], 0,   nullptr);   
                }

                // Bind model info
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &renderInfos[modelIndex].meshBuffer, &renderInfos[modelIndex].vertexOffset);
                vkCmdBindIndexBuffer(commandBuffer, renderInfos[modelIndex].meshBuffer, renderInfos[modelIndex].indexOffset, renderInfos[modelIndex].indexType);
                
                // Bind entity transform
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineItems.layout,
                                        2, 1, &descriptorSets[frameIndex], 1,&renderInfos[modelIndex].uniformBufferOffset);

                vkCmdDrawIndexed(commandBuffer, renderInfos[modelIndex].indexCount, 1, 0, 0, 0);
            }

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
        uint32 startUniformBufferOffset = Vulkan::GetModelUniformBufferOffsetForSwapchainImages(&context, imageIndex);

        int32 modelCount = loadedModels.size();
        for (int modelIndex = 0; modelIndex < modelCount; ++modelIndex)
        {
            vkMapMemory(context.device, modelUniformBuffer.memory, 
                        startUniformBufferOffset + loadedModels[modelIndex].uniformBufferOffset,
                        sizeof(pModelMatrix), 0, (void**)&pModelMatrix);

            *pModelMatrix = renderInfo->modelMatrixArray[modelIndex];

            vkUnmapMemory(context.device, modelUniformBuffer.memory);
        }

        // Note (Leo): map vulkan memory directly to right type so we can easily avoid one (small) memcpy per frame
        VulkanCameraUniformBufferObject * pUbo;
        vkMapMemory(context.device, sceneUniformBuffer.memory,
                    Vulkan::GetSceneUniformBufferOffsetForSwapchainImages(&context, imageIndex),
                    sizeof(VulkanCameraUniformBufferObject), 0, (void**)&pUbo);

        pUbo->view          = renderInfo->cameraView;
        pUbo->perspective   = renderInfo->cameraPerspective;

        vkUnmapMemory(context.device, sceneUniformBuffer.memory);
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
        submitInfo[0].pCommandBuffers = &frameCommandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = { syncObjects.renderFinishedSemaphores[frameIndex] };
        submitInfo[0].signalSemaphoreCount = ARRAY_COUNT(signalSemaphores);
        submitInfo[0].pSignalSemaphores = signalSemaphores;

        /* Note(Leo): reset here, so that if we have recreated swapchain above,
        our fences will be left in signaled state */
        vkResetFences(context.device, 1, &syncObjects.inFlightFences[frameIndex]);

        if (vkQueueSubmit(context.graphicsQueue, 1, submitInfo, syncObjects.inFlightFences[frameIndex]) != VK_SUCCESS)
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


        VkResult result = vkQueuePresentKHR(context.presentQueue, &presentInfo);

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

        vkDeviceWaitIdle(context.device);

        CleanupSwapchain();

        swapchainItems      = CreateSwapchainAndImages(&context, window);
        renderPass          = CreateRenderPass(&context, &swapchainItems, msaaSamples);
        pipelineItems       = CreateGraphicsPipeline(context.device, descriptorSetLayouts, 3, renderPass, &swapchainItems, msaaSamples);
        drawingResources    = CreateDrawingResources(context.device, context.physicalDevice, context.commandPool, context.graphicsQueue, &swapchainItems, msaaSamples);
        frameBuffers        = CreateFrameBuffers(context.device, renderPass, &swapchainItems, &drawingResources);
        descriptorPool      = CreateDescriptorPool(context.device, swapchainItems.images.size(), textureCount);

        descriptorSets
            = CreateModelDescriptorSets(&context, descriptorPool, descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MODEL_UNIFORM],
                                        &swapchainItems, &sceneUniformBuffer, &modelUniformBuffer,
                                        texture.view, texture2.view, textureSampler); 

        sceneDescriptorSets 
            = CreateSceneDescriptorSets(&context, descriptorPool, descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_SCENE_UNIFORM],
                                        &swapchainItems, &sceneUniformBuffer);
        materialDescriptorSets
            = CreateMaterialDescriptorSets( &context, descriptorPool, descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MATERIAL],
                                            &swapchainItems, texture.view, texture2.view, textureSampler);

        materialDescriptorSets2
            = CreateMaterialDescriptorSets( &context, descriptorPool, descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MATERIAL],
                                            &swapchainItems, texture3.view, texture2.view, textureSampler); 
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