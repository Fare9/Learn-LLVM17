#include <iostream>

int
main()
{
	auto mylambda = [](int a, int b)
	{
		return a+b;
	};

	std::cout << mylambda(2,3) << "\n";

	return 0;
}
