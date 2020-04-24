#include "vulkanEngine.h"
#include <stdio.h>
#include <assert.h>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include "vulkanEngineInfo.h"
#include "vulkanDebug.h"

// Include SPIR-V
#include "simpleVertex.h"
#include "simpleFragment.h"

// Using SDL2 to simplify cross platform displays.
#include <SDL.h>
#include <SDL_vulkan.h>

#define ENABLE_VALIDATION_LAYER 1

PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerFunc;
PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXTFunc;

VkBool32 debugUtilsMessengerCallbackFunc(
	VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                  messageType,
	const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
	void*                                            pUserData)
{
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: printf("VERBOSE "); break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: printf("INFO "); break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: printf("WARNING "); break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: printf("ERROR "); break;
	default: printf("Unknown severity ");
	}

	switch (messageType)
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: printf(" (GENERAL)"); break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: printf(" (VALIDATION)"); break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: printf(" (PERFORMANCE)"); break;
	default: printf(" (Unknown type)");
	}

	printf(": %s ", pCallbackData->pMessageIdName);
	printf(": %d ", pCallbackData->messageIdNumber);
	printf(": %s\n\n", pCallbackData->pMessage);

	return VK_FALSE;
}

VulkanEngine::VulkanEngine(void)
{
}

VulkanEngine::~VulkanEngine(void)
{
	// Wait for the devices to finish their work.
	for (auto device : devices)
	{
		VkResult result = vkDeviceWaitIdle(devices[0]);
		if (result != VK_SUCCESS)
			fprintf(stderr, "Vulkan Error: Failed to wait for device 0 to idle : %X\n", result);
	}

	// Destroy the buffers
	if (triangleVertexBuffer)
		vkDestroyBuffer(devices[0], triangleVertexBuffer, nullptr);
	for (VkBuffer &ubo : triangleUniformBuffers)
		vkDestroyBuffer(devices[0], ubo, nullptr);
	
	// Destroy the allocated device memory
	if (deviceMemory)
		vkFreeMemory(devices[0], deviceMemory, nullptr);

	// Destroy the graphics pipeline
	if (simpleGraphicsPipeline)
		vkDestroyPipeline(devices[0], simpleGraphicsPipeline, nullptr);

	// Destroy the pipeline layout
	if (simplePipelineLayout)
		vkDestroyPipelineLayout(devices[0], simplePipelineLayout, nullptr);

	// Destroy the pipeline cache
	if (pipelineCache)
		vkDestroyPipelineCache(devices[0], pipelineCache, nullptr);
	
	// Destroy the descriptor set layout
	if (simpleDescriptorSetLayout)
		vkDestroyDescriptorSetLayout(devices[0], simpleDescriptorSetLayout, nullptr);

	// Kill the render pass
	if (simpleRenderPass)
		vkDestroyRenderPass(devices[0], simpleRenderPass, nullptr);

	// Destroy the shader modules
	if (simpleVertexShaderModule)
		vkDestroyShaderModule(devices[0], simpleVertexShaderModule, nullptr);
	if (simpleFragmentShaderModule)
		vkDestroyShaderModule(devices[0], simpleFragmentShaderModule, nullptr);

	// Kill the swapchain
	if (swapchain)
		vkDestroySwapchainKHR(devices[0], swapchain, nullptr);

	// Kill the command pool
	for (uint32_t i = 0; i < commandPools.size(); i++)
		vkDestroyCommandPool(devices[i], commandPools[i], nullptr);

	// Kill the devices
	for (auto device : devices)
		vkDestroyDevice(device, nullptr);

	// Cleanup the memory used for the physical devices
	if (physicalDevices) delete[] physicalDevices;

	if (debugUtilsMessenger)
		vkDestroyDebugUtilsMessengerEXTFunc(instance, debugUtilsMessenger, nullptr);

	// Kill the instance.
	if (instance)
		vkDestroyInstance(instance, nullptr);
}

void VulkanEngine::init(SDL_Window *sdlWindow, int screenWidth, int screenHeight)
{
	createInstance(sdlWindow);
	createDevices();
	createSurface(sdlWindow);
	createSwapchain(static_cast<uint32_t>(screenWidth), static_cast<uint32_t>(screenHeight));
	createDepthBuffers();
	createCommandPools();
	createGraphicsPipeline();
	createFramebuffers();
	initializeSynchronization();
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

	// Print the instance capabilities out to the user
	if (VERBOSE)
		printInstanceCapabilities();

	//////////////////////////////////////////////////////////////
	// Check for the required instances
	//////////////////////////////////////////////////////////////
	std::vector<const char *> requiredInstanceLayers;
	if (ENABLE_VALIDATION_LAYER)
		requiredInstanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");

	if (requiredInstanceLayers.size())
	{
		// Get what the instance layer properties are available.
		uint32_t numInstanceProperties;
		HANDLE_VK(vkEnumerateInstanceLayerProperties(&numInstanceProperties, nullptr),
			"Querying number of Vulkan instance layer properties");
		VkLayerProperties *instanceLayerProperties = numInstanceProperties ? new VkLayerProperties[numInstanceProperties] : nullptr;
		if (instanceLayerProperties)
		{
			HANDLE_VK(vkEnumerateInstanceLayerProperties(&numInstanceProperties, instanceLayerProperties),
				"Querying Vulkan instance layer properties");

			for (const char *requiredInstanceLayer : requiredInstanceLayers)
			{
				bool found = false;
				for (uint32_t i = 0; i < numInstanceProperties; i++)
				{
					if (strcmp(requiredInstanceLayer, instanceLayerProperties[i].layerName) == 0)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					fprintf(stderr, "Error (%s:%u): Failed to find required instance layer \"%s\"\n",
						__FILE__, __LINE__, requiredInstanceLayer);
					throw std::runtime_error("Failed to find required instance layers");
				}
			}

			// Cleanup
			if (instanceLayerProperties) delete[] instanceLayerProperties;
		}
		else
		{
			fprintf(stderr, "ERROR (%s:%u): No instance layers found, but %zu are required.\n",
				__FILE__, __LINE__, requiredInstanceLayers.size());
			throw std::runtime_error("Failed to find required instance layers");
		}
	}

	/////////////////////////////////////////////////////////////
	// Check for required instance extensions
	/////////////////////////////////////////////////////////////
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
	if (ENABLE_VALIDATION_LAYER)
		requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	requiredExtensions.resize(requiredExtensions.size() + numRequiredExtensionsSDL);
	if (SDL_Vulkan_GetInstanceExtensions(sdlWindow, &numRequiredExtensionsSDL, &requiredExtensions.data()[requiredExtensions.size() - numRequiredExtensionsSDL]) == SDL_FALSE)
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

	if (instanceExtensions)
		delete[] instanceExtensions;

	/////////////////////////////////////////////////////////////
	// Create the Vulkan instance.
	/////////////////////////////////////////////////////////////
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
		static_cast<uint32_t>(requiredInstanceLayers.size()), // Number of enabled layers
		requiredInstanceLayers.data(), // Enabled layer names
		static_cast<uint32_t>(requiredExtensions.size()), // Number of enabled extensions
		requiredExtensions.data()  // Enabled extension names
	};

	HANDLE_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance), "Creating Vulkan instance");
	khrSurfaceExtEnabled = true; // SDL requires KHR_surface so we know it's enabled.

	// Enable debugging
	if (ENABLE_VALIDATION_LAYER)
	{
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
			VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			nullptr, // pNext
			0, // Flags
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				// | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			debugUtilsMessengerCallbackFunc,
			nullptr
		};

		vkCreateDebugUtilsMessengerFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		assert(vkCreateDebugUtilsMessengerFunc);

		vkDestroyDebugUtilsMessengerEXTFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		assert(vkDestroyDebugUtilsMessengerEXTFunc);

		HANDLE_VK(vkCreateDebugUtilsMessengerFunc(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugUtilsMessenger),
			"Creating the Debug Utils Messenger. Cause let's face it... This got a lot harder than I expected...");
	}
}

