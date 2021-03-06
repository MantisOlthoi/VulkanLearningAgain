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

void VulkanEngine::createRenderPass(void)
{
	VkAttachmentDescription simpleRenderPassAttachments[] = {
		{ // Depth Buffer
			0, // flags
			VK_FORMAT_R32_SFLOAT, // Format
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
