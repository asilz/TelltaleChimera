#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/matrix4x4.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <set>
#include <ttc/render/vulkan2.hpp>
#include <tth/core/errno.hpp>
#include <tth/core/log.hpp>

constexpr std::array<const char *, 1> validationLayers = {"VK_LAYER_KHRONOS_validation"};
constexpr std::array<const char *, 1> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifndef DEBUG
#define DEBUG 0
#endif

enum GFXPlatformVertexAttribute
{
    eGFXPlatformAttribute_Position,
    eGFXPlatformAttribute_Normal,
    eGFXPlatformAttribute_Tangent,
    eGFXPlatformAttribute_BlendWeight,
    eGFXPlatformAttribute_BlendIndex,
    eGFXPlatformAttribute_Color,
    eGFXPlatformAttribute_TexCoord,
    eGFXPlatformAttribute_Count,
    eGFXPlatformAttribute_None = -1
};

enum GFXPlatformFormat
{
    eGFXPlatformFormat_None,
    eGFXPlatformFormat_F32,
    eGFXPlatformFormat_F32x2,
    eGFXPlatformFormat_F32x3,
    eGFXPlatformFormat_F32x4,
    eGFXPlatformFormat_F16x2,
    eGFXPlatformFormat_F16x4,
    eGFXPlatformFormat_S32,
    eGFXPlatformFormat_U32,
    eGFXPlatformFormat_S32x2,
    eGFXPlatformFormat_U32x2,
    eGFXPlatformFormat_S32x3,
    eGFXPlatformFormat_U32x3,
    eGFXPlatformFormat_S32x4,
    eGFXPlatformFormat_U32x4,
    eGFXPlatformFormat_S16,
    eGFXPlatformFormat_U16,
    eGFXPlatformFormat_S16x2,
    eGFXPlatformFormat_U16x2,
    eGFXPlatformFormat_S16x4,
    eGFXPlatformFormat_U16x4,
    eGFXPlatformFormat_SN16,
    eGFXPlatformFormat_UN16,
    eGFXPlatformFormat_SN16x2,
    eGFXPlatformFormat_UN16x2,
    eGFXPlatformFormat_SN16x4,
    eGFXPlatformFormat_UN16x4,
    eGFXPlatformFormat_S8,
    eGFXPlatformFormat_U8,
    eGFXPlatformFormat_S8x2,
    eGFXPlatformFormat_U8x2,
    eGFXPlatformFormat_S8x4,
    eGFXPlatformFormat_U8x4,
    eGFXPlatformFormat_SN8,
    eGFXPlatformFormat_UN8,
    eGFXPlatformFormat_SN8x2,
    eGFXPlatformFormat_UN8x2,
    eGFXPlatformFormat_SN8x4,
    eGFXPlatformFormat_UN8x4,
    eGFXPlatformFormat_SN10_SN11_SN11,
    eGFXPlatformFormat_SN10x3_SN2,
    eGFXPlatformFormat_UN10x3_UN2,
    eGFXPlatformFormat_D3DCOLOR,
    eGFXPlatformFormat_Count
};

enum eGFXPlatformBufferUsage
{
    eGFXPlatformBuffer_None = 0x0,
    eGFXPlatformBuffer_Vertex = 0x1,
    eGFXPlatformBuffer_Index = 0x2,
    eGFXPlatformBuffer_Uniform = 0x4,
    eGFXPlatformBuffer_ShaderRead = 0x8,
    eGFXPlatformBuffer_ShaderWrite = 0x10,
    eGFXPlatformBuffer_ShaderReadWrite = 0x18,
    eGFXPlatformBuffer_ShaderRawAccess = 0x20,
    eGFXPlatformBuffer_ShaderReadRaw = 0x28,
    eGFXPlatformBuffer_ShaderWriteRaw = 0x30,
    eGFXPlatformBuffer_ShaderReadWriteRaw = 0x38,
    eGFXPlatformBuffer_DrawIndirectArgs = 0x40,
    eGFXPlatformBuffer_SingleValue = 0x80
};

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() { return {.binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}; }
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescription()
    {
        return std::array<VkVertexInputAttributeDescription, 3>{
            VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos)},
            VkVertexInputAttributeDescription{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, color)},
            VkVertexInputAttributeDescription{.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, texCoord)}};
    };
};

struct VertexD3D
{
    uint16_t position[4];

    static std::array<VkVertexInputBindingDescription, 3> getBindingDescription()
    {
        return std::array<VkVertexInputBindingDescription, 3>{
            VkVertexInputBindingDescription{.binding = 0, .stride = sizeof(VertexD3D), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
            VkVertexInputBindingDescription{.binding = 1, .stride = sizeof(uint8_t) * 4, .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
            VkVertexInputBindingDescription{.binding = 2, .stride = sizeof(float) * 4, .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
        };
    }
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescription()
    {
        return std::array<VkVertexInputAttributeDescription, 3>{
            VkVertexInputAttributeDescription{.location = 0, .binding = 0, .format = VK_FORMAT_R16G16B16A16_UNORM, .offset = offsetof(VertexD3D, position)},
            VkVertexInputAttributeDescription{.location = 1, .binding = 1, .format = VK_FORMAT_R8G8B8A8_UINT, .offset = offsetof(VertexD3D, position)},
            VkVertexInputAttributeDescription{.location = 2, .binding = 2, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(VertexD3D, position)},
        };
    }
};

static const Vertex vertices[] = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},   {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
};

template <size_t N, size_t M> struct Matrix
{
    float data[N][M];

    float *operator[](size_t index) { return data[index]; }
};

struct UniformBufferObject
{
    glm::mat4x4 baseTransforms[256];
    glm::mat4x4 boneTransforms[256];
    glm::mat4x4 model;
    glm::mat4x4 view;
    glm::mat4x4 proj;
    glm::mat4x4 vertexTransform;
    int boneCount;
};

struct JointTransform
{
    glm::mat4x4 transform;
};

static const uint32_t indexData[] = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

int64_t Renderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    return -1;
}

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    for (const auto &availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }
    return availableFormats[0]; // TODO: Use ranking system
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, SDL_Window *window)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    VkExtent2D actualExtent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) { return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR; }