void VulkanEngine::createDevices(void)
{
	std::vector<const char *> requiredDeviceLayers;
	if (ENABLE_VALIDATION_LAYER)
		requiredDeviceLayers.push_back("VK_LAYER_LUNARG_standard_validation");

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

		// Check for required layers.
		if (requiredDeviceLayers.size())
		{
			uint32_t numLayers;
			HANDLE_VK(vkEnumerateDeviceLayerProperties(physicalDevices[i], &numLayers, nullptr),
				"Getting number of device layers on physical device %u", i);
			if (!numLayers)
			{
				fprintf(stderr, "Error (%s:%u): No device layers found, but there are %zu required layers\n",
					__FILE__, __LINE__, requiredDeviceLayers.size());
				throw std::runtime_error("Failed to find required device layers");
			}
			VkLayerProperties *layers = new VkLayerProperties[numLayers];
			if (layers)
			{
				HANDLE_VK(vkEnumerateDeviceLayerProperties(physicalDevices[i], &numLayers, layers),
					"Getting device layers on physical device %u", i);
				for (const char *requiredLayer : requiredDeviceLayers)
				{
					bool found = false;
					for (uint32_t j = 0; j < numLayers; j++)
					{
						if (strcmp(requiredLayer, layers[j].layerName) == 0)
						{
							found = true;
							break;
						}
					}
					if (!found)
					{
						fprintf(stderr, "Error (%s:%u): Failed to find required layer \"%s\"\n",
							__FILE__, __LINE__, requiredLayer);
						throw std::runtime_error("Failed to find required device layers");
					}
				}

				delete[] layers;
			}
		}

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

		if (exts)
			delete[] exts;
		
		// Create the device
		float queuePriorities[] = { 1.0f };
		VkDeviceQueueCreateInfo deviceQueueCreateInfo[] = {
			{ // Graphics Queue
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				nullptr, // pNext
				0, // Flags
				graphicsQueueIndex, // Queue Family Index
				1, // Number of queues to make
				queuePriorities // Queue priorities
			},
			{ // Transfer queue
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				nullptr, // pNext
				0, // Flags
				transferQueueIndex, // Queue Family Index
				1, // Number of queues to make
				queuePriorities // Queue priorities
			}
		};
		VkDeviceCreateInfo deviceCreateInfo = {
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			nullptr, // pNext - TODO: There are some features we can add here.
			0, // Flags, reserved for future use.
			USE_MULTI_GPU && graphicsQueueIndex != transferQueueIndex ? 2U : 1U, // Number of queue families to create.
			deviceQueueCreateInfo,
			static_cast<uint32_t>(requiredDeviceLayers.size()), // Number of layers to enable
			requiredDeviceLayers.data(), // Layers to enable
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
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // Flags
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
		printf("Selected swap chain image format/colorspace: %s : %s",
			formatToString(desiredImageFormats[selectedFormatIndex]),
			colorSpaceToString(desiredImageColorSpaces[selectedColorSpaceIndex]));
		putc('\n', stdout);
	}

	//////////////////////////////////////////////////////////////////////////////
	//
	// Create the swapchain
	//
	//////////////////////////////////////////////////////////////////////////////
	VkBool32 surfaceSupported = VK_FALSE;
	HANDLE_VK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[0], graphicsQueueFamilyIndex[0], surface, &surfaceSupported),
		"Getting device surface support");
	if (surfaceSupported == VK_FALSE)
	{
		fprintf(stderr, "Error (%s:%u): Physical device 0 does NOT support presenting to the given surface\n", __FILE__, __LINE__);
		throw std::runtime_error("Physical devices 0 does not support presenting to the SDL surface");
	}

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
	swapchainImageFormat = desiredImageFormats[selectedFormatIndex];

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

