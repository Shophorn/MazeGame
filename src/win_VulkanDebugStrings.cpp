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
