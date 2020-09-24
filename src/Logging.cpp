struct LogFileAddress
{
	const char * 	file;
	s32 			line;
} address;

struct LogChannel
{
	char const * 	title;
	s32 			verbosity;

	template<typename TFirst, typename ... TOthers>
	void operator() (int verbosity, TFirst first, TOthers ... others)
	{
		// Todo(Leo): handle verbosity
		// Todo(Leo): add temp string support
		local_persist thread_local char bufferMemory[1024] = {};
		String buffer = {0, bufferMemory};

		if constexpr(is_same_type<TFirst, LogFileAddress>)
		{
			string_append_format(buffer, array_count(bufferMemory), "[", title, ":", first.file, ":", first.line, "]: ", others..., "\n");
		}

		if constexpr(is_same_type<TFirst, LogFileAddress> == false)
		{
			string_append_format(buffer, array_count(bufferMemory), "[", title, "]: ", first, others..., "\n");
		}
		
		platform_log_write(buffer.length, buffer.memory);
	}
};

enum LogChannelNames : s32
{
	LOG_GAME,
	LOG_VULKAN,
};

struct enabled_LogChannel
{
	template<typename TFirst, typename ... TOthers>
	void operator() (char const * title, TFirst first, TOthers ... others)
	{
		// Todo(Leo): handle verbosity
		// Todo(Leo): add temp string support
		local_persist thread_local char bufferMemory[1024] = {};
		String buffer = {0, bufferMemory};

		if constexpr(is_same_type<TFirst, LogFileAddress>)
		{
			string_append_format(buffer, array_count(bufferMemory), "[", title, ":", first.file, ":", first.line, "]: ", others..., "\n");
		}

		if constexpr(is_same_type<TFirst, LogFileAddress> == false)
		{
			string_append_format(buffer, array_count(bufferMemory), "[", title, "]: ", first, others..., "\n");
		}
		
		platform_log_write(buffer.length, buffer.memory);
	}	
};

struct disabled_LogChannel
{
	template<typename TFirst, typename ... TOthers>
	void operator() (char const * title, TFirst first, TOthers ... others) {}	
};

enabled_LogChannel log_always = {};
disabled_LogChannel log_common = {};
disabled_LogChannel log_rare = {};



LogChannel logConsole 	= {"LOG", 5};

LogChannel logDebug 	= {"DEBUG", 5};
LogChannel logWarning	= {"WARNING", 5};
LogChannel logAnim 		= {"ANIMATION", 5};
LogChannel logVulkan 	= {"VULKAN", 5};
LogChannel logWindow	= {"WINDOW", 5};
LogChannel logSystem	= {"SYSTEM", 5};
LogChannel logNetwork	= {"NETWORK", 5};
LogChannel logAudio		= {"AUDIO", 5};

#define FILE_ADDRESS LogFileAddress{__FILE__, __LINE__}

#if MAZEGAME_DEVELOPMENT
void log_assert(char const * filename, int line, char const * message, char const * expression)
{
	// Todo(Leo): lol
	if (message && expression)
	{
		log_always("ASSERT", LogFileAddress{filename, line}, "Assertion failed!", message, ", (", expression, ")");
	}
	else if(message)
	{
		log_always("ASSERT", LogFileAddress{filename, line}, "Assertion failed!", message);
	}
	else if(expression)
	{
		log_always("ASSERT", LogFileAddress{filename, line}, "Assertion failed! (", expression, ")");
	}
	else
	{
		log_always("ASSERT", LogFileAddress{filename, line}, "Asssertion failed!");
	}
}
#endif