VkResult QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface, SwapChainSupportDetails &details)
{
    VkResult err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    uint32_t frameCount;
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &frameCount, nullptr);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    if (frameCount > 0)
    {
        details.formats.resize(frameCount);
        err = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &frameCount, details.formats.data());
        if (err != VkResult::VK_SUCCESS)
        {
            return err;
        }
    }

    uint32_t presentationModeCount;
    err = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationModeCount, nullptr);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    if (presentationModeCount > 0)
    {
        details.presentModes.resize(presentationModeCount);
        err = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationModeCount, details.presentModes.data());
        if (err != VkResult::VK_SUCCESS)
        {
            return err;
        }
    }
    return VkResult::VK_SUCCESS;
}

void PrintVkExtensions()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    VkExtensionProperties *properties = new VkExtensionProperties[extensionCount];
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, properties);
    for (size_t i = 0; i < extensionCount; ++i)
    {
        printf("%s\n", properties[i].extensionName);
    }
    delete[] properties;
}

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    uint32_t queueCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);

    VkQueueFamilyProperties *properties = new VkQueueFamilyProperties[queueCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, properties);

    QueueFamilyIndices indices{-1, -1, -1};
    for (uint32_t i = 0; i < queueCount; ++i)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }
        if (properties[i].queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }
        else if (properties[i].queueFlags & VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT)
        {
            indices.transferFamily = i;
        }
    }
    delete[] properties;
    return indices;
}

bool CheckValidationLayerSupport()
{
    uint32_t layerCount;
    VkResult err = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (err != VkResult::VK_SUCCESS)
    {
        return false;
    }

    VkLayerProperties *properties = new VkLayerProperties[layerCount];
    err = vkEnumerateInstanceLayerProperties(&layerCount, properties);
    if (err != VkResult::VK_SUCCESS)
    {
        return false;
    }

    for (const char *name : validationLayers)
    {
        bool layerNotFound = true;
        for (size_t i = 0; i < layerCount; ++i)
        {
            if (strcmp(properties[i].layerName, name) == 0)
            {
                layerNotFound = false;
                break;
            }
        }
        if (layerNotFound)
        {
            delete[] properties;
            return false;
        }
    }
    delete[] properties;
    return true;
}

bool DeviceIsSuitable(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && features.geometryShader;
}
bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    VkExtensionProperties *availableExtensions = new VkExtensionProperties[extensionCount];
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions);

    for (const auto &extension : deviceExtensions)
    {
        bool extensionAvailable = false;
        for (size_t i = 0; i < extensionCount; ++i)
        {
            if (availableExtensions[i].extensionName == extension)
            {
                extensionAvailable = true;
                break;
            }
        }
        if (!extensionAvailable)
        {
            delete[] availableExtensions;
            return false;
        }
    }

    delete[] availableExtensions;
    return true;
}

int GetDeviceRating(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    int score = 1;
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);
    QueueFamilyIndices indices = FindQueueFamilies(device, surface);

    if (!features.geometryShader || indices.graphicsFamily < 0 || indices.presentFamily < 0 || indices.transferFamily < 0 || CheckDeviceExtensionSupport(device))
    {
        return 0;
    }

    SwapChainSupportDetails details;
    QuerySwapChainSupport(device, surface, details);
    if (details.formats.empty() || details.presentModes.empty())
    {
        return 0;
    }

    if (properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 100;
    }
    else if (properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += 10;
    }

    // score += properties.limits.maxImageDimension3D;

    TTH_LOG_INFO("score = %d, name = %s, graphicsQueue = %ld, presentQueue = %ld, transferQueue = %ld\n", score, properties.deviceName, indices.graphicsFamily, indices.presentFamily,
                 indices.transferFamily);

    return score;
}

VkResult Renderer::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    VkResult err = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    if (deviceCount == 0)
    {
        physicalDevice = VK_NULL_HANDLE;
        return VkResult::VK_ERROR_UNKNOWN;
    }

    VkPhysicalDevice *devices = new VkPhysicalDevice[deviceCount];
    err = vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    size_t bestIndex = 0;
    for (size_t i = 0; i < deviceCount; ++i)
    {
        if (GetDeviceRating(devices[i], surface) > GetDeviceRating(devices[bestIndex], surface))
        {
            bestIndex = i;
        }
    }
    physicalDevice = devices[bestIndex];
    delete[] devices;

    if (GetDeviceRating(physicalDevice, surface) > 0)
    {
        return VK_SUCCESS;
    }
    return VkResult::VK_ERROR_UNKNOWN;
}

VkResult Renderer::CreateLogicalDevice(const QueueFamilyIndices &indices)
{
    float queuePriority = 1.0f;
    std::set<int64_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily, indices.transferFamily};
    VkDeviceQueueCreateInfo *queueCreateInfos = new VkDeviceQueueCreateInfo[uniqueQueueFamilies.size()];
    size_t index = 0;
    for (int64_t queueFamily : uniqueQueueFamilies)
    {
        queueCreateInfos[index].sType = VkStructureType::VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[index].queueFamilyIndex = queueFamily;
        queueCreateInfos[index].queueCount = 1;
        queueCreateInfos[index].pQueuePriorities = &queuePriority;
        queueCreateInfos[index].flags = 0;
        queueCreateInfos[index].pNext = nullptr;
        ++index;
    }

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo createInfo{};
    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.queueCreateInfoCount = uniqueQueueFamilies.size();
    createInfo.pEnabledFeatures = &features;
    if constexpr (DEBUG)
    {
        createInfo.enabledLayerCount = validationLayers.size(); // These are ignored in the newer versions of vulkan
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VkResult err = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
    delete[] queueCreateInfos;
    return err;
}

