#include <vector>
#include <iostream>

std::vector<int>
retvector()
{
	std::vector<int> d = {100};

	return d;
}

int
main()
{
	std::vector<int> d = retvector();

	for (auto val : d)
		std::cout << val << std::endl;

	return 0;
}
