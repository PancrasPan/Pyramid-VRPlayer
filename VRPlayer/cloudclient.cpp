#include"hellovr_opengl_main.h"


/*int main()
{
	//Windows Socket的定义
	WSAData wsaData;
	SOCKET sockfd;
	char *data;
	int f_len;
	//初始化socket
	if (client_transfer_Init(&wsaData, &sockfd) == -1) {
		printf("Socket error!\n");
		exit(1);
	}
	short int seq_number = htons(5);
	data = (char*)&seq_number;
	cout << seq_number<<" "<<data << endl;
	send_non_Block(sockfd, data, 2,0);
	recv_non_Block(sockfd,buffer, SIZE, 0);
	memcpy((char*)&f_len, buffer, 4);
	f_len = ntohl(f_len);
	cout << f_len <<" "<<strlen(buffer)<< endl;
	fout.open("5.jpg", ios::out|ios::binary);
	fout.write(buffer + 4, f_len);
	fout.close();
	return 0;
}*/

//初始化socket操作
int client_transfer_Init(WSAData *wsaData, SOCKET *sockfd)
{
	SOCKADDR_IN servaddr;
	//初始化Window Socket
	if (WSAStartup(MAKEWORD(2, 2), wsaData)) {
		printf("Fail to initialize windows socket!\n");
		return -1;
	}
	//创建一个套接字
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("create socket error: (errno: %d)\n", WSAGetLastError());
		return 1;
	}

	/*设置套接字为非阻塞模式*/
	if (set_non_Block(*sockfd)) {
		closesocket(*sockfd);
		return -1;
	}

	/*初始化套接字*/
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT_NUMBER);
	/*client端输入地址*/
	if (inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) == -1) {
		printf("inet_pton error for %s\n", SERVER_IP);
		return -1;
	}

	/*连接server端*/
	if (connect_non_Block(*sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
		closesocket(*sockfd);
		return -1;
	}
	////////
	printf("Start transfer!\n");
	return 0;
}

/*设置套接字为非阻塞模式*/
int set_non_Block(SOCKET socket)
{
	/*标识符非0允许非阻塞模式*/
	int ret;
	unsigned long flag = 1;
	ret = ioctlsocket(socket, FIONBIO, (u_long*)&flag);
	if (ret)
		printf("set nonblock error: (errno: %d)\n", WSAGetLastError());
	return ret;
}

/*非阻塞套接字的connect*/
int connect_non_Block(SOCKET socket, const struct sockaddr *address, int address_len)
{
	int sel;
	struct timeval tm;
	if (connect(socket, address, address_len) < 0)
	{
		fd_set wfd;
		FD_ZERO(&wfd);
		FD_SET(socket, &wfd);
		tm.tv_sec = 3;    //3秒
		tm.tv_usec = 1;    //1u秒

		sel = select(socket + 1, NULL, &wfd, NULL, NULL);
		if (sel <0)
		{
			printf("select socket error: (errno: %d)\n", WSAGetLastError());
			return -1;
		}
		else if (sel == 0) {
			printf("connect error: (errno: %d)\n", WSAGetLastError());
		}
		else {
			printf("Connet successfully!\n");
		}
	}
	return 0;
}

/*非阻塞套接字的recv*/
int recv_non_Block(SOCKET socket, char *buffer, int length, int flags)
{
	int recv_len, ret_val, sel;
	struct timeval tm;

	for (recv_len = 0; recv_len < length;)
	{
		/*置读集*/
		fd_set read_fd;
		FD_ZERO(&read_fd);
		FD_SET(socket, &read_fd);
		//等1s收不到数据就返回
		tm.tv_sec = 1;    //秒
		tm.tv_usec = 1;    //1u秒

		sel = select(socket + 1, &read_fd, NULL, NULL, &tm);  /*调用select*/
		if (sel < 0) {   //连接失败
			printf("select socket error: (errno: %d)\n", WSAGetLastError());
			return -1;
		}
		else if (sel == 0) { //超时返回接收的最小数据
			printf("Receive time out!(errno: %d) length=%d\n", WSAGetLastError(), recv_len);
			return recv_len;
		}
		else {
			if (FD_ISSET(socket, &read_fd)) { //如果真正可读
				ret_val = recv(socket, buffer + recv_len, length - recv_len, flags);
				if (ret_val < 0) {
					printf("recv error\n");
					return ret_val;
				}
				else if (ret_val == 0) {
					printf("connection closed\n");
					return ret_val;
				}
				else
					recv_len += ret_val;
			}
		}
	}
	return recv_len;
}

/*非阻塞套接字的send*/
int send_non_Block(SOCKET socket, char *buffer, int length, int flags)
{
	int send_len, ret_val, sel;
	struct timeval tm;

	for (send_len = 0; send_len < length;)
	{
		/*置写集*/
		fd_set write_fd;
		FD_ZERO(&write_fd);
		FD_SET(socket, &write_fd);
		//等1s发不出数据就返回
		tm.tv_sec = 1;    //1秒
		tm.tv_usec = 1;    //1u秒

						   /*调用select*/
		sel = select(socket + 1, NULL, &write_fd, NULL, &tm);
		if (sel < 0) {   //连接失败
			printf("select socket error: (errno: %d)\n", WSAGetLastError());
			return -1;
		}
		else if (sel == 0) {
			printf("Send time out! (errno: %d)\n", WSAGetLastError());
			return send_len;
		}
		else {
			if (FD_ISSET(socket, &write_fd)) { //如果真正可写
				ret_val = send(socket, buffer + send_len, length - send_len, flags);
				if (ret_val < 0) {
					printf("send error\n");
					return -2;
				}
				else if (ret_val == 0) {
					printf("connection closed\n");
					return 0;
				}
				else
					send_len += ret_val;
			}
		}
	}
	return send_len;
}

//销毁客户端传输socket
void client_transfer_Destroy(SOCKET *socket)
{
	closesocket(*socket);   //客户端关闭连接
	WSACleanup();
}