#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <array>
#include <tth/animation/animation.hpp>
#include <tth/d3dmesh/d3dmesh.hpp>
#include <tth/skeleton/skeleton.hpp>
#include <vector>
#include <vulkan/vulkan.h>

struct QueueFamilyIndices
{
    int64_t graphicsFamily;
    int64_t presentFamily;
    int64_t transferFamily;
};

struct Renderer
{
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    float time = 0.0f;

    TTH::D3DMesh d3dmesh;
    TTH::Skeleton skeleton;
    TTH::Animation animation;

    TTH::KeyframedValue<TTH::Quaternion> *animationRotations;
    TTH::KeyframedValue<TTH::Vector3> *animationTranslations;

    SDL_Window *window = nullptr;

    VkDeviceMemory hostMemory = VK_NULL_HANDLE;   // Memory that can be mapped
    VkDeviceMemory deviceMemory = VK_NULL_HANDLE; // Memory that cannot be mapped
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    void *stagingBufferMemory = VK_NULL_HANDLE;

    // Should be allocated from device memory. That means I can't access these from CPU and I need to transfer data to it using vkCmdCopyBuffer and
    // submitting the command buffer to a queue with VK_QUEUE_TRANSFER_BIsT.
    VkBuffer vertexBuffer = VK_NULL_HANDLE;  // VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    VkBuffer indexBuffer = VK_NULL_HANDLE;   // VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    VkBuffer uniformBuffer = VK_NULL_HANDLE; // VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT

    VkSemaphore uniformBufferSemaphore = VK_NULL_HANDLE;

    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> commandBuffers;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> uniformCommandBuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandPool transferPool = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;

    VkExtent2D swapchainExtent;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> uniformBufferReadySemaphores;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences;

    uint32_t currentFrameIndex = 0;
    uint32_t imageCount = 0;
    VkImage *swapchainImages = nullptr;
    VkImageView *swapchainImageViews = nullptr;
    VkFramebuffer *swapchainFramebuffers = nullptr;

    VkDescriptorPool descriptorPool;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImage depthImage = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    VkResult DrawFrame();
    VkResult RecordCommandBuffer(uint32_t imageIndex);
    VkResult VulkanInit();
    VkResult PickPhysicalDevice();
    VkResult CreateLogicalDevice(const QueueFamilyIndices &indices);
    VkResult CreateSwapchain(VkSurfaceFormatKHR &surfaceFormat, const QueueFamilyIndices &indices);
    VkResult CreateImageViews(const VkSurfaceFormatKHR &surfaceFormat);
    VkResult CreateFramebuffers();
    VkResult CreateGraphicsPipeline();
    VkResult CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
    VkResult CreateVertexBuffers();
    VkResult CreateVertexBuffersD3D();
    VkResult CreateIndexBuffer();
    VkResult CreateIndexBufferD3D();
    VkResult CreateUniformBuffers();
    VkResult UpdateUniformBuffer();
    VkResult CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkResult RecreateSwapchain();
    VkResult CreateDescriptorSetLayout();
    VkResult CreateDescriptorPool();
    VkResult CreateDescriptorSets();
    VkResult CreateUniformBoneBuffers();
    VkResult CreateTextureImage();
    VkResult CreateDepthResources();
    VkResult InitializeBuffers();
    VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat FindDepthFormat();
    VkResult CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                         VkDeviceMemory &imageMemory);
    int64_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);
    void CleanupSwapchain();
    ~Renderer();
};