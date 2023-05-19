#include <iostream>

int bar(int x)
{
	if (x == 1) throw 1;
	if (x == 2) throw 42.0;
	return x;
}

int foo(int x) noexcept(false)
{
	int y = 0;
	try
	{
		y = bar(x);
	}
	catch (int e)
	{
		y = e;
	}
	return y;
}