VkShaderModule CreateShaderModule(VkDevice device, const char *bytecode, size_t bytecodeSize)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytecodeSize;
    createInfo.pCode = reinterpret_cast<const uint32_t *>(bytecode);

    VkShaderModule module;
    VkResult err = vkCreateShaderModule(device, &createInfo, nullptr, &module);
    (void)err;
    return module;
}

VkResult Renderer::RecordCommandBuffer(uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult err = vkBeginCommandBuffer(commandBuffers[currentFrameIndex], &beginInfo);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{1.0f, 1.0f, 1.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[currentFrameIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffers[currentFrameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainExtent.width;
    viewport.height = (float)swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffers[currentFrameIndex], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(commandBuffers[currentFrameIndex], 0, 1, &scissor);

    VkDeviceSize offsets[3] = {0, 0, 0};

    assert(d3dmesh.mMeshData.mVertexStates[0].mAttributes[0].mAttribute == GFXPlatformVertexAttribute::eGFXPlatformAttribute_Position);

    offsets[1] = offsets[0] + d3dmesh.mMeshData.mVertexStates[0].mpVertexBuffer[0].mStride * d3dmesh.mMeshData.mVertexStates[0].mpVertexBuffer[0].mCount;
    offsets[2] = offsets[1] + d3dmesh.mMeshData.mVertexStates[0].mpVertexBuffer[0].mCount * sizeof(uint8_t) * 4;

    VkBuffer bufs[3] = {vertexBuffer, vertexBuffer, vertexBuffer};
    vkCmdBindVertexBuffers(commandBuffers[currentFrameIndex], 0, sizeof(offsets) / sizeof(*offsets), bufs, offsets);
    vkCmdBindIndexBuffer(commandBuffers[currentFrameIndex], indexBuffer, 0, VkIndexType::VK_INDEX_TYPE_UINT16);
    // vkCmdDraw(commandBuffers[currentFrameIndex], sizeof(vertices) / sizeof(*vertices), 1, 0, 0);
    vkCmdBindDescriptorSets(commandBuffers[currentFrameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrameIndex], 0, nullptr);
    vkCmdDrawIndexed(commandBuffers[currentFrameIndex], d3dmesh.mMeshData.mVertexStates[0].mpIndexBuffer[0].mCount, 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffers[currentFrameIndex]);

    return vkEndCommandBuffer(commandBuffers[currentFrameIndex]);
}

VkResult Renderer::DrawFrame()
{

    VkResult err = vkWaitForFences(device, 1, &inFlightFences[currentFrameIndex], VK_TRUE, UINT64_MAX);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    uint32_t imageIndex;
    err = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrameIndex], VK_NULL_HANDLE, &imageIndex);
    if (err == VkResult::VK_ERROR_OUT_OF_DATE_KHR || err == VkResult::VK_SUBOPTIMAL_KHR)
    {
        return RecreateSwapchain();
    }
    else if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = vkResetFences(device, 1, &inFlightFences[currentFrameIndex]);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = vkResetCommandBuffer(commandBuffers[currentFrameIndex], 0);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    err = RecordCommandBuffer(imageIndex);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    UpdateUniformBuffer(currentFrameIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrameIndex]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrameIndex];
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrameIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    err = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrameIndex]);
    if (err == VkResult::VK_ERROR_OUT_OF_DATE_KHR || err == VkResult::VK_SUBOPTIMAL_KHR)
    {
        err = RecreateSwapchain();
    }
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional
    err = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    currentFrameIndex = (currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

    return VkResult::VK_SUCCESS;
}

VkResult Renderer::CreateImageViews(const VkSurfaceFormatKHR &surfaceFormat)
{
    VkResult err = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    swapchainImages = new VkImage[imageCount];
    err = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    swapchainImageViews = new VkImageView[imageCount];
    for (size_t i = 0; i < imageCount; ++i)
    {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchainImages[i];
        imageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = surfaceFormat.format;

        imageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;

        imageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1; // 2D

        err = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]);
        if (err != VkResult::VK_SUCCESS)
        {
            return err;
        }
    }

    return VkResult::VK_SUCCESS;
}

VkResult Renderer::CreateSwapchain(VkSurfaceFormatKHR &surfaceFormat, const QueueFamilyIndices &indices)
{
    SwapChainSupportDetails swapChainSupport;
    VkResult err = QuerySwapChainSupport(physicalDevice, surface, swapChainSupport);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);

    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    swapchainExtent = ChooseSwapExtent(swapChainSupport.capabilities, window);

    imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && swapChainSupport.capabilities.maxImageCount < imageCount) // 0 max image count means no max image limit
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = surface;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = swapchainExtent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {static_cast<uint32_t>(indices.graphicsFamily), static_cast<uint32_t>(indices.transferFamily), static_cast<uint32_t>(indices.presentFamily)};

    swapChainCreateInfo.imageSharingMode = VkSharingMode::VK_SHARING_MODE_CONCURRENT;
    swapChainCreateInfo.queueFamilyIndexCount = 2 + (indices.presentFamily != indices.graphicsFamily && indices.presentFamily != indices.transferFamily);
    swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;

    swapChainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VkCompositeAlphaFlagBitsKHR::VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore alpha channel
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    err = vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapchain);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    return VkResult::VK_SUCCESS;
}

VkResult Renderer::RecreateSwapchain()
{
    VkResult err = vkDeviceWaitIdle(device);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    CleanupSwapchain();

    VkSurfaceFormatKHR surfaceFormat;
    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);

    err = CreateSwapchain(surfaceFormat, indices);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    err = CreateImageViews(surfaceFormat);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    err = CreateDepthResources();
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    err = CreateFramebuffers();
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    return VkResult::VK_SUCCESS;
}

