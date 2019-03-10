#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

void printInstanceCapabilities(void);
void printPhysicalDeviceDetails(uint32_t numPhysicalDevices, VkPhysicalDevice *physicalDevices, bool printFullDeviceDetails);
void printPhysicalSurfaceDetails(VkPhysicalDevice *physicalDevices, uint32_t numDevices, VkSurfaceKHR surface);
void printFormatColorSpacePair(VkFormat format, VkColorSpaceKHR colorSpace);
