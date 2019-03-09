#include "vulkanEngineInfo.h"
#include "vulkanDebug.h"

// Squashing the overall definition of the tabbedPrintf to save on repeat lines.
// NOTE: Kinda brute forcing it here to try and make it quick.
// Another Note: 
//		Being lazy and using a lambda here so I don't have to add a tabs argument to every printf below.
//		Not sure if this is slower than doing printf("%s" ..., tabs, ...), but it honestly doesn't matter here.
#define DEFINE_TABBED_PRINTF(tabLayer) \
	char tabs[16] = { '\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t','\t',0 }; \
	tabs[tabLayer] = '\0'; \
	auto tabbedPrintf = [tabs](auto ...args) \
	{ \
		fputs(tabs, stdout); \
		printf(args...); \
	};

void printInstanceLayerExtensions(const char *layerName, uint8_t tabLayer = 0)
{
	DEFINE_TABBED_PRINTF(tabLayer);

	// Get what extensions are available.
	uint32_t numInstanceLayerExtensions;
	HANDLE_VK(vkEnumerateInstanceExtensionProperties(layerName,
			&numInstanceLayerExtensions, nullptr),
		"Querying number of Vulkan instance extensions for layer \"%s\"",
		layerName);
	VkExtensionProperties *extensionProperties = numInstanceLayerExtensions ?
		new VkExtensionProperties[numInstanceLayerExtensions] : nullptr;
	if (extensionProperties)
	{
		HANDLE_VK(vkEnumerateInstanceExtensionProperties(layerName,
				&numInstanceLayerExtensions,
				extensionProperties),
			"Querying Vulkan instance layer properties (layer: %s)",
			layerName);
	}

	tabbedPrintf("Num %sExtensions: %u\n", strlen(layerName) ? "" : "Instance ", numInstanceLayerExtensions);
	for (uint32_t j = 0; j < numInstanceLayerExtensions; j++)
	{
		tabbedPrintf("\t%s : %u.%u.%u\n",
			extensionProperties[j].extensionName,
			VK_VERSION_MAJOR(extensionProperties[j].specVersion),
			VK_VERSION_MINOR(extensionProperties[j].specVersion),
			VK_VERSION_PATCH(extensionProperties[j].specVersion));
	}

	// Cleanup
	delete[] extensionProperties;
}

void printInstanceCapabilities(void)
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
	}

	// Print the instance capabilities out to the user
	printf("Number of Vulkan Instance Layer Properties: %u\n", numInstanceProperties);
	for (uint32_t i = 0; i < numInstanceProperties; i++)
	{
		printf("\t%s : %u.%u.%u : %u.%u.%u : %s\n",
			instanceLayerProperties[i].layerName,
			VK_VERSION_MAJOR(instanceLayerProperties[i].specVersion),
			VK_VERSION_MINOR(instanceLayerProperties[i].specVersion),
			VK_VERSION_PATCH(instanceLayerProperties[i].specVersion),
			VK_VERSION_MAJOR(instanceLayerProperties[i].implementationVersion),
			VK_VERSION_MINOR(instanceLayerProperties[i].implementationVersion),
			VK_VERSION_PATCH(instanceLayerProperties[i].implementationVersion),
			instanceLayerProperties[i].description);

		// Get what extensions are available.
		printInstanceLayerExtensions(instanceLayerProperties[i].layerName, 2);
	}


	// Get what extensions are available for the instance itself.
	printInstanceLayerExtensions("", 0);

	printf("\n");
}

