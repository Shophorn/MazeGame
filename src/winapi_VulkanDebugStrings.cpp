namespace vulkan
{
	template<typename VulkanEnumType>
	const char * EnumToString(VulkanEnumType value);
}

template <> const char *
vulkan::EnumToString<VkSampleCountFlagBits>(VkSampleCountFlagBits value)
{
	switch(value)
	{
		case VK_SAMPLE_COUNT_1_BIT: 				return "VK_SAMPLE_COUNT_1_BIT";
		case VK_SAMPLE_COUNT_2_BIT: 				return "VK_SAMPLE_COUNT_2_BIT";
		case VK_SAMPLE_COUNT_4_BIT: 				return "VK_SAMPLE_COUNT_4_BIT";
		case VK_SAMPLE_COUNT_8_BIT: 				return "VK_SAMPLE_COUNT_8_BIT";
		case VK_SAMPLE_COUNT_16_BIT: 				return "VK_SAMPLE_COUNT_16_BIT";
		case VK_SAMPLE_COUNT_32_BIT: 				return "VK_SAMPLE_COUNT_32_BIT";
		case VK_SAMPLE_COUNT_64_BIT: 				return "VK_SAMPLE_COUNT_64_BIT";
		case VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM: 	return "VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM";

		default:
			return "Unknown VkSampleCountFlagBits";
	}
}

template <> const char *
vulkan::EnumToString <VkResult> (VkResult value)
{
	switch(value)
	{	
		case VK_SUCCESS:                                            return "VK_SUCCESS";
		case VK_NOT_READY:                                          return "VK_NOT_READY";
		case VK_TIMEOUT:                                            return "VK_TIMEOUT";
		case VK_EVENT_SET:                                          return "VK_EVENT_SET";
		case VK_EVENT_RESET:                                        return "VK_EVENT_RESET";
		case VK_INCOMPLETE:                                         return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY:                           return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:                         return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:                        return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:                                  return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:                            return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:                            return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:                        return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:                          return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:                          return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:                             return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:                         return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL:                              return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_OUT_OF_POOL_MEMORY:                           return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:                      return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_SURFACE_LOST_KHR:                             return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:                     return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR:                                     return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:                              return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:                     return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:                        return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV:                            return "VK_ERROR_INVALID_SHADER_NV";                            
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_FRAGMENTATION_EXT:                            return "VK_ERROR_FRAGMENTATION_EXT";
		case VK_ERROR_NOT_PERMITTED_EXT:                            return "VK_ERROR_NOT_PERMITTED_EXT";
		case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:                   return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:          return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
	
		default:
			return "Unknown VkResult";
	}
}


constexpr internal bool VkResultCollide(int32 value)
{
	switch(value)
	{	
		case VK_SUCCESS:
		case VK_NOT_READY:
		case VK_TIMEOUT:
		case VK_EVENT_SET:
		case VK_EVENT_RESET:
		case VK_INCOMPLETE:
		case VK_ERROR_OUT_OF_HOST_MEMORY:
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		case VK_ERROR_INITIALIZATION_FAILED:
		case VK_ERROR_DEVICE_LOST:
		case VK_ERROR_MEMORY_MAP_FAILED:
		case VK_ERROR_LAYER_NOT_PRESENT:
		case VK_ERROR_EXTENSION_NOT_PRESENT:
		case VK_ERROR_FEATURE_NOT_PRESENT:
		case VK_ERROR_INCOMPATIBLE_DRIVER:
		case VK_ERROR_TOO_MANY_OBJECTS:
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
		case VK_ERROR_FRAGMENTED_POOL:
		case VK_ERROR_OUT_OF_POOL_MEMORY:
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:
		case VK_ERROR_SURFACE_LOST_KHR:
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
		case VK_SUBOPTIMAL_KHR:
		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
		case VK_ERROR_VALIDATION_FAILED_EXT:
		case VK_ERROR_INVALID_SHADER_NV:
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
		case VK_ERROR_FRAGMENTATION_EXT:
		case VK_ERROR_NOT_PERMITTED_EXT:
		case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
			return true;

		default:
			return false;
	}
}