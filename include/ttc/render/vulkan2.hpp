#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <array>
#include <vulkan/vulkan.h>

struct QueueFamilyIndices
{
    int64_t graphicsFamily;
    int64_t presentFamily;
};

struct Renderer
{
    SDL_Window *window = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VkExtent2D swapchainExtent;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;

    uint32_t imageCount = 0;
    VkImage *swapchainImages = nullptr;
    VkImageView *swapchainImageViews = nullptr;
    VkFramebuffer *swapchainFramebuffers = nullptr;

    VkResult DrawFrame();
    VkResult RecordCommandBuffer(uint32_t imageIndex);
    VkResult VulkanInit();
    VkResult PickPhysicalDevice();
    VkResult CreateLogicalDevice(QueueFamilyIndices &indices);
    ~Renderer();
};