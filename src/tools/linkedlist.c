#include "linkedlist.h"
/*
 *	Односвязный список
 */


/* ----------------------------------------------------------------------------
 * @brief	создание связ списка
 * @return	в слулае успеха возвр указа на список, если ошибка возвра NULL,
 *		текст ошибки передается через errTxt 
 */
LinkedList *initLList(char *errTxt)
{
	errTxt = NULL;
	LinkedList *llist = malloc(sizeof(LinkedList));
	if(llist == NULL) {
		errTxt = strerror(errno);
	} else {
		llist->Head = NULL;
		llist->Tail = NULL;
		llist->length = 0;
	}
	return llist;
}

/* ----------------------------------------------------------------------------
 * @brief	добав в хвост
 * @llist	список которому добаляем item
 * @obj		указат на вставляемый объект
 * @size	размер вставл объекта
 * @return	NULL ошибка errno в errTxt
 */
int addLLItem(LinkedList *llist, void *obj, int size, char *errTxt)
{
	int ans      = 1;
	LLItem *buf  = NULL;
	LLItem *tail = NULL;
	errTxt       = NULL;
	/* выделяем память для встав элемента*/
	buf = malloc(sizeof(LLItem));
	if(buf == NULL) {
		errTxt = strerror(errno);
		ans = 0;
		goto exit;
	}
	/* иниц. LLItem задан значениями*/
	buf->Next = NULL;
	buf->Size = size;
	buf->Object = malloc(size);
	if(buf->Object == NULL) {
		errTxt = strerror(errno);
		ans = 0;
		buf->Size = 0;
		free(buf);
		buf = NULL;
		goto exit;
	}
	memcpy(buf->Object, obj, size);
	/* вставка первого элемента в список */
	if(llist->length == 0) {
		llist->Head = buf;
		llist->Tail = buf;
		llist->length = 1;
	} else {
	/* вставка в хвост*/
		tail = llist->Tail;
		tail->Next = buf;
		llist->Tail = buf;
		llist->length++;
	}
	exit:
	return ans;
}


/* ----------------------------------------------------------------------------
 * @brief	удаление списка и всех его элементов расм вариант когда 
 *		элементы списка стандартные
 * @return	
 */
void delLList(LinkedList *llist)
{
	LLItem *item = NULL;
	LLItem *next = NULL;
	assert(llist != NULL);
	if(llist != NULL) {
		item = llist->Head;
		while(llist->length != 0) {
			if(item != NULL) {
				next = item->Next;
				item->Size = 0;
				free(item);
				item = NULL;
				item = next;
				next = NULL;
				llist->length--;
			} else {
				assert(llist->length == 0);
			}
		}
		free(llist);
		llist = NULL;
	}
}

/* 0x42 */