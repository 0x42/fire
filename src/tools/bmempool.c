#define obstack_chunk_alloc b_malloc_stack
#define obstack_chunk_free  b_free
#include "bmempool.h"

char b_global[5];

static jmp_buf bufferEnv;

/*которая вызывается вслучае обнаружения ошибки*/
void printMemErr() 
{
	printf("	!!! MEMORY ERROR.OCUPIED STRANGIER MEMORY !!!\n");
}

void *b_malloc(int size)
{
/*  проверяет память. Выход за пределы выдел памяти. работает только на память 
 *  выделеную malloc и обязательно освобожденную.
 *  printMemErr будет вызванна в случае ошибки
 */
	mcheck(printMemErr);	
	void *ans = malloc(size);
	if(ans == NULL) {
	    printf("b_malloc(size=%d) error %s\n", size, strerror(errno));
	    exit(1);
	}
//	printf("\nb_malloc() size=%d line=%d\n", size, line);
//	printf("    ptr = %d\n", ans);
	return ans;
}

void *b_malloc_stack(size_t size)
{
	void *ans = NULL;
	ans = malloc(size);
	if(ans == NULL) {
		longjmp(bufferEnv, -1);
	}
	return ans;
}

void b_free(void *ptr)
{
//    printf("\nb_free() line= %d\n   ptr=%d\n", line, ptr);
    free(ptr);
}


/* ----------------------------------------------------------------------------
 * @brief		 созд. obstack в кучи и инициал.
 * @return		 возв указатель на obstack или NULL если не удалось 
 *			выделить память
 */
struct obstack *b_crtStack()
{
	/* функция b_malloc должна заверш только успешно выдел памятью
	 */
	struct obstack *stack_ptr = 
		(struct obstack *)b_malloc(sizeof(struct obstack));
	/* вызывает функцию опред макросом obstack_chunk_alloc*/
	if(stack_ptr != NULL) {
		if(setjmp(bufferEnv)) {
			return NULL;
		} else {
			obstack_init(stack_ptr);
		}
	}
	return stack_ptr;
}
/* ----------------------------------------------------------------------------
 * @brief		выделение памяти размером size в obstack
 * @return		возв указ на место где была выделена память
 */
void *b_allocInStack(struct obstack *stack_ptr, size_t size)
{
	void *ans = NULL;
	/* если функция вызвана не longjmp то всегда возв NULL*/
	if(setjmp(bufferEnv)) {
	/* обработка ошибки вызв функ в блоке else */
		return NULL;
	} else {
		ans = obstack_alloc(stack_ptr, size);
	}
	return ans;
}
/* ----------------------------------------------------------------------------
 * @brief		удал obstack и все что в нем наход-ся
 * @ptr			указ на удал-ый obstack
 */
void b_delStack(struct obstack *ptr)
{
	/* NULL указ на удаление всех объектов в пуле*/
	obstack_free(ptr, NULL);
	/*  удал самого пула */
	b_free(ptr);
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
    sprintf(str, "%s%s", str, str2);
    printf("str = %s\n", str);
    printf("str2 = %s\n", str2);
    /* освобождение объектов из obstack все объекты из стека если NULL */
    obstack_free(stack_ptr, NULL);
    b_free(stack_ptr);
    return 1;
}

int b_broken() 
{
	printf("broken start\n");
	int ans = 1;
	char *dyn = NULL;
	char local[5];
	/* переполнение буфера*/
	dyn = b_malloc(5);
	// strcpy(dyn, "123456");
	strcpy(dyn, "1234");
	b_free(dyn);

	/* переполнить буфер  изрядно */
	dyn = b_malloc(5);
	// strcpy(dyn, "12345678");
	strcpy(dyn, "1234");
	printf("dyn[%s]\n", dyn);
//	b_free(dyn, __LINE__);
	/*записать перед выделенной областью памяти*/
	b_free(dyn);
	/*  Ошибки будут не найдены тк free до */
	*(dyn-1) = '\0';
	/*указатель dyn не освобожден*/

	/* атака на local найти инструмент для выявления таких ошибок */
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