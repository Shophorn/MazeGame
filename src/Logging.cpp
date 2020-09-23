// Todo(Leo): we use internal macro all around, but this also uses it
#undef internal
#include <iostream>
#define internal static


struct LogOutput
{
	void * outputHandle;
	void (*outputFunc)(void * outputHandle, String const & message);

	void operator()(String const & message)
	{
		outputFunc(outputHandle, message);
	}
};

struct LogInput
{
	char bufferMemory[1024];
	String buffer2;

	LogOutput output2;

	struct FileAddress
	{
		const char * 	file;
		s32 			line;
	} address;
	bool32 hasFileAddress = false;

	char const * title;
	s32 verbosity;

	bool doPrint = true;

	LogInput () 				= default;
	LogInput (LogInput &&) 		= default;

	LogInput (LogInput const &) = delete; 

	template<typename T>
	LogInput & operator << (T const & value)
	{
		if (doPrint)
		{
			string_append(buffer2, array_count(bufferMemory), value);
		}

	
		return *this;
	}

	template<>
	LogInput & operator << <FileAddress>(FileAddress const & value)
	{
		address 		= value;
		hasFileAddress 	= true;

		return *this;
	} 

	~LogInput()
	{
		if (doPrint)
		{
			local_persist thread_local char outputMemory [1024];
			String outputString {0, outputMemory};
			string_append_format(outputString, array_count(outputMemory), "[", title);

			if (hasFileAddress)
			{
				string_append_format(outputString, array_count(outputMemory), ":", address.file, ":", address.line);
			}

			string_append_format(outputString, array_count(outputMemory), "]: ", buffer2, "\n");

			output2(outputString);
		}
	}
};

struct LogChannel
{
	char const * 	title;
	s32 			verbosity;

	LogOutput output2;

	LogInput operator()(int verbosity = 1)
	{
		LogInput result;

		if (verbosity <= this->verbosity)
		{
			result.title 		= title;
			result.verbosity 	= verbosity;
			result.output2 		= output2;

			result.buffer2 		= {0, result.bufferMemory};
		}
		else
		{
			result.doPrint = false;
		}

		return result;
	}

	template<typename ... Args>
	void operator() (int verbosity, Args ... args)
	{
		// Todo(Leo): handle file address, and verbosity
		// Todo(Leo): add temp string support
		local_persist thread_local char bufferMemory[1024];
		String buffer = {0, bufferMemory};

		string_append_format(buffer, array_count(bufferMemory), "[", title, "]: ", args..., "\n");

		output2(buffer);
	}
};

LogChannel logConsole 	= {"LOG", 5};

LogChannel logDebug 	= {"DEBUG", 5};
LogChannel logWarning	= {"WARNING", 5};
LogChannel logAnim 		= {"ANIMATION", 5};
LogChannel logVulkan 	= {"VULKAN", 5};
LogChannel logWindow	= {"WINDOW", 5};
LogChannel logSystem	= {"SYSTEM", 5};
LogChannel logNetwork	= {"NETWORK", 5};
LogChannel logAudio		= {"AUDIO", 5};

#define FILE_ADDRESS LogInput::FileAddress{__FILE__, __LINE__}


LogInput & operator << (LogInput & log, String const & string)
{
	for (s32 i = 0; i < string.length; ++i)
	{
		log << string[i];
	}

	return log;
}










#if MAZEGAME_DEVELOPMENT
void log_assert(char const * filename, int line, char const * message, char const * expression)
{
	auto log = logDebug(0);
	log << LogInput::FileAddress{filename, line} << "Assertion failed: ";

	if (message)
		log << message;

	if (expression)
		log << " (" << expression << ")";
}
#endif