VkResult Renderer::CreateFramebuffers()
{

    swapchainFramebuffers = new VkFramebuffer[imageCount];
    for (size_t i = 0; i < imageCount; ++i)
    {
        VkImageView attachments[]{swapchainImageViews[i], depthImageView};
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        VkResult err = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]);
        if (err != VkResult::VK_SUCCESS)
        {
            return err;
        }
    }

    return VkResult::VK_SUCCESS;
}

VkResult Renderer::CreateGraphicsPipeline()
{
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;

    {
        TTH::Stream vertShaderStream = TTH::Stream("/home/asil/Documents/decryption/TelltaleChimera/shaders/build/vert.spv", "rb");
        vertShaderStream.Seek(0, vertShaderStream.END);
        size_t vertShaderCodeSize = vertShaderStream.tell();
        char *vertShaderCode = new char[vertShaderCodeSize];
        vertShaderStream.Seek(0, vertShaderStream.SET);
        vertShaderStream.Read(vertShaderCode, vertShaderCodeSize);

        TTH::Stream fragShaderStream = TTH::Stream("/home/asil/Documents/decryption/TelltaleChimera/shaders/build/frag.spv", "rb");
        fragShaderStream.Seek(0, fragShaderStream.END);
        size_t fragShaderCodeSize = fragShaderStream.tell();
        char *fragShaderCode = new char[fragShaderCodeSize];
        fragShaderStream.Seek(0, fragShaderStream.SET);
        fragShaderStream.Read(fragShaderCode, fragShaderCodeSize);

        vertShaderModule = CreateShaderModule(device, vertShaderCode, vertShaderCodeSize);
        fragShaderModule = CreateShaderModule(device, fragShaderCode, fragShaderCodeSize);

        delete[] fragShaderCode;
        delete[] vertShaderCode;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";                // entrypoint
    vertShaderStageInfo.pSpecializationInfo = nullptr; // Specify values for constants

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";                // entrypoint
    fragShaderStageInfo.pSpecializationInfo = nullptr; // Specify values for constants

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    auto vertexBindingDescription = VertexD3D::getBindingDescription();
    auto vertexAttributDescriptions = VertexD3D::getAttributeDescription();

    vertexInputInfo.vertexBindingDescriptionCount = vertexBindingDescription.size();
    vertexInputInfo.pVertexBindingDescriptions = vertexBindingDescription.data(); // Optional, vertex buffer stuff
    vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributDescriptions.data(); // Optional, vertex buffer stuff

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainExtent.width;
    viewport.height = (float)swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f; // Lines thicker than 1.0f require wideLines GPU feature
    rasterizer.cullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f;          // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;          // Optional
    multisampling.pSampleMask = nullptr;            // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE;      // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VkLogicOp::VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {};  // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;                 // Optional
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;         // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr;      // Optional
    VkResult err = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
    if (err != VK_SUCCESS)
    {
        return err;
    }

    std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1;              // Optional
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.pDepthStencilState = &depthStencil;

    err = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);

    return VkResult::VK_SUCCESS;
}

VkResult Renderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VkResult err = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    err = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    return vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

VkResult Renderer::CreateVertexBuffersD3D()
{
    uint8_t *vertexData = d3dmesh.async;
    for (size_t i = 0; i < d3dmesh.mMeshData.mVertexStates[0].mIndexBufferCount; ++i)
    {
        vertexData += d3dmesh.mMeshData.mVertexStates[0].mpIndexBuffer[i].mCount * d3dmesh.mMeshData.mVertexStates[0].mpIndexBuffer[i].mStride;
    }

    VkDeviceSize bufferSize = d3dmesh.mMeshData.mVertexStates[0].mpVertexBuffer[0].mCount * d3dmesh.mMeshData.mVertexStates[0].mpVertexBuffer[0].mStride +
                              d3dmesh.mMeshData.mVertexCount * 4 * sizeof(uint8_t) + d3dmesh.mMeshData.mVertexCount * 4 * sizeof(float);
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkResult err = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    void *data;
    err = vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    memcpy(data, vertexData, static_cast<size_t>(d3dmesh.mMeshData.mVertexCount * d3dmesh.mMeshData.mVertexStates[0].mpVertexBuffer[0].mStride));
    uint8_t *blendIndices = (uint8_t *)data + d3dmesh.mMeshData.mVertexCount * d3dmesh.mMeshData.mVertexStates[0].mpVertexBuffer[0].mStride;
    uint8_t *vertexDataCopy = vertexData;

    for (size_t i = 0; i < d3dmesh.mMeshData.mVertexStates[0].mAttributeCount; ++i)
    {
        if (d3dmesh.mMeshData.mVertexStates[0].mAttributes[i].mAttribute == GFXPlatformVertexAttribute::eGFXPlatformAttribute_BlendIndex)
        {
            break;
        }
        else
        {
            vertexData += d3dmesh.mMeshData.mVertexStates[0].mpVertexBuffer[i].mCount * d3dmesh.mMeshData.mVertexStates[0].mpVertexBuffer[i].mStride;
        }
    }
    memcpy(blendIndices, vertexData, d3dmesh.mMeshData.mVertexCount * sizeof(uint8_t) * 4);
    vertexData = vertexDataCopy;

    float *blendWeights = (float *)(blendIndices + d3dmesh.mMeshData.mVertexCount * sizeof(uint8_t) * 4);
    for (size_t i = 0; i < d3dmesh.mMeshData.mVertexStates[0].mAttributeCount; ++i)
    {
        if (d3dmesh.mMeshData.mVertexStates[0].mAttributes[i].mAttribute == GFXPlatformVertexAttribute::eGFXPlatformAttribute_BlendWeight)
        {
            break;
        }
        else
        {
            vertexData += d3dmesh.mMeshData.mVertexStates[0].mpVertexBuffer[i].mCount * d3dmesh.mMeshData.mVertexStates[0].mpVertexBuffer[i].mStride;
        }
    }
    for (size_t i = 0; i < d3dmesh.mMeshData.mVertexCount * 4; i += 4)
    {
        blendWeights[i] = 1.0f - (float)(*((uint32_t *)vertexData) & 0x3ff) / 1023.0f / 8.0f - (float)(*((uint32_t *)vertexData) >> 30) / 8.0f -
                          (float)(*((uint32_t *)vertexData) >> 10 & 0x3ff) / 1023.0f / 3.0f - (float)(*((uint32_t *)vertexData) >> 20 & 0x3ff) / 1023.0f / 4.0f;
        blendWeights[i + 1] = (float)(*((uint32_t *)vertexData) & 0x3ff) / 1023.0f / 8.0f + (float)(*((uint32_t *)vertexData) >> 30) / 8.0f;
        blendWeights[i + 2] = (float)(*((uint32_t *)vertexData) >> 10 & 0x3ff) / 1023.0f / 3.0f;
        blendWeights[i + 3] = (float)(*((uint32_t *)vertexData) >> 20 & 0x3ff) / 1023.0f / 4.0f;

        // printf("%f, %f, %f, %f\n", blendWeights[i], blendWeights[i + 1], blendWeights[i + 2], blendWeights[i + 3]);

        vertexData += sizeof(uint32_t);
    }

    vkUnmapMemory(device, stagingBufferMemory);

    err = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    err = CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    return VkResult::VK_SUCCESS;
}

