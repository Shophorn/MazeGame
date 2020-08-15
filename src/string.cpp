// struct String
// {
// 	u16 	firstSlot;
// 	u16 	slots;

// 	s32 	length;
// 	char * 	memory;
// }

// constexpr u64 stringSlotSize 		= 512;
// constexpr u64 stringSlotCount 		= 128;
// constexpr u64 stringMemoryCapacity 	= stringSlotSize * stringSlotCount;

// static_assert(stringSlotCount < max_value_u16);

// global_variable u64 	global_stringMemoryUsed = 0;
// global_variable s32 	global_stringMemoryNextFreeSlot = 0;
// global_variable s32		global_stringMemoryNextFreeSlotsCount = stringSlotCount;

// global_variable char 	global_stringMemory [stringMemoryCapacity] = {};
// global_variable bool8 	global_stringMemoryUsedSlots [stringSlotCount] = {};

// String make_string(char const * cstring)
// {
// 	s32 length 			= cstring_length(cstring);
// 	s32 requiredSlots 	= length / stringSlotSize + 1;

// 	String result;
// 	if (requiredSlots <= global_stringMemoryNextFreeSlotsCount)
// 	{
// 		result.firstSlot 	= global_stringMemoryNextFreeSlot;
// 		result.slots 		= requiredSlots; 

// 		global_stringMemoryNextFreeSlot += requiredSlots;
// 		global_nextFreeSlotsCount 		-= requiredSlots;

// 		for (s32 i = 0; i < result.slots; ++i)
// 		{
// 			global_stringMemoryUsedSlots[i + result.firstSlot] = true;
// 		}

// 		u64 offset = result.firstSlot * stringSlotSize;

// 		result.length = length;
// 		result.memory = global_stringMemory + offset;

// 		copy_memory(result.memory, cstring, result.length);
// 	}
// 	else
// 	{
// 		u16 firstSlot 			= global_stringMemoryNextFreeSlot;
// 		u16 lastPossibleSlot 	= stringSlotCount - requiredSlots;

// 		while()
// 	}

// 	return result;
// }