void VulkanEngine::createDepthBuffers(void)
{
	// Pick a depth format to use.
	std::vector<VkFormat> desiredDepthFormats = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT
	};
	for (VkFormat format : desiredDepthFormats)
	{
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevices[0], format, &formatProperties);

		if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			depthBufferImageFormat = format;
			if (VERBOSE)
				printf("Using depth buffer format: %s\n", formatToString(format));
			break;
		}
	}

	/////////////////////////////////////////////////////
	// Create the depth buffer images
	/////////////////////////////////////////////////////
	for (size_t i = 0; i < swapchainImages.size(); i++)
	{
		// Create the depth/stencil buffers
		VkImageCreateInfo depthCreateInfo = {
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr, // pNext
			0, // flags
			VK_IMAGE_TYPE_2D, // image Type
			depthBufferImageFormat, // format
			{ screenWidth, screenHeight, 1 }, // Extent
			1, // Mip levels
			1, // Array layers
			VK_SAMPLE_COUNT_1_BIT, // sample count
			VK_IMAGE_TILING_OPTIMAL, // tiling mode
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, // usage
			VK_SHARING_MODE_EXCLUSIVE, // sharing mode
			1, &graphicsQueueFamilyIndex[0], // queue family
			VK_IMAGE_LAYOUT_UNDEFINED // initial layout
		};
		VkImage depthBuffer;
		HANDLE_VK(vkCreateImage(devices[0], &depthCreateInfo, nullptr, &depthBuffer),
			"Creating depth buffer %zu", i);
		depthBuffers.push_back(depthBuffer);
	}

	/////////////////////////////////////////////////////
	// Allocate memory for the depth buffers
	/////////////////////////////////////////////////////
	VkMemoryRequirements depthBufferMemoryRequirements;
	vkGetImageMemoryRequirements(devices[0], depthBuffers[0], &depthBufferMemoryRequirements);
	
	// Get the other depth buffers' memory requirements so we can silence the Validation layer's warnings.
	for (size_t i = 1; i < depthBuffers.size(); i++)
	{
		VkMemoryRequirements dontCare;
		vkGetImageMemoryRequirements(devices[0], depthBuffers[i], &dontCare);
	}

	// Figure out the total memory space we need for the depth buffers.
	VkDeviceSize memorySpaceRequired = depthBufferMemoryRequirements.size * swapchainImages.size();
	VkDeviceSize requiredPadding = (depthBufferMemoryRequirements.alignment
									- (depthBufferMemoryRequirements.size % depthBufferMemoryRequirements.alignment));
	memorySpaceRequired += (swapchainImages.size() - 1) * requiredPadding;

	// Determine the buffer offsets
	for (size_t i = 0; i < swapchainImages.size(); i++)
	{
		depthBufferMemoryOffsets.push_back(i * (depthBufferMemoryRequirements.size + requiredPadding));
	}

	// Pick a heap to allocate the memory from.
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevices[0], &physicalDeviceMemoryProperties);
	uint32_t pickedType = ~0U;
	VkMemoryPropertyFlags desiredMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	auto countMatchingBits = [desiredMemoryProperties](VkMemoryPropertyFlags a) {
		uint8_t numMatchingBits = 0;
		for (VkMemoryPropertyFlags bit = 1; bit < VK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM; bit <<= 1)
			if ((a & bit) == (desiredMemoryProperties & bit))
				numMatchingBits++;
		return numMatchingBits;
	};
	uint8_t pickedTypeNumMatchingBits = 0;
	for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < physicalDeviceMemoryProperties.memoryTypeCount; memoryTypeIndex++)
	{
		if (depthBufferMemoryRequirements.memoryTypeBits & (1 << memoryTypeIndex))
		{
			// Pick this type if we don't have a picked type yet.
			if (pickedType == ~0U)
			{
				pickedType = memoryTypeIndex;
				pickedTypeNumMatchingBits = countMatchingBits(pickedType);
				break;
			}

			// Pick the type that most matches our desired properties
			uint8_t numMatchingBits = countMatchingBits(physicalDeviceMemoryProperties.memoryTypes[memoryTypeIndex].propertyFlags);
			if (numMatchingBits > pickedTypeNumMatchingBits)
			{
				pickedType = memoryTypeIndex;
				pickedTypeNumMatchingBits = numMatchingBits;
				break;
			}
			
			// If the types match, prefer the larger heap
			if (numMatchingBits == pickedTypeNumMatchingBits)
			{
				uint32_t pickedTypeHeap = physicalDeviceMemoryProperties.memoryTypes[pickedType].heapIndex;
				uint32_t currentTypeHeap = physicalDeviceMemoryProperties.memoryTypes[memoryTypeIndex].heapIndex;
				VkDeviceSize pickedTypeHeapSize = physicalDeviceMemoryProperties.memoryHeaps[pickedTypeHeap].size;
				VkDeviceSize currentTypeHeapSize = physicalDeviceMemoryProperties.memoryHeaps[currentTypeHeap].size;
				if (pickedTypeHeapSize < currentTypeHeapSize)
				{
					pickedType = memoryTypeIndex;
					pickedTypeNumMatchingBits = numMatchingBits;
					break;
				}
			}
		}
	}

	// Allocate the memory
	VkMemoryAllocateInfo memoryAllocateInfo = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr, // pNext
		memorySpaceRequired, // AllocationSize
		pickedType // MemoryTypeIndex
	};
	HANDLE_VK(vkAllocateMemory(devices[0], &memoryAllocateInfo, nullptr, &depthBufferMemory),
		"Allocating memory for the depth buffers\n");
	
	// Bind the memory
	for (size_t i = 0; i < swapchainImages.size(); i++)
	{
		HANDLE_VK(vkBindImageMemory(devices[0], depthBuffers[i], depthBufferMemory, depthBufferMemoryOffsets[i]),
			"Binding memory for depth buffer %zu\n", i);
	}
}

void VulkanEngine::createFramebuffers(void)
{
	// Make sure the swapchain and renderpass are already created.
	assert(swapchain);
	assert(simpleRenderPass);

	/////////////////////////////////////////////////////
	// Create the framebuffers
	/////////////////////////////////////////////////////
	for (size_t i = 0; i < swapchainImages.size(); i++)
	{
		// Create the color buffer attachment
		VkImageViewCreateInfo imageViewCreateInfo = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr, // pNext
			0, // flags
			swapchainImages[i], // image
			VK_IMAGE_VIEW_TYPE_2D, // viewType
			swapchainImageFormat, // format
			{ // Components
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A
			},
			{ // Subresource Range
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, // Base Mip Level
				1, // Level Count
				0, // Base Array Layer
				1, // Layer Count
			}
		};
		VkImageView colorAttachmentImageView;
		HANDLE_VK(vkCreateImageView(devices[0], &imageViewCreateInfo, nullptr, &colorAttachmentImageView),
			"Creating color image view for frambuffer %zu\n", i);

		// Create the depth buffer attachment
		VkImageView depthAttachmentImageView;
		imageViewCreateInfo.image = depthBuffers[i];
		imageViewCreateInfo.format = depthBufferImageFormat;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		HANDLE_VK(vkCreateImageView(devices[0], &imageViewCreateInfo, nullptr, &depthAttachmentImageView),
			"Creating depth image view for framebuffer %zu\n", i);

		VkImageView attachments[2] = { depthAttachmentImageView, colorAttachmentImageView };
		VkFramebufferCreateInfo framebufferCreateInfo = {
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr, // pNext
			0, // flags
			simpleRenderPass, // renderPass
			2, attachments, // pAttachments
			screenWidth, // width
			screenHeight, // height
			1 // Layers
		};
		VkFramebuffer framebuffer;
		HANDLE_VK(vkCreateFramebuffer(devices[0], &framebufferCreateInfo, nullptr, &framebuffer),
			"Creating framebuffer %zu\n", i);

		framebufferAttachmentImageViews.push_back(colorAttachmentImageView);
		framebuffers.push_back(framebuffer);
	}
}

