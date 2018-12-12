#include"hellovr_opengl_main.h"


/*int main()
{
	//Windows Socket�Ķ���
	WSAData wsaData;
	SOCKET sockfd;
	char *data;
	int f_len;
	//��ʼ��socket
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

//��ʼ��socket����
int client_transfer_Init(WSAData *wsaData, SOCKET *sockfd)
{
	SOCKADDR_IN servaddr;
	//��ʼ��Window Socket
	if (WSAStartup(MAKEWORD(2, 2), wsaData)) {
		printf("Fail to initialize windows socket!\n");
		return -1;
	}
	//����һ���׽���
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("create socket error: (errno: %d)\n", WSAGetLastError());
		return 1;
	}

	/*�����׽���Ϊ������ģʽ*/
	if (set_non_Block(*sockfd)) {
		closesocket(*sockfd);
		return -1;
	}

	/*��ʼ���׽���*/
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT_NUMBER);
	/*client�������ַ*/
	if (inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) == -1) {
		printf("inet_pton error for %s\n", SERVER_IP);
		return -1;
	}

	/*����server��*/
	if (connect_non_Block(*sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
		closesocket(*sockfd);
		return -1;
	}
	////////
	printf("Start transfer!\n");
	return 0;
}

/*�����׽���Ϊ������ģʽ*/
int set_non_Block(SOCKET socket)
{
	/*��ʶ����0���������ģʽ*/
	int ret;
	unsigned long flag = 1;
	ret = ioctlsocket(socket, FIONBIO, (u_long*)&flag);
	if (ret)
		printf("set nonblock error: (errno: %d)\n", WSAGetLastError());
	return ret;
}

/*�������׽��ֵ�connect*/
int connect_non_Block(SOCKET socket, const struct sockaddr *address, int address_len)
{
	int sel;
	struct timeval tm;
	if (connect(socket, address, address_len) < 0)
	{
		fd_set wfd;
		FD_ZERO(&wfd);
		FD_SET(socket, &wfd);
		tm.tv_sec = 3;    //3��
		tm.tv_usec = 1;    //1u��

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

/*�������׽��ֵ�recv*/
int recv_non_Block(SOCKET socket, char *buffer, int length, int flags)
{
	int recv_len, ret_val, sel;
	struct timeval tm;

	for (recv_len = 0; recv_len < length;)
	{
		/*�ö���*/
		fd_set read_fd;
		FD_ZERO(&read_fd);
		FD_SET(socket, &read_fd);
		//��1s�ղ������ݾͷ���
		tm.tv_sec = 1;    //��
		tm.tv_usec = 1;    //1u��

		sel = select(socket + 1, &read_fd, NULL, NULL, &tm);  /*����select*/
		if (sel < 0) {   //����ʧ��
			printf("select socket error: (errno: %d)\n", WSAGetLastError());
			return -1;
		}
		else if (sel == 0) { //��ʱ���ؽ��յ���С����
			printf("Receive time out!(errno: %d) length=%d\n", WSAGetLastError(), recv_len);
			return recv_len;
		}
		else {
			if (FD_ISSET(socket, &read_fd)) { //��������ɶ�
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

/*�������׽��ֵ�send*/
int send_non_Block(SOCKET socket, char *buffer, int length, int flags)
{
	int send_len, ret_val, sel;
	struct timeval tm;

	for (send_len = 0; send_len < length;)
	{
		/*��д��*/
		fd_set write_fd;
		FD_ZERO(&write_fd);
		FD_SET(socket, &write_fd);
		//��1s���������ݾͷ���
		tm.tv_sec = 1;    //1��
		tm.tv_usec = 1;    //1u��

						   /*����select*/
		sel = select(socket + 1, NULL, &write_fd, NULL, &tm);
		if (sel < 0) {   //����ʧ��
			printf("select socket error: (errno: %d)\n", WSAGetLastError());
			return -1;
		}
		else if (sel == 0) {
			printf("Send time out! (errno: %d)\n", WSAGetLastError());
			return send_len;
		}
		else {
			if (FD_ISSET(socket, &write_fd)) { //���������д
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

//���ٿͻ��˴���socket
void client_transfer_Destroy(SOCKET *socket)
{
	closesocket(*socket);   //�ͻ��˹ر�����
	WSACleanup();
}