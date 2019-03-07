#include "vulkanEngineInfo.h"
#include "vulkanDebug.h"

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
		uint32_t numInstanceLayerExtensions;
		HANDLE_VK(vkEnumerateInstanceExtensionProperties(instanceLayerProperties[i].layerName,
				&numInstanceLayerExtensions, nullptr),
			"Querying number of Vulkan instance extensions");
		VkExtensionProperties *extensionProperties = numInstanceLayerExtensions ?
			new VkExtensionProperties[numInstanceLayerExtensions] : nullptr;
		if (extensionProperties)
		{
			HANDLE_VK(vkEnumerateInstanceExtensionProperties(instanceLayerProperties[i].layerName,
					&numInstanceLayerExtensions,
					extensionProperties),
				"Querying Vulkan instance layer properties (layer: %s)",
				instanceLayerProperties[i].layerName);
		}

		printf("\t\tNum Extensions: %u\n", numInstanceLayerExtensions);
		for (uint32_t j = 0; j < numInstanceLayerExtensions; j++)
		{
			printf("\t\t%s : %u.%u.%u\n",
				extensionProperties[j].extensionName,
				VK_VERSION_MAJOR(extensionProperties[j].specVersion),
				VK_VERSION_MINOR(extensionProperties[j].specVersion),
				VK_VERSION_PATCH(extensionProperties[j].specVersion));
		}

		// Cleanup
		delete[] extensionProperties;
	}


	// Get what extensions are available.
	uint32_t numInstanceLayerExtensions;
	HANDLE_VK(vkEnumerateInstanceExtensionProperties("",
			&numInstanceLayerExtensions, nullptr),
		"Querying number of Vulkan instance extensions");
	VkExtensionProperties *extensionProperties = numInstanceLayerExtensions ?
		new VkExtensionProperties[numInstanceLayerExtensions] : nullptr;
	if (extensionProperties)
	{
		HANDLE_VK(vkEnumerateInstanceExtensionProperties("",
				&numInstanceLayerExtensions,
				extensionProperties),
			"Querying Vulkan instance layer properties (layer: %s)",
			"");
	}

	printf("Num Extensions for instance level: %u\n", numInstanceLayerExtensions);
	for (uint32_t j = 0; j < numInstanceLayerExtensions; j++)
	{
		printf("\t%s : %u.%u.%u\n",
			extensionProperties[j].extensionName,
			VK_VERSION_MAJOR(extensionProperties[j].specVersion),
			VK_VERSION_MINOR(extensionProperties[j].specVersion),
			VK_VERSION_PATCH(extensionProperties[j].specVersion));
	}

	printf("\n");
}