void VulkanEngine::createRenderPass(void)
{
	VkAttachmentDescription simpleRenderPassAttachments[] = {
		{ // Depth Buffer
			0, // flags
			depthBufferImageFormat, // Format
			VK_SAMPLE_COUNT_1_BIT, // Sample count
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, // Load Op
			VK_ATTACHMENT_STORE_OP_DONT_CARE, // Store Op
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, // Stencil Load Op
			VK_ATTACHMENT_STORE_OP_DONT_CARE, // Stencil Store Op
			VK_IMAGE_LAYOUT_UNDEFINED, // Initial layout
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL  // Final layout
		},
		{ // Back Buffer
			0, // flags
			swapchainImageFormat, // Format
			VK_SAMPLE_COUNT_1_BIT, // Sample count
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, // Load Op
			VK_ATTACHMENT_STORE_OP_STORE, // Store Op
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, // Stencil Load Op
			VK_ATTACHMENT_STORE_OP_DONT_CARE, // Stencil Store Op
			VK_IMAGE_LAYOUT_UNDEFINED, // Initial layout
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // Final layout
		}
	};

	VkAttachmentReference depthBufferAttachmentReference = {
		0, // Attachment index
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL // Image layout
	};

	VkAttachmentReference backBufferAttachmentReference = {
		1, // attachment index
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // Image layout
	};

	VkSubpassDescription simpleRenderSubPass = {
		0, // Flags
		VK_PIPELINE_BIND_POINT_GRAPHICS, // Pipeline Bind Point
		0, // Input attachment count
		nullptr, // Input attachments
		1, // Color attachment count
		&backBufferAttachmentReference, // color attachments
		nullptr, // resolve attachments (used for multisampling)
		&depthBufferAttachmentReference, // Depth/Stencil Attachment
		0, // Preserve attachments count
		nullptr // Preserve attachments
	};

	VkRenderPassCreateInfo simpleRenderPassCreateInfo = {
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr, // pNext
		0, // flags
		2, // Attachment count
		simpleRenderPassAttachments, // Attachment descriptions
		1, // Subpass count
		&simpleRenderSubPass, // Subpasses
		0, // Dependency Count
		nullptr // Dependencies
	};

	HANDLE_VK(vkCreateRenderPass(devices[0], &simpleRenderPassCreateInfo, nullptr, &simpleRenderPass),
		"Creating the simple render pass on device 0");
}

void VulkanEngine::createGraphicsPipelineLayout(void)
{
	//////////////////////////////////////////////////////////////////////////////
	//
	// Create descriptor set layout
	//
	//////////////////////////////////////////////////////////////////////////////
	VkDescriptorSetLayoutBinding uboBinding = {
		1, // Binding
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // Descriptor Type
		1, // Descriptor count
		VK_SHADER_STAGE_VERTEX_BIT, // Stage flags
		nullptr // Immuntable samplers
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr, // pNext
		0, // flags
		1, // Binding Count
		&uboBinding
	};

	HANDLE_VK(vkCreateDescriptorSetLayout(devices[0], &descriptorSetLayoutCreateInfo, nullptr, &simpleDescriptorSetLayout),
		"Creating descriptor set layout");

	//////////////////////////////////////////////////////////////////////////////
	//
	// Create pipeline layout
	//
	//////////////////////////////////////////////////////////////////////////////
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		nullptr, // pNext
		0, // flags
		1, // Set Layout Count
		&simpleDescriptorSetLayout, // Set Layouts
		0, // Num Push Constant Ranges
		nullptr // Push Constant Ranges
	};

	HANDLE_VK(vkCreatePipelineLayout(devices[0], &pipelineLayoutCreateInfo, nullptr, &simplePipelineLayout),
		"Creating pipeline layout");
}

