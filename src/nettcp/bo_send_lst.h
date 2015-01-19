#ifndef BO_SEND_LST_H
#define	BO_SEND_LST_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bo_net.h"
#include "../tools/dbgout.h"
#include "../log/bologging.h"    

struct BO_ITEM_SOCK_LST {
	int sock;
	char ip[16]; /* XXX.XXX.XXX.XXX */
        int rate; 
};

struct BO_SOCK_LST {
	struct BO_ITEM_SOCK_LST *arr;
	int *prev;
	int *next;
	int *free;
	int head;
	int tail;
	int size;
	int n;
} sock_lst;

/* ----------------------------------------------------------------------------
 * @brief   иниц-ет список сокетов
 */
struct BO_SOCK_LST * bo_init_sock_lst(int size, int port);

/* ----------------------------------------------------------------------------
 * @brief	добавление элемента в список
 * @ip		must C string with last element '\0'
 * @return	[1] - OK [-1] ERROR
 */
int bo_add_sock_lst(struct BO_SOCK_LST *sock_lst, char *ip);

void bo_del_sock_lst(struct BO_SOCK_LST *sock_lst);

/* ----------------------------------------------------------------------------
 * @return	[-1] no sock [>0]  sock 
 */
int bo_get_sock_by_ip(struct BO_SOCK_LST *sock_lst, char *ip);

void bo_print_sock_lst(struct BO_SOCK_LST *sock_lst);

#endif	/* BO_SEND_LST_H */

