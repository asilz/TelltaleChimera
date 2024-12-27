#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <algorithm>
#include <set>
#include <tth/core/log.hpp>
#include <tth/meta/skeleton/skeleton.hpp>
#include <vulkan/vulkan.h>

constexpr std::array<const char *, 1> validationLayers = {"VK_LAYER_KHRONOS_validation"};
constexpr std::array<const char *, 1> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifndef DEBUG
#define DEBUG 0
#endif

struct QueueFamilyIndices
{
    int64_t graphicsFamily;
    int64_t presentFamily;
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

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

    QueueFamilyIndices indices{-1, -1};
    for (size_t i = 0; i < queueCount; ++i)
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
            break;
        }
    }
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
            const char *layerName = properties[i].layerName;
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
            return false;
        }
    }
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

    if (!features.geometryShader || indices.graphicsFamily < 0 || indices.presentFamily < 0 || CheckDeviceExtensionSupport(device))
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

    score += properties.limits.maxImageDimension3D;

    return score;
}

VkResult PickPhysicalDevice(VkInstance instance, VkPhysicalDevice &dev, VkSurfaceKHR surface)
{
    uint32_t deviceCount = 0;
    VkResult err = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    if (deviceCount == 0)
    {
        dev = VK_NULL_HANDLE;
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
    dev = devices[bestIndex];
    delete[] devices;

    if (GetDeviceRating(dev, surface) > 0)
    {
        return VK_SUCCESS;
    }
    return VkResult::VK_ERROR_UNKNOWN;
}

VkResult CreateLogicalDevice(VkPhysicalDevice device, VkDevice &result, QueueFamilyIndices &indices, VkSurfaceKHR surface)
{
    indices = FindQueueFamilies(device, surface);

    float queuePriority = 1.0f;
    std::set<int64_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(uniqueQueueFamilies.size());
    size_t index = 0;
    for (int64_t queueFamily : uniqueQueueFamilies)
    {
        queueCreateInfos[index].sType = VkStructureType::VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[index].queueFamilyIndex = queueFamily;
        queueCreateInfos[index].queueCount = 1;
        queueCreateInfos[index].pQueuePriorities = &queuePriority;
        ++index;
    }

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo createInfo{};
    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
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

    return vkCreateDevice(device, &createInfo, nullptr, &result);
}

VkShaderModule CreateShaderModule(VkDevice device, const char *bytecode, size_t bytecodeSize)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytecodeSize;
    createInfo.pCode = reinterpret_cast<const uint32_t *>(bytecode);

    VkShaderModule module;
    VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &module);
    (void)result;
    return module;
}

void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, const VkRenderPass &renderPass, VkFramebuffer *swapChainFramebuffers, const VkExtent2D &swapChainExtent,
                         VkPipeline &graphicsPipeline)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}

VkResult drawFrame(VkDevice device, VkCommandBuffer commandBuffer, VkRenderPass &renderPass, VkFramebuffer *frameBuffer, VkPipeline &graphicsPipeline, VkQueue graphicsQueue, VkQueue presentQueue,
                   const VkExtent2D &extent, VkSwapchainKHR swapChain, VkSemaphore imageAvailableSemaphore, VkSemaphore renderFinishedSemaphore, VkFence inFlightFence)
{
    VkResult err = vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = vkResetFences(device, 1, &inFlightFence);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    uint32_t imageIndex;
    err = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = vkResetCommandBuffer(commandBuffer, 0);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    RecordCommandBuffer(commandBuffer, imageIndex, renderPass, frameBuffer, extent, graphicsPipeline);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    err = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional
    return vkQueuePresentKHR(presentQueue, &presentInfo);
}

