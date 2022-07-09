#include "server_utils.h"

int main(int argc,char *argv[])
{
	if(argc!=2)
	{
		printf("Usage: %s <port_number>\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	int socket_fd,conn_fd;
	struct sockaddr_in serv_addr;

	if((socket_fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))==-1)
		perror_exit("socket()");

	memset(&serv_addr,0,sizeof(serv_addr));

	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(atoi(argv[1]));

#ifdef debug
	printf("port number=%s\n",argv[1]);
#endif

	if(bind(socket_fd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1)
		perror_exit("bind()");

	if(listen(socket_fd,5)==-1)
		perror_exit("listen()");

	while(TRUE)
	{
#ifdef debug
		printf("Waiting for Connection...\n");
#endif
		if((conn_fd=accept(socket_fd,(struct sockaddr*)NULL,NULL))==-1)
			perror_exit("accept()");

#ifdef debug
		printf("Connection received!\n");
#endif

		int ret=fork();
		if(ret==0)
		{
			close(socket_fd);
			service_client((void*)&conn_fd);
		}
		else if(ret>0)
			close(conn_fd);
		else
			perror_exit("fork()")
	}

	close(socket_fd);

	return 0;
}
