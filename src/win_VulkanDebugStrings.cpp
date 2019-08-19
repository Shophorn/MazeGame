constexpr const char invalid_value_string [] = "INVALID_VALUE";

constexpr const char 
	sample_count_1_bit_string [] 			= "VK_SAMPLE_COUNT_1_BIT",
	sample_count_2_bit_string [] 			= "VK_SAMPLE_COUNT_2_BIT",
	sample_count_4_bit_string [] 			= "VK_SAMPLE_COUNT_4_BIT",
	sample_count_8_bit_string [] 			= "VK_SAMPLE_COUNT_8_BIT",
	sample_count_16_bit_string [] 			= "VK_SAMPLE_COUNT_16_BIT",
	sample_count_32_bit_string [] 			= "VK_SAMPLE_COUNT_32_BIT",
	sample_count_64_bit_string [] 			= "VK_SAMPLE_COUNT_64_BIT",
	sample_count_flag_bits_max_string []	= "VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM";

const char *
VulkanSampleCountFlagBitsString(VkSampleCountFlagBits bit)
{

	switch(bit)
	{
		case VK_SAMPLE_COUNT_1_BIT: 				return sample_count_1_bit_string;
		case VK_SAMPLE_COUNT_2_BIT: 				return sample_count_2_bit_string;
		case VK_SAMPLE_COUNT_4_BIT: 				return sample_count_4_bit_string;
		case VK_SAMPLE_COUNT_8_BIT: 				return sample_count_8_bit_string;
		case VK_SAMPLE_COUNT_16_BIT: 				return sample_count_16_bit_string;
		case VK_SAMPLE_COUNT_32_BIT: 				return sample_count_32_bit_string;
		case VK_SAMPLE_COUNT_64_BIT: 				return sample_count_64_bit_string;
		case VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM: 	return sample_count_flag_bits_max_string;

		default: return invalid_value_string;
	}
}

const char * 
VulkanVkResultString(VkResult value)
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
	
		default: return "Unknown VkResult";
	}
}