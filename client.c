#include "utils.h"

int main(int argc,char *argv[])
{
	if(argc!=3)
	{
		printf("Usage: %s <IP_address_of_server> <port_number>\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	int client_fd;

	if((client_fd=socket(AF_INET,SOCK_STREAM,0))==-1)
		perror_exit("socket()");

	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));

	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(atoi(argv[2]));

	if(inet_pton(AF_INET,argv[1],&server_addr.sin_addr)!=1)
		perror_exit("inet_pton()");

	printf("%s %s\n",argv[1],argv[2]);

	if(connect(client_fd,(struct sockaddr *)&server_addr,sizeof(server_addr))==-1)
		perror_exit("connect()");

	int choice,choice_htonl;

	while(TRUE)
	{
		system("clear");
		printf("===================================================\n");
		printf("****RAILWAY TICKET RESERVATION SYSTEM****\n");
		printf("===================================================\n");
		printf("1) Sign in\n");
		printf("2) Sign up\n");
		printf("3) Exit\n");

		scanf("%d",&choice);
		choice_htonl=htonl(choice);
		switch(choice)
		{
		case 1:
			if(write(client_fd,&choice_htonl,sizeof(choice_htonl))==-1)
				perror_exit("write()");
			Sign_in(client_fd);
			break;
		case 2:
			if(write(client_fd,&choice_htonl,sizeof(choice_htonl))==-1)
				perror_exit("write()");
			Sign_up(client_fd);
			break;
		case 3:
			if(write(client_fd,&choice_htonl,sizeof(choice_htonl))==-1)
				perror_exit("write()");
			if(close(client_fd)==-1)
				perror_exit("close()");
			exit(EXIT_SUCCESS);
			break;
		default:
			printf("Enter valid choice!\n");
			printf("Press Enter to show menu.\n");
			getchar();
		}
	}

	return 0;
}