VkResult Renderer::CreateVertexBuffers()
{
    VkDeviceSize bufferSize = sizeof(vertices);
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkResult err = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    void *data;
    err = vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    memcpy(data, vertices, (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    err = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    err = CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    return VkResult::VK_SUCCESS;
}

VkResult Renderer::CreateIndexBufferD3D()
{
    VkDeviceSize bufferSize = d3dmesh.mMeshData.mVertexStates[0].mpIndexBuffer[0].mCount * d3dmesh.mMeshData.mVertexStates[0].mpIndexBuffer[0].mStride;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkResult err = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    void *data;
    err = vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    memcpy(data, d3dmesh.async, (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    err = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    err = CopyBuffer(stagingBuffer, indexBuffer, bufferSize);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    return VkResult::VK_SUCCESS;
}

VkResult Renderer::CreateIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indexData);
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkResult err = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    void *data;
    err = vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    memcpy(data, indexData, (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    err = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    err = CopyBuffer(stagingBuffer, indexBuffer, bufferSize);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    return VkResult::VK_SUCCESS;
}

VkResult Renderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo{
        .sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = transferPool,
        .level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    VkResult err = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    err = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    err = vkEndCommandBuffer(commandBuffer);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    err = vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    err = vkQueueWaitIdle(transferQueue);

    vkFreeCommandBuffers(device, transferPool, 1, &commandBuffer);

    return err;
}

VkResult Renderer::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding bindings[1] = {0};

    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1; // USE THIS FOR SKELETON ANIMATION
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    layoutInfo.pBindings = bindings;

    return vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
}

VkResult Renderer::CreateUniformBuffers()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VkResult err = CreateBuffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i],
                                    uniformBuffersMemory[i]);
        if (err != VkResult::VK_SUCCESS)
        {
            return err;
        }
        err = vkMapMemory(device, uniformBuffersMemory[i], 0, sizeof(UniformBufferObject), 0, &uniformBuffersMapped[i]);
        if (err != VkResult::VK_SUCCESS)
        {
            return err;
        }
    }
    return VkResult::VK_SUCCESS;
}

VkResult Renderer::CreateUniformBoneBuffers() { return VkResult::VK_SUCCESS; }

VkResult Renderer::CreateDescriptorPool()
{
    VkDescriptorPoolSize poolSize{
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = MAX_FRAMES_IN_FLIGHT,
    };

    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
    };

    return vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
}

VkResult Renderer::CreateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(descriptorSets.size()),
        .pSetLayouts = layouts.data(),
    };

    VkResult err = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VkDescriptorBufferInfo bufferInfo{
            .buffer = uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject),
        };

        VkWriteDescriptorSet descriptorWrite{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &bufferInfo,
            .pTexelBufferView = nullptr,
        };

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }

    return VkResult::VK_SUCCESS;
}

struct ActiveSample
{
    glm::vec3 v;
    glm::quat q;
    float vTime;
    float qTime;

    ActiveSample() : v(0.0f), q(0.0f, 0.0f, 0.0f, 1.0f), vTime(-1.0f), qTime(-1.0f) {}
};

static void SetGlobalTransforms(JointTransform *transforms, const TTH::Skeleton &skeleton, size_t childIndex)
{
    glm::mat4 localTransform = transforms[childIndex].transform;
    transforms[childIndex].transform = glm::mat4(0.0f);
    if (skeleton.mEntries[childIndex].mParentIndex >= 0)
    {
        glm::mat4 parentGlobalTransform;
        SetGlobalTransforms(transforms, skeleton, skeleton.mEntries[childIndex].mParentIndex);
        transforms[childIndex].transform *= parentGlobalTransform;
    }
    transforms[childIndex].transform *= localTransform;
}

