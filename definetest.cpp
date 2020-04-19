#include <iostream>

int main()
{
	#if defined TEST_VALUE
		std::cout << "test value: " << TEST_VALUE << "\n";
	#else
		std::cout << "no test value\n";
	#endif
}