void printPhysicalDeviceProperties(VkPhysicalDeviceProperties &properties, bool printDeviceName, uint8_t tabLayer)
{
	DEFINE_TABBED_PRINTF(tabLayer);

	// Allow excluding the name in case it was already printed out already.
	if (printDeviceName)
		tabbedPrintf("Device Name: %s\n", properties.deviceName);

	tabbedPrintf("API Version: %u.%u.%u\n",
		VK_VERSION_MAJOR(properties.apiVersion),
		VK_VERSION_MINOR(properties.apiVersion),
		VK_VERSION_PATCH(properties.apiVersion));
	tabbedPrintf("Driver Version: %u.%u.%u\n",
		VK_VERSION_MAJOR(properties.driverVersion),
		VK_VERSION_MINOR(properties.driverVersion),
		VK_VERSION_PATCH(properties.driverVersion));
	tabbedPrintf("Vendor ID: %u\n", properties.vendorID);
	tabbedPrintf("Device ID: %u\n", properties.deviceID);
	tabbedPrintf("Device Type: ");
	switch (properties.deviceType)
	{
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: tabbedPrintf("Integrated GPU\n"); break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: tabbedPrintf("Discrete GPU\n"); break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: tabbedPrintf("Virtual GPU\n"); break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU: tabbedPrintf("CPU\n"); break;
	}
	tabbedPrintf("Pipeline Cache UUID: %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
		properties.pipelineCacheUUID[0],
		properties.pipelineCacheUUID[1],
		properties.pipelineCacheUUID[2],
		properties.pipelineCacheUUID[3],
		properties.pipelineCacheUUID[4],
		properties.pipelineCacheUUID[5],
		properties.pipelineCacheUUID[6],
		properties.pipelineCacheUUID[7],
		properties.pipelineCacheUUID[8],
		properties.pipelineCacheUUID[9],
		properties.pipelineCacheUUID[10],
		properties.pipelineCacheUUID[11],
		properties.pipelineCacheUUID[12],
		properties.pipelineCacheUUID[13],
		properties.pipelineCacheUUID[14],
		properties.pipelineCacheUUID[15]);

	tabbedPrintf("Device Limits:\n");
	tabbedPrintf("\tmaxImageDimension1D: %u\n", properties.limits.maxImageDimension1D);
	tabbedPrintf("\tmaxImageDimension2D: %u\n", properties.limits.maxImageDimension2D);
	tabbedPrintf("\tmaxImageDimension3D: %u\n", properties.limits.maxImageDimension3D);
	tabbedPrintf("\tmaxImageDimensionCube: %u\n", properties.limits.maxImageDimensionCube);
	tabbedPrintf("\tmaxImageArrayLayers: %u\n", properties.limits.maxImageArrayLayers);
	tabbedPrintf("\tmaxTexelBufferElements: %u\n", properties.limits.maxTexelBufferElements);
	tabbedPrintf("\tmaxUniformBufferRange: %u\n", properties.limits.maxUniformBufferRange);
	tabbedPrintf("\tmaxStorageBufferRange: %u\n", properties.limits.maxStorageBufferRange);
	tabbedPrintf("\tmaxPushConstantsSize: %u\n", properties.limits.maxPushConstantsSize);
	tabbedPrintf("\tmaxMemoryAllocationCount: %u\n", properties.limits.maxMemoryAllocationCount);
	tabbedPrintf("\tmaxSamplerAllocationCount: %u\n", properties.limits.maxSamplerAllocationCount);
	tabbedPrintf("\tbufferImageGranularity: %llu\n", properties.limits.bufferImageGranularity);
	tabbedPrintf("\tsparseAddressSpaceSize: %llu\n", properties.limits.sparseAddressSpaceSize);
	tabbedPrintf("\tmaxBoundDescriptorSets: %u\n", properties.limits.maxBoundDescriptorSets);
	tabbedPrintf("\tmaxPerStageDescriptorSamplers: %u\n", properties.limits.maxPerStageDescriptorSamplers);
	tabbedPrintf("\tmaxPerStageDescriptorUniformBuffers: %u\n", properties.limits.maxPerStageDescriptorUniformBuffers);
	tabbedPrintf("\tmaxPerStageDescriptorStorageBuffers: %u\n", properties.limits.maxPerStageDescriptorStorageBuffers);
	tabbedPrintf("\tmaxPerStageDescriptorSampledImages: %u\n", properties.limits.maxPerStageDescriptorSampledImages);
	tabbedPrintf("\tmaxPerStageDescriptorStorageImages: %u\n", properties.limits.maxPerStageDescriptorStorageImages);
	tabbedPrintf("\tmaxPerStageDescriptorInputAttachments: %u\n", properties.limits.maxPerStageDescriptorInputAttachments);
	tabbedPrintf("\tmaxPerStageResources: %u\n", properties.limits.maxPerStageResources);
	tabbedPrintf("\tmaxDescriptorSetSamplers: %u\n", properties.limits.maxDescriptorSetSamplers);
	tabbedPrintf("\tmaxDescriptorSetUniformBuffers: %u\n", properties.limits.maxDescriptorSetUniformBuffers);
	tabbedPrintf("\tmaxDescriptorSetUniformBuffersDynamic: %u\n", properties.limits.maxDescriptorSetUniformBuffersDynamic);
	tabbedPrintf("\tmaxDescriptorSetStorageBuffers: %u\n", properties.limits.maxDescriptorSetStorageBuffers);
	tabbedPrintf("\tmaxDescriptorSetStorageBuffersDynamic: %u\n", properties.limits.maxDescriptorSetStorageBuffersDynamic);
	tabbedPrintf("\tmaxDescriptorSetSampledImages: %u\n", properties.limits.maxDescriptorSetSampledImages);
	tabbedPrintf("\tmaxDescriptorSetStorageImages: %u\n", properties.limits.maxDescriptorSetStorageImages);
	tabbedPrintf("\tmaxDescriptorSetInputAttachments: %u\n", properties.limits.maxDescriptorSetInputAttachments);
	tabbedPrintf("\tmaxVertexInputAttributes: %u\n", properties.limits.maxVertexInputAttributes);
	tabbedPrintf("\tmaxVertexInputBindings: %u\n", properties.limits.maxVertexInputBindings);
	tabbedPrintf("\tmaxVertexInputAttributeOffset: %u\n", properties.limits.maxVertexInputAttributeOffset);
	tabbedPrintf("\tmaxVertexInputBindingStride: %u\n", properties.limits.maxVertexInputBindingStride);
	tabbedPrintf("\tmaxVertexOutputComponents: %u\n", properties.limits.maxVertexOutputComponents);
	tabbedPrintf("\tmaxTessellationGenerationLevel: %u\n", properties.limits.maxTessellationGenerationLevel);
	tabbedPrintf("\tmaxTessellationPatchSize: %u\n", properties.limits.maxTessellationPatchSize);
	tabbedPrintf("\tmaxTessellationControlPerVertexInputComponents: %u\n", properties.limits.maxTessellationControlPerVertexInputComponents);
	tabbedPrintf("\tmaxTessellationControlPerVertexOutputComponents: %u\n", properties.limits.maxTessellationControlPerVertexOutputComponents);
	tabbedPrintf("\tmaxTessellationControlPerPatchOutputComponents: %u\n", properties.limits.maxTessellationControlPerPatchOutputComponents);
	tabbedPrintf("\tmaxTessellationControlTotalOutputComponents: %u\n", properties.limits.maxTessellationControlTotalOutputComponents);
	tabbedPrintf("\tmaxTessellationEvaluationInputComponents: %u\n", properties.limits.maxTessellationEvaluationInputComponents);
	tabbedPrintf("\tmaxTessellationEvaluationOutputComponents: %u\n", properties.limits.maxTessellationEvaluationOutputComponents);
	tabbedPrintf("\tmaxGeometryShaderInvocations: %u\n", properties.limits.maxGeometryShaderInvocations);
	tabbedPrintf("\tmaxGeometryInputComponents: %u\n", properties.limits.maxGeometryInputComponents);
	tabbedPrintf("\tmaxGeometryOutputComponents: %u\n", properties.limits.maxGeometryOutputComponents);
	tabbedPrintf("\tmaxGeometryOutputVertices: %u\n", properties.limits.maxGeometryOutputVertices);
	tabbedPrintf("\tmaxGeometryTotalOutputComponents: %u\n", properties.limits.maxGeometryTotalOutputComponents);
	tabbedPrintf("\tmaxFragmentInputComponents: %u\n", properties.limits.maxFragmentInputComponents);
	tabbedPrintf("\tmaxFragmentOutputAttachments: %u\n", properties.limits.maxFragmentOutputAttachments);
	tabbedPrintf("\tmaxFragmentDualSrcAttachments: %u\n", properties.limits.maxFragmentDualSrcAttachments);
	tabbedPrintf("\tmaxFragmentCombinedOutputResources: %u\n", properties.limits.maxFragmentCombinedOutputResources);
	tabbedPrintf("\tmaxComputeSharedMemorySize: %u\n", properties.limits.maxComputeSharedMemorySize);
	tabbedPrintf("\tmaxComputeWorkGroupCount: [ %u, %u, %u ]\n",
		properties.limits.maxComputeWorkGroupCount[0],
		properties.limits.maxComputeWorkGroupCount[1],
		properties.limits.maxComputeWorkGroupCount[2]);
	tabbedPrintf("\tmaxComputeWorkGroupInvocations: %u\n", properties.limits.maxComputeWorkGroupInvocations);
	tabbedPrintf("\tmaxComputeWorkGroupSize: [ %u, %u, %u ]\n",
		properties.limits.maxComputeWorkGroupSize[0],
		properties.limits.maxComputeWorkGroupSize[1],
		properties.limits.maxComputeWorkGroupSize[2]);
	tabbedPrintf("\tsubPixelPrecisionBits: %u\n", properties.limits.subPixelPrecisionBits);
	tabbedPrintf("\tsubTexelPrecisionBits: %u\n", properties.limits.subTexelPrecisionBits);
	tabbedPrintf("\tmipmapPrecisionBits: %u\n", properties.limits.mipmapPrecisionBits);
	tabbedPrintf("\tmaxDrawIndexedIndexValue: %u\n", properties.limits.maxDrawIndexedIndexValue);
	tabbedPrintf("\tmaxDrawIndirectCount: %u\n", properties.limits.maxDrawIndirectCount);
	tabbedPrintf("\tmaxSamplerLodBias: %f\n", properties.limits.maxSamplerLodBias);
	tabbedPrintf("\tmaxSamplerAnisotropy: %f\n", properties.limits.maxSamplerAnisotropy);
	tabbedPrintf("\tmaxViewports: %u\n", properties.limits.maxViewports);
	tabbedPrintf("\tmaxViewportDimensions: [ %u, %u ]\n",
		properties.limits.maxViewportDimensions[0],
		properties.limits.maxViewportDimensions[1]);
	tabbedPrintf("\tviewportBoundsRange: [ %f, %f ]\n",
		properties.limits.viewportBoundsRange[0],
		properties.limits.viewportBoundsRange[1]);
	tabbedPrintf("\tviewportSubPixelBits: %u\n", properties.limits.viewportSubPixelBits);
	tabbedPrintf("\tminMemoryMapAlignment: %zu\n", properties.limits.minMemoryMapAlignment);
	tabbedPrintf("\tminTexelBufferOffsetAlignment: %llu\n", properties.limits.minTexelBufferOffsetAlignment);
	tabbedPrintf("\tminUniformBufferOffsetAlignment: %llu\n", properties.limits.minUniformBufferOffsetAlignment);
	tabbedPrintf("\tminStorageBufferOffsetAlignment: %llu\n", properties.limits.minStorageBufferOffsetAlignment);
	tabbedPrintf("\tminTexelOffset: %d\n", properties.limits.minTexelOffset);
	tabbedPrintf("\tmaxTexelOffset: %u\n", properties.limits.maxTexelOffset);
	tabbedPrintf("\tminTexelGatherOffset: %d\n", properties.limits.minTexelGatherOffset);
	tabbedPrintf("\tmaxTexelGatherOffset: %u\n", properties.limits.maxTexelGatherOffset);
	tabbedPrintf("\tminInterpolationOffset: %f\n", properties.limits.minInterpolationOffset);
	tabbedPrintf("\tmaxInterpolationOffset: %f\n", properties.limits.maxInterpolationOffset);
	tabbedPrintf("\tsubPixelInterpolationOffsetBits: %u\n", properties.limits.subPixelInterpolationOffsetBits);
	tabbedPrintf("\tmaxFramebufferWidth: %u\n", properties.limits.maxFramebufferWidth);
	tabbedPrintf("\tmaxFramebufferHeight: %u\n", properties.limits.maxFramebufferHeight);
	tabbedPrintf("\tmaxFramebufferLayers: %u\n", properties.limits.maxFramebufferLayers);
	tabbedPrintf("\tframebufferColorSampleCounts: 0x%X\n", properties.limits.framebufferColorSampleCounts);
	tabbedPrintf("\tframebufferDepthSampleCounts: 0x%X\n", properties.limits.framebufferDepthSampleCounts);
	tabbedPrintf("\tframebufferStencilSampleCounts: 0x%X\n", properties.limits.framebufferStencilSampleCounts);
	tabbedPrintf("\tframebufferNoAttachmentsSampleCounts: 0x%X\n", properties.limits.framebufferNoAttachmentsSampleCounts);
	tabbedPrintf("\tmaxColorAttachments: %u\n", properties.limits.maxColorAttachments);
	tabbedPrintf("\tsampledImageColorSampleCounts: 0x%X\n", properties.limits.sampledImageColorSampleCounts);
	tabbedPrintf("\tsampledImageIntegerSampleCounts: 0x%X\n", properties.limits.sampledImageIntegerSampleCounts);
	tabbedPrintf("\tsampledImageDepthSampleCounts: 0x%X\n", properties.limits.sampledImageDepthSampleCounts);
	tabbedPrintf("\tsampledImageStencilSampleCounts: 0x%X\n", properties.limits.sampledImageStencilSampleCounts);
	tabbedPrintf("\tstorageImageSampleCounts: 0x%X\n", properties.limits.storageImageSampleCounts);
	tabbedPrintf("\tmaxSampleMaskWords: %u\n", properties.limits.maxSampleMaskWords);
	tabbedPrintf("\ttimestampComputeAndGraphics: %s\n", properties.limits.timestampComputeAndGraphics ? "TRUE" : "FALSE");
	tabbedPrintf("\ttimestampPeriod: %f\n", properties.limits.timestampPeriod);
	tabbedPrintf("\tmaxClipDistances: %u\n", properties.limits.maxClipDistances);
	tabbedPrintf("\tmaxCullDistances: %u\n", properties.limits.maxCullDistances);
	tabbedPrintf("\tmaxCombinedClipAndCullDistances: %u\n", properties.limits.maxCombinedClipAndCullDistances);
	tabbedPrintf("\tdiscreteQueuePriorities: %u\n", properties.limits.discreteQueuePriorities);
	tabbedPrintf("\tpointSizeRange: [ %f, %f ]\n",
		properties.limits.pointSizeRange[0],
		properties.limits.pointSizeRange[1]);
	tabbedPrintf("\tlineWidthRange: [ %f, %f ]\n",
		properties.limits.lineWidthRange[0],
		properties.limits.lineWidthRange[1]);
	tabbedPrintf("\tpointSizeGranularity: %f\n", properties.limits.pointSizeGranularity);
	tabbedPrintf("\tlineWidthGranularity: %f\n", properties.limits.lineWidthGranularity);
	tabbedPrintf("\tstrictLines: %s\n", properties.limits.strictLines ? "TRUE" : "FALSE");
	tabbedPrintf("\tstandardSampleLocations: %s\n", properties.limits.standardSampleLocations ? "TRUE" : "FALSE");
	tabbedPrintf("\toptimalBufferCopyOffsetAlignment: %llu\n", properties.limits.optimalBufferCopyOffsetAlignment);
	tabbedPrintf("\toptimalBufferCopyRowPitchAlignment: %llu\n", properties.limits.optimalBufferCopyRowPitchAlignment);
	tabbedPrintf("\tnonCoherentAtomSize: %llu\n", properties.limits.nonCoherentAtomSize);

	tabbedPrintf("Sparse Properties:\n");
	tabbedPrintf("\tresidencyStandard2DBlockShape: %s\n", properties.sparseProperties.residencyStandard2DBlockShape ? "TRUE" : "FALSE");
	tabbedPrintf("\tresidencyStandard2DMultisampleBlockShape: %s\n", properties.sparseProperties.residencyStandard2DMultisampleBlockShape ? "TRUE" : "FALSE");
	tabbedPrintf("\tresidencyStandard3DBlockShape: %s\n", properties.sparseProperties.residencyStandard3DBlockShape ? "TRUE" : "FALSE");
	tabbedPrintf("\tresidencyAlignedMipSize: %s\n", properties.sparseProperties.residencyAlignedMipSize ? "TRUE" : "FALSE");
	tabbedPrintf("\tresidencyNonResidentStrict: %s\n", properties.sparseProperties.residencyNonResidentStrict ? "TRUE" : "FALSE");
}

void printPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint8_t tabLayer)
{
	DEFINE_TABBED_PRINTF(tabLayer);

	// Print the physical device queue family properties.
	uint32_t numQueueFamilies;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, nullptr);
	VkQueueFamilyProperties *queueFamilyProperties = new VkQueueFamilyProperties[numQueueFamilies];
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, queueFamilyProperties);
	
	tabbedPrintf("Num queue family properties: %u\n", numQueueFamilies);
	for (uint32_t j = 0; j < numQueueFamilies; j++)
	{
		tabbedPrintf("\tQueue Flags:");
		if (queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			printf("Graphics");
		if (queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT)
			printf("Compute");
		if (queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT)
			printf("Transfer");
		if (queueFamilyProperties[j].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
			printf("Sparse-Binding");
		if (queueFamilyProperties[j].queueFlags & VK_QUEUE_PROTECTED_BIT)
			printf("Protected");
		if (vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, j) == VK_TRUE)
			printf("Present-Win32");
		putc('\n', stdout);
		tabbedPrintf("\tQueue Count: %u\n", queueFamilyProperties[j].queueCount);
		tabbedPrintf("\tTimestamp Valid Bits: %u\n", queueFamilyProperties[j].timestampValidBits);
		tabbedPrintf("\tMin Image Transfer Granularity: [ %u, %u, %u ]\n",
			queueFamilyProperties[j].minImageTransferGranularity.width,
			queueFamilyProperties[j].minImageTransferGranularity.height,
			queueFamilyProperties[j].minImageTransferGranularity.depth);
		putc('\n', stdout);
	}

	delete[] queueFamilyProperties;
};