VkResult Renderer::UpdateUniformBuffer(uint32_t currentImage)
{
    UniformBufferObject *ubo = static_cast<UniformBufferObject *>(uniformBuffersMapped[currentFrameIndex]);
    ubo->model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo->view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo->proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
    ubo->vertexTransform = glm::translate(glm::mat4(1.0f), glm::vec3{d3dmesh.mMeshData.mPositionOffset.x, d3dmesh.mMeshData.mPositionOffset.y, d3dmesh.mMeshData.mPositionOffset.z}) *
                           glm::scale(glm::mat4(1.0f), glm::vec3{d3dmesh.mMeshData.mPositionScale.x, d3dmesh.mMeshData.mPositionScale.y, d3dmesh.mMeshData.mPositionScale.z});
    ubo->proj[1][1] *= -1;
    ubo->boneCount = skeleton.mEntries.size();

    time += 0.001f;
    if (time > animation.mLength)
    {
        time = 0.0f;
    }

    /* Animation, doing it like telltale even though it is a horrible way of doing it, but unlike telltale, I am using step interpolation for simplicity */
    const TTH::CompressedSkeletonPoseKeys2 *cspk = nullptr;
    for (int32_t i = 0; i < animation.mInterfaceCount && cspk == nullptr; ++i)
    {
        cspk = animation.mValues[i].GetTypePtr<TTH::CompressedSkeletonPoseKeys2>();
    }
    if (cspk == nullptr)
    {
        return VkResult::VK_ERROR_UNKNOWN;
    }

    TTH::CompressedSkeletonPoseKeys2::Header header = *(TTH::CompressedSkeletonPoseKeys2::Header *)(cspk->mpData); // TODO: undefined behaviour? Should probably fix
    const uint8_t *cspkBuf = cspk->mpData + sizeof(header) + sizeof(int64_t);

    header.mRangeVector.x *= 9.536752e-07f;
    header.mRangeVector.y *= 2.384186e-07f;
    header.mRangeVector.z *= 2.384186e-07f;

    header.mRangeDeltaV.x *= 0.0009775171f;
    header.mRangeDeltaV.y *= 0.0004885198f;
    header.mRangeDeltaV.z *= 0.0004885198f;

    header.mRangeDeltaQ.x *= 0.0009775171f;
    header.mRangeDeltaQ.y *= 0.0004885198f;
    header.mRangeDeltaQ.z *= 0.0004885198f;

    size_t stagedDelQ = 4;
    size_t stagedAbsQ = 4;
    size_t stagedDelV = 4;
    size_t stagedAbsV = 4;
    glm::quat delQ[4];
    glm::quat absQ[4];
    glm::vec3 delV[4];
    glm::vec3 absV[4];

    ActiveSample *currentSamples = new ActiveSample[header.mBoneCount]();
    ActiveSample *previousSamples = new ActiveSample[header.mBoneCount]();

    for (const uint32_t *headerData = (uint32_t *)(cspkBuf + header.mSampleDataSize + header.mBoneCount * sizeof(uint64_t)); headerData < (uint32_t *)(cspk->mpData + cspk->mDataSize); ++headerData)
    {
        if ((*headerData & 0x40000000) == 0) // Vector
        {
            previousSamples[(*headerData >> 0x10) & 0xfff].v = currentSamples[(*headerData >> 0x10) & 0xfff].v;
            previousSamples[(*headerData >> 0x10) & 0xfff].vTime = currentSamples[(*headerData >> 0x10) & 0xfff].vTime;
            if (previousSamples[(*headerData >> 0x10) & 0xfff].vTime > time)
            {
                break;
            }
            currentSamples[(*headerData >> 0x10) & 0xfff].vTime = (float)(*headerData & 0xffff) * 1.525902e-05 * header.mRangeTime;
            if ((int32_t)*headerData < 0)
            {
                if (stagedDelV > 3)
                {
                    for (uint32_t i = 0; i < 4; ++i)
                    {
                        delV[i].x = (float)(((uint32_t *)cspkBuf)[i] & 0x3ff) * header.mRangeDeltaV.x + header.mMinDeltaV.x;
                        delV[i].y = (float)(((uint32_t *)cspkBuf)[i] >> 10 & 0x7ff) * header.mRangeDeltaV.y + header.mMinDeltaV.y;
                        delV[i].z = (float)(((uint32_t *)cspkBuf)[i] >> 21) * header.mRangeDeltaV.z + header.mMinDeltaV.z;
                    }
                    cspkBuf += 4 * sizeof(uint32_t);
                    stagedDelV = 0;
                }
                if (1)
                {
                    delV[stagedDelV] += previousSamples[(*headerData >> 0x10) & 0xfff].v;
                }
                currentSamples[(*headerData >> 0x10) & 0xfff].v = delV[stagedDelV];
                ++stagedDelV;
            }
            else
            {
                if (stagedAbsV > 3)
                {
                    for (uint32_t i = 0; i < 4; ++i)
                    {
                        absV[i].x = (float)(((((uint32_t *)cspkBuf)[4 + i] & 0x3ff) << 10) | (((uint32_t *)cspkBuf)[i] & 0x3ff)) * header.mRangeVector.x + header.mMinVector.x;
                        absV[i].y = (float)(((((uint32_t *)cspkBuf)[4 + i] >> 10 & 0x7ff) << 11) | (((uint32_t *)cspkBuf)[i] >> 10 & 0x7ff)) * header.mRangeVector.y + header.mMinVector.y;
                        absV[i].z = (float)(((((uint32_t *)cspkBuf)[4 + i] >> 21) << 11) | (((uint32_t *)cspkBuf)[i] >> 21)) * header.mRangeVector.z + header.mMinVector.z;
                    }
                    cspkBuf += 8 * sizeof(uint32_t);
                    stagedAbsV = 0;
                }
                currentSamples[(*headerData >> 0x10) & 0xfff].v = absV[stagedAbsV];
                ++stagedAbsV;
            }
        }
        else // Quaternion
        {
            previousSamples[(*headerData >> 0x10) & 0xfff].q = currentSamples[(*headerData >> 0x10) & 0xfff].q;
            previousSamples[(*headerData >> 0x10) & 0xfff].qTime = currentSamples[(*headerData >> 0x10) & 0xfff].qTime;
            if (previousSamples[(*headerData >> 0x10) & 0xfff].qTime > time)
            {
                break;
            }
            currentSamples[(*headerData >> 0x10) & 0xfff].qTime = (float)(*headerData & 0xffff) * 1.525902e-05 * header.mRangeTime;
            if ((int32_t)*headerData < 0)
            {
                if (stagedDelQ > 3)
                {
                    for (uint32_t i = 0; i < 4; ++i)
                    {
                        delQ[i].x = (float)(((uint32_t *)cspkBuf)[i] & 0x3ff) * header.mRangeDeltaQ.x + header.mMinDeltaQ.x;
                        delQ[i].y = (float)(((uint32_t *)cspkBuf)[i] >> 10 & 0x7ff) * header.mRangeDeltaQ.y + header.mMinDeltaQ.y;
                        delQ[i].z = (float)(((uint32_t *)cspkBuf)[i] >> 21) * header.mRangeDeltaQ.z + header.mMinDeltaQ.z;
                        delQ[i].w = ((1.0 - delQ[i].x * delQ[i].x) - delQ[i].y * delQ[i].y) - delQ[i].z * delQ[i].z;

                        if (delQ[i].w > 0.0f)
                        {
                            delQ[i].w = sqrtf(delQ[i].w);
                        }
                        else
                        {
                            delQ[i].w = 0.0f;
                        }
                    }
                    cspkBuf += 4 * sizeof(uint32_t);
                    stagedDelQ = 0;
                }
                if (1)
                {
                    delQ[stagedDelQ] *= previousSamples[(*headerData >> 0x10) & 0xfff].q;
                }
                currentSamples[(*headerData >> 0x10) & 0xfff].q = delQ[stagedDelQ];
                ++stagedDelQ;
            }
            else
            {
                if (stagedAbsQ > 3)
                {
                    for (uint32_t i = 0; i < 4; ++i)
                    {
                        absQ[i].x = (float)(((((uint32_t *)cspkBuf)[4 + i] & 0x3ff) << 10) | (((uint32_t *)cspkBuf)[i] & 0x3ff)) * 1.3487e-06 - 0.7071068;
                        absQ[i].y = (float)(((((uint32_t *)cspkBuf)[4 + i] >> 10 & 0x7ff) << 11) | (((uint32_t *)cspkBuf)[i] >> 10 & 0x7ff)) * 3.371749e-07 - 0.7071068;
                        absQ[i].z = (float)(((((uint32_t *)cspkBuf)[4 + i] >> 21) << 11) | (((uint32_t *)cspkBuf)[i] >> 21)) * 3.371749e-07 - 0.7071068;
                        absQ[i].w = ((1.0 - absQ[i].x * absQ[i].x) - absQ[i].y * absQ[i].y) - absQ[i].z * absQ[i].z;
                        if (absQ[i].w > 0.0f)
                        {
                            absQ[i].w = sqrtf(absQ[i].w);
                        }
                        else
                        {
                            absQ[i].w = 0.0f;
                        }
                    }
                    cspkBuf += 8 * sizeof(uint32_t);
                    stagedAbsQ = 0;
                }
                uint32_t axisOrder = *headerData >> 0x1c & 3;
                currentSamples[(*headerData >> 0x10) & 0xfff].q.x = *(((float *)absQ + (axisOrder ^ 1)) + 4 * stagedAbsQ);
                currentSamples[(*headerData >> 0x10) & 0xfff].q.y = *(((float *)absQ + (axisOrder ^ 2)) + 4 * stagedAbsQ);
                currentSamples[(*headerData >> 0x10) & 0xfff].q.z = *(((float *)absQ + (axisOrder ^ 3)) + 4 * stagedAbsQ);
                currentSamples[(*headerData >> 0x10) & 0xfff].q.w = *(((float *)absQ + (axisOrder)) + 4 * stagedAbsQ);
                ++stagedAbsQ;
            }
        }
    }

    cspkBuf = cspk->mpData + sizeof(header) + sizeof(int64_t) + header.mSampleDataSize;

    for (size_t i = 0; i < skeleton.mEntries.size(); ++i)
    {
        ubo->boneTransforms[i] = glm::translate(glm::mat4(1.0f), glm::vec3{skeleton.mEntries[i].mLocalPos.x, skeleton.mEntries[i].mLocalPos.y, skeleton.mEntries[i].mLocalPos.z}) *
                                 glm::toMat4(glm::quat{skeleton.mEntries[i].mLocalQuat.w, skeleton.mEntries[i].mLocalQuat.x, skeleton.mEntries[i].mLocalQuat.y, skeleton.mEntries[i].mLocalQuat.z});
        ubo->baseTransforms[i] = ubo->boneTransforms[i];
        for (size_t j = 0; j < header.mBoneCount; ++j)
        {
            if (((uint64_t *)(cspkBuf))[j] == skeleton.mEntries[i].mJointName.mCrc64)
            {
                float length = sqrtf(skeleton.mEntries[i].mLocalPos.x * skeleton.mEntries[i].mLocalPos.x + skeleton.mEntries[i].mLocalPos.y * skeleton.mEntries[i].mLocalPos.y +
                                     skeleton.mEntries[i].mLocalPos.z * skeleton.mEntries[i].mLocalPos.z);
                ubo->boneTransforms[i] =
                    glm::translate(glm::mat4(1.0f), glm::vec3{currentSamples[j].v.x * length, currentSamples[j].v.y * length, currentSamples[j].v.z * length}) * glm::toMat4(currentSamples[j].q);
                printf("%d [%f,%f,%f] [%f,%f,%f]\n", i, ubo->boneTransforms[i][3][0], ubo->boneTransforms[i][3][1], ubo->boneTransforms[i][3][2], skeleton.mEntries[i].mLocalPos.x,
                       skeleton.mEntries[i].mLocalPos.y, skeleton.mEntries[i].mLocalPos.z);
                break;
            }
        }

        if (i == 177)
        {
            Matrix<4, 4> *mat = (Matrix<4, 4> *)(&ubo->boneTransforms[i]);
            glm::mat4x4 x = glm::translate(glm::mat4(1.0f), glm::vec3{skeleton.mEntries[i].mLocalPos.x, skeleton.mEntries[i].mLocalPos.y, skeleton.mEntries[i].mLocalPos.z}) *
                            glm::toMat4(glm::quat{skeleton.mEntries[i].mLocalQuat.w, skeleton.mEntries[i].mLocalQuat.x, skeleton.mEntries[i].mLocalQuat.y, skeleton.mEntries[i].mLocalQuat.z});
            Matrix<4, 4> *mat2 = (Matrix<4, 4> *)(&x);
        }
    }
    for (size_t i = 0; i < skeleton.mEntries.size(); ++i)
    {
        if (skeleton.mEntries[i].mParentIndex >= 0)
        {
            assert(skeleton.mEntries[i].mParentIndex < i);
            ubo->boneTransforms[i] = ubo->boneTransforms[skeleton.mEntries[i].mParentIndex] * ubo->boneTransforms[i];
            ubo->baseTransforms[i] = ubo->baseTransforms[skeleton.mEntries[i].mParentIndex] * ubo->baseTransforms[i];
            // printf("%ld [%f,%f,%f]\n", i, ubo->baseTransforms[i][3][0], ubo->baseTransforms[i][3][1], ubo->baseTransforms[i][3][2]);
        }
    }

    delete[] currentSamples;
    delete[] previousSamples;

    return VkResult::VK_SUCCESS;
}