void VulkanEngine::createGraphicsPipeline(void)
{
	//////////////////////////////////////////////////////////////////////////////
	//
	// Create the shaders we'll use for the graphics pipeline
	//
	//////////////////////////////////////////////////////////////////////////////
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

	//////////////////////////////////////////////////////////////////////////////
	// Create the render pass and pipeline layout
	//////////////////////////////////////////////////////////////////////////////
	createRenderPass();
	createGraphicsPipelineLayout();

	//////////////////////////////////////////////////////////////////////////////
	//
	// Create the pipeline cache
	//
	//////////////////////////////////////////////////////////////////////////////
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		nullptr, // pNext
		0, // flags
		0, // Initial data size
		nullptr // Initial data
	};

	HANDLE_VK(vkCreatePipelineCache(devices[0], &pipelineCacheCreateInfo, nullptr, &pipelineCache),
		"Creating pipeline cache");

	//////////////////////////////////////////////////////////////////////////////
	//
	// Create the graphics pipeline.
	//
	//////////////////////////////////////////////////////////////////////////////
	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[] = {
		{ // Vertex Shader
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr, // pNext
			0, // Flags
			VK_SHADER_STAGE_VERTEX_BIT, // Stage
			simpleVertexShaderModule, // shader module
			"main", // Shader entry point
			nullptr // Specialization info
		},
		{ // Fragment Shader
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr, // pNext
			0, // Flags
			VK_SHADER_STAGE_FRAGMENT_BIT, // Stage
			simpleFragmentShaderModule, // Shader module
			"main", // Shader entry point
			nullptr // Specialization info
		}
	};

	VkVertexInputBindingDescription vertexInputBindingDescription = {
		0, // Binding
		sizeof(SimpleVertex), // Stride
		VK_VERTEX_INPUT_RATE_VERTEX // Vertex Input Rate
	};

	VkVertexInputAttributeDescription vertexAttributeDescription = {
		0, // Location
		0, // Binding
		VK_FORMAT_R32G32B32_SFLOAT, // Format
		0  // Offset
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr, // pNext
		0, // Flags
		1, // Vertex Binding Description Count
		&vertexInputBindingDescription, // Vertex Binding Descriptions
		1, // Vertex Attribute Description Count
		&vertexAttributeDescription, // Vertex Attribute Descriptions
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		nullptr, // pNext
		0, // Flags
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // Topology
		VK_FALSE // Primitive restart enable
	};

	VkViewport viewport = {
		0, 0, // Starting X,Y position (top left)
		static_cast<float>(screenWidth), // View width
		static_cast<float>(screenHeight), // View height
		0.0f, 1.0f // Depth Min,Max 
	};

	VkRect2D scissor = {
		{ 0, 0 }, // offset
		{ 1, 1 }  // extent
	};

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		nullptr, // pNext
		0, // Flags
		1, // Viewport Count
		&viewport, // Viewport
		1,
		&scissor
		//0, // Scissor Count
		//nullptr // Scissors
	};

	VkPipelineRasterizationStateCreateInfo rasterizationState = {
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		nullptr, // pNext,
		0, // Flags
		VK_FALSE, // Depth Clamp Enable
		VK_FALSE, // Rasterizer Discard Enable (Turn off the Rasterizer)
		VK_POLYGON_MODE_FILL, // Polygon Mode
		VK_CULL_MODE_BACK_BIT, // Cull Mode
		VK_FRONT_FACE_COUNTER_CLOCKWISE, // Front Face (Which way triangle vertices turn)
		VK_FALSE, // Depth Bias Enable
		0.0f, // Depth Bias constant factor
		0.0f, // Depth Bias clamp
		0.0f, // Depth Bias slope factor
		1.0f // Line Width
	};

	VkPipelineMultisampleStateCreateInfo multisampleState = {
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		nullptr, // pNext,
		0, // Flags
		VK_SAMPLE_COUNT_1_BIT, // Resterization samples (sample count)
		VK_FALSE, // Sample Shading Enable
		1.0f, // Min Sample Shading
		nullptr, // Sample Mask
		VK_FALSE, // Alpha to Coverage Enable
		VK_FALSE, // Alpha to One Enable
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		nullptr, // pNext
		0, // Flags
		VK_TRUE, // Depth Test Enable
		VK_TRUE, // Depth Write Enable
		VK_COMPARE_OP_LESS, // Depth Compare Op
		VK_FALSE, // Depth Bounds Test Enable
		VK_FALSE, // Stencil Test Enable
		{ // Front Stencil Op State
			VK_STENCIL_OP_KEEP, // Fail Op
			VK_STENCIL_OP_KEEP, // Pass Op
			VK_STENCIL_OP_KEEP, // Depth Fail Op
			VK_COMPARE_OP_ALWAYS, // Compare Op
			0U, // Compare Mask
			0U, // Write Mask
			0U  // Reference
		},
		{ // Back Stencil Op State
			VK_STENCIL_OP_KEEP, // Fail Op
			VK_STENCIL_OP_KEEP, // Pass Op
			VK_STENCIL_OP_KEEP, // Depth Fail Op
			VK_COMPARE_OP_ALWAYS, // Compare Op
			0U, // Compare Mask
			0U, // Write Mask
			0U  // Reference
		},
		0.0f, // Min Depth Bounds
		1.0f // Max Depth Bounds
	};

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
		VK_FALSE, // Blend enable
		VK_BLEND_FACTOR_ZERO, // Source color blend factor
		VK_BLEND_FACTOR_ZERO, // Destination color blend factor
		VK_BLEND_OP_ADD, // Color Blend operator
		VK_BLEND_FACTOR_ZERO, // Source Alpha Blend factor
		VK_BLEND_FACTOR_ZERO, // Destination alpha blend factor
		VK_BLEND_OP_ADD, // Alhpa blend operator
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT // Color Write mask
	};

	VkPipelineColorBlendStateCreateInfo colorBlendState = {
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		nullptr, // pNext
		0, // Flags
		VK_FALSE, // Logic Op Enable
		VK_LOGIC_OP_COPY, // Logic Op
		1, // Attachment count
		&colorBlendAttachmentState, // Attachment states
		{ 1.0f, 1.0f, 1.0f, 1.0f } // Blend Constants
	};

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		nullptr, // pNext,
		0, // flags
		2, // Stage count
		pipelineShaderStageCreateInfos, // Stages
		&vertexInputStateCreateInfo, // Vertex Input State
		&inputAssemblyStateCreateInfo, // Input Assembly State
		nullptr, // Tessellation state
		&viewportStateCreateInfo, // Viewport State
		&rasterizationState, // Rasterization state
		&multisampleState, // Multisample state
		&depthStencilState, // Depth Stencil State
		&colorBlendState, // Color Blend State
		nullptr, // Dynamic State
		simplePipelineLayout, // Layout. TODO: Create the pipeline layout and add it here.
		simpleRenderPass, // Render pass
		0, // Sub-pass index
		VK_NULL_HANDLE, // Base Pipeline Handle
		0 // Base pipeline index
	};

	HANDLE_VK(vkCreateGraphicsPipelines(devices[0], pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &simpleGraphicsPipeline),
		"Creating graphics pipeline");
}

