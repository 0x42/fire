/*
 * Тестирование производится с испол - ем библиотеки Unity
 * http://sourceforge.net/projects/unity/
 * домашняя страница проекта:
 * http://throwtheswitch.org/white-papers/unity-intro.html
 * запуск тестов производится командой 
 * $ make runtest 
 */

#include <stdio.h>
#include <stdlib.h>
#include "unity/src/unity.h"

extern void loggingINFO_writeNullMsg();
extern void loggingINFO_writeOneChar();

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char** argv)
{
	RUN_TEST(loggingINFO_writeNullMsg, 11);
	RUN_TEST(loggingINFO_writeOneChar, 19);
	return 0;
}

