#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

void printInstanceCapabilities(void);
void printPhysicalDeviceDetails(uint32_t numPhysicalDevices, VkPhysicalDevice *physicalDevices, bool printFullDeviceDetails);