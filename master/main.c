/* ----------------------------------------------------------------------------
 * Опрос устройств указ в конф файле
 * получаем изменения -> сохр в таблицу -> отпр изм-ия устр-м из списка 
 */

/*#include <mcheck.h> */
#include <stdio.h>
#include <stdlib.h>
#include "../src/tools/dbgout.h"
#include "../src/log/bologging.h"

extern void bo_master_main(int argc, char **argv);

int main(int argc, char **argv)
{
/*	mtrace(); */
	dbgout("START master-> %s\n", *argv);
	bo_master_main(argc, argv);
	dbgout("END\n");
/*	muntrace(); */
	return 0;
}
