#include "bo_fifo.h"

static struct FIFO {
	void *head;
	void *tail;
	void *mem;
	void *cur;
	void *last;
	int count;
} fifo;

int createFIFO(int size)
{
	int ans = -1;
	fifo.head = NULL;
	fifo.tail = NULL;
	fifo.mem = malloc(size);
	if(fifo.mem == NULL) {
		goto exit;
	}
	fifo.cur = fifo.mem;
	fifo.last = (fifo.mem + size);
	fifo.count = 0;
	ans = 1;
	exit:
	return ans;
}

int delFIFO()
{
	free(fifo.mem);
	fifo.mem  = NULL;
	fifo.head = NULL;
	fifo.tail = NULL;
	fifo.cur  = NULL;
	fifo.last = NULL;
}

void *add(void *obj, int size)
{
	if(fifo.count == 0) {
		
	}
}