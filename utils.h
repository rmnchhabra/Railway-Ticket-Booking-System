#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "struct.h"

#define perror_exit(msg) {perror(msg); printf("Error at line=%d and file=%s\n",__LINE__,__FILE__); exit(EXIT_FAILURE);}

void Book_Ticket(int,struct account_details*);
void View_Previous_Bookings(int,struct account_details*);
void Update_Booking(int,struct account_details*);
void Cancel_Booking(int,struct account_details*);
void Sign_in(int);
void Sign_up(int);
void customer_menu(int,struct account_details*);
void delete_account(int,struct account_details*);
void modify_account_password(int,struct account_details*);
void search_account(int,struct account_details*);
void add_train(int,struct account_details*);
void delete_train(int,struct account_details*);
void modify_ticket_price(int,struct account_details*);
void search_train(int, struct account_details*);
void admin_menu(int,struct account_details*);

/*-----------------------Functions-----------------------*/
void Book_Ticket(int client_fd, struct account_details *details)
{
	int count;
	struct train_data train_data;
	if(read(client_fd, &count, sizeof(count)) == -1)
		perror_exit("read()");
	count=ntohl(count);

#ifdef debug
	printf("total train count=%d\n",count);
#endif

	if(count==0)
		printf("No trains are available!\n");
	else
	{
		while(count--)
		{
			if(read(client_fd, &train_data, sizeof(train_data)) == -1)
				perror_exit("read()");
			printf("train_no=%d \ttrain_name=%s \tboarding_point=%s \tdestination_point=%s \ttotal_seats=%d \tbooked_seats=%d \tticket_price=%d \tdate=%s\n\n",
					train_data.train_no,
					train_data.train_name,
					train_data.boarding_point,
					train_data.destination_point,
					train_data.total_seats,
					train_data.booked_seats,
					train_data.ticket_price,
					train_data.date);
		}

		int train_no,status,tickets;
		while(TRUE)
		{
			printf("Enter the train number you want to book=");
			scanf("%d",&train_no);
			if(train_no>=0)
				break;
			else
				printf("train number cannot be negative\n");
		}
		train_no=htonl(train_no);

		while(TRUE)
		{
			printf("Enter number of tickets you want to book=");
			scanf("%d",&tickets);
			if(tickets>0)
				break;
			else
				printf("Number of tickets must be positive\n");
		}
		tickets=htonl(tickets);

		if(write(client_fd, &train_no, sizeof(train_no)) == -1)
			perror_exit("write()");

		if(write(client_fd, &tickets, sizeof(tickets)) == -1)
			perror_exit("write()");

		if(read(client_fd, &status, sizeof(status)) == -1)
			perror_exit("read()");
		status=ntohl(status);

		if(status==SUCCESS)
			printf("Booking successful!\n");
		else
			printf("Booking Failed, try again!\n");
	}

	printf("Press any key to show menu, Don't press ENTER\n");
	while(getchar()=='\n');
}

void View_Previous_Bookings(int client_fd, struct account_details *details)
{
	int count;
	struct booked_train_data booking_data;
	if(read(client_fd, &count, sizeof(count)) == -1)
		perror_exit("read()");
	count=ntohl(count);

#ifdef debug
	printf("total booking count received=%d\n",count);
#endif

	if(count==0)
		printf("No booking found!\n");
	else
	{
		while(count--)
		{
			if(read(client_fd, &booking_data, sizeof(booking_data)) == -1)
				perror_exit("read()");
			printf("account_type=%d \tuser_id=%d \ttrain_no=%d \ttrain_nanme=%s \tboarding_station=%s \tdestination_station=%s \tseats_booked=%d \tticket_price=%d \tdate=%s\n\n",
					booking_data.account_type,
					booking_data.userid,
					booking_data.train_no,
					booking_data.train_name,
					booking_data.boarding_point,
					booking_data.destination_point,
					booking_data.seats_booked,
					booking_data.ticket_price,
					booking_data.date);
		}
	}

	printf("Press any key to show menu, Don't press ENTER\n");
	while(getchar()=='\n');
}
void Update_Booking(int client_fd, struct account_details *details)
{
	int train_no,tickets,status;
	while(TRUE)
	{
		printf("Enter the train number you want to update ticket = ");
		scanf("%d",&train_no);
		if(train_no>=0)
			break;
		else
			printf("Train number cannot be negative\n");
	}
	train_no = htonl(train_no);
	if(write(client_fd, &train_no, sizeof(train_no)) == -1)
		perror_exit("write()");

	while(TRUE)
	{
		printf("Enter the train new required number of seats = ");
		scanf("%d",&tickets);
		if(tickets>0)
			break;
		else
			printf("Number of tickets should be positive\n");
	}
	tickets = htonl(tickets);
	if(write(client_fd, &tickets, sizeof(tickets)) == -1)
		perror_exit("write()");

	if(read(client_fd, &status, sizeof(status)) == -1)
		perror_exit("read()");

	status=ntohl(status);
	if(status==SUCCESS)
		printf("Booking updated successfully!\n");
	else
		printf("Booking is not updated!\n");

	printf("Press any key to show menu, Don't press ENTER\n");
	while(getchar()=='\n');
}
void Cancel_Booking(int client_fd, struct account_details *details)
{
	int train_no,status;
	while(TRUE)
	{
		printf("Enter Train no to Cancel: ");
		scanf("%d",&train_no);
		if(train_no>=0)
			break;
		else
			printf("train number cannot be negative\n");
	}
	train_no = htonl(train_no);
	if(write(client_fd, &train_no, sizeof(train_no)) == -1)
		perror_exit("write()");

	if(read(client_fd, &status, sizeof(status)) == -1)
		perror_exit("read()");
	status=ntohl(status);

	if(status==SUCCESS)
		printf("Booking cancelled successfully!\n");
	else
		printf("No booking found, Booking cancellation failed!.\n");

	printf("Press any key to show menu, Don't press ENTER\n");
	while(getchar()=='\n');
}