VkResult Renderer::CreateTextureImage()
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    return VkResult::VK_SUCCESS;
}

bool hasStencilComponent(VkFormat format) { return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; }

VkFormat Renderer::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }
    return VkFormat::VK_FORMAT_UNDEFINED;
}

VkFormat Renderer::FindDepthFormat()
{
    return FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkResult Renderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                               VkDeviceMemory &imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult err = vkCreateImage(device, &imageInfo, nullptr, &image);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = static_cast<uint32_t>(FindMemoryType(memRequirements.memoryTypeBits, properties));

    err = vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    return vkBindImageMemory(device, image, imageMemory, 0);
}

VkResult Renderer::CreateDepthResources()
{
    VkFormat depthFormat = FindDepthFormat();
    VkResult err = CreateImage(swapchainExtent.width, swapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               depthImage, depthImageMemory);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    return vkCreateImageView(device, &viewInfo, nullptr, &depthImageView);
}

VkResult Renderer::VulkanInit()
{
    if (DEBUG && !CheckValidationLayerSupport())
    {
        return VkResult::VK_ERROR_LAYER_NOT_PRESENT;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Chimera";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.pEngineName = "No engine";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if constexpr (DEBUG)
    {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        return VK_ERROR_UNKNOWN;
    }
    SDL_Window *window = SDL_CreateWindow("SDL3+Vulkan", 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    createInfo.ppEnabledExtensionNames = SDL_Vulkan_GetInstanceExtensions(&createInfo.enabledExtensionCount);

    VkResult err = vkCreateInstance(&createInfo, nullptr, &instance); // TODO: Look into allocators
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface))
    {
        return VK_ERROR_UNKNOWN;
    }

    err = PickPhysicalDevice();
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);
    err = CreateLogicalDevice(indices);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
    vkGetDeviceQueue(device, indices.transferFamily, 0, &transferQueue);

    VkSurfaceFormatKHR surfaceFormat;

    err = CreateSwapchain(surfaceFormat, indices);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    err = CreateImageViews(surfaceFormat);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    /* Render passes */
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = FindDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = surfaceFormat.format;
    colorAttachment.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    err = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = CreateDescriptorSetLayout();
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = CreateDepthResources();
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = CreateGraphicsPipeline();
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = CreateFramebuffers();
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    /* Command Pool */
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily; // Graphics family queue because we want to record drawing commands
    err = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    poolInfo.queueFamilyIndex = indices.transferFamily;
    err = vkCreateCommandPool(device, &poolInfo, nullptr, &transferPool);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    /* Vertex Buffers */
    err = CreateVertexBuffersD3D();
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    /* Index buffer */
    err = CreateIndexBufferD3D();
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = CreateUniformBuffers();
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = CreateDescriptorPool();
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = CreateDescriptorSets();
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    /* CommandBuffer */
    VkCommandBufferAllocateInfo commandBufferAllocInfo{};
    commandBufferAllocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.commandPool = commandPool;
    commandBufferAllocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    err = vkAllocateCommandBuffers(device, &commandBufferAllocInfo, commandBuffers.data());
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    err = vkBeginCommandBuffer(commandBuffers[currentFrameIndex], &beginInfo);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    /* synchronization */

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        err = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
        if (err != VkResult::VK_SUCCESS)
        {
            return err;
        }
        err = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
        if (err != VkResult::VK_SUCCESS)
        {
            return err;
        }

        err = vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]);
        if (err != VkResult::VK_SUCCESS)
        {
            return err;
        }
    }

    return VkResult::VK_SUCCESS;
}

void Renderer::CleanupSwapchain()
{
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
    depthImageView = VK_NULL_HANDLE;
    depthImage = VK_NULL_HANDLE;
    depthImageMemory = VK_NULL_HANDLE;

    for (uint32_t i = 0; i < imageCount; ++i)
    {
        vkDestroyFramebuffer(device, swapchainFramebuffers[i], nullptr);
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    }
    delete[] swapchainImageViews;
    delete[] swapchainImages;
    delete[] swapchainFramebuffers;
    swapchainImageViews = nullptr;
    swapchainImages = nullptr;
    swapchainFramebuffers = nullptr;
    imageCount = 0;

    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

Renderer::~Renderer()
{
    vkFreeCommandBuffers(device, commandPool, MAX_FRAMES_IN_FLIGHT, commandBuffers.data());
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyCommandPool(device, transferPool, nullptr);
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    CleanupSwapchain();

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
