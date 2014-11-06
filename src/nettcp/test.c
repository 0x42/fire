int bo_sendDataFIFO(char *ip, unsigned int port, 
	char *data, unsigned int dataSize)
{
	const int excep = -1;
	int ans  = -1;
	int sock = -1;
	int exec = -1;
	char *head = "SET";
	unsigned char len[2] = {0};
	char buf[4] = {0};
	char *ok = NULL;
	
	sock = bo_setConnect(ip, port);
	if(sock != -1) {
		boIntToChar(dataSize, len);
		exec = bo_sendAllData(sock, (unsigned char*)head, 3);
		if(exec == -1) goto error;
		exec = bo_sendAllData(sock, len, 2);
		if(exec == -1) goto error;
		exec = bo_sendAllData(sock, (unsigned char*)data, dataSize);
		if(exec == -1) goto error;
		exec = bo_recvAllData(sock, (unsigned char*)buf, 3, 3);
		if(exec == -1) goto error;
			
		ok = strstr(buf, "OK");
		if(ok) ans = 1;
		else {
			bo_log("bo_sendDataFIFO() ip[%s] wait[OK] but recv[%s]:\n%s", 
				ip,  
				buf,
				"data don't write in FIFO.");
		}

		if(excep == 1) {
error:
			bo_log("bo_sendDataFIFO() errno[%s]\n ip[%s]\nport[%d]\n", 
				strerror(errno), ip, port);
		}
		if(close(sock) == -1) {
			bo_log("bo_sendDataFIFO() when close socket errno[%s]\n ip[%s]\nport[%d]\n", 
				strerror(errno), ip, port);
		}
	} else {
		bo_log("bo_sendDataFIFO().bo_crtSock() errno[%s]\n ip[%s]\nport[%d]\nsize[%d]", 
			strerror(errno),
			ip,
			port,
			dataSize);
	}
	return ans;
}