void Sign_in(int client_fd)
{
	struct account_details details;

	printf("Enter account type:\n");
	printf("1. NORMAL_ACCOUNT\n");
	printf("2. AGENT_ACCOUNT\n");
	printf("3. ADMINISTRATOR_ACCOUNT\n");

	scanf("%d",&details.acc_type);

	if(!(details.acc_type==1 || details.acc_type==2 || details.acc_type==3))
	{
		printf("Enter valid account type!\n");
		printf("Press any key to show menu, Don't press ENTER\n");
		while(getchar()=='\n');
		return;
	}

	details.acc_type=htonl(details.acc_type);

	while(TRUE)
	{
		printf("Enter Userid:");
		scanf("%d",&details.userid);
		if(details.userid>=0)
			break;
		else
			printf("Userid cannot be negative\n");
	}
	details.userid=htonl(details.userid);

	strcpy(details.password,getpass("Enter password:"));

	if(write(client_fd,&details,sizeof(struct account_details))==-1)
		perror_exit("write()");

	int login_response;
	if(read(client_fd,&login_response,sizeof(login_response))==-1)
		perror_exit("read()");
	login_response=ntohl(login_response);

	while(getchar()!='\n'); //clear stdin

	if(login_response==0)
	{
		printf("Login Successful!\n");
		printf("Press enter to show menu.\n");
		getchar();
		if(ntohl(details.acc_type)==ADMINISTRATOR_ACCOUNT)
			admin_menu(client_fd,&details);
		else
			customer_menu(client_fd,&details);
	}
	else if(login_response==-1)
	{
		printf("Invalid Login!\n");
		printf("Press any key to retry.\n");
		getchar();
	}
	else if(login_response==2)//for admin and normal account; for agent multiple logins possible
	{
		printf("Already Logged in!\n");
		printf("Press any key to retry.\n");
		getchar();
	}
}

void Sign_up(int client_fd)
{
	struct account_details details;

	printf("Enter account type:\n");
	printf("1. NORMAL_ACCOUNT\n");
	printf("2. AGENT_ACCOUNT\n");
	printf("3. ADMINISTRATOR_ACCOUNT\n");

	int type;
	scanf("%d",&details.acc_type);
	type=details.acc_type;

	details.acc_type=htonl(details.acc_type);

	printf("Enter Username:");
	scanf("%s",details.username);

	strcpy(details.password,getpass("Enter password:"));

	if(write(client_fd,&details,sizeof(struct account_details))==-1)
		perror_exit("write()");

	char super_admin_password[PASSWORD_LEN];
	char super_admin_username[USERNAME_LEN];
	/*new admin accound need super admin password*/
	if(type==ADMINISTRATOR_ACCOUNT)
	{
		printf("Enter Super Admin username:");
		scanf("%s",super_admin_username);
		if(write(client_fd,super_admin_username,sizeof(super_admin_username))==-1)
			perror_exit("write()");

		strcpy(super_admin_password,getpass("Enter Super Admin password:"));
		if(write(client_fd,super_admin_password,sizeof(super_admin_password))==-1)
			perror_exit("write()");
	}

	if(read(client_fd,&details.userid,sizeof(details.userid))==-1)
		perror_exit("read()");
	details.userid=ntohl(details.userid);

	if(details.userid!=-1)
	{
		printf("Sign up successful.\n");
		printf("Your account number for further login: %d\n", details.userid);
	}
	else
		printf("Sign up not successful.\n");

	printf("Press any key to show menu, Don't press ENTER\n");
	while(getchar()=='\n');
}

