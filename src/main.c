#include <stdio.h>

void greet()
{
	printf("Hello, 0x42!");
}

int main()
{
	printf("main() -> run ...");
	greet();
	printf("main() -> ... end");
	return 0;
}
