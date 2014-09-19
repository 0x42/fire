#include "bmempool.h"

char b_global[5];
void printMemErr(int line) 
{
	printf("line=%d memory error \n", line);
}

void *b_malloc(int size, int line)
{
	mcheck(printMemErr);
	void *ans = malloc(size);
	return ans;
}

int b_broken() 
{
	printf("broken start\n");
	int ans = 1;
	char *dyn = NULL;
	char local[5];
	/* переполнение буфера*/
	dyn = b_malloc(5, __LINE__);
	strcpy(dyn, "12345789");
//	printf("file=%s line=%d\n", __FILE__, __LINE__);
	free(dyn);
	/* переполнить буфер  изрядно */
	dyn = malloc(5);
	strcpy(dyn, "12345789");
	printf("dyn[%s]\n", dyn);
	/*записать перед выделенной областью памяти*/
	*(dyn-1) = '\0';
	/*указатель dyn не освобожден*/
	/*атака на local*/
	strcpy(local, "12345");
	printf("local[%s]\n", local);
	local[-1] = '\0';
	printf("local[%s]\n", local);
	/*атака на global*/
	strcpy(b_global, "12345");
	printf("b_global[%s]\n", b_global);
	b_global[-1] = '\0';
	
	printf("broken end\n");
	return ans;
}
