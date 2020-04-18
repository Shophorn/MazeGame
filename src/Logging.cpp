#include <sstream>
#include <ostream>

struct LogInput
{
	// Todo(Leo): Do make use of our own string classes at some point
	std::stringstream 	buffer;
	std::ostream * 		output;

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
			buffer << value;
	
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
			std::stringstream header;
			header << "[" << title << ":" << verbosity;
			if (hasFileAddress)
			{
				header << ":" << address.file << ":" << address.line;
			}
			header << "]: ";	

			buffer << "\n";

			*output << header.str() << buffer.str();

			// // Note(Leo): We flush so we get immediate output to file.
			// // Todo(Leo): Heard this is unnecessay though, so find out more.
			*output << std::flush;
		}
	}
};

struct LogChannel
{
	char const * 	title;
	s32 			verbosity;
	std::ostream * 	output = &std::cout;

	LogInput operator()(int verbosity = 1)
	{
		LogInput result;

		if (verbosity <= this->verbosity)
		{
			result.title 		= title;
			result.verbosity 	= verbosity;
			result.output 		= output;
		}
		else
		{
			result.doPrint = false;
		}

		return result;
	}
};

LogChannel logConsole 	= {"LOG", 5};

LogChannel logDebug 	= {"DEBUG", 5};
LogChannel logAnim 		= {"ANIMATION", 5};
LogChannel logVulkan 	= {"VULKAN", 5};
LogChannel logWindow	= {"WINDOW", 5};
LogChannel logSystem	= {"SYSTEM", 5};
LogChannel logNetwork	= {"NETWORK", 5};

#define FILE_ADDRESS LogInput::FileAddress{__FILE__, __LINE__}