void customer_menu(int client_fd,struct account_details *details)
{
	int choice,choice_htonl;
	while(TRUE)
	{
		system("clear");
		printf("===================================================\n");
		printf("****************Customer menu****************\n");
		printf("===================================================\n");
		printf("Enter your choice\n");
		printf("1) Book Ticket\n");
		printf("2) View Previous Bookings\n");
		printf("3) Update Booking\n");
		printf("4) Cancel Booking\n");
		printf("5 and other) Exit\n");
		scanf("%d",&choice);
		choice_htonl=htonl(choice);

		if(write(client_fd,&choice_htonl,sizeof(choice_htonl))==-1)
			perror_exit("write()");

		switch(choice)
		{
		case 1://Book Ticket
			Book_Ticket(client_fd,details);
			break;
		case 2:
			View_Previous_Bookings(client_fd,details);
			break;
		case 3:
			Update_Booking(client_fd,details);
			break;
		case 4:
			Cancel_Booking(client_fd,details);
			break;
		default:
			if(close(client_fd)==-1)
				perror_exit("close()");
			exit(EXIT_SUCCESS);
		}
	}
}
void delete_account(int client_fd,struct account_details *details)
{
	int userid,status,acct_type,acct_type_htonl;
	while(TRUE)
	{
		printf("Enter userid related to account:\n");
		scanf("%d",&userid);
		if(userid>=0)
			break;
		else
			printf("userid cannot be negative\n");
	}
	userid=htonl(userid);

	if((write(client_fd,&userid,sizeof(userid)))==-1)
		perror_exit("write()");

	printf("Enter account type related to account:\n");
	printf("1. NORMAL_ACCOUNT\n");
	printf("2. AGENT_ACCOUNT\n");
	printf("3. ADMINISTRATOR_ACCOUNT\n");
	scanf("%d",&acct_type);
	acct_type_htonl=htonl(acct_type);

	if((write(client_fd,&acct_type_htonl,sizeof(acct_type_htonl)))==-1)
		perror_exit("write()");

	if(acct_type==ADMINISTRATOR_ACCOUNT)
	{
		char super_admin_password[PASSWORD_LEN];
		char super_admin_username[USERNAME_LEN];

		printf("Enter Super Admin username:");
		scanf("%s",super_admin_username);
		if(write(client_fd,super_admin_username,sizeof(super_admin_username))==-1)
			perror_exit("write()");

		strcpy(super_admin_password,getpass("Enter Super Admin password:"));
		if(write(client_fd,super_admin_password,sizeof(super_admin_password))==-1)
			perror_exit("write()");
	}

	if(read(client_fd,&status,sizeof(status))==-1)
		perror_exit("read()");
	status=ntohl(status);

	if(status==SUCCESS)
		printf("Account deletion successful!\n");
	else
		printf("Account deletion is not successful!\n");

	printf("Press any key to show menu, Don't press ENTER\n");
	while(getchar()=='\n');

}

void modify_account_password(int client_fd,struct account_details *details)
{
	int userid,status,acct_type,acct_type_htonl;
	while(TRUE)
	{
		printf("Enter userid related to account:\n");
		scanf("%d",&userid);
		if(userid>=0)
			break;
		else
			printf("Userid cannot be negative\n");
	}
	userid=htonl(userid);

	if((write(client_fd,&userid,sizeof(userid)))==-1)
		perror_exit("write()");

	printf("Enter account type related to account:\n");
	printf("1. NORMAL_ACCOUNT\n");
	printf("2. AGENT_ACCOUNT\n");
	printf("3. ADMINISTRATOR_ACCOUNT\n");
	scanf("%d",&acct_type);
	acct_type_htonl=htonl(acct_type);

	if((write(client_fd,&acct_type_htonl,sizeof(acct_type_htonl)))==-1)
		perror_exit("write()");

	char new_password[PASSWORD_LEN];
	strcpy(new_password,getpass("Enter new password:"));
	if(write(client_fd,new_password,sizeof(new_password))==-1)
		perror_exit("write()");

	if(acct_type==ADMINISTRATOR_ACCOUNT)
	{
		char super_admin_password[PASSWORD_LEN];
		char super_admin_username[USERNAME_LEN];

		printf("Enter Super Admin username:");
		scanf("%s",super_admin_username);
		if(write(client_fd,super_admin_username,sizeof(super_admin_username))==-1)
			perror_exit("write()");

		strcpy(super_admin_password,getpass("Enter Super Admin password:"));
		if(write(client_fd,super_admin_password,sizeof(super_admin_password))==-1)
			perror_exit("write()");
	}

	if(read(client_fd,&status,sizeof(status))==-1)
		perror_exit("read()");
	status=ntohl(status);

	if(status==SUCCESS)
		printf("Account password change successful!\n");
	else
		printf("Account password change failed!\n");

	printf("Press any key to show menu, Don't press ENTER\n");
	while(getchar()=='\n');
}

