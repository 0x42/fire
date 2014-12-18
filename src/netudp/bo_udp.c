
#include "bo_udp.h"

static const int CLI_PORT = 60101;

/* ----------------------------------------------------------------------------
 * @brief	создаем UDP сокет
 * @return	[0] ERROR [>0] socket
 */
int bo_udp_sock()
{
	struct sockaddr_in saddr;
	int s = 0;
	int flag = 1;
    
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		bo_log("bo_udp_sock() ERROR can't create sock ");
		goto exit;
	}
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) != 0) {
		bo_log("bo_udp_sock() ERROR can't setsockopt ");
		goto exit;
	}
	memset((char *) &saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(CLI_PORT);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (bind(s, (struct sockaddr *) &saddr, sizeof(saddr)) != 0) {
		bo_log("bo_udp_sock() ERROR bind() ");
		close(s);
		s = 0;
		goto exit;
	}
	fcntl(s, F_SETFL, O_NONBLOCK);
	exit:
	return s;
}

/* ---------------------------------------------------------------------------- 
 * @brief	отправка сообщения
 * @return	[-1] ERROR	[1] OK
 */
int bo_send_udp_packet(int socket, unsigned char *packet, int len, char *ip)
{
	struct sockaddr_in saddr;
	struct in_addr servip;
	int ans = 1;
	int exec = -1;
	memset((char *) &saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(161);
	inet_aton(ip, &servip);
	saddr.sin_addr.s_addr = servip.s_addr;
	exec = sendto(socket, (void *)packet, len, 0, (struct sockaddr *) &saddr, 
		      sizeof(saddr)); 
	if (exec == -1) ans = -1;

	return ans;
}

int bo_recv_udp_packet(void *buf, int bufSize, 
		       int socket, 
		       char **sender_host, 
		       int *sender_port)
{
	struct sockaddr_in saddr;
	socklen_t len = sizeof(saddr);
	int exec = -1;

	exec = recvfrom(socket, buf, bufSize, MSG_DONTWAIT, (struct sockaddr *) &saddr, &len);
	if (exec == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			*sender_host = NULL;
			return 0;
		}
	}

	if (sender_host)
		*sender_host = inet_ntoa(saddr.sin_addr);

	if (sender_port)
		*sender_port = ntohs(saddr.sin_port);

	return exec;
}