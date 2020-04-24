#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <vector>
#include <utility>
#include "graphicsMath.h"

struct SDL_Window;

class VulkanEngine
{
	VkInstance instance = 0;
	bool khrSurfaceExtEnabled = false;
	uint32_t numPhysicalDevices = 0;
	VkPhysicalDevice *physicalDevices = nullptr;
	std::vector<std::pair<uint32_t, VkQueueFamilyProperties *>> physicalDeviceQueueFamilies;
	std::vector<uint32_t> graphicsQueueFamilyIndex; // One per physical device
	std::vector<uint32_t> transferQueueFamilyIndex; // One per physical device
	std::vector<VkQueue> graphicsQueues; // One per physical device
	std::vector<VkDevice> devices;
	std::vector<VkCommandPool> commandPools; // One per device.
	std::vector<VkCommandBuffer> commandBuffers; // One per swapchain image. (ignoring multi-device for now)
	uint32_t screenWidth = 0;
	uint32_t screenHeight = 0;
	VkSurfaceKHR surface = nullptr;
	VkSwapchainKHR swapchain = nullptr;
	std::vector<VkImage> swapchainImages;
	VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
	std::vector<VkImage> depthBuffers;
	VkFormat depthBufferImageFormat  = VK_FORMAT_UNDEFINED;
	std::vector<VkImageView> framebufferAttachmentImageViews;
	std::vector<VkFramebuffer> framebuffers;
	VkShaderModule simpleVertexShaderModule = nullptr;
	VkShaderModule simpleFragmentShaderModule = nullptr;
	VkRenderPass simpleRenderPass = nullptr;
	VkDescriptorSetLayout simpleDescriptorSetLayout = nullptr;
	VkPipelineLayout simplePipelineLayout = nullptr;
	VkPipelineCache pipelineCache = nullptr;
	VkPipeline simpleGraphicsPipeline = nullptr;
	VkDebugUtilsMessengerEXT debugUtilsMessenger = nullptr; // (added cause the driver threw nullptr expressions =) )

	VkDeviceMemory deviceMemory = nullptr;
	VkDeviceSize deviceMemorySize = ~0ULL;
	VkDeviceSize uboOffset = ~0ULL;
	std::vector<VkDeviceSize> triangleMvpOffsets;
	VkBuffer triangleVertexBuffer = nullptr;
	std::vector<VkBuffer> triangleUniformBuffers;
	void *mappedTriangleUniformBufferSpace = nullptr;

	VkDeviceMemory depthBufferMemory = nullptr;
	VkDeviceSize depthBufferMemorySize = ~0ULL;
	std::vector<VkDeviceSize> depthBufferMemoryOffsets;

	std::vector<VkSemaphore> swapchainImageReadySems;
	std::vector<VkSemaphore> renderFinishedSems;
	std::vector<VkFence> renderCompleteFence;

	void createInstance(SDL_Window *sdlWindow);
	void createDevices(void);
	void createCommandPools(void);
	void createSurface(SDL_Window *sdlWindow);
	void createSwapchain(uint32_t width, uint32_t height);
	void createDepthBuffers(void);
	void createFramebuffers(void);
	void createRenderPass(void);
	void createGraphicsPipelineLayout(void);
	void createGraphicsPipeline(void);
	void initializeSynchronization(void);

	struct SimpleVertex
	{
		float pos[3];
	};

	const std::vector<SimpleVertex> triangle = {
		{ -0.5f, -0.5f, 0.0f }, // Bottom Left
		{  0.5f, -0.5f, 0.0f }, // Bottom Right
		{  0.0f,  0.5f, 0.0f }  // Top Center
	};
	GraphicsMath::Matrix triangleModelMatrix = { 0.0f };
	GraphicsMath::Matrix viewMatrix = { 0.0f };
	GraphicsMath::Matrix projectionMatrix = { 0.0f };

public:
	VulkanEngine(void);
	~VulkanEngine(void);

	void init(SDL_Window *sdlWindow, int screenWidth, int screenHeight);
	void loadGeometry(void);
	void render(void);
};