void printPhysicalDeviceLayerExtensions(VkPhysicalDevice physicalDevice, const char *layerName, uint8_t tabLayer)
{
	DEFINE_TABBED_PRINTF(tabLayer);

	uint32_t numExt;
	HANDLE_VK(vkEnumerateDeviceExtensionProperties(physicalDevice,
			layerName, &numExt, nullptr),
		"Querying for number of extensions of layer \"%s\" on physical device\n",
		layerName);
	tabbedPrintf("Number of %sextensions: %u\n", strlen(layerName) ? "" : "device ", numExt);
	VkExtensionProperties *exts = numExt ? new VkExtensionProperties[numExt] : nullptr;
	if (exts)
	{
		HANDLE_VK(vkEnumerateDeviceExtensionProperties(physicalDevice,
				layerName, &numExt, exts),
			"Querying for number of extensions of layer \"%s\" on physical device\n",
			layerName);

		for (uint32_t k = 0; k < numExt; k++)
		{
			tabbedPrintf("\tExtension: %s : %u.%u.%u\n",
				exts[k].extensionName,
				VK_VERSION_MAJOR(exts[k].specVersion),
				VK_VERSION_MINOR(exts[k].specVersion),
				VK_VERSION_PATCH(exts[k].specVersion));
		}

		delete[] exts;
	}
}