void search_account(int client_fd,struct account_details *details)
{
	int userid,status,acct_type,acct_type_htonl;
	while(TRUE)
	{
		printf("Enter userid related to account:\n");
		scanf("%d",&userid);
		if(userid>=0)
			break;
		else
			printf("Userid cannot be negative\n");
	}
	userid=htonl(userid);

	if((write(client_fd,&userid,sizeof(userid)))==-1)
		perror_exit("write()");

	printf("Enter account type related to account:\n");
	printf("1. NORMAL_ACCOUNT\n");
	printf("2. AGENT_ACCOUNT\n");
	printf("3. ADMINISTRATOR_ACCOUNT\n");
	scanf("%d",&acct_type);
	acct_type_htonl=htonl(acct_type);

	if((write(client_fd,&acct_type_htonl,sizeof(acct_type_htonl)))==-1)
		perror_exit("write()");

	if(read(client_fd,&status,sizeof(status))==-1)
		perror_exit("read()");
	status=ntohl(status);

	struct account_details recv_details;
	if(status==SUCCESS)
	{
		printf("Account search successful!\n");
		if(read(client_fd,&recv_details,sizeof(recv_details))==-1)
			perror_exit("read()");

		recv_details.acc_type=ntohl(recv_details.acc_type);
		recv_details.userid=ntohl(recv_details.userid);

		printf("acc_type=%d \n userid=%d \n username=%s \n password=%s \n is_deleted=%d\n"
				,recv_details.acc_type
				,recv_details.userid
				,recv_details.username
				,recv_details.password
				,recv_details.is_deleted);
	}
	else
		printf("Account search is not successful!\n");

	printf("Press any key to show menu, Don't press ENTER\n");
	while(getchar()=='\n');
}

void add_train(int client_fd,struct account_details *details)
{
	int status;
	struct train_data td;

	printf("Enter train name:\n");
	scanf("%s",td.train_name);

	printf("Enter date:\n");
	scanf("%s",td.date);

	printf("Enter boarding_point:\n");
	scanf("%s",td.boarding_point);

	printf("Enter destination_point:\n");
	scanf("%s",td.destination_point);

	td.train_no=-1;//server provides train number
	td.train_no=htonl(td.train_no);

	while(TRUE)
	{
		printf("Enter total_seats:\n");
		scanf("%d",&td.total_seats);
		if(td.total_seats>0)
			break;
		else
			printf("Total seats must be positive\n");
	}
	td.total_seats=htonl(td.total_seats);

	td.booked_seats=0;
	td.booked_seats=htonl(td.booked_seats);

	while(TRUE)
	{
		printf("Enter 1 seat price:\n");
		scanf("%d",&td.ticket_price);
		if(td.ticket_price>0)
			break;
		else
			printf("Ticket price should be positive\n");
	}
	td.ticket_price=htonl(td.ticket_price);

	td.is_deleted=false;

	if((write(client_fd,&td,sizeof(td)))==-1)
		perror_exit("write()");

	if(read(client_fd,&td.train_no,sizeof(td.train_no))==-1)
		perror_exit("read()");

	td.train_no=ntohl(td.train_no);
	if(td.train_no>=0)
		printf("New train added successfully with train number=%d!\n",td.train_no);
	else
		printf("New train addition is not successful!\n");

	printf("Press any key to show menu, Don't press ENTER\n");
	while(getchar()=='\n');
}

