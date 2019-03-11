#include "vulkanEngine.h"
#include <stdio.h>
#include <assert.h>
#include <stdexcept>
#include <sstream>
#include "vulkanEngineInfo.h"
#include "vulkanDebug.h"

// Include SPIR-V
#include "simpleVertex.h"
#include "simpleFragment.h"

// Using SDL2 to simplify cross platform displays.
#include <SDL.h>
#include <SDL_vulkan.h>

VulkanEngine::VulkanEngine(void)
{
}

VulkanEngine::~VulkanEngine(void)
{
	if (swapchain)
	{
		VkResult result = vkDeviceWaitIdle(devices[0]);
		if (result != VK_SUCCESS)
			fprintf(stderr, "Vulkan Error: Failed to wait for device 0 to idle : %X\n", result);
		vkDestroySwapchainKHR(devices[0], swapchain, nullptr);
	}
	for (uint32_t i = 0; i < commandPools.size(); i++)
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

void VulkanEngine::init(SDL_Window *sdlWindow, int screenWidth, int screenHeight)
{
	createInstance(sdlWindow);
	createDevices();
	createSurface(sdlWindow);
	createSwapchain(static_cast<uint32_t>(screenWidth), static_cast<uint32_t>(screenHeight));
	createCommandPools();
	createGraphicsPipeline();
}

void VulkanEngine::createInstance(SDL_Window *sdlWindow)
{
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

	// Ask SDL what extensions it needs.
	unsigned int numRequiredExtensionsSDL;
	if (SDL_Vulkan_GetInstanceExtensions(sdlWindow, &numRequiredExtensionsSDL, nullptr) == SDL_FALSE)
	{
		char errMsg[1024];
		snprintf(errMsg, 1024, "Error (%s:%u): Failed to get the required Vulkan extensions count for the SDL Window : %s",
			__FILE__, __LINE__, SDL_GetError());
		fputs(errMsg, stderr);
		throw std::runtime_error(errMsg);
	}

	std::vector<const char *> requiredExtensions;
	requiredExtensions.resize(requiredExtensions.size() + numRequiredExtensionsSDL);
	if (SDL_Vulkan_GetInstanceExtensions(sdlWindow, &numRequiredExtensionsSDL, requiredExtensions.data()) == SDL_FALSE)
	{
		char errMsg[1024];
		snprintf(errMsg, 1024, "Error (%s:%u): Failed to get the required Vulkan extensions count for the SDL Window : %s",
			__FILE__, __LINE__, SDL_GetError());
		fputs(errMsg, stderr);
		throw std::runtime_error(errMsg);
	}

	// Get the available instance extensions.
	uint32_t numInstanceExtensions;
	HANDLE_VK(vkEnumerateInstanceExtensionProperties("", &numInstanceExtensions, nullptr), "Fetching number of Vulkan instance extensions");
	VkExtensionProperties *instanceExtensions = numInstanceExtensions ? new VkExtensionProperties[numInstanceExtensions] : nullptr;
	HANDLE_VK(vkEnumerateInstanceExtensionProperties("", &numInstanceExtensions, instanceExtensions), "Fetching Vulkan instance extensions");

	// Verify the required extensions are available.
	for (const char *requiredExtension : requiredExtensions)
	{
		bool found = false;
		for (unsigned int i = 0; i < numInstanceExtensions; i++)
			if (strcmp(requiredExtension, instanceExtensions[i].extensionName) == 0)
			{
				found = true;
				break;
			}
		if (!found)
		{
			char errMsg[1024];
			snprintf(errMsg, 1024, "Error(%s:%u): Required Vulkan instance extension \"%s\" is not available.\n",
				__FILE__, __LINE__, requiredExtension);
			fputs(errMsg, stderr);
			throw std::runtime_error(errMsg);
		}
	}

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
		static_cast<uint32_t>(requiredExtensions.size()), // Number of enabled extensions
		requiredExtensions.data()  // Enabled extension names
	};

	HANDLE_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance), "Creating Vulkan instance");
	khrSurfaceExtEnabled = true; // SDL requires KHR_surface so we know it's enabled.

	// Cleanup
	if (instanceLayerProperties) delete[] instanceLayerProperties;
}