void VulkanEngine::loadGeometry(void)
{
	///////////////////////////////////////////////////
	//
	// Create the buffers that will be used.
	//
	///////////////////////////////////////////////////
	VkBufferCreateInfo vertexBufferInfo = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr, // pNext,
		0, // flags
		sizeof(SimpleVertex) * triangle.size(), // size
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // usage
		VK_SHARING_MODE_EXCLUSIVE, // sharing mode
		1, // Num queue families
		&graphicsQueueFamilyIndex[0] // queue families
	};
	HANDLE_VK(vkCreateBuffer(devices[0], &vertexBufferInfo, nullptr, &triangleVertexBuffer),
		"Creating vertex buffer for the simple triangle");

	// Create uniform buffers for each swap image.
	VkBufferCreateInfo uniformBufferInfo = {
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr, // pNext
		0, // flags
		sizeof(float) * 16, // size
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, // usage
		VK_SHARING_MODE_EXCLUSIVE, // sharing mode
		1, // Num queue families
		&graphicsQueueFamilyIndex[0] // queue families
	};
	for (size_t i = 0; i < swapchainImages.size(); i++)
	{
		VkBuffer buf;
		HANDLE_VK(vkCreateBuffer(devices[0], &uniformBufferInfo, nullptr, &buf),
			"Creating uniform buffer %zu for triangle", i);
		triangleUniformBuffers.push_back(buf);
	}

	///////////////////////////////////////////////////
	//
	// Determine the amount of memory we'll need.
	//
	///////////////////////////////////////////////////
	// Get the physical device properties so we can know the alignment requirements.
	VkPhysicalDeviceProperties deviceProps;
	vkGetPhysicalDeviceProperties(physicalDevices[0], &deviceProps);

	// Sum up memory requirements
	size_t minMemoryRequirement = 0;
	auto padToAlignment = [&minMemoryRequirement](size_t alignment)
	{
		size_t offsetFromAlignment = minMemoryRequirement % alignment;
		minMemoryRequirement += alignment - offsetFromAlignment;
	};

	auto printMemTypeFlags = [](const char *label, uint32_t types)
	{
		printf("Required mem flags (%s): ", label);
		if (types & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			printf("VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ");
		if (types & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			printf("VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ");
		if (types & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			printf("VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ");
		if (types & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
			printf("VK_MEMORY_PROPERTY_HOST_CACHED_BIT ");
		if (types & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
			printf("VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ");
		if (types & VK_MEMORY_PROPERTY_PROTECTED_BIT)
			printf("VK_MEMORY_PROPERTY_PROTECTED_BIT ");
		putc('\n', stdout);
	};
	
	// Get the requirements for the triangle vertex buffer.
	VkMemoryRequirements memReqs;
	uint32_t bufferMemTypeRequirements = 0;
	vkGetBufferMemoryRequirements(devices[0], triangleVertexBuffer, &memReqs);
	bufferMemTypeRequirements |= memReqs.memoryTypeBits;
	minMemoryRequirement += memReqs.size;
	if (VERBOSE && 0)
		printMemTypeFlags("Triangle Vertex Buffer", memReqs.memoryTypeBits);

	// Get the memory requirements for the uniform buffers.
	vkGetBufferMemoryRequirements(devices[0], triangleUniformBuffers[0], &memReqs); // The buffer requirements should all be identical.
	VkDeviceSize requiredMemAlignment = std::max<VkDeviceSize>(memReqs.alignment, deviceProps.limits.nonCoherentAtomSize);
	assert((requiredMemAlignment % std::min<VkDeviceSize>(memReqs.alignment, deviceProps.limits.nonCoherentAtomSize)) == 0);
	bufferMemTypeRequirements |= memReqs.memoryTypeBits;
	padToAlignment(requiredMemAlignment); // Pad to the required alignment
	uboOffset = minMemoryRequirement; // Snapshot where the UBOs will go.
	for (size_t i = 0; i < swapchainImages.size(); i++)
	{
		vkGetBufferMemoryRequirements(devices[0], triangleUniformBuffers[i], &memReqs); // Calling for each VkBuffer so that the validation layer won't give us a warning.
		padToAlignment(requiredMemAlignment); // First one will be redundant and do nothing.
		triangleMvpOffsets.push_back(minMemoryRequirement);
		minMemoryRequirement += memReqs.size; // Reserve space for the ModelViewProjection matrix
	}
	if (VERBOSE && 0)
		printMemTypeFlags("Triangle MVP buffers", memReqs.memoryTypeBits);

	// NOTE: On my AMD FuryXs, the required flags is 0xF for both buffers.
	//	This implies that these buffers require BOTH device-local AND Host-Accessible,
	//	which as far as I know is infeasible, especially since there are NO memory heaps
	//	that support each of those types together on any of the AMD or NVIDIA cards that
	//	I've gathered information for so far.
	// So for now I'm just going to disregard those particular "requirements" and will
	//	just request host-visible.
	// TODO: After everything is working, try allocating buffers for host-side and device-side
	//	and manually copy the data from the host-side to the device-local buffer to compare
	//	performance

	///////////////////////////////////////////////////
	//
	//	Select a memory pool to use
	//
	///////////////////////////////////////////////////
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevices[0], &memProperties);
	VkMemoryPropertyFlags requiredMemProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	uint32_t selectedTypeIndex = ~0U;
	uint32_t selectedTypeIndexNumFlags = ~0U;

	// Find a heap index that fits the memory requirements with the least number of flags.
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		// First make sure there is enough memory available in the heap.
		if (memProperties.memoryHeaps[memProperties.memoryTypes[i].heapIndex].size < minMemoryRequirement)
			continue;

		// Make sure the type is supported by the buffer.
		if (!(memProperties.memoryTypes[i].propertyFlags & requiredMemProperties))
			continue;

		// Count the number of flags.
		uint32_t numFlags = 0;
		VkMemoryPropertyFlags typeFlags = memProperties.memoryTypes[i].propertyFlags;
		while (typeFlags)
		{
			numFlags += typeFlags & 0x1;
			typeFlags >>= 1;
		}

		if (numFlags < selectedTypeIndexNumFlags)
		{
			selectedTypeIndex = i;
			selectedTypeIndexNumFlags = numFlags;
		}
	}

	///////////////////////////////////////////////////
	//
	//	Allocate the device memory
	//
	///////////////////////////////////////////////////
	VkMemoryAllocateInfo allocInfo = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr, // pNext
		minMemoryRequirement, // allocation size
		selectedTypeIndex
	};
	HANDLE_VK(vkAllocateMemory(devices[0], &allocInfo, nullptr, &deviceMemory),
		"Allocating %zu bytes on device 0 (mem type index %u)",
		minMemoryRequirement, selectedTypeIndex);
	deviceMemorySize = minMemoryRequirement;

	///////////////////////////////////////////////////
	//
	//	Bind the buffers to memory.
	//
	///////////////////////////////////////////////////
	HANDLE_VK(vkBindBufferMemory(devices[0], triangleVertexBuffer, deviceMemory, 0),
		"Binding Triangle vertex buffer to memory at offset 0");
	for (size_t i = 0; i < triangleUniformBuffers.size(); i++)
	{
		HANDLE_VK(vkBindBufferMemory(devices[0], triangleUniformBuffers[i], deviceMemory, triangleMvpOffsets[i]),
			"Binding triangle uniform buffer %zu to memory at offset %zu\n",
			i, triangleMvpOffsets[i]);
	}

	///////////////////////////////////////////////////
	//
	//	Load the triangle into device memory.
	//
	///////////////////////////////////////////////////
	// Give the triangle an initial identity model matrix.
	triangleModelMatrix = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	// Map the allocated memory.
	SimpleVertex *vertexBuffer;
	VkDeviceSize vertexBufferSize = sizeof(SimpleVertex) * triangle.size();
	HANDLE_VK(vkMapMemory(devices[0], deviceMemory, 0, vertexBufferSize, 0, reinterpret_cast<void**>(&vertexBuffer)),
		"Mapping in the vertex buffer for the simple triangle");

	// Copy over the vertices
	memcpy(vertexBuffer, triangle.data(), sizeof(SimpleVertex) * triangle.size());

	// Flush the newly written vertex buffer.
	VkMappedMemoryRange rangeToFlush = {
		VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		nullptr, // pNext
		deviceMemory,
		0,
		VK_WHOLE_SIZE
	};
	HANDLE_VK(vkFlushMappedMemoryRanges(devices[0], 1, &rangeToFlush),
		"Flushing vertex buffer for triangle to device");

	// Unmap the buffer since we're done with it now.
	vkUnmapMemory(devices[0], deviceMemory);

	///////////////////////////////////////////////////
	//
	//	Map the UBOs to memory and keep them there.
	//
	///////////////////////////////////////////////////
	HANDLE_VK(vkMapMemory(devices[0], deviceMemory, uboOffset, minMemoryRequirement - uboOffset, 0, &mappedTriangleUniformBufferSpace),
		"Mapping the triangle uniform buffer space to host memory");
}

void VulkanEngine::initializeSynchronization(void)
{
	for (size_t i = 0; i < swapchainImages.size(); i++)
	{
		VkFenceCreateInfo fenceCreateInfo = {
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			nullptr, // pNext
			0 // flags
		};
		VkFence fence;
		HANDLE_VK(vkCreateFence(devices[0], &fenceCreateInfo, nullptr, &fence),
			"Creating fence for acquiring the next back buffer image");
		renderCompleteFence.push_back(fence);

		VkSemaphore sem;
		VkSemaphoreCreateInfo semCreateInfo = {
			VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			nullptr, // pNext
			0 // Flags
		};
		HANDLE_VK(vkCreateSemaphore(devices[0], &semCreateInfo, nullptr, &sem),
			"Creating semaphore for tracking swapchain image %zu's ready state", i);
		swapchainImageReadySems.push_back(sem);

		HANDLE_VK(vkCreateSemaphore(devices[0], &semCreateInfo, nullptr, &sem),
			"Creating semaphore for tracking render queue for image %zu finished", i);
		renderFinishedSems.push_back(sem);
	}
}

void VulkanEngine::render(void)
{
	///////////////////////////////////////////////////////////
	//
	// Get the next back buffer image index.
	//
	///////////////////////////////////////////////////////////
	static uint32_t renderPassRollingIndex = ~0U;
	renderPassRollingIndex = (renderPassRollingIndex + 1) % static_cast<uint32_t>(swapchainImages.size());
	uint32_t swapchainImageIndex = ~0U;
	HANDLE_VK(vkAcquireNextImageKHR(devices[0],
			swapchain,
			0,
			swapchainImageReadySems[renderPassRollingIndex],
			renderCompleteFence[renderPassRollingIndex],
			&swapchainImageIndex),
		"Acquiring next image index (renderPassIndex: %u)", renderPassRollingIndex);

	VkImage backbuffer = swapchainImages[swapchainImageIndex];

	///////////////////////////////////////////////////////////
	//
	// Load in the next ModelViewProjection matrix
	//
	///////////////////////////////////////////////////////////
	GraphicsMath::Matrix *mvpMatrix = reinterpret_cast<GraphicsMath::Matrix *>(
		(char *)mappedTriangleUniformBufferSpace + (triangleMvpOffsets[swapchainImageIndex] - uboOffset));
	*mvpMatrix = triangleModelMatrix * viewMatrix * projectionMatrix;

	// Flush the new data to the device
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevices[0], &deviceProperties);
	VkMappedMemoryRange mvpMatrixRange = {
		VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		nullptr, // pNext
		deviceMemory,
		triangleMvpOffsets[swapchainImageIndex],
		sizeof(GraphicsMath::Matrix) % deviceProperties.limits.nonCoherentAtomSize ? 
			sizeof(GraphicsMath::Matrix) + (deviceProperties.limits.nonCoherentAtomSize - (sizeof(GraphicsMath::Matrix) % deviceProperties.limits.nonCoherentAtomSize)) :
			sizeof(GraphicsMath::Matrix)
	};
	HANDLE_VK(vkFlushMappedMemoryRanges(devices[0], 1, &mvpMatrixRange),
		"Flushing MVP matrix memory region");

	///////////////////////////////////////////////////////////
	//
	// Draw the next scene
	//
	///////////////////////////////////////////////////////////
	// TODO: See about keeping the command buffers around later
	HANDLE_VK(vkResetCommandBuffer(commandBuffers[swapchainImageIndex], 0),
		"Resetting command buffer");

	VkCommandBufferBeginInfo beginInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr, // pNext
		0, // flags
		nullptr // inheritance info
	};
	HANDLE_VK(vkBeginCommandBuffer(commandBuffers[swapchainImageIndex], &beginInfo),
		"Beginning command buffer");

	// Start the render pass.
	VkRenderPassBeginInfo renderPassBeginInfo = {
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		nullptr, // pNext,
		simpleRenderPass, // RenderPass
		framebuffers[swapchainImageIndex], // Framebuffer
		{ // Render Area
			{ 0, 0 }, // Offset
			{ screenWidth, screenHeight } // Extent
		},
		0, // Clear value count
		nullptr, // pClearValues
	};
	vkCmdBeginRenderPass(commandBuffers[swapchainImageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	///////////////////////////////////////////////////////////
	// Clear the back buffer
	///////////////////////////////////////////////////////////
	// Enqueue a barrier to ready the back buffer for clearing
	VkImageMemoryBarrier clearColorBarrier = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr, // pNext
		VK_ACCESS_TRANSFER_READ_BIT, // Source Access
		VK_ACCESS_TRANSFER_WRITE_BIT, // Destination access
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // Source layout
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Destination layout
		VK_QUEUE_FAMILY_IGNORED, // Source queue - Ignoring since we're not transfering queues
		VK_QUEUE_FAMILY_IGNORED, // Destination queue
		backbuffer, // image
		{ // subresource range
			VK_IMAGE_ASPECT_COLOR_BIT, // Aspect mask
			0, // base mip level
			1, // Level count
			0, // Base array layer
			1, // Layer count
		}
	};
	vkCmdPipelineBarrier(commandBuffers[swapchainImageIndex],
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // Source stage (end of previous render pass)
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Destination stage
		0, // Dependency flags
		0, // Num global memory barriers
		nullptr, // Global memory barriers
		0, // Num buffer memory barriers
		nullptr, // Buffer memory barriers
		1, // Num image memory barriers
		&clearColorBarrier // Image barriers
	);

	// Clear the back buffer. The previous pipeline shifts it to the transfer destination optimal layout.
	VkClearColorValue clearColor = { 0.0f, 0.0f, 0.0f, 0.0f }; // Black and completely transparent.
	VkImageSubresourceRange backbufferColorClearRange = {
		VK_IMAGE_ASPECT_COLOR_BIT, // Aspect mask
		0, // Base MIP Level
		1, // Level count
		0, // Base array layer
		1, // Layer count
	};
	vkCmdClearColorImage(commandBuffers[swapchainImageIndex],
		backbuffer, // Image to clear
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Current layout of the image
		&clearColor, // Color color
		1, // Num ranges
		&backbufferColorClearRange);

	// TODO: Add the depth stencil buffer clear.
	// Repeat for the depth/stencil buffer
	//VkImageMemoryBarrier clearDepthStencilBarrier = {
	//	VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
	//	nullptr, // pNext
	//	VK_ACCESS_TRANSFER_READ_BIT, // Source access
	//	VK_ACCESS_TRANSFER_WRITE_BIT, // Destination access
	//	VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // Source layout
	//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Destination layout
	//	VK_QUEUE_FAMILY_IGNORED, // Source queue - Ignoring since we're not transfering queues
	//	VK_QUEUE_FAMILY_IGNORED, // Destination queue
	//};

	///////////////////////////////////////////////////////////
	// Add the barrier marking the MVP update
	///////////////////////////////////////////////////////////
	// Enqueue the barrier to ready up the MVP matrix for the vertex input stage
	VkMemoryBarrier mvpMemBarrier = {
		VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		nullptr, // pNext
		VK_ACCESS_HOST_WRITE_BIT, // Source access
		VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, // Destination access
	}; // TODO: Determine if this also protects the vertex buffer write during load geometry
	VkBufferMemoryBarrier mvpBufferMemBarrier = {
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		nullptr, // pNext
		VK_ACCESS_HOST_WRITE_BIT, // Source access
		VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, // Destination access
		VK_QUEUE_FAMILY_IGNORED, // Source queue - Ignoring since we're not transfering queues
		VK_QUEUE_FAMILY_IGNORED, // Destination queue
		triangleUniformBuffers[swapchainImageIndex], // buffer to protect
		0, // Offset
		sizeof(GraphicsMath::Matrix) // Size
	};
	vkCmdPipelineBarrier(commandBuffers[swapchainImageIndex],
		VK_PIPELINE_STAGE_HOST_BIT, // src stage
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, // dst stage
		0, // Dependency flags
		1, // Num global memory barriers
		&mvpMemBarrier, // global memory barriers
		1, // Num buffer memory barriers
		&mvpBufferMemBarrier, // buffer memory barriers
		0, // Num image barriers
		nullptr // image barriers
	);

	///////////////////////////////////////////////////////////
	// Draw the geometry
	///////////////////////////////////////////////////////////
	VkDeviceSize zeroSize = 0;
	vkCmdBindVertexBuffers(commandBuffers[swapchainImageIndex], 0, 1, &triangleVertexBuffer, &zeroSize);

	// Pipeline barrier to sync clear color and the subsequent draw
	VkImageMemoryBarrier clearToFragmentShaderBarrier = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr, // pNext
		VK_ACCESS_TRANSFER_WRITE_BIT, // srcAccessMask
		VK_ACCESS_SHADER_WRITE_BIT, // dstAccessMask
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Source layout
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // Dest layout
		VK_QUEUE_FAMILY_IGNORED, // src queue family index
		VK_QUEUE_FAMILY_IGNORED, // dst queue family index
		swapchainImages[swapchainImageIndex], // image
		{ // subresource range
			VK_IMAGE_ASPECT_COLOR_BIT, // aspect mask
			0, 1, 0, 1 // levels and layers
		}
	};
	vkCmdPipelineBarrier(commandBuffers[swapchainImageIndex],
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // src stage
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // dst stage
		VK_DEPENDENCY_BY_REGION_BIT, // dependency
		0, nullptr, // memory barriers
		0, nullptr, // buffer memory barriers
		1, &clearToFragmentShaderBarrier);

	vkCmdDraw(commandBuffers[swapchainImageIndex], static_cast<uint32_t>(triangle.size()), 1, 0, 0);

	// Finish up the render pass
	vkCmdEndRenderPass(commandBuffers[swapchainImageIndex]);

	// Get the framebuffer ready for presenting.
	VkImageMemoryBarrier fragmentToPresentBarrier = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr, // pNext
		VK_ACCESS_SHADER_WRITE_BIT, // srcAccessMask
		VK_ACCESS_TRANSFER_READ_BIT, // dstAccessMask
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // Source layout
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // Dest layout
		VK_QUEUE_FAMILY_IGNORED, // src queue family index
		VK_QUEUE_FAMILY_IGNORED, // dst queue family index
		swapchainImages[swapchainImageIndex], // image
		{ // subresource range
			VK_IMAGE_ASPECT_COLOR_BIT, // aspect mask
			0, 1, 0, 1 // levels and layers
		}
	};
	vkCmdPipelineBarrier(commandBuffers[swapchainImageIndex],
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // src stage
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // dst stage
		VK_DEPENDENCY_BY_REGION_BIT, // dependency
		0, nullptr, // memory barriers
		0, nullptr, // buffer memory barriers
		1, &fragmentToPresentBarrier);

	vkEndCommandBuffer(commandBuffers[swapchainImageIndex]);

	VkSubmitInfo submitInfo = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr, // pNext
		1, &swapchainImageReadySems[renderPassRollingIndex], // Semaphores to wait on
		nullptr, // pWaitDstStageMask
		1, &commandBuffers[swapchainImageIndex], // Command buffers
		1, &renderFinishedSems[swapchainImageIndex] // semaphores to signal
	};
	vkQueueSubmit(graphicsQueues[0], 1, &submitInfo, VK_NULL_HANDLE);

	VkResult presentResult = VK_SUCCESS;
	VkPresentInfoKHR presentInfo = {
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		nullptr, // pNext
		1, &renderFinishedSems[swapchainImageIndex], // Semaphores to wait on
		1, &swapchain, // Swapchains
		&swapchainImageIndex, // image index
		&presentResult // results
	};
	vkQueuePresentKHR(graphicsQueues[0], &presentInfo);
	HANDLE_VK(presentResult, "Queueing swapchain image index %u\n", swapchainImageIndex);
}