void printPhysicalDeviceLayers(VkPhysicalDevice physicalDevice, uint8_t tabLayer)
{
	DEFINE_TABBED_PRINTF(tabLayer);

	uint32_t numLayers;
	HANDLE_VK(vkEnumerateDeviceLayerProperties(physicalDevice, &numLayers, nullptr),
		"Querying number of device layers on physical device\n");
	VkLayerProperties *layers = numLayers ? new VkLayerProperties[numLayers] : nullptr;
	if (layers)
	{
		HANDLE_VK(vkEnumerateDeviceLayerProperties(physicalDevice, &numLayers, layers),
			"Querying device layers on physical device\n");

		tabbedPrintf("Number of device layers: %u\n", numLayers);
		for (uint32_t j = 0; j < numLayers; j++)
		{
			tabbedPrintf("\tLayer: %s : %u.%u.%u : %u.%u.%u : %s\n",
				layers[j].layerName,
				VK_VERSION_MAJOR(layers[j].specVersion),
				VK_VERSION_MINOR(layers[j].specVersion),
				VK_VERSION_PATCH(layers[j].specVersion),
				VK_VERSION_MAJOR(layers[j].implementationVersion),
				VK_VERSION_MINOR(layers[j].implementationVersion),
				VK_VERSION_PATCH(layers[j].implementationVersion),
				layers[j].description);

			// Get the extensions for this layer
			printPhysicalDeviceLayerExtensions(physicalDevice, layers[j].layerName, tabLayer+2);
		}

		delete[] layers;
	}
}

void printPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, uint8_t tabLayer)
{
	DEFINE_TABBED_PRINTF(tabLayer);

	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	tabbedPrintf("robustBufferAccess: %s\n", features.robustBufferAccess ? "True" : "False");
	tabbedPrintf("fullDrawIndexUint32: %s\n", features.fullDrawIndexUint32 ? "True" : "False");
	tabbedPrintf("imageCubeArray: %s\n", features.imageCubeArray ? "True" : "False");
	tabbedPrintf("independentBlend: %s\n", features.independentBlend ? "True" : "False");
	tabbedPrintf("geometryShader: %s\n", features.geometryShader ? "True" : "False");
	tabbedPrintf("tessellationShader: %s\n", features.tessellationShader ? "True" : "False");
	tabbedPrintf("sampleRateShading: %s\n", features.sampleRateShading ? "True" : "False");
	tabbedPrintf("dualSrcBlend: %s\n", features.dualSrcBlend ? "True" : "False");
	tabbedPrintf("logicOp: %s\n", features.logicOp ? "True" : "False");
	tabbedPrintf("multiDrawIndirect: %s\n", features.multiDrawIndirect ? "True" : "False");
	tabbedPrintf("drawIndirectFirstInstance: %s\n", features.drawIndirectFirstInstance ? "True" : "False");
	tabbedPrintf("depthClamp: %s\n", features.depthClamp ? "True" : "False");
	tabbedPrintf("depthBiasClamp: %s\n", features.depthBiasClamp ? "True" : "False");
	tabbedPrintf("fillModeNonSolid: %s\n", features.fillModeNonSolid ? "True" : "False");
	tabbedPrintf("depthBounds: %s\n", features.depthBounds ? "True" : "False");
	tabbedPrintf("wideLines: %s\n", features.wideLines ? "True" : "False");
	tabbedPrintf("largePoints: %s\n", features.largePoints ? "True" : "False");
	tabbedPrintf("alphaToOne: %s\n", features.alphaToOne ? "True" : "False");
	tabbedPrintf("multiViewport: %s\n", features.multiViewport ? "True" : "False");
	tabbedPrintf("samplerAnisotropy: %s\n", features.samplerAnisotropy ? "True" : "False");
	tabbedPrintf("textureCompressionETC2: %s\n", features.textureCompressionETC2 ? "True" : "False");
	tabbedPrintf("textureCompressionASTC_LDR: %s\n", features.textureCompressionASTC_LDR ? "True" : "False");
	tabbedPrintf("textureCompressionBC: %s\n", features.textureCompressionBC ? "True" : "False");
	tabbedPrintf("occlusionQueryPrecise: %s\n", features.occlusionQueryPrecise ? "True" : "False");
	tabbedPrintf("pipelineStatisticsQuery: %s\n", features.pipelineStatisticsQuery ? "True" : "False");
	tabbedPrintf("vertexPipelineStoresAndAtomics: %s\n", features.vertexPipelineStoresAndAtomics ? "True" : "False");
	tabbedPrintf("fragmentStoresAndAtomics: %s\n", features.fragmentStoresAndAtomics ? "True" : "False");
	tabbedPrintf("shaderTessellationAndGeometryPointSize: %s\n", features.shaderTessellationAndGeometryPointSize ? "True" : "False");
	tabbedPrintf("shaderImageGatherExtended: %s\n", features.shaderImageGatherExtended ? "True" : "False");
	tabbedPrintf("shaderStorageImageExtendedFormats: %s\n", features.shaderStorageImageExtendedFormats ? "True" : "False");
	tabbedPrintf("shaderStorageImageMultisample: %s\n", features.shaderStorageImageMultisample ? "True" : "False");
	tabbedPrintf("shaderStorageImageReadWithoutFormat: %s\n", features.shaderStorageImageReadWithoutFormat ? "True" : "False");
	tabbedPrintf("shaderStorageImageWriteWithoutFormat: %s\n", features.shaderStorageImageWriteWithoutFormat ? "True" : "False");
	tabbedPrintf("shaderUniformBufferArrayDynamicIndexing: %s\n", features.shaderUniformBufferArrayDynamicIndexing ? "True" : "False");
	tabbedPrintf("shaderSampledImageArrayDynamicIndexing: %s\n", features.shaderSampledImageArrayDynamicIndexing ? "True" : "False");
	tabbedPrintf("shaderStorageBufferArrayDynamicIndexing: %s\n", features.shaderStorageBufferArrayDynamicIndexing ? "True" : "False");
	tabbedPrintf("shaderStorageImageArrayDynamicIndexing: %s\n", features.shaderStorageImageArrayDynamicIndexing ? "True" : "False");
	tabbedPrintf("shaderClipDistance: %s\n", features.shaderClipDistance ? "True" : "False");
	tabbedPrintf("shaderCullDistance: %s\n", features.shaderCullDistance ? "True" : "False");
	tabbedPrintf("shaderFloat64: %s\n", features.shaderFloat64 ? "True" : "False");
	tabbedPrintf("shaderInt64: %s\n", features.shaderInt64 ? "True" : "False");
	tabbedPrintf("shaderInt16: %s\n", features.shaderInt16 ? "True" : "False");
	tabbedPrintf("shaderResourceResidency: %s\n", features.shaderResourceResidency ? "True" : "False");
	tabbedPrintf("shaderResourceMinLod: %s\n", features.shaderResourceMinLod ? "True" : "False");
	tabbedPrintf("sparseBinding: %s\n", features.sparseBinding ? "True" : "False");
	tabbedPrintf("sparseResidencyBuffer: %s\n", features.sparseResidencyBuffer ? "True" : "False");
	tabbedPrintf("sparseResidencyImage2D: %s\n", features.sparseResidencyImage2D ? "True" : "False");
	tabbedPrintf("sparseResidencyImage3D: %s\n", features.sparseResidencyImage3D ? "True" : "False");
	tabbedPrintf("sparseResidency2Samples: %s\n", features.sparseResidency2Samples ? "True" : "False");
	tabbedPrintf("sparseResidency4Samples: %s\n", features.sparseResidency4Samples ? "True" : "False");
	tabbedPrintf("sparseResidency8Samples: %s\n", features.sparseResidency8Samples ? "True" : "False");
	tabbedPrintf("sparseResidency16Samples: %s\n", features.sparseResidency16Samples ? "True" : "False");
	tabbedPrintf("sparseResidencyAliased: %s\n", features.sparseResidencyAliased ? "True" : "False");
	tabbedPrintf("variableMultisampleRate: %s\n", features.variableMultisampleRate ? "True" : "False");
	tabbedPrintf("inheritedQueries: %s\n", features.inheritedQueries ? "True" : "False");
}

