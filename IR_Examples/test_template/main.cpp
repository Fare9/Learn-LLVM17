#include "test.h"

int
main()
{
	Test<int> a(5);
	Test<char> b('b');
	
	a.show();
	b.show();

	return 0;
}
