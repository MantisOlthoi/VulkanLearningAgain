#include "vulkanEngine.h"
#include <stdio.h>
#include <assert.h>
#include <stdexcept>
#include "vulkanEngineInfo.h"
#include "vulkanDebug.h"

#define VERBOSE 1
#define PRINT_FULL_DEVICE_DETAILS 1
#define USE_MULTI_GPU 1

VulkanEngine::VulkanEngine(void)
{
}

VulkanEngine::~VulkanEngine(void)
{
	for (uint32_t i = 0; i < devices.size(); i++)
		vkDestroyCommandPool(devices[i], commandPools[i], nullptr);
	for (auto device : devices)
	{
		VkResult result = vkDeviceWaitIdle(device);
		if (result != VK_SUCCESS)
			fprintf(stderr, "Vulkan Error: Failed to wait for device to idle : %d\n", result);
		vkDestroyDevice(device, nullptr);
	}
	if (physicalDevices) delete[] physicalDevices;
	if (instance) vkDestroyInstance(instance, nullptr);
}

void VulkanEngine::init(void)
{
	VkResult result;

	// Get the Vulkan instance version
	if (VERBOSE)
	{
		uint32_t apiVersion = 0;
		HANDLE_VK(vkEnumerateInstanceVersion(&apiVersion), "Getting Vulkan instance version");
		printf("Vulkan Instance Version: %u.%u.%u\n",
			VK_VERSION_MAJOR(apiVersion),
			VK_VERSION_MINOR(apiVersion),
			VK_VERSION_PATCH(apiVersion));
	}

	// Get what the instance layer properties are available.
	uint32_t numInstanceProperties;
	HANDLE_VK(vkEnumerateInstanceLayerProperties(&numInstanceProperties, nullptr),
		"Querying number of Vulkan instance layer properties");
	VkLayerProperties *instanceLayerProperties = numInstanceProperties ? new VkLayerProperties[numInstanceProperties] : nullptr;
	if (instanceLayerProperties)
	{
		HANDLE_VK(vkEnumerateInstanceLayerProperties(&numInstanceProperties, instanceLayerProperties),
			"Querying Vulkan instance layer properties");
	}

	// Print the instance capabilities out to the user
	if (VERBOSE)
		printInstanceCapabilities();

	// TODO: Insert layer select code here.

	// Create the Vulkan instance.
	VkApplicationInfo applicationInfo = {
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		nullptr, // pNext
		"Learning Vulkan Again", // Application name
		VK_MAKE_VERSION(0,0,1), // Application version
		"Learning Vulkan (WSB)", // Engine name
		VK_MAKE_VERSION(0,0,1), // Engine version
		VK_MAKE_VERSION(1,1,0), // Vulkan API version
	};

	VkInstanceCreateInfo instanceCreateInfo = {
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		nullptr, // pNext - TODO: Should check out the options here.
		0, // flags - reserved for future use.
		&applicationInfo,
		0, // Number of enabled layers
		nullptr, // Enabled layer names
		0, // Number of enabled extensions
		nullptr  // Enabled extension names
	};

	HANDLE_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance), "Creating Vulkan instance");

	// Cleanup
	if (instanceLayerProperties) delete[] instanceLayerProperties;

	// Get the physical devices.
	HANDLE_VK(vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, nullptr),
		"Querying the number of Vulkan physical devices");
	if (!numPhysicalDevices)
	{
		fprintf(stderr, "ERROR (%s:%u): No Vulkan physical devices detected!\n", __FILE__, __LINE__);
		throw std::runtime_error("No Vulkan physical devices detected");
	}
	physicalDevices = new VkPhysicalDevice[numPhysicalDevices];
	HANDLE_VK(vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, physicalDevices),
		"Querying Vulkan physical devices");

	// Print the physical device properties for the user.
	if (VERBOSE)
		printPhysicalDeviceDetails(numPhysicalDevices, physicalDevices, PRINT_FULL_DEVICE_DETAILS);

	// Get the physical device queue families.
	for (uint32_t i = 0; i < numPhysicalDevices; i++)
	{
		uint32_t numQueueFamilies;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &numQueueFamilies, nullptr);
		VkQueueFamilyProperties *queueFamilyProperties = numQueueFamilies ? new VkQueueFamilyProperties[numQueueFamilies] : nullptr;
		if (queueFamilyProperties)
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &numQueueFamilies, queueFamilyProperties);
		physicalDeviceQueueFamilies.push_back(std::make_pair(numQueueFamilies, queueFamilyProperties));
	}

	if (VERBOSE && USE_MULTI_GPU && numPhysicalDevices > 1)
		printf("Using Multi-GPU\n");
	for (uint32_t i = 0; i < (USE_MULTI_GPU ? numPhysicalDevices : 1); i++)
	{
		VkDevice device;

		// Find which queue family has the graphics capability
		uint32_t graphicsQueueIndex = ~0U;
		uint32_t transferQueueIndex = ~0U;
		uint8_t selectedTransferQueueNumFlags = 0xFF;
		for (uint32_t j = 0; j < physicalDeviceQueueFamilies[i].first; j++)
		{
			// Find the first graphics capable queue family
			if (graphicsQueueIndex == ~0U
				&& physicalDeviceQueueFamilies[i].second[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				graphicsQueueIndex = j;
			}

			// Select the Transfer capable queue with the least other capabilities (preferably a transfer-only queue family).
			uint8_t numFlags = 0;
			VkQueueFlags flags = physicalDeviceQueueFamilies[i].second[j].queueFlags;
			do numFlags += flags & 1; while (flags = (flags >> 1));
			if (physicalDeviceQueueFamilies[i].second[j].queueFlags & VK_QUEUE_TRANSFER_BIT
				&& numFlags < selectedTransferQueueNumFlags)
			{
				transferQueueIndex = j;
				selectedTransferQueueNumFlags = numFlags;
			}
		}

		if (VERBOSE)
			printf("Picked %u and %u\n", graphicsQueueIndex, transferQueueIndex);

		VkDeviceQueueCreateInfo deviceQueueCreateInfo[] = {
			{ // Graphics Queue
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				nullptr, // pNext
				0, // Flags
				graphicsQueueIndex, // Queue Family Index
				1, // Number of queues to make
				nullptr // Queue priorities
			},
			{ // Transfer queue
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				nullptr, // pNext
				0, // Flags
				transferQueueIndex, // Queue Family Index
				1, // Number of queues to make
				nullptr // Queue priorities
			}
		};
		VkDeviceCreateInfo deviceCreateInfo = {
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			nullptr, // pNext - TODO: There are some features we can add here.
			0, // Flags, reserved for future use.
			USE_MULTI_GPU && graphicsQueueIndex != transferQueueIndex ? 2U : 1U, // Number of queue families to create.
			deviceQueueCreateInfo,
			0, // Number of layers to enable
			nullptr, // Layers to enable
			0, // Number of extensions to enable
			nullptr, // Extensions to enable
			nullptr  // Features to enable
		};

		HANDLE_VK(vkCreateDevice(physicalDevices[i], &deviceCreateInfo, nullptr, &device),
			"Creating Vulkan device from physical device %u\n", i);

		devices.push_back(device);
		graphicsQueueFamilyIndex.push_back(graphicsQueueIndex);
		transferQueueFamilyIndex.push_back(transferQueueIndex);
	}

	// Create the command pool
	for (uint32_t i = 0; i < devices.size(); i++)
	{
		VkCommandPool commandPool;
		VkCommandPoolCreateInfo createInfo = {
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			nullptr, // pNext,
			0, // Flags
			graphicsQueueFamilyIndex[i] // Queue Family Index
		};
		HANDLE_VK(vkCreateCommandPool(devices[i], &createInfo, nullptr, &commandPool),
			"Creating graphics command pool for device %u", i);

		commandPools.push_back(commandPool);
	}
}