void printPhysicalDeviceDetails(uint32_t numPhysicalDevices, VkPhysicalDevice * physicalDevices, bool printFullDeviceDetails)
{
	printf("Number of Vulkan physical devices: %u\n", numPhysicalDevices);
	for (uint32_t i = 0; i < numPhysicalDevices; i++)
	{
		// Print the physical device properties.
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevices[i], &physicalDeviceProperties);

		printf("\tDevice Name: %s\n", physicalDeviceProperties.deviceName);
		if (printFullDeviceDetails)
		{
			printf("\t\tAPI Version: %u.%u.%u\n",
				VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion),
				VK_VERSION_MINOR(physicalDeviceProperties.apiVersion),
				VK_VERSION_PATCH(physicalDeviceProperties.apiVersion));
			printf("\t\tDriver Version: %u.%u.%u\n",
				VK_VERSION_MAJOR(physicalDeviceProperties.driverVersion),
				VK_VERSION_MINOR(physicalDeviceProperties.driverVersion),
				VK_VERSION_PATCH(physicalDeviceProperties.driverVersion));
			printf("\t\tVendor ID: %u\n", physicalDeviceProperties.vendorID);
			printf("\t\tDevice ID: %u\n", physicalDeviceProperties.deviceID);
			printf("\t\tDevice Type: ");
			switch (physicalDeviceProperties.deviceType)
			{
				case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: printf("Integrated GPU\n"); break;
				case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: printf("Discrete GPU\n"); break;
				case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: printf("Virtual GPU\n"); break;
				case VK_PHYSICAL_DEVICE_TYPE_CPU: printf("CPU\n"); break;
			}
			printf("\t\tPipeline Cache UUID: %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
				physicalDeviceProperties.pipelineCacheUUID[0],
				physicalDeviceProperties.pipelineCacheUUID[1],
				physicalDeviceProperties.pipelineCacheUUID[2],
				physicalDeviceProperties.pipelineCacheUUID[3],
				physicalDeviceProperties.pipelineCacheUUID[4],
				physicalDeviceProperties.pipelineCacheUUID[5],
				physicalDeviceProperties.pipelineCacheUUID[6],
				physicalDeviceProperties.pipelineCacheUUID[7],
				physicalDeviceProperties.pipelineCacheUUID[8],
				physicalDeviceProperties.pipelineCacheUUID[9],
				physicalDeviceProperties.pipelineCacheUUID[10],
				physicalDeviceProperties.pipelineCacheUUID[11],
				physicalDeviceProperties.pipelineCacheUUID[12],
				physicalDeviceProperties.pipelineCacheUUID[13],
				physicalDeviceProperties.pipelineCacheUUID[14],
				physicalDeviceProperties.pipelineCacheUUID[15]);

			printf("\t\tDevice Limits:\n");
			printf("\t\t\tmaxImageDimension1D: %u\n", physicalDeviceProperties.limits.maxImageDimension1D);
			printf("\t\t\tmaxImageDimension2D: %u\n", physicalDeviceProperties.limits.maxImageDimension2D);
			printf("\t\t\tmaxImageDimension3D: %u\n", physicalDeviceProperties.limits.maxImageDimension3D);
			printf("\t\t\tmaxImageDimensionCube: %u\n", physicalDeviceProperties.limits.maxImageDimensionCube);
			printf("\t\t\tmaxImageArrayLayers: %u\n", physicalDeviceProperties.limits.maxImageArrayLayers);
			printf("\t\t\tmaxTexelBufferElements: %u\n", physicalDeviceProperties.limits.maxTexelBufferElements);
			printf("\t\t\tmaxUniformBufferRange: %u\n", physicalDeviceProperties.limits.maxUniformBufferRange);
			printf("\t\t\tmaxStorageBufferRange: %u\n", physicalDeviceProperties.limits.maxStorageBufferRange);
			printf("\t\t\tmaxPushConstantsSize: %u\n", physicalDeviceProperties.limits.maxPushConstantsSize);
			printf("\t\t\tmaxMemoryAllocationCount: %u\n", physicalDeviceProperties.limits.maxMemoryAllocationCount);
			printf("\t\t\tmaxSamplerAllocationCount: %u\n", physicalDeviceProperties.limits.maxSamplerAllocationCount);
			printf("\t\t\tbufferImageGranularity: %llu\n", physicalDeviceProperties.limits.bufferImageGranularity);
			printf("\t\t\tsparseAddressSpaceSize: %llu\n", physicalDeviceProperties.limits.sparseAddressSpaceSize);
			printf("\t\t\tmaxBoundDescriptorSets: %u\n", physicalDeviceProperties.limits.maxBoundDescriptorSets);
			printf("\t\t\tmaxPerStageDescriptorSamplers: %u\n", physicalDeviceProperties.limits.maxPerStageDescriptorSamplers);
			printf("\t\t\tmaxPerStageDescriptorUniformBuffers: %u\n", physicalDeviceProperties.limits.maxPerStageDescriptorUniformBuffers);
			printf("\t\t\tmaxPerStageDescriptorStorageBuffers: %u\n", physicalDeviceProperties.limits.maxPerStageDescriptorStorageBuffers);
			printf("\t\t\tmaxPerStageDescriptorSampledImages: %u\n", physicalDeviceProperties.limits.maxPerStageDescriptorSampledImages);
			printf("\t\t\tmaxPerStageDescriptorStorageImages: %u\n", physicalDeviceProperties.limits.maxPerStageDescriptorStorageImages);
			printf("\t\t\tmaxPerStageDescriptorInputAttachments: %u\n", physicalDeviceProperties.limits.maxPerStageDescriptorInputAttachments);
			printf("\t\t\tmaxPerStageResources: %u\n", physicalDeviceProperties.limits.maxPerStageResources);
			printf("\t\t\tmaxDescriptorSetSamplers: %u\n", physicalDeviceProperties.limits.maxDescriptorSetSamplers);
			printf("\t\t\tmaxDescriptorSetUniformBuffers: %u\n", physicalDeviceProperties.limits.maxDescriptorSetUniformBuffers);
			printf("\t\t\tmaxDescriptorSetUniformBuffersDynamic: %u\n", physicalDeviceProperties.limits.maxDescriptorSetUniformBuffersDynamic);
			printf("\t\t\tmaxDescriptorSetStorageBuffers: %u\n", physicalDeviceProperties.limits.maxDescriptorSetStorageBuffers);
			printf("\t\t\tmaxDescriptorSetStorageBuffersDynamic: %u\n", physicalDeviceProperties.limits.maxDescriptorSetStorageBuffersDynamic);
			printf("\t\t\tmaxDescriptorSetSampledImages: %u\n", physicalDeviceProperties.limits.maxDescriptorSetSampledImages);
			printf("\t\t\tmaxDescriptorSetStorageImages: %u\n", physicalDeviceProperties.limits.maxDescriptorSetStorageImages);
			printf("\t\t\tmaxDescriptorSetInputAttachments: %u\n", physicalDeviceProperties.limits.maxDescriptorSetInputAttachments);
			printf("\t\t\tmaxVertexInputAttributes: %u\n", physicalDeviceProperties.limits.maxVertexInputAttributes);
			printf("\t\t\tmaxVertexInputBindings: %u\n", physicalDeviceProperties.limits.maxVertexInputBindings);
			printf("\t\t\tmaxVertexInputAttributeOffset: %u\n", physicalDeviceProperties.limits.maxVertexInputAttributeOffset);
			printf("\t\t\tmaxVertexInputBindingStride: %u\n", physicalDeviceProperties.limits.maxVertexInputBindingStride);
			printf("\t\t\tmaxVertexOutputComponents: %u\n", physicalDeviceProperties.limits.maxVertexOutputComponents);
			printf("\t\t\tmaxTessellationGenerationLevel: %u\n", physicalDeviceProperties.limits.maxTessellationGenerationLevel);
			printf("\t\t\tmaxTessellationPatchSize: %u\n", physicalDeviceProperties.limits.maxTessellationPatchSize);
			printf("\t\t\tmaxTessellationControlPerVertexInputComponents: %u\n", physicalDeviceProperties.limits.maxTessellationControlPerVertexInputComponents);
			printf("\t\t\tmaxTessellationControlPerVertexOutputComponents: %u\n", physicalDeviceProperties.limits.maxTessellationControlPerVertexOutputComponents);
			printf("\t\t\tmaxTessellationControlPerPatchOutputComponents: %u\n", physicalDeviceProperties.limits.maxTessellationControlPerPatchOutputComponents);
			printf("\t\t\tmaxTessellationControlTotalOutputComponents: %u\n", physicalDeviceProperties.limits.maxTessellationControlTotalOutputComponents);
			printf("\t\t\tmaxTessellationEvaluationInputComponents: %u\n", physicalDeviceProperties.limits.maxTessellationEvaluationInputComponents);
			printf("\t\t\tmaxTessellationEvaluationOutputComponents: %u\n", physicalDeviceProperties.limits.maxTessellationEvaluationOutputComponents);
			printf("\t\t\tmaxGeometryShaderInvocations: %u\n", physicalDeviceProperties.limits.maxGeometryShaderInvocations);
			printf("\t\t\tmaxGeometryInputComponents: %u\n", physicalDeviceProperties.limits.maxGeometryInputComponents);
			printf("\t\t\tmaxGeometryOutputComponents: %u\n", physicalDeviceProperties.limits.maxGeometryOutputComponents);
			printf("\t\t\tmaxGeometryOutputVertices: %u\n", physicalDeviceProperties.limits.maxGeometryOutputVertices);
			printf("\t\t\tmaxGeometryTotalOutputComponents: %u\n", physicalDeviceProperties.limits.maxGeometryTotalOutputComponents);
			printf("\t\t\tmaxFragmentInputComponents: %u\n", physicalDeviceProperties.limits.maxFragmentInputComponents);
			printf("\t\t\tmaxFragmentOutputAttachments: %u\n", physicalDeviceProperties.limits.maxFragmentOutputAttachments);
			printf("\t\t\tmaxFragmentDualSrcAttachments: %u\n", physicalDeviceProperties.limits.maxFragmentDualSrcAttachments);
			printf("\t\t\tmaxFragmentCombinedOutputResources: %u\n", physicalDeviceProperties.limits.maxFragmentCombinedOutputResources);
			printf("\t\t\tmaxComputeSharedMemorySize: %u\n", physicalDeviceProperties.limits.maxComputeSharedMemorySize);
			printf("\t\t\tmaxComputeWorkGroupCount: [ %u, %u, %u ]\n",
				physicalDeviceProperties.limits.maxComputeWorkGroupCount[0],
				physicalDeviceProperties.limits.maxComputeWorkGroupCount[1],
				physicalDeviceProperties.limits.maxComputeWorkGroupCount[2]);
			printf("\t\t\tmaxComputeWorkGroupInvocations: %u\n", physicalDeviceProperties.limits.maxComputeWorkGroupInvocations);
			printf("\t\t\tmaxComputeWorkGroupSize: [ %u, %u, %u ]\n",
				physicalDeviceProperties.limits.maxComputeWorkGroupSize[0],
				physicalDeviceProperties.limits.maxComputeWorkGroupSize[1],
				physicalDeviceProperties.limits.maxComputeWorkGroupSize[2]);
			printf("\t\t\tsubPixelPrecisionBits: %u\n", physicalDeviceProperties.limits.subPixelPrecisionBits);
			printf("\t\t\tsubTexelPrecisionBits: %u\n", physicalDeviceProperties.limits.subTexelPrecisionBits);
			printf("\t\t\tmipmapPrecisionBits: %u\n", physicalDeviceProperties.limits.mipmapPrecisionBits);
			printf("\t\t\tmaxDrawIndexedIndexValue: %u\n", physicalDeviceProperties.limits.maxDrawIndexedIndexValue);
			printf("\t\t\tmaxDrawIndirectCount: %u\n", physicalDeviceProperties.limits.maxDrawIndirectCount);
			printf("\t\t\tmaxSamplerLodBias: %f\n", physicalDeviceProperties.limits.maxSamplerLodBias);
			printf("\t\t\tmaxSamplerAnisotropy: %f\n", physicalDeviceProperties.limits.maxSamplerAnisotropy);
			printf("\t\t\tmaxViewports: %u\n", physicalDeviceProperties.limits.maxViewports);
			printf("\t\t\tmaxViewportDimensions: [ %u, %u ]\n",
				physicalDeviceProperties.limits.maxViewportDimensions[0],
				physicalDeviceProperties.limits.maxViewportDimensions[1]);
			printf("\t\t\tviewportBoundsRange: [ %f, %f ]\n",
				physicalDeviceProperties.limits.viewportBoundsRange[0],
				physicalDeviceProperties.limits.viewportBoundsRange[1]);
			printf("\t\t\tviewportSubPixelBits: %u\n", physicalDeviceProperties.limits.viewportSubPixelBits);
			printf("\t\t\tminMemoryMapAlignment: %zu\n", physicalDeviceProperties.limits.minMemoryMapAlignment);
			printf("\t\t\tminTexelBufferOffsetAlignment: %llu\n", physicalDeviceProperties.limits.minTexelBufferOffsetAlignment);
			printf("\t\t\tminUniformBufferOffsetAlignment: %llu\n", physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
			printf("\t\t\tminStorageBufferOffsetAlignment: %llu\n", physicalDeviceProperties.limits.minStorageBufferOffsetAlignment);
			printf("\t\t\tminTexelOffset: %d\n", physicalDeviceProperties.limits.minTexelOffset);
			printf("\t\t\tmaxTexelOffset: %u\n", physicalDeviceProperties.limits.maxTexelOffset);
			printf("\t\t\tminTexelGatherOffset: %d\n", physicalDeviceProperties.limits.minTexelGatherOffset);
			printf("\t\t\tmaxTexelGatherOffset: %u\n", physicalDeviceProperties.limits.maxTexelGatherOffset);
			printf("\t\t\tminInterpolationOffset: %f\n", physicalDeviceProperties.limits.minInterpolationOffset);
			printf("\t\t\tmaxInterpolationOffset: %f\n", physicalDeviceProperties.limits.maxInterpolationOffset);
			printf("\t\t\tsubPixelInterpolationOffsetBits: %u\n", physicalDeviceProperties.limits.subPixelInterpolationOffsetBits);
			printf("\t\t\tmaxFramebufferWidth: %u\n", physicalDeviceProperties.limits.maxFramebufferWidth);
			printf("\t\t\tmaxFramebufferHeight: %u\n", physicalDeviceProperties.limits.maxFramebufferHeight);
			printf("\t\t\tmaxFramebufferLayers: %u\n", physicalDeviceProperties.limits.maxFramebufferLayers);
			printf("\t\t\tframebufferColorSampleCounts: 0x%X\n", physicalDeviceProperties.limits.framebufferColorSampleCounts);
			printf("\t\t\tframebufferDepthSampleCounts: 0x%X\n", physicalDeviceProperties.limits.framebufferDepthSampleCounts);
			printf("\t\t\tframebufferStencilSampleCounts: 0x%X\n", physicalDeviceProperties.limits.framebufferStencilSampleCounts);
			printf("\t\t\tframebufferNoAttachmentsSampleCounts: 0x%X\n", physicalDeviceProperties.limits.framebufferNoAttachmentsSampleCounts);
			printf("\t\t\tmaxColorAttachments: %u\n", physicalDeviceProperties.limits.maxColorAttachments);
			printf("\t\t\tsampledImageColorSampleCounts: 0x%X\n", physicalDeviceProperties.limits.sampledImageColorSampleCounts);
			printf("\t\t\tsampledImageIntegerSampleCounts: 0x%X\n", physicalDeviceProperties.limits.sampledImageIntegerSampleCounts);
			printf("\t\t\tsampledImageDepthSampleCounts: 0x%X\n", physicalDeviceProperties.limits.sampledImageDepthSampleCounts);
			printf("\t\t\tsampledImageStencilSampleCounts: 0x%X\n", physicalDeviceProperties.limits.sampledImageStencilSampleCounts);
			printf("\t\t\tstorageImageSampleCounts: 0x%X\n", physicalDeviceProperties.limits.storageImageSampleCounts);
			printf("\t\t\tmaxSampleMaskWords: %u\n", physicalDeviceProperties.limits.maxSampleMaskWords);
			printf("\t\t\ttimestampComputeAndGraphics: %s\n", physicalDeviceProperties.limits.timestampComputeAndGraphics ? "TRUE" : "FALSE");
			printf("\t\t\ttimestampPeriod: %f\n", physicalDeviceProperties.limits.timestampPeriod);
			printf("\t\t\tmaxClipDistances: %u\n", physicalDeviceProperties.limits.maxClipDistances);
			printf("\t\t\tmaxCullDistances: %u\n", physicalDeviceProperties.limits.maxCullDistances);
			printf("\t\t\tmaxCombinedClipAndCullDistances: %u\n", physicalDeviceProperties.limits.maxCombinedClipAndCullDistances);
			printf("\t\t\tdiscreteQueuePriorities: %u\n", physicalDeviceProperties.limits.discreteQueuePriorities);
			printf("\t\t\tpointSizeRange: [ %f, %f ]\n",
				physicalDeviceProperties.limits.pointSizeRange[0],
				physicalDeviceProperties.limits.pointSizeRange[1]);
			printf("\t\t\tlineWidthRange: [ %f, %f ]\n",
				physicalDeviceProperties.limits.lineWidthRange[0],
				physicalDeviceProperties.limits.lineWidthRange[1]);
			printf("\t\t\tpointSizeGranularity: %f\n", physicalDeviceProperties.limits.pointSizeGranularity);
			printf("\t\t\tlineWidthGranularity: %f\n", physicalDeviceProperties.limits.lineWidthGranularity);
			printf("\t\t\tstrictLines: %s\n", physicalDeviceProperties.limits.strictLines ? "TRUE" : "FALSE");
			printf("\t\t\tstandardSampleLocations: %s\n", physicalDeviceProperties.limits.standardSampleLocations ? "TRUE" : "FALSE");
			printf("\t\t\toptimalBufferCopyOffsetAlignment: %llu\n", physicalDeviceProperties.limits.optimalBufferCopyOffsetAlignment);
			printf("\t\t\toptimalBufferCopyRowPitchAlignment: %llu\n", physicalDeviceProperties.limits.optimalBufferCopyRowPitchAlignment);
			printf("\t\t\tnonCoherentAtomSize: %llu\n", physicalDeviceProperties.limits.nonCoherentAtomSize);

			printf("\t\tSparse Properties:\n");
			printf("\t\t\tresidencyStandard2DBlockShape: %s\n", physicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape ? "TRUE" : "FALSE");
			printf("\t\t\tresidencyStandard2DMultisampleBlockShape: %s\n", physicalDeviceProperties.sparseProperties.residencyStandard2DMultisampleBlockShape ? "TRUE" : "FALSE");
			printf("\t\t\tresidencyStandard3DBlockShape: %s\n", physicalDeviceProperties.sparseProperties.residencyStandard3DBlockShape ? "TRUE" : "FALSE");
			printf("\t\t\tresidencyAlignedMipSize: %s\n", physicalDeviceProperties.sparseProperties.residencyAlignedMipSize ? "TRUE" : "FALSE");
			printf("\t\t\tresidencyNonResidentStrict: %s\n", physicalDeviceProperties.sparseProperties.residencyNonResidentStrict ? "TRUE" : "FALSE");

			// Print the physical device queue family properties.
			uint32_t numQueueFamilies;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &numQueueFamilies, nullptr);
			VkQueueFamilyProperties *queueFamilyProperties = new VkQueueFamilyProperties[numQueueFamilies];
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &numQueueFamilies, queueFamilyProperties);
			
			printf("\t\tNum queue family properties: %u\n", numQueueFamilies);
			for (uint32_t j = 0; j < numQueueFamilies; j++)
			{
				printf("\t\t\tQueue Flags:");
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					printf(" Graphics");
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT)
					printf(" Compute");
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT)
					printf(" Transfer");
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
					printf(" Sparse-Binding");
				if (queueFamilyProperties[j].queueFlags & VK_QUEUE_PROTECTED_BIT)
					printf(" Protected");
				if (vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevices[i], j) == VK_TRUE)
					printf(" Present-Win32");
				printf("\n\t\t\tQueue Count: %u\n", queueFamilyProperties[j].queueCount);
				printf("\t\t\tTimestamp Valid Bits: %u\n", queueFamilyProperties[j].timestampValidBits);
				printf("\t\t\tMin Image Transfer Granularity: [ %u, %u, %u ]\n",
					queueFamilyProperties[j].minImageTransferGranularity.width,
					queueFamilyProperties[j].minImageTransferGranularity.height,
					queueFamilyProperties[j].minImageTransferGranularity.depth);
				putc('\n', stdout);
			}

			// Print out layers and extensions
			uint32_t numLayers;
			HANDLE_VK(vkEnumerateDeviceLayerProperties(physicalDevices[i], &numLayers, nullptr),
				"Querying number of device layers on physical device %u\n", i);
			VkLayerProperties *layers = numLayers ? new VkLayerProperties[numLayers] : nullptr;
			if (layers)
			{
				HANDLE_VK(vkEnumerateDeviceLayerProperties(physicalDevices[i], &numLayers, layers),
					"Querying device layers on physical device %u\n", i);

				printf("\t\tNumber of device layers: %u\n", numLayers);
				for (uint32_t j = 0; j < numLayers; j++)
				{
					printf("\t\t\tLayer: %s : %u.%u.%u : %u.%u.%u : %s\n",
						layers[j].layerName,
						VK_VERSION_MAJOR(layers[j].specVersion),
						VK_VERSION_MINOR(layers[j].specVersion),
						VK_VERSION_PATCH(layers[j].specVersion),
						VK_VERSION_MAJOR(layers[j].implementationVersion),
						VK_VERSION_MINOR(layers[j].implementationVersion),
						VK_VERSION_PATCH(layers[j].implementationVersion),
						layers[j].description);

					// Get the extensions for this layer
					uint32_t numExt;
					HANDLE_VK(vkEnumerateDeviceExtensionProperties(physicalDevices[i],
							layers[j].layerName, &numExt, nullptr),
						"Querying for number of extensions of layer \"%s\" on physical device %u\n",
						layers[j].layerName,
						i);
					VkExtensionProperties *exts = numExt ? new VkExtensionProperties[numExt] : nullptr;
					if (exts)
					{
						HANDLE_VK(vkEnumerateDeviceExtensionProperties(physicalDevices[i],
								layers[j].layerName, &numExt, exts),
							"Querying for number of extensions of layer \"%s\" on physical device %u\n",
							layers[j].layerName, i);

						for (uint32_t k = 0; k < numExt; k++)
						{
							printf("\t\t\t\tExtension: %s : %u.%u.%u\n",
								exts[k].extensionName,
								VK_VERSION_MAJOR(exts[k].specVersion),
								VK_VERSION_MINOR(exts[k].specVersion),
								VK_VERSION_PATCH(exts[k].specVersion));
						}
					}
				}

				// Get the extensions for the overall device.
				uint32_t numExt;
				HANDLE_VK(vkEnumerateDeviceExtensionProperties(physicalDevices[i],
						"", &numExt, nullptr),
					"Querying for number of extensions of layer \"%s\" on physical device %u\n",
					"",
					i);
				VkExtensionProperties *exts = numExt ? new VkExtensionProperties[numExt] : nullptr;
				if (exts)
				{
					HANDLE_VK(vkEnumerateDeviceExtensionProperties(physicalDevices[i],
							"", &numExt, exts),
						"Querying for number of extensions of layer \"%s\" on physical device %u\n",
						"", i);

					printf("\t\tNum Device Extensions: %u\n", numExt);
					for (uint32_t k = 0; k < numExt; k++)
					{
						printf("\t\t\tExtension: %s : %u.%u.%u\n",
							exts[k].extensionName,
							VK_VERSION_MAJOR(exts[k].specVersion),
							VK_VERSION_MINOR(exts[k].specVersion),
							VK_VERSION_PATCH(exts[k].specVersion));
					}
				}
			}

			// Get features
			printf("\t\tFeatures:\n");
			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures(physicalDevices[i], &features);
			printf("\t\t\trobustBufferAccess: %s\n", features.robustBufferAccess ? "True" : "False");
			printf("\t\t\tfullDrawIndexUint32: %s\n", features.fullDrawIndexUint32 ? "True" : "False");
			printf("\t\t\timageCubeArray: %s\n", features.imageCubeArray ? "True" : "False");
			printf("\t\t\tindependentBlend: %s\n", features.independentBlend ? "True" : "False");
			printf("\t\t\tgeometryShader: %s\n", features.geometryShader ? "True" : "False");
			printf("\t\t\ttessellationShader: %s\n", features.tessellationShader ? "True" : "False");
			printf("\t\t\tsampleRateShading: %s\n", features.sampleRateShading ? "True" : "False");
			printf("\t\t\tdualSrcBlend: %s\n", features.dualSrcBlend ? "True" : "False");
			printf("\t\t\tlogicOp: %s\n", features.logicOp ? "True" : "False");
			printf("\t\t\tmultiDrawIndirect: %s\n", features.multiDrawIndirect ? "True" : "False");
			printf("\t\t\tdrawIndirectFirstInstance: %s\n", features.drawIndirectFirstInstance ? "True" : "False");
			printf("\t\t\tdepthClamp: %s\n", features.depthClamp ? "True" : "False");
			printf("\t\t\tdepthBiasClamp: %s\n", features.depthBiasClamp ? "True" : "False");
			printf("\t\t\tfillModeNonSolid: %s\n", features.fillModeNonSolid ? "True" : "False");
			printf("\t\t\tdepthBounds: %s\n", features.depthBounds ? "True" : "False");
			printf("\t\t\twideLines: %s\n", features.wideLines ? "True" : "False");
			printf("\t\t\tlargePoints: %s\n", features.largePoints ? "True" : "False");
			printf("\t\t\talphaToOne: %s\n", features.alphaToOne ? "True" : "False");
			printf("\t\t\tmultiViewport: %s\n", features.multiViewport ? "True" : "False");
			printf("\t\t\tsamplerAnisotropy: %s\n", features.samplerAnisotropy ? "True" : "False");
			printf("\t\t\ttextureCompressionETC2: %s\n", features.textureCompressionETC2 ? "True" : "False");
			printf("\t\t\ttextureCompressionASTC_LDR: %s\n", features.textureCompressionASTC_LDR ? "True" : "False");
			printf("\t\t\ttextureCompressionBC: %s\n", features.textureCompressionBC ? "True" : "False");
			printf("\t\t\tocclusionQueryPrecise: %s\n", features.occlusionQueryPrecise ? "True" : "False");
			printf("\t\t\tpipelineStatisticsQuery: %s\n", features.pipelineStatisticsQuery ? "True" : "False");
			printf("\t\t\tvertexPipelineStoresAndAtomics: %s\n", features.vertexPipelineStoresAndAtomics ? "True" : "False");
			printf("\t\t\tfragmentStoresAndAtomics: %s\n", features.fragmentStoresAndAtomics ? "True" : "False");
			printf("\t\t\tshaderTessellationAndGeometryPointSize: %s\n", features.shaderTessellationAndGeometryPointSize ? "True" : "False");
			printf("\t\t\tshaderImageGatherExtended: %s\n", features.shaderImageGatherExtended ? "True" : "False");
			printf("\t\t\tshaderStorageImageExtendedFormats: %s\n", features.shaderStorageImageExtendedFormats ? "True" : "False");
			printf("\t\t\tshaderStorageImageMultisample: %s\n", features.shaderStorageImageMultisample ? "True" : "False");
			printf("\t\t\tshaderStorageImageReadWithoutFormat: %s\n", features.shaderStorageImageReadWithoutFormat ? "True" : "False");
			printf("\t\t\tshaderStorageImageWriteWithoutFormat: %s\n", features.shaderStorageImageWriteWithoutFormat ? "True" : "False");
			printf("\t\t\tshaderUniformBufferArrayDynamicIndexing: %s\n", features.shaderUniformBufferArrayDynamicIndexing ? "True" : "False");
			printf("\t\t\tshaderSampledImageArrayDynamicIndexing: %s\n", features.shaderSampledImageArrayDynamicIndexing ? "True" : "False");
			printf("\t\t\tshaderStorageBufferArrayDynamicIndexing: %s\n", features.shaderStorageBufferArrayDynamicIndexing ? "True" : "False");
			printf("\t\t\tshaderStorageImageArrayDynamicIndexing: %s\n", features.shaderStorageImageArrayDynamicIndexing ? "True" : "False");
			printf("\t\t\tshaderClipDistance: %s\n", features.shaderClipDistance ? "True" : "False");
			printf("\t\t\tshaderCullDistance: %s\n", features.shaderCullDistance ? "True" : "False");
			printf("\t\t\tshaderFloat64: %s\n", features.shaderFloat64 ? "True" : "False");
			printf("\t\t\tshaderInt64: %s\n", features.shaderInt64 ? "True" : "False");
			printf("\t\t\tshaderInt16: %s\n", features.shaderInt16 ? "True" : "False");
			printf("\t\t\tshaderResourceResidency: %s\n", features.shaderResourceResidency ? "True" : "False");
			printf("\t\t\tshaderResourceMinLod: %s\n", features.shaderResourceMinLod ? "True" : "False");
			printf("\t\t\tsparseBinding: %s\n", features.sparseBinding ? "True" : "False");
			printf("\t\t\tsparseResidencyBuffer: %s\n", features.sparseResidencyBuffer ? "True" : "False");
			printf("\t\t\tsparseResidencyImage2D: %s\n", features.sparseResidencyImage2D ? "True" : "False");
			printf("\t\t\tsparseResidencyImage3D: %s\n", features.sparseResidencyImage3D ? "True" : "False");
			printf("\t\t\tsparseResidency2Samples: %s\n", features.sparseResidency2Samples ? "True" : "False");
			printf("\t\t\tsparseResidency4Samples: %s\n", features.sparseResidency4Samples ? "True" : "False");
			printf("\t\t\tsparseResidency8Samples: %s\n", features.sparseResidency8Samples ? "True" : "False");
			printf("\t\t\tsparseResidency16Samples: %s\n", features.sparseResidency16Samples ? "True" : "False");
			printf("\t\t\tsparseResidencyAliased: %s\n", features.sparseResidencyAliased ? "True" : "False");
			printf("\t\t\tvariableMultisampleRate: %s\n", features.variableMultisampleRate ? "True" : "False");
			printf("\t\t\tinheritedQueries: %s\n", features.inheritedQueries ? "True" : "False");

			putc('\n', stdout);

			// Get the physical memory details
			VkPhysicalDeviceMemoryProperties memProps;
			vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &memProps);

			printf("\t\tPhysical Memory Types:\n");
			for (uint32_t j = 0; j < memProps.memoryTypeCount; j++)
			{
				printf("\t\t\tIndex %d: ", memProps.memoryTypes[j].heapIndex);
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) printf(" Device-Local");
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) printf(" Host-Visible");
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) printf(" Host-Coherent");
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) printf(" Host-Cached");
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) printf(" Lazily-Allocated");
				if (memProps.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT) printf(" Protected");
				putc('\n', stdout);
			}

			printf("\t\tPhysical Memory Heaps:\n");
			for (uint32_t j = 0; j < memProps.memoryHeapCount; j++)
			{
				printf("\t\t\tHeap %d: %lf MB :", j, memProps.memoryHeaps[j].size / pow(2.0, 20.0));
				if (memProps.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) printf(" Device-Local");
				if (memProps.memoryHeaps[j].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) printf(" Multi-Instance");
				putc('\n', stdout);
			}
		}
	}
	putc('\n', stdout);
}
