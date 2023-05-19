#include <string>
#include <iostream>

std::string
retvector(int b)
{
	std::string a = "cacafuti\n";

	if (b == 0)
		throw std::runtime_error("farenain");

	return a;
}

int
main()
{
	std::string d; 
	try
	{
		d = retvector(0);
	} catch(...)
	{
		std::cout << "An exception has ocurred\n";
		std::cout << "Value of d: " << d;
	}
	return 0;
}
