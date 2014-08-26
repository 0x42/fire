#include <stdio.h>

void greet()
{
	printf("Hello, 0x42!\n");
}

int main()
{
	printf("main() -> run ...\n");
	greet();
	printf("main() -> ... end\n");
	return 0;
}
