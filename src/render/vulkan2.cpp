#include <algorithm>
#include <set>
#include <ttc/render/vulkan2.hpp>
#include <tth/core/log.hpp>
#include <tth/meta/skeleton/skeleton.hpp>

constexpr std::array<const char *, 1> validationLayers = {"VK_LAYER_KHRONOS_validation"};
constexpr std::array<const char *, 1> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifndef DEBUG
#define DEBUG 0
#endif

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

VkResult Renderer::CreateLogicalDevice(QueueFamilyIndices &indices)
{
    indices = FindQueueFamilies(physicalDevice, surface);

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

    return vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
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

    VkResult err = vkBeginCommandBuffer(commandBuffer, &beginInfo);
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

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainExtent.width;
    viewport.height = (float)swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    return vkEndCommandBuffer(commandBuffer);
}

VkResult Renderer::DrawFrame()
{

    uint32_t imageIndex;
    VkResult err = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = vkResetCommandBuffer(commandBuffer, 0);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    err = RecordCommandBuffer(imageIndex);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

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

    err = vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = vkResetFences(device, 1, &inFlightFence);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    return VkResult::VK_SUCCESS;
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
    SDL_Window *window = SDL_CreateWindow("SDL3+Vulkan", 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
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

    QueueFamilyIndices indices;
    err = CreateLogicalDevice(indices);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);

    SwapChainSupportDetails swapChainSupport;
    err = QuerySwapChainSupport(physicalDevice, surface, swapChainSupport);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
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

    err = vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapchain);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    err = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
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

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    err = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    /* Graphics pipeline */
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
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional, vertex buffer stuff
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional, vertex buffer stuff

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

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;            // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    err = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
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

    err = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);

    swapchainFramebuffers = new VkFramebuffer[imageCount];
    for (size_t i = 0; i < imageCount; ++i)
    {
        VkImageView attachments[]{swapchainImageViews[i]};
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        err = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]);
        if (err != VkResult::VK_SUCCESS)
        {
            return err;
        }
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

    /* CommandBuffer */
    VkCommandBufferAllocateInfo commandBufferAllocInfo{};
    commandBufferAllocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.commandPool = commandPool;
    commandBufferAllocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandBufferCount = 1;

    err = vkAllocateCommandBuffers(device, &commandBufferAllocInfo, &commandBuffer);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    err = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    /* synchronization */

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    err = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }
    err = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0;
    err = vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence);
    if (err != VkResult::VK_SUCCESS)
    {
        return err;
    }

    return VkResult::VK_SUCCESS;
}

Renderer::~Renderer()
{
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);

    vkDestroyCommandPool(device, commandPool, nullptr);
    for (size_t i = 0; i < imageCount; ++i)
    {
        vkDestroyFramebuffer(device, swapchainFramebuffers[i], nullptr);
    }
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (size_t i = 0; i < imageCount; ++i)
    {
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    }
    delete[] swapchainImageViews;
    delete[] swapchainImages;
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
