#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <vector>
#include <utility>

class VulkanEngine
{
	VkInstance instance = 0;
	uint32_t numPhysicalDevices = 0;
	VkPhysicalDevice *physicalDevices = nullptr;
	std::vector<std::pair<uint32_t, VkQueueFamilyProperties *>> physicalDeviceQueueFamilies;
	std::vector<uint32_t> graphicsQueueFamilyIndex; // One per physical device
	std::vector<uint32_t> transferQueueFamilyIndex; // One per physical device
	std::vector<VkDevice> devices;
	std::vector<VkCommandPool> commandPools; // One per device.

public:
	VulkanEngine(void);
	~VulkanEngine(void);

	void init(void);
};
