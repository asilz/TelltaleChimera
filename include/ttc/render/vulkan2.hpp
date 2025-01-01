#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <array>
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

    SDL_Window *window = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> commandBuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandPool transferPool = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;

    VkExtent2D swapchainExtent;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences;

    uint32_t currentFrameIndex = 0;
    uint32_t imageCount = 0;
    VkImage *swapchainImages = nullptr;
    VkImageView *swapchainImageViews = nullptr;
    VkFramebuffer *swapchainFramebuffers = nullptr;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

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
    VkResult CreateIndexBuffer();
    VkResult CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkResult RecreateSwapchain();
    VkResult CreateDescriptorSetLayout();
    int64_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);
    void CleanupSwapchain();
    ~Renderer();
};