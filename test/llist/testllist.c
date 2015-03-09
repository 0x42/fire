#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../../src/tools/listsock.h"
#include "../../src/nettcp/bo_send_lst.h"
#include "unity_fixture.h"

TEST_GROUP(llist);

TEST_SETUP(llist) {}

TEST_TEAR_DOWN(llist) {}

TEST(llist, simpleTest)
{
 	struct bo_llsock *list = bo_crtLLSock(10);
	struct bo_sock *val = NULL;
	int s[] = {1,2,3,4,5,6,7,8,9,10};
	int exec = -1;
	int pos = -1;
	int i = 0;
	int ans = 1;
	printf("simpleTest ... \n");
	/* 
	 * добавляем один элемент один снимаем
	 */
	for(; i < 10; i++) {
		exec = bo_addll(list, s[i]);
		if(exec == -1) {
			ans = -1; 
			printf("list is full\n");
			break;
		}
		pos = bo_get_head(list);
		if(pos == -1) {
			printf("list is empty\n");
			ans = -1;
			break;
		} 

		exec = bo_get_val(list, &val, pos);
		if(val == NULL) {
			printf("val is NULL\n"); ans = -1; break;
		}
		
		if(exec != -1) {
			printf("error in list \n"); ans = -1; break;
		}
		
		if(val->sock != s[i]) { printf("list return bad param\n");
			printf(" error s[%d] != val[%d]\n", s[i], val->sock); 
			ans = -1; break;
		}
		
		bo_del_val(list, pos);
	}
	bo_del_lsock(list);
	
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(llist, overflowListTest)
{
	struct bo_llsock *list = bo_crtLLSock(10);
	struct bo_sock *val = NULL;
	int s[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14};
	int exec = -1;
	int i = 0;
	int ans = 1;
	printf("overflowListTest() ... \n");
	/* 
	 * добавляем 
	 */

	for(i = 0; i < 14; i++) {
		exec = bo_addll(list, s[i]);

		if(i < 9) {
			if(exec == -1) {
				ans = -1; 
				printf("list is full\n"); break;
			}
		} else {
			if(exec != -1) {
				bo_print_list(list);
				ans = -1;
				printf("list is not full\n"); break;
			}
		}
		
	}
	bo_del_lsock(list);
	
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(llist, checkDelValTest)
{
	struct bo_llsock *list = bo_crtLLSock(10);
	struct bo_sock *val = NULL;
	int s[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14};
	int exec = -1;
	int i = 0;
	int ans = 1;
	int stop = 1;
	printf("checkDelValTest() ... \n");
	
	exec = bo_addll(list, 1);
	exec = bo_addll(list, 2);
	exec = bo_addll(list, 3);

	bo_del_val(list, 2);
	
	i = bo_get_head(list);
	
	exec = bo_get_val(list, &val, i);
	if(exec == -1) { ans = -1; printf("list is empty\n"); goto end; }
	if(val->sock != 1) { ans = -1; printf("bad param\n");goto end; }
	
	exec = bo_get_val(list, &val, exec);
	if(exec != -1) { ans = -1; printf("list is not empty\n"); goto end; }
	if(val->sock != 3) { ans = -1; printf("bad param\n");goto end; }
	
	end:
	bo_del_lsock(list);
	
	TEST_ASSERT_EQUAL(1, ans);
}

/* ************************************************************************** */
/* test bo_send_lst							      */
/* ************************************************************************** */
void *startServer(void *arg)
{
	printf("startServer ...\n");
	int sock = bo_servStart(8888, 10);
	int client_sock;
	int exec = -1;
	char *errTxt;
	if(sock == -1) {
		printf("test_sock_lst_add() startServer sock can't start\n");
	} else {
		exec = bo_waitConnect(sock, &client_sock, &errTxt);
		if(exec == 1) {
			close(client_sock);
			close(sock);
			return (void *)1;
		}
		
	}
	close(sock);
	return (void *)-1;
}

TEST(llist, test_sock_lst_add)
{
	int ans  = -1;
	int exec = -1;
	int size = 4, port = 8888;
	void *res;
	struct BO_SOCK_LST *lst = NULL;
	pthread_t thr;
	printf("test_sock_lst_add run ... \n");
	lst = bo_init_sock_lst(size, port);
	if(lst == NULL) { printf("bo_init_sock_lst() - ERROR\n"); goto end; }
	
	exec = pthread_create(&thr, NULL, &startServer, NULL);
	
	exec = bo_add_sock_lst(lst, "127.0.0.1");
	if(exec == -1) { printf("bo_add_sock_lst() - ERROR\n"); goto end;}
	


	exec = pthread_join(thr, &res);
	if(exec == 0) {
		exec = (int)res;
		if(exec == -1) { 
			printf("startServer ERROR\n");
			goto end;
		}
	} else {
		printf("pthread_join ERROR\n"); goto end;
	}
	
	ans = 1;
	end:
	TEST_ASSERT_EQUAL(1, ans);
}

void *startServer2(void *arg)
{
	printf("startServer2 ...\n");
	int sock = bo_servStart(8888, 10);
	int client_sock;
	int cl_sock[10] = {-1};
	int index = 0, i;
	int exec = -1, stop;
	fd_set r_set;
	struct timeval tval;
	int count = 0;
	char buf[4] = {0}; int bufN;
	if(sock == -1) {
		printf("test_sock_lst_add2() startServer sock can't start\n");
	} else {
		stop = 1;
		int time_out = 20*10; // 10 sec
		
		while(stop == 1) {
			tval.tv_sec = 0; tval.tv_usec = 50000;
			FD_ZERO(&r_set);
			FD_SET(sock, &r_set);
			
			for(i = 0; i < index; i++) {
				if(cl_sock[i] != -1) FD_SET(cl_sock[i], &r_set);
			}
			
			exec = select(FD_SETSIZE, &r_set, NULL, NULL, &tval);
			time_out--;
			
			if(time_out == 0) { printf("time is out\n"); stop = -1;}
			
			if(exec == -1) {
				printf("select errno[%s]\n", strerror(errno)); 
				stop = -1;
			} else if(exec > 0 ) {
				if(FD_ISSET(sock, &r_set) == 1) {
					client_sock = accept(sock, NULL, NULL);
					if(client_sock == -1) printf("server accept error[%s]\n", 
						strerror(errno));
					cl_sock[index] = client_sock;
					count++;
					index++;
				}
				for(i = 0; i < index; i++) {
					if(FD_ISSET(cl_sock[i], &r_set) == 1) {
						bufN = recv(cl_sock[i], buf, 3, 0);
						if(bufN == 3) {
							if(strstr(buf, "END")) {
								return (void *)count;
							}
						}
					}
				}
			}
			
		}
		printf("stopServer2 ... \n");
	}
	end:
	return (void *)-1;
}

TEST(llist, test_sock_lst_add2)
{
	int ans  = -1;
	int exec = -1;
	int size = 3, port = 8888, sock = -1;
	void *res;
	pthread_t thr;
	struct BO_SOCK_LST *lst = NULL;
	
	printf("\n\n test_sock_lst_add run ... \n");
	lst = bo_init_sock_lst(size, port);
	if(lst == NULL) { printf("bo_init_sock_lst() - ERROR\n"); goto end; }
	
	exec = pthread_create(&thr, NULL, &startServer2, NULL);
	sleep(3);
	
	exec = bo_add_sock_lst(lst, "127.0.0.001");
	if(exec == -1) { printf("bo_add_sock_lst()1 - ERROR\n"); goto end;}
	
	exec = bo_add_sock_lst(lst, "127.0.0.1");
	
	if(exec == -1) { printf("bo_add_sock_lst()2 - ERROR\n"); goto end;}
	
	
	sock = bo_get_sock_by_ip(lst, "127.0.0.001");
	if(sock == -1) { printf("bo_get_sock_by_ip - ERROR\n"); goto end;}
	
	
	exec = send(sock, "END", 3, MSG_NOSIGNAL);
	if(exec != 3) { printf("send - ERROR\n"); goto end;}
	
	exec = pthread_join(thr, &res);
	if(exec == 0) {
		exec = (int)res;
		if(exec == -1) { 
			printf("startServer2 return ERROR\n");
			goto end;
		}
	} else {
		printf("pthread_join ERROR\n"); goto end;
	}
	ans = 1;
	end:
	TEST_ASSERT_EQUAL(1, ans);
}