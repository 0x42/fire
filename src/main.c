#include <stdio.h>
#include "log/logging.h"
#include "log/tools.h"

void greet()
{
	debugPrint("Hello, 0x42!\n");
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
