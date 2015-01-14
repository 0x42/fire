#include "bo_send_lst.h"

struct BO_ITEM_SOCK_LST {
    char ip[16];
    int rate;
    int sock;
};

static struct SOCK_LST {
	int itemN;
	struct BO_ITEM_SOCK_LST *arr;
	int head;
	int tail;
	int last;
	int count;
	int free;
} sock_lst = {0};

int bo_init_sock_lst()
{
	int ans = -1;
	int itemN = 255;
	
	sock_lst.arr = (struct BO_ITEM_SOCK_LST *)
		malloc(sizeof(struct BO_ITEM_SOCK_LST)*itemN);
	
	if(sock_lst.arr == NULL) goto exit;
	memset(sock_lst.arr, 0, sizeof(struct BO_ITEM_SOCK_LST)*itemN);
	
	sock_lst.itemN	= itemN;
	sock_lst.head	= 0;
	sock_lst.tail	= 0;
	sock_lst.last	= itemN - 1;
	sock_lst.count	= 0;
	sock_lst.free	= itemN;

	ans		= 1;
	
	exit:
	return ans;
}

int bo_add_new_sock_lst(char *ip)
{
	int ans = -1;
	int rate = 12000;
	
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	возвращает сокет у которого рейтинг минимальный
 * @return	[0 - 254] index from sock_lst.arr array 
 */
static int get_lazy_sock()
{
	int index = -1, i = -1, min_rate = 1000000;
	
	i = sock_lst.head;
	do {
		
	} while(i != sock_lst.tail);
	
	
	return index;
}