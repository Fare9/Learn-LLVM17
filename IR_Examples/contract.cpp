#include <iostream>

class X
{
public:
	int div(int a,int b)
	[[pre: a > b]]
	{
		return a/b;
	}
};


int
main()
{
	X x;

	std::cout << x.div(2,3);
}