void VulkanEngine::createDevices(void)
{
	std::vector<const char *> requiredDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

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
			printf("Graphics Queue Family Index: %u\n"
				   "Transfer Queue Family Index: %u\n",
				graphicsQueueIndex, transferQueueIndex);

		// Check for required extensions.
		uint32_t numDeviceExtensions;
		HANDLE_VK(vkEnumerateDeviceExtensionProperties(physicalDevices[i], "", &numDeviceExtensions, nullptr),
			"Getting number of device extensions on physical device %u", i);
		if (!numDeviceExtensions)
		{
			fprintf(stderr, "Error (%s:%u): Physical device %u does not have any device extensions\n", __FILE__, __LINE__, i);
			throw std::runtime_error("Device does not have required extensions");
		}
		VkExtensionProperties *exts = new VkExtensionProperties[numDeviceExtensions];
		HANDLE_VK(vkEnumerateDeviceExtensionProperties(physicalDevices[i], "", &numDeviceExtensions, exts),
			"Getting device extensions on physical device %u", i);

		for (uint32_t j = 0; j < requiredDeviceExtensions.size(); j++)
		{
			bool found = false;
			for (uint32_t k = 0; k < numDeviceExtensions; k++)
			{
				if (strcmp(requiredDeviceExtensions[j], exts[k].extensionName) == 0)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				fprintf(stderr, "Error (%s:%u): Failed to find required device extension \"%s\" on physical device %u",
					__FILE__, __LINE__,
					requiredDeviceExtensions[j],
					i);
				throw std::runtime_error("Device does not have required extensions");
			}
		}
		
		// Create the device
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
			static_cast<uint32_t>(requiredDeviceExtensions.size()), // Number of extensions to enable
			requiredDeviceExtensions.data(), // Extensions to enable
			nullptr  // Features to enable
		};

		HANDLE_VK(vkCreateDevice(physicalDevices[i], &deviceCreateInfo, nullptr, &device),
			"Creating Vulkan device from physical device %u\n", i);

		devices.push_back(device);
		graphicsQueueFamilyIndex.push_back(graphicsQueueIndex);
		transferQueueFamilyIndex.push_back(transferQueueIndex);

		// Go ahead and get the grapics queue.
		VkQueue graphicsQueue;
		vkGetDeviceQueue(device, graphicsQueueIndex, 0, &graphicsQueue);
		graphicsQueues.push_back(graphicsQueue);
	}
}

void VulkanEngine::createCommandPools(void)
{
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

	// Create one command buffer for each swap image.
	commandBuffers.resize(swapchainImages.size());
	VkCommandBufferAllocateInfo commandBufferAllocInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr, // pNext
		commandPools[0], // Command Pool
		VK_COMMAND_BUFFER_LEVEL_PRIMARY, // Buffer level
		static_cast<uint32_t>(swapchainImages.size()) // Num command buffers to alloc
	};
	HANDLE_VK(vkAllocateCommandBuffers(devices[0], &commandBufferAllocInfo, commandBuffers.data()),
		"Allocating %zu command buffers on device 0", swapchainImages.size());
}

void VulkanEngine::createSurface(SDL_Window *sdlWindow)
{
	if (SDL_Vulkan_CreateSurface(sdlWindow, instance, &surface) == SDL_FALSE)
	{
		fprintf(stderr, "Error (%s:%u): Failed to create Vulkan surface from SDL window : %s\n",
			__FILE__, __LINE__, SDL_GetError());
		throw std::runtime_error("Failed to create Vulkan surface from SDL window");
	}

	if (VERBOSE && PRINT_FULL_DEVICE_DETAILS)
		printPhysicalSurfaceDetails(physicalDevices, numPhysicalDevices, surface);
}

