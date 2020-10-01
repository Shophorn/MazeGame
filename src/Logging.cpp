struct LogFileAddress
{
	const char * 	file;
	s32 			line;
} address;

struct enabled_LogChannel
{
	char const * 	title;

	constexpr static s32 verbosityLevel = 0;

	template<typename TFirst, typename ... TOthers>
	void operator() (int verbosity, TFirst first, TOthers ... others)
	{
		if (verbosity > verbosityLevel)
		{
			return;
		}

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

struct debug_LogChannel
{
	char const * 	title;

	template<typename ... TOthers>
	void operator() (LogFileAddress address, TOthers ... others)
	{
		// Todo(Leo): handle verbosity
		// Todo(Leo): add temp string support
		local_persist thread_local char bufferMemory[1024] = {};
		String buffer = {0, bufferMemory};

		string_append_format(buffer, array_count(bufferMemory), "[", title, ":", address.file, ":", address.line, "]: ", others..., "\n");
		
		platform_log_write(buffer.length, buffer.memory);
	}
};

struct disabled_LogChannel
{
	char const * title;

	template<typename TFirst, typename ... TOthers>
	void operator() (int verbosity, TFirst first, TOthers ... others) {}	
};

debug_LogChannel log_debug 		= {"DEBUG"};
enabled_LogChannel log_graphics 	= {"GRAPHICS"};
enabled_LogChannel log_application	= {"APPLICATION"};

disabled_LogChannel log_asset		= {"ASSET"};
disabled_LogChannel log_animation	= {"ANIMATION"};
disabled_LogChannel log_network		= {"NETWORK"};
disabled_LogChannel log_audio		= {"AUDIO"};

#define FILE_ADDRESS LogFileAddress{__FILE__, __LINE__}

#if FS_DEVELOPMENT
void log_assert(char const * filename, int line, char const * message, char const * expression)
{
	// Todo(Leo): lol
	if (message && expression)
	{
		log_debug(LogFileAddress{filename, line}, "Assertion failed! ", message, ", (", expression, ")");
	}
	else if(message)
	{
		log_debug(LogFileAddress{filename, line}, "Assertion failed! ", message);
	}
	else if(expression)
	{
		log_debug(LogFileAddress{filename, line}, "Assertion failed! (", expression, ")");
	}
	else
	{
		log_debug(LogFileAddress{filename, line}, "Asssertion failed!");
	}
}
#endif
