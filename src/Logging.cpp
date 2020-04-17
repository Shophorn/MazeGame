#include <sstream>
#include <ostream>

struct LogInput
{
	// Todo(Leo): Do make use of our own string classes at some point
	std::stringstream 	buffer;
	std::ostream * 		output;

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

	~LogInput()
	{
		if (doPrint)
		{
			buffer << "\n";
			*output << buffer.str();

			// Note(Leo): We flush so we get immediate output to file.
			// Todo(Leo): Heard this is unnecessay though, so find out more.
			*output << std::flush;
		}
	}
};

struct LogChannel
{
	char const * title;
	int verbosity;

	std::ostream * output = &std::cout;

	LogInput operator()(int verbosity = 1)
	{
		LogInput result;

		if (verbosity <= this->verbosity)
		{
			// Prebuild header
			result.buffer << "[" << title << ":" << verbosity << "]: ";
			result.output = output;
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

#define F_A_HELPER_1_(x) #x
#define F_A_HELPER_2_(x) F_A_HELPER_1_(x)
#define FILE_ADDRESS "[" __FILE__ ":" F_A_HELPER_2_(__LINE__) "] "
