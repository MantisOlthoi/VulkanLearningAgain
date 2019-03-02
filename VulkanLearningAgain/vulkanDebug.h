#pragma once

#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdexcept>
#include <assert.h>

#define HANDLE_VK(vulkanExpression, ...) { \
	VkResult vulkanResultVar = vulkanExpression; \
	if (vulkanResultVar != VK_SUCCESS) { \
		const char *errStr = "Unknown"; \
		char usrMsg[1024]; \
		char msg[1024]; \
		snprintf(usrMsg, 1024, "" __VA_ARGS__); \
		switch(vulkanResultVar) { \
			case VK_SUCCESS: errStr = "VK_SUCCESS"; break; \
			case VK_NOT_READY: errStr = "VK_NOT_READY"; break; \
			case VK_TIMEOUT: errStr = "VK_TIMEOUT"; break; \
			case VK_EVENT_SET: errStr = "VK_EVENT_SET"; break; \
			case VK_EVENT_RESET: errStr = "VK_EVENT_RESET"; break; \
			case VK_INCOMPLETE: errStr = "VK_INCOMPLETE"; break; \
			case VK_ERROR_OUT_OF_HOST_MEMORY: errStr = "VK_ERROR_OUT_OF_HOST_MEMORY"; break; \
			case VK_ERROR_OUT_OF_DEVICE_MEMORY: errStr = "VK_ERROR_OUT_OF_DEVICE_MEMORY"; break; \
			case VK_ERROR_INITIALIZATION_FAILED: errStr = "VK_ERROR_INITIALIZATION_FAILED"; break; \
			case VK_ERROR_DEVICE_LOST: errStr = "VK_ERROR_DEVICE_LOST"; break; \
			case VK_ERROR_MEMORY_MAP_FAILED: errStr = "VK_ERROR_MEMORY_MAP_FAILED"; break; \
			case VK_ERROR_LAYER_NOT_PRESENT: errStr = "VK_ERROR_LAYER_NOT_PRESENT"; break; \
			case VK_ERROR_EXTENSION_NOT_PRESENT: errStr = "VK_ERROR_EXTENSION_NOT_PRESENT"; break; \
			case VK_ERROR_FEATURE_NOT_PRESENT: errStr = "VK_ERROR_FEATURE_NOT_PRESENT"; break; \
			case VK_ERROR_INCOMPATIBLE_DRIVER: errStr = "VK_ERROR_INCOMPATIBLE_DRIVER"; break; \
			case VK_ERROR_TOO_MANY_OBJECTS: errStr = "VK_ERROR_TOO_MANY_OBJECTS"; break; \
			case VK_ERROR_FORMAT_NOT_SUPPORTED: errStr = "VK_ERROR_FORMAT_NOT_SUPPORTED"; break; \
			case VK_ERROR_FRAGMENTED_POOL: errStr = "VK_ERROR_FRAGMENTED_POOL"; break; \
			case VK_ERROR_OUT_OF_POOL_MEMORY: errStr = "VK_ERROR_OUT_OF_POOL_MEMORY"; break; \
			case VK_ERROR_INVALID_EXTERNAL_HANDLE: errStr = "VK_ERROR_INVALID_EXTERNAL_HANDLE"; break; \
			case VK_ERROR_SURFACE_LOST_KHR: errStr = "VK_ERROR_SURFACE_LOST_KHR"; break; \
			case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: errStr = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"; break; \
			case VK_SUBOPTIMAL_KHR: errStr = "VK_SUBOPTIMAL_KHR"; break; \
			case VK_ERROR_OUT_OF_DATE_KHR: errStr = "VK_ERROR_OUT_OF_DATE_KHR"; break; \
			case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: errStr = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR"; break; \
			case VK_ERROR_VALIDATION_FAILED_EXT: errStr = "VK_ERROR_VALIDATION_FAILED_EXT"; break; \
			case VK_ERROR_INVALID_SHADER_NV: errStr = "VK_ERROR_INVALID_SHADER_NV"; break; \
			case VK_ERROR_NOT_PERMITTED_EXT: errStr = "VK_ERROR_NOT_PERMITTED_EXT"; break; \
		} \
		snprintf(msg, 1024, "Vulkan Error (%s:%u): %s : %s\n", __FILE__, __LINE__, usrMsg, errStr); \
		fprintf(stderr, "%s", msg); \
		throw std::runtime_error(msg); \
	} \
}
