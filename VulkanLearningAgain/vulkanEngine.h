#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <vector>
#include <utility>

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
	uint32_t screenWidth;
	uint32_t screenHeight;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	VkFormat swapchainImageFormat;
	VkShaderModule simpleVertexShaderModule;
	VkShaderModule simpleFragmentShaderModule;
	VkRenderPass simpleRenderPass;

	void createInstance(SDL_Window *sdlWindow);
	void createDevices(void);
	void createCommandPools(void);
	void createSurface(SDL_Window *sdlWindow);
	void createSwapchain(uint32_t width, uint32_t height);
	void createRenderPass(void);
	void createGraphicsPipeline(void);

	struct SimpleVertex
	{
		float pos[3];
	};

public:
	VulkanEngine(void);
	~VulkanEngine(void);

	void init(SDL_Window *sdlWindow, int screenWidth, int screenHeight);
};