void delete_train(int client_fd, struct account_details *details)
{
	int train_no;
	while(TRUE)
	{
		printf("Enter train_no:\n");
		scanf("%d",&train_no);
		if(train_no>=0)
			break;
		else
			printf("train number cannot be negative\n");
	}

	train_no=htonl(train_no);
	if(write(client_fd,&train_no,sizeof(train_no))==-1)
		perror_exit("write()");

	int status;
	if(read(client_fd,&status,sizeof(status))==-1)
		perror_exit("read()");
	status=ntohl(status);

	if(status==SUCCESS)
		printf("Train deleted successfully!\n");
	else
		printf("Train deletion failed.\n");

	printf("Press any key to show menu, Don't press ENTER\n");
	while(getchar()=='\n');
}

void modify_ticket_price(int client_fd, struct account_details *details)
{
	int train_no,new_price;
	while(TRUE)
	{
		printf("Enter train_no:\n");
		scanf("%d",&train_no);
		if(train_no>=0)
			break;
		else
			printf("Train number cannot be negative\n");
	}
	train_no=htonl(train_no);

	if(write(client_fd,&train_no,sizeof(train_no))==-1)
		perror_exit("write()");

	while(TRUE)
	{
		printf("Enter new ticket price:\n");
		scanf("%d",&new_price);
		if(new_price>0)
			break;
		else
			printf("Ticket price must be positive\n");
	}

	new_price=htonl(new_price);
	if(write(client_fd,&new_price,sizeof(new_price))==-1)
		perror_exit("write()");

	int status;
	if(read(client_fd,&status,sizeof(status))==-1)
		perror_exit("read()");
	status=ntohl(status);

	if(status==SUCCESS)
		printf("Train price changed successfully!\n");
	else
		printf("Train price change failed.\n");

	printf("Press any key to show menu, Don't press ENTER\n");
	while(getchar()=='\n');
}

void search_train(int client_fd, struct account_details *details)
{
	int train_no;
	while(TRUE)
	{
		printf("Enter train_no:\n");
		scanf("%d",&train_no);
		if(train_no>=0)
			break;
		else
			printf("Train number cannot be negative\n");
	}

#ifdef debug
	printf("train number=%d\n",train_no);
#endif
	train_no=htonl(train_no);

	if(write(client_fd,&train_no,sizeof(train_no))==-1)
		perror_exit("write()");

	int status;
	if(read(client_fd,&status,sizeof(status))==-1)
		perror_exit("read()");
	status=ntohl(status);

#ifdef debug
	printf("status=%d\n",status);
#endif

	if(status==SUCCESS)
	{
		struct train_data td;
		if(read(client_fd,&td,sizeof(td))==-1)
			perror_exit("read()");

		printf("Train information:\n");
		printf("train_no=%d \n train_name=%s \n boarding_point=%s \n destination_point=%s \n total_seats=%d \n booked_seats=%d \n ticket_price=%d \n date=%s \n is_deleted=%d\n"
				,ntohl(td.train_no)
				,td.train_name
				,td.boarding_point
				,td.destination_point
				,ntohl(td.total_seats)
				,ntohl(td.booked_seats)
				,ntohl(td.ticket_price)
				,td.date
				,td.is_deleted);
	}
	else
		printf("Train not found.\n");

	printf("Press any key to show menu, Don't press ENTER\n");
	while(getchar()=='\n');
}

void admin_menu(int client_fd,struct account_details *details)
{
	int choice,choice_htonl;
	while(TRUE)
	{
		system("clear");
		printf("===================================================\n");
		printf("****************Admin menu****************\n");
		printf("===================================================\n");
		printf("Enter your choice\n");
		printf("1) Add Account\n");
		printf("2) Delete Account\n");
		printf("3) Modify Account Password\n");
		printf("4) Search Account\n");
		printf("5) Add Train\n");
		printf("6) Delete Train\n");
		printf("7) Modify Ticket Price Of Train\n");
		printf("8) Search Train\n");
		printf("9 and other) Exit\n");
		scanf("%d",&choice);
		choice_htonl=htonl(choice);
		if(write(client_fd,&choice_htonl,sizeof(choice_htonl))==-1)
			perror_exit("write()");

		switch(choice)
		{
		case 1:
			Sign_up(client_fd);
			break;
		case 2:
			delete_account(client_fd,details);
			break;
		case 3:
			modify_account_password(client_fd,details);
			break;
		case 4:
			search_account(client_fd,details);
			break;
		case 5:
			add_train(client_fd,details);
			break;
		case 6:
			delete_train(client_fd,details);
			break;
		case 7:
			modify_ticket_price(client_fd,details);
			break;
		case 8:
			search_train(client_fd,details);
			break;
		default:
			if(close(client_fd)==-1)
				perror_exit("close()");
			exit(EXIT_SUCCESS);
		}
	}
}