VkResult VulkanTest()
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
    SDL_Window *window = SDL_CreateWindow("SDL3+Vulkan", 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    createInfo.ppEnabledExtensionNames = SDL_Vulkan_GetInstanceExtensions(&createInfo.enabledExtensionCount);

    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance); // TODO: Look into allocators
    if (result != VkResult::VK_SUCCESS)
    {
        return result;
    }

    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface))
    {
        return VK_ERROR_UNKNOWN;
    }

    VkPhysicalDevice physicalDevice;
    result = PickPhysicalDevice(instance, physicalDevice, surface);
    if (result != VkResult::VK_SUCCESS)
    {
        return result;
    }

    QueueFamilyIndices indices;
    VkDevice device;
    result = CreateLogicalDevice(physicalDevice, device, indices, surface);
    if (result != VkResult::VK_SUCCESS)
    {
        return result;
    }

    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);

    VkQueue presentQueue;
    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);

    SwapChainSupportDetails swapChainSupport;
    QuerySwapChainSupport(physicalDevice, surface, swapChainSupport);
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
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
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {static_cast<uint32_t>(indices.graphicsFamily), static_cast<uint32_t>(indices.presentFamily)};

    if (indices.graphicsFamily != indices.presentFamily)
    {
        swapChainCreateInfo.imageSharingMode = VkSharingMode::VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        swapChainCreateInfo.imageSharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }
    swapChainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VkCompositeAlphaFlagBitsKHR::VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore alpha channel
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain;
    result = vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapChain);
    if (result != VkResult::VK_SUCCESS)
    {
        return result;
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    VkImage *swapChainImages = new VkImage[imageCount];
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages);

    VkImageView *swapChainImageViews = new VkImageView[imageCount];
    for (size_t i = 0; i < imageCount; ++i)
    {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapChainImages[i];
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

        result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapChainImageViews[i]);
        if (result != VkResult::VK_SUCCESS)
        {
            return result;
        }
    }

    /* Render passes */
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

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPass renderPass;
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    if (result != VkResult::VK_SUCCESS)
    {
        return result;
    }

    /* Graphics pipeline */
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

    VkShaderModule vertShaderModule = CreateShaderModule(device, vertShaderCode, vertShaderCodeSize);
    VkShaderModule fragShaderModule = CreateShaderModule(device, fragShaderCode, fragShaderCodeSize);

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

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional, vertex buffer stuff
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional, vertex buffer stuff

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    delete[] fragShaderCode;
    delete[] vertShaderCode;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;

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
    rasterizer.frontFace = VkFrontFace::VK_FRONT_FACE_CLOCKWISE;
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
    colorBlendAttachment.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT |
                                          VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
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

    VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;            // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
    if (result != VK_SUCCESS)
    {
        return result;
    }

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

    VkPipeline graphicsPipeline;
    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
    if (result != VkResult::VK_SUCCESS)
    {
        return result;
    }
    VkFramebuffer *swapChainFramebuffers = new VkFramebuffer[imageCount];
    for (size_t i = 0; i < imageCount; ++i)
    {
        VkImageView attachments[]{swapChainImageViews[i]};
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]);
        if (result != VkResult::VK_SUCCESS)
        {
            return result;
        }
    }

    /* Command Pool */
    VkCommandPool commandPool;
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily; // Graphics family queue because we want to record drawing commands
    result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    if (result != VkResult::VK_SUCCESS)
    {
        return result;
    }

    /* CommandBuffer */
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo commandBufferAllocInfo{};
    commandBufferAllocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.commandPool = commandPool;
    commandBufferAllocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(device, &commandBufferAllocInfo, &commandBuffer);
    if (result != VkResult::VK_SUCCESS)
    {
        return result;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult err = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    /* synchronization */
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
    if (result != VkResult::VK_SUCCESS)
    {
        return result;
    }
    result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore);
    if (result != VkResult::VK_SUCCESS)
    {
        return result;
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT; // Fence is initially signaled
    result = vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence);
    if (result != VkResult::VK_SUCCESS)
    {
        return result;
    }

    SDL_Event sdlEvent;
    do
    {
        drawFrame(device, commandBuffer, renderPass, swapChainFramebuffers, graphicsPipeline, graphicsQueue, presentQueue, extent, swapChain, imageAvailableSemaphore, renderFinishedSemaphore,
                  inFlightFence);
        SDL_PollEvent(&sdlEvent);
    } while (sdlEvent.type != SDL_EVENT_QUIT);

    /* Cleanup */

    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);

    vkDestroyCommandPool(device, commandPool, nullptr);
    for (size_t i = 0; i < imageCount; ++i)
    {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
    }
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (size_t i = 0; i < imageCount; ++i)
    {
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }
    delete[] swapChainImageViews;
    delete[] swapChainImages;
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return VkResult::VK_SUCCESS;
}

int TestSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) == 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // Create a window with the specified title, width, height, and flags
    SDL_Window *window = SDL_CreateWindow("SDL3 Window", 640, 480, SDL_WINDOW_VULKAN);
    if (!window)
    {
        SDL_Log("Could not create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Event loop to keep the window open
    SDL_Event event;
    int running = 1;
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = 0;
            }
        }
        // Additional rendering can be done here
    }

    // Clean up resources before exiting
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}