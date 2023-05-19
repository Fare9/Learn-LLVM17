#include <stdio.h>

unsigned char array[8] = {3,1,8,4,5,2,6,7};

void show_array()
{
	int i = 0;
	for (i = 0; i < 8; i++)
		printf("%c\n", array[i]);
}