void printPhysicalDeviceMemoryDetails(VkPhysicalDevice physicalDevice, uint8_t tabLayer)
{
	DEFINE_TABBED_PRINTF(tabLayer);

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

	tabbedPrintf("Physical Memory Types:\n");
	for (uint32_t j = 0; j < memProps.memoryTypeCount; j++)
	{
		tabbedPrintf("\tIndex %d: ", memProps.memoryTypes[j].heapIndex);
		if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) printf(" Device-Local");
		if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) printf(" Host-Visible");
		if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) printf(" Host-Coherent");
		if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) printf(" Host-Cached");
		if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) printf(" Lazily-Allocated");
		if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT) printf(" Protected");
		putc('\n', stdout);
	}

	tabbedPrintf("Physical Memory Heaps:\n");
	for (uint32_t j = 0; j < memProps.memoryHeapCount; j++)
	{
		tabbedPrintf("\tHeap %d: %lf MB :", j, memProps.memoryHeaps[j].size / pow(2.0, 20.0));
		if (memProps.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) printf(" Device-Local");
		if (memProps.memoryHeaps[j].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) printf(" Multi-Instance");
		putc('\n', stdout);
	}
}

void printPhysicalDeviceDetails(uint32_t numPhysicalDevices, VkPhysicalDevice * physicalDevices, bool printFullDeviceDetails)
{
	printf("Number of Vulkan physical devices: %u\n", numPhysicalDevices);
	for (uint32_t i = 0; i < numPhysicalDevices; i++)
	{
		// Start with the physical device properties.
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevices[i], &physicalDeviceProperties);

		printf("\tDevice Name: %s\n", physicalDeviceProperties.deviceName);
		if (printFullDeviceDetails)
		{
			// Print the remaining physical device properties without the name.
			printPhysicalDeviceProperties(physicalDeviceProperties, false, 2);

			// Print the physical device queue family properties.
			printPhysicalDeviceQueueFamilyProperties(physicalDevices[i], 2);

			// Print out layers and extensions
			printPhysicalDeviceLayers(physicalDevices[i], 2);

			// Get the extensions for the overall device.
			printPhysicalDeviceLayerExtensions(physicalDevices[i], "", 2);

			// Get features
			printf("\t\tFeatures:\n");
			printPhysicalDeviceFeatures(physicalDevices[i], 3);
			putc('\n', stdout);

			// Get the physical memory details
			printPhysicalDeviceMemoryDetails(physicalDevices[i], 2);
		}
	}
	putc('\n', stdout);
}
