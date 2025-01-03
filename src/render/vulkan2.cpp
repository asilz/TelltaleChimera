#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/matrix4x4.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <set>
#include <ttc/render/vulkan2.hpp>
#include <tth/core/log.hpp>
#include <tth/meta/linalg/vector.hpp>
#include <tth/meta/skeleton/skeleton.hpp>

constexpr std::array<const char *, 1> validationLayers = {"VK_LAYER_KHRONOS_validation"};
constexpr std::array<const char *, 1> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifndef DEBUG
#define DEBUG 0
#endif

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
    glm::mat4x4 model;
    glm::mat4x4 view;
    glm::mat4x4 proj;
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
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
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

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(commandBuffers[currentFrameIndex], 0, 1, &vertexBuffer, offsets);
    vkCmdBindIndexBuffer(commandBuffers[currentFrameIndex], indexBuffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
    // vkCmdDraw(commandBuffers[currentFrameIndex], sizeof(vertices) / sizeof(*vertices), 1, 0, 0);
    vkCmdBindDescriptorSets(commandBuffers[currentFrameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrameIndex], 0, nullptr);
    vkCmdDrawIndexed(commandBuffers[currentFrameIndex], sizeof(indexData) / sizeof(indexData[0]), 1, 0, 0, 0);

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
    VkVertexInputBindingDescription vertexBindingDescription = Vertex::getBindingDescription();
    std::array<VkVertexInputAttributeDescription, 3> vertexAttributDescriptions = Vertex::getAttributeDescription();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription; // Optional, vertex buffer stuff
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
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1; // USE THIS FOR SKELETON ANIMATION
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

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
        .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
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

VkResult Renderer::UpdateUniformBuffer(uint32_t currentImage)
{
    UniformBufferObject *ubo = static_cast<UniformBufferObject *>(uniformBuffersMapped[currentFrameIndex]);
    ubo->model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo->view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo->proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
    ubo->proj[1][1] *= -1;
    time += 0.01f;

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
    err = CreateVertexBuffers();
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    /* Index buffer */
    err = CreateIndexBuffer();
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
