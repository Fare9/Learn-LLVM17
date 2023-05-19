#ifndef TEST_H
#define TEST_H

template <class T>
class Test 
{
private:
	T var;
	int a1;
	long long a2;
	
public:
	Test(T var) : var(var)
	{
	}

	virtual T show() const
	{ 
		return var;
	}
};

#endif
