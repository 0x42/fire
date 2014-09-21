#define obstack_chunk_alloc b_malloc
#define obstack_chunk_free  b_free
#include "bmempool.h"

char b_global[5];

/*которая вызывается вслучае обнаружения ошибки*/
void printMemErr() 
{
	printf("	!!! MEMORY ERROR.OCUPIED STRANGIER MEMORY !!!\n");
}

void *b_malloc(int size)
{
/*  проверяет память. Выход за пределы выдел памяти. работает только на память 
 *  выделеную malloc и обязательно освобожденную.
 * printMemErr бубет вызваннаа в случае ошибки
 */
	mcheck();	
	void *ans = malloc(size);
	if(ans == NULL) {
	    printf("b_malloc(size=%d) error %s\n", size, strerror(errno));
	}
//	printf("\nb_malloc() size=%d line=%d\n", size, line);
//	printf("    ptr = %d\n", ans);
	return ans;
}

void b_free(void *ptr)
{
//    printf("\nb_free() line= %d\n   ptr=%d\n", line, ptr);
    free(ptr);
}

int b_broken() 
{
	printf("broken start\n");
	int ans = 1;
	char *dyn = NULL;
	char local[5];
	/* переполнение буфера*/
	dyn = b_malloc(5);
	strcpy(dyn, "1234");
	b_free(dyn);

	/* переполнить буфер  изрядно */
	dyn = b_malloc(5);
	strcpy(dyn, "1234");
	printf("dyn[%s]\n", dyn);
//	b_free(dyn, __LINE__);
	/*записать перед выделенной областью памяти*/
	b_free(dyn);
	/*  Ошибки будут не найдены*/
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

/* ----------------------------------------------------------------------------
 * @brief	используем obstack для хранения маленьких объектов
 */
int b_pool()
{
    struct obstack *stack_ptr = 
	(struct obstack *) b_malloc(sizeof(struct obstack));
    /* нач. инициализация */
    obstack_init(stack_ptr);
    char *str = (char *)obstack_alloc(stack_ptr, 12);
    memcpy(str, "Hello World", 12);
    char *str2 = (char *)obstack_alloc(stack_ptr, 4);
    memcpy(str2, " :P", 4);
    printf("str = %s\n", str);
    printf("str2 = %s\n", str2);
    sprintf();
    /* освобождение объектов из obstack все объекты из стека если NULL */
    obstack_free(stack_ptr, NULL);
    b_free(stack_ptr);
}   