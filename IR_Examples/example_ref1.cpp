#include <iostream>
#include <string>

class A
{
public:
	A() : a(0) {}
	~A() = default;
	int a;
};

void manageA(A &A)
{
	A.a = 2;
}

int
main()
{
	A a{};

	std::cout << a.a << "\n";

	manageA(a);

	std::cout << a.a << "\n";

	return 0;
}
