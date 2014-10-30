#include "bo_net_master_core.h"

static void coreReadHead(struct paramThr *p);
static void coreQuit    (struct paramThr *p);
/* ----------------------------------------------------------------------------
 *	КОНЕЧНЫЙ АВТОМАТ
 */
static char *coreStatusTxt[] = {"READHEAD", "SET", "GET", "QUIT", "ANSOK",
				  "ANSERR", "ANSNO", "ADDTAB", "READCRC"};

enum coreStatus {READHEAD = 0, SET, GET, QUIT, ANSOK, ANSERR, ANSNO,
		 ADDTAB, READCRC};

static void(*statusTable[])(struct paramThr *) = {
	coreReadHead,
	coreQuit
};

/* ----------------------------------------------------------------------------
 * @brief	принимаем данные от клиента SET - изменения для табл роутов
 *					    LOG - лог команд для устр 485
 */
void bo_master_core(struct paramThr *p)
{
	int stop = 1;
	p->status = READHEAD;
	
	while(stop) {
		if(p->status == QUIT) break;
		statusTable[p->status](p);
	}
}

/* ----------------------------------------------------------------------------
 * @brief	читаем заголовок
 */
static void coreReadHead(struct paramThr *p)
{
	
	p->status = QUIT;
}







