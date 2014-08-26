#include <stdio.h>
#include "log/logging.h"

void greet()
{
	printf("Hello, 0x42!\n");
}

int main()
{
	printf("main() -> run ...\n");
	greet();
	loggingINIT();
	loggingFIN();
	printf("main() -> ... end\n");
	return 0;
}