void VulkanEngine::createSwapchain(uint32_t width, uint32_t height)
{
	//////////////////////////////////////////////////////////////////////////////
	//
	// Select an image format and color space to use based on what is supported.
	//
	//////////////////////////////////////////////////////////////////////////////
	// Declare what we want.
	// NOTE: We'll weight the preference for lowest index format over lowest index
	//		 color space.
	std::vector<VkFormat> desiredImageFormats = {
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_R8G8B8A8_UNORM,
	};
	std::vector<VkColorSpaceKHR> desiredImageColorSpaces = {
		VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
	};

	// Get the available formats.
	uint32_t numFormats;
	HANDLE_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[0], surface, &numFormats, nullptr),
		"Getting number of supported formats");
	if (!numFormats)
	{
		fprintf(stderr, "Error (%s:%u): No surface formats found!\n", __FILE__, __LINE__);
		throw std::runtime_error("Failed to find any surface formats");
	}
	VkSurfaceFormatKHR *formats = new VkSurfaceFormatKHR[numFormats];
	HANDLE_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[0], surface, &numFormats, formats),
		"Getting surface formats");

	// Find the best fit format.
	uint32_t selectedFormatIndex = ~0U;
	uint32_t selectedColorSpaceIndex = ~0U;
	for (uint32_t i = 0; i < numFormats; i++)
	{
		for (uint32_t j = 0; j < desiredImageFormats.size() && j <= selectedFormatIndex; j++)
		{
			if (formats[i].format == desiredImageFormats[j])
			{
				for (uint32_t k = 0; k < desiredImageColorSpaces.size() && k < selectedColorSpaceIndex; k++)
				{
					if (formats[i].colorSpace == desiredImageColorSpaces[k])
					{
						selectedColorSpaceIndex = k;
						selectedFormatIndex = j;
						break;
					}
				}
			}
		}
	}
	if (selectedFormatIndex == ~0U || selectedColorSpaceIndex == ~0U)
	{
		fprintf(stderr, "Error (%s:%u): Unable to find a suitable image format for the swap chain.\n", __FILE__, __LINE__);
		throw std::runtime_error("Failed to find a suitable image format for the swap chain");
	}

	if (VERBOSE)
	{
		printf("Selected swap chain image format/colorspace: ");
		printFormatColorSpacePair(desiredImageFormats[selectedFormatIndex],
			desiredImageColorSpaces[selectedColorSpaceIndex]);
		putc('\n', stdout);
	}

	//////////////////////////////////////////////////////////////////////////////
	//
	// Create the swapchain
	//
	//////////////////////////////////////////////////////////////////////////////
	VkSwapchainCreateInfoKHR swapchainCreateInfo =
	{
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		nullptr, // pNext
		0, // flags
		surface,
		3, // Min # images
		desiredImageFormats[selectedFormatIndex], // image format
		desiredImageColorSpaces[selectedColorSpaceIndex], // image color space
		{ width, height }, // image extent
		1, // Number of image array layers
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // Image Usage
		VK_SHARING_MODE_EXCLUSIVE, // Sharing mode.
		1, // Number of queue families
		&graphicsQueueFamilyIndex[0], // Queue family indices
		VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, // Pre-Transform. TODO: Add checking for this.
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // Composite Alpha. TODO: Add checking for this.
		VK_PRESENT_MODE_FIFO_KHR, // Present Mode. TODO: Add checking for this.
		VK_FALSE, // Clipped
		VK_NULL_HANDLE // Old Swapchain.
	};

	HANDLE_VK(vkCreateSwapchainKHR(devices[0], &swapchainCreateInfo, nullptr, &swapchain),
		"Creating the Vulkan swapchain for device 0");
	screenWidth = width;
	screenHeight = height;


	//////////////////////////////////////////////////////////////////////////////
	//
	// Get the swapchain images.
	//
	//////////////////////////////////////////////////////////////////////////////
	uint32_t numSwapchainImages;
	HANDLE_VK(vkGetSwapchainImagesKHR(devices[0], swapchain, &numSwapchainImages, nullptr),
		"Getting number of swap chain images");
	swapchainImages.resize(numSwapchainImages);
	HANDLE_VK(vkGetSwapchainImagesKHR(devices[0], swapchain, &numSwapchainImages, swapchainImages.data()));
}

void VulkanEngine::createGraphicsPipeline(void)
{
	VkShaderModuleCreateInfo simpleVertexShaderCreateInfo = {
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr, // pNext
		0, // flags
		simpleVertexSPRVLength,
		simpleVertexSPRV
	};

	HANDLE_VK(vkCreateShaderModule(devices[0], &simpleVertexShaderCreateInfo, nullptr, &simpleVertexShaderModule),
		"Creating simpleVertex shader module");

	VkShaderModuleCreateInfo simpleFragmentShaderCreateInfo = {
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr, // pNext
		0, // flags
		simpleFragmentSPRVLength,
		simpleFragmentSPRV
	};

	HANDLE_VK(vkCreateShaderModule(devices[0], &simpleFragmentShaderCreateInfo, nullptr, &simpleFragmentShaderModule),
		"Creating simpleFragment shader module");


}
