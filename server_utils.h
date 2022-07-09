#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "struct.h"

#define SUPER_ADMIN_USERNAME "ADMIN"
#define SUPER_ADMIN_PASSWORD "ADMIN"

#define NORMAL_ACCOUNT_FILE "./database/NORMAL_ACCOUNT.dat"
#define AGENT_ACCOUNT_FILE "./database/AGENT_ACCOUNT.dat"
#define ADMINISTRATOR_ACCOUNT_FILE "./database/ADMINISTRATOR_ACCOUNT.dat"
#define TRAIN_FILE "./database/TRAIN.dat"
#define BOOKED_TRAIN_FILE "./database/BOOKED_TRAIN.dat"

#define perror_exit(msg) {perror(msg); printf("Error at line=%d and file=%s\n",__LINE__,__FILE__); exit(EXIT_FAILURE);}

//simultaneous logins of normal and admin not possible return 2
//on success return 0
//for invalid login return -1
int authenticate(struct account_details *details)
{
	int fd,ret;
	int flags=O_RDWR|O_CREAT;
	int mode =S_IRUSR|S_IWUSR;
	struct account_details stored_details;
	off_t offset;

#ifdef debug
	printf("userid=%d acc_type=%d\n", details->userid,details->acc_type);
#endif

	if(details->acc_type==NORMAL_ACCOUNT)
	{
		if((fd=open(NORMAL_ACCOUNT_FILE,flags,mode))==-1)
			perror_exit("open()");

		if((offset=lseek(fd,details->userid * sizeof(struct account_details),SEEK_SET))==(off_t) -1)
			perror_exit("lseek()");

#ifdef debug
		printf("offset=%ld lock_offset=%ld sizeof_account_details=%ld sizeof_stored_details=%ld\n ",offset,details->userid*sizeof(stored_details),sizeof(struct account_details),sizeof(stored_details));
#endif

		struct flock fl;
		fl.l_type=F_WRLCK;//so that multiple logins can be prevented
		fl.l_whence=SEEK_SET;
		fl.l_start=details->userid*sizeof(stored_details);
		fl.l_len=sizeof(stored_details);
		fl.l_pid=getpid();

		if(fcntl(fd,F_SETLK,&fl)==-1)
			return 2;

		if((ret=read(fd,&stored_details,sizeof(stored_details)))==-1)
			perror_exit("read()");

#ifdef debug
		printf("stored_userid=%d userid=%d stored_acc_type=%d ret_from_read=%d stored_password=%s password=%s\n", stored_details.userid,details->userid, stored_details.acc_type,ret,details->password,stored_details.password);
#endif

		if(ret==0||stored_details.is_deleted) //offest is at or beyond EOF
			return -1;

		if(details->userid==stored_details.userid)
		{
			if(strcmp(details->password,stored_details.password)==0)
			{
#ifdef debug
				printf("password matched! returning 0.\n");
#endif
				return 0;
			}
		}
	}
	else if(details->acc_type==AGENT_ACCOUNT)
	{
		if((fd=open(AGENT_ACCOUNT_FILE,flags,mode))==-1)
			perror_exit("open()");

		if((offset=lseek(fd,details->userid * sizeof(struct account_details),SEEK_SET))==(off_t) -1)
			perror_exit("lseek()");

#ifdef debug
		printf("offset=%ld\n",offset);
#endif
		/*write lock is not needed since multiple simultaneous logins are possible*/

		if((ret=read(fd,&stored_details,sizeof(stored_details)))==-1)
			perror_exit("read()");

#ifdef debug
		printf("stored_userid=%d stored_acc_type=%d ret_from_read=%d\n", stored_details.userid,stored_details.acc_type,ret);
#endif

		if(ret==0||stored_details.is_deleted) //offest is at or beyond EOF
			return -1;

		if(details->userid==stored_details.userid)
		{
			if(strcmp(details->password,stored_details.password)==0)
			{
				if(close(fd)==-1)
					perror_exit("close()");
				return 0;
			}
		}
	}
	else if(details->acc_type==ADMINISTRATOR_ACCOUNT)
	{
		if((fd=open(ADMINISTRATOR_ACCOUNT_FILE,flags,mode))==-1)
			perror_exit("open()");

		if((offset=lseek(fd,details->userid * sizeof(struct account_details),SEEK_SET))==(off_t) -1)
			perror_exit("lseek()");

#ifdef debug
		printf("offset=%ld\n",offset);
#endif

		struct flock fl;
		fl.l_type=F_WRLCK;
		fl.l_whence=SEEK_SET;
		fl.l_start=details->userid*sizeof(stored_details);
		fl.l_len=sizeof(stored_details);
		fl.l_pid=getpid();

		if(fcntl(fd,F_SETLK,&fl)==-1)
			return 2;

		if((ret=read(fd,&stored_details,sizeof(stored_details)))==-1)
			perror_exit("read()");

#ifdef debug
		printf("stored_userid=%d stored_acc_type=%d ret_from_read=%d\n", stored_details.userid,stored_details.acc_type,ret);
#endif

		if(ret==0||stored_details.is_deleted) //offest is at or beyond EOF
			return -1;

		if(details->userid==stored_details.userid)
		{
			if(strcmp(details->password,stored_details.password)==0)
			{
#ifdef debug
				printf("password matched! returning 0.\n");
#endif
				return 0;
			}
		}
	}
	return -1;
}

int create_account(struct account_details *details,int type)
{
	int fd;
	int flags = O_RDWR | O_CREAT;
	int mode = S_IRUSR | S_IWUSR; // 0600
	if(type==NORMAL_ACCOUNT)
	{
		if((fd = open(NORMAL_ACCOUNT_FILE, flags, mode))==-1)
			perror_exit("open()");
	}
	else if(type==AGENT_ACCOUNT)
	{
		if((fd = open(AGENT_ACCOUNT_FILE, flags, mode))==-1)
			perror_exit("open()");
	}
	else if(type==ADMINISTRATOR_ACCOUNT)//ADMINISTRATOR_ACCOUNT
	{
		if((fd = open(ADMINISTRATOR_ACCOUNT_FILE, flags, mode))==-1)
			perror_exit("open()");
	}
	else
		return -1;

	off_t offset;
	if((offset=lseek(fd,0,SEEK_END))==-1)
		perror_exit("lseek()");

#ifdef debug
	printf("offset=%ld\n",offset);
#endif

	if(offset==0)//file is empty
	{
		details->userid=0;
		if(write(fd,details,sizeof(*details))==-1)
			perror_exit("write()");
	}
	else//new userid=previous userid+1
	{
		struct account_details previous_record;
		if(lseek(fd,-sizeof(struct account_details),SEEK_END)==-1)
			perror_exit("lseek()");
		if(read(fd,&previous_record,sizeof(struct account_details))==-1)
			perror_exit("read()");
		details->userid=previous_record.userid+1;
		if(lseek(fd,0,SEEK_END)==-1)
			perror_exit("lseek()");
		if(write(fd,details,sizeof(struct account_details))==-1)
			perror_exit("write()");
	}
	if(close(fd)==-1)
		perror_exit("close()");
	return details->userid;
}
void Book_Ticket(int conn_fd,struct account_details *details)
{
	off_t offset;
	int fd,fd1,ret,train_no,status,tickets,count=0,found=FALSE;
	int flags = O_RDWR|O_CREAT;
	int mode = S_IRUSR | S_IWUSR; // 0600
	struct train_data_node *head=NULL,*end=NULL,*temp;
	struct train_data_node *new_node=calloc(1,sizeof(struct train_data_node)); new_node->next=NULL;

	if((fd = open(TRAIN_FILE, flags ,mode)) == -1)
		perror_exit("open()");

	while((ret=read(fd,&new_node->data,sizeof(struct train_data))))
	{
		if(ret==-1)
			perror_exit("read()");

		if(new_node->data.is_deleted==false)
		{
			count++;
			if(!head)
				head=end=new_node;
			else
			{
				end->next=new_node;
				end=end->next;
			}
		}
		new_node=calloc(1,sizeof(struct booked_train_data_node));
		new_node->next=NULL;
	}

#ifdef debug
	printf("total train count=%d\n",count);
#endif

	int count_htons=htonl(count);
	if((write(conn_fd,&count_htons,sizeof(count_htons)))==-1)
		perror_exit("write()");

	temp=head;
	while(temp)
	{
		if((write(conn_fd,&temp->data,sizeof(temp->data)))==-1)
			perror_exit("write()");
		temp=temp->next;
	}

	while(head)
	{
		temp=head;
		head=head->next;
		free(temp);
	}

	if(read(conn_fd, &train_no, sizeof(int)) == -1)
		perror_exit("read()");
	train_no = ntohl(train_no);

	if(read(conn_fd, &tickets, sizeof(int)) == -1)
		perror_exit("read()");
	tickets = ntohl(tickets);

#ifdef debug
	printf("train number=%d tickets=%d\n",train_no,tickets);
#endif

	struct train_data td;
	struct booked_train_data btd,btd_temp;

	if((lseek(fd,train_no * sizeof(struct train_data),SEEK_SET))==(off_t) -1)
		perror_exit("lseek()");

	struct flock fl;
	fl.l_type=F_WRLCK;
	fl.l_whence=SEEK_SET;
	fl.l_start=train_no*sizeof(struct train_data);
	fl.l_len=sizeof(struct train_data);
	fl.l_pid=getpid();

	if(fcntl(fd,F_SETLK,&fl)==-1)
	{
		status=FAILURE;
	}
	else
	{
		if((ret=read(fd,&td,sizeof(td)))==-1)
			perror_exit("read()");

		if(ret==0) //offest is at or beyond EOF
			status=FAILURE;
		else
		{
			if(td.is_deleted==FALSE && td.train_no==train_no && td.booked_seats+tickets<=td.total_seats)
			{
				td.booked_seats+=tickets;
				status=SUCCESS;
				if((lseek(fd,-sizeof(struct train_data),SEEK_CUR))==(off_t) -1)
					perror_exit("Book_Ticket()-->lseek()");

				if((write(fd,&td,sizeof(td)))==-1)
					perror_exit("write()");

				btd.account_type = details->acc_type;
				btd.userid = details->userid;
				btd.train_no = td.train_no;
				strcpy(btd.train_name, td.train_name);
				strcpy(btd.boarding_point, td.boarding_point);
				strcpy(btd.destination_point, td.destination_point);
				btd.ticket_price = td.ticket_price*tickets;
				btd.seats_booked=tickets;
				strcpy(btd.date,td.date);
				btd.is_cancelled=false;

				if((fd1= open(BOOKED_TRAIN_FILE, flags ,mode)) == -1)
					perror_exit("Book_Ticket() -> open()");

				struct flock fl_booked;
				fl_booked.l_type=F_WRLCK;
				fl_booked.l_whence=SEEK_CUR;
				fl_booked.l_start=0;
				fl_booked.l_len=0;
				fl_booked.l_pid=getpid();

				if(fcntl(fd1,F_SETLK,&fl_booked)==-1)
				{
					status=FAILURE;
				}
				else
				{
					while((ret=read(fd1,&btd_temp,sizeof(btd_temp))))
					{
						if(ret==-1)
							perror_exit("read()");

						if(btd_temp.account_type==details->acc_type && btd_temp.userid==details->userid && btd_temp.train_no==train_no)
						{
							if(btd_temp.is_cancelled==false)
							{
								btd_temp.ticket_price=btd_temp.ticket_price/btd_temp.seats_booked*(btd_temp.seats_booked+tickets);
								btd_temp.seats_booked+=tickets;
							}
							else
							{
								btd_temp.is_cancelled=false;
								btd_temp.ticket_price=btd.ticket_price;
								btd_temp.seats_booked=btd.seats_booked;
							}

							if((offset=lseek(fd1,-sizeof(btd_temp),SEEK_CUR))==(off_t) -1)
								perror_exit("Book_Ticket()-->lseek()");

#ifdef debug
							printf("Found=TRUE...offset=%ld\n",offset);
#endif
							if((write(fd1,&btd_temp,sizeof(btd_temp)))==-1)
								perror_exit("write()");

							found=TRUE;
							break;
						}
					}
					if(found==FALSE)
					{
						if((offset=lseek(fd1,0,SEEK_END))==(off_t) -1)
							perror_exit("lseek()");
#ifdef debug
						printf("Found=FALSE...offset=%ld train_name=%s\n",offset,btd.train_name);
#endif

						if((write(fd1,&btd,sizeof(btd)))==-1)
							perror_exit("write()");
					}
				}
				fl_booked.l_type=F_UNLCK;
				if(fcntl(fd1,F_SETLK,&fl_booked)==-1)
				{
					perror_exit("fcntl()");
				}
				if(close(fd1)==-1)
					perror_exit("close()");
			}
			else
				status=FAILURE;
		}
	}
	fl.l_type=F_UNLCK;
	if(fcntl(fd,F_SETLK,&fl)==-1)
	{
		perror_exit("fcntl()");
	}

	status=htonl(status);
	if((write(conn_fd,&status,sizeof(status)))==-1)
		perror_exit("write()");

	if(close(fd)==-1)
		perror_exit("close()");
}
void View_Previous_Bookings(int conn_fd,struct account_details *details)
{
	int flags = O_RDWR|O_CREAT;
	int mode = S_IRUSR | S_IWUSR; // 0600
	struct booked_train_data_node *head=NULL,*end=NULL,*temp;
	struct booked_train_data_node *new_node=calloc(1,sizeof(struct booked_train_data_node)); new_node->next=NULL;

	int count=0,fd1,ret;
	if((fd1= open(BOOKED_TRAIN_FILE, flags,mode)) == -1)
		perror_exit("open()");

	struct flock fl_booked;
	fl_booked.l_type=F_RDLCK;
	fl_booked.l_whence=SEEK_CUR;
	fl_booked.l_start=0;
	fl_booked.l_len=0;
	fl_booked.l_pid=getpid();

	if(fcntl(fd1,F_SETLK,&fl_booked)==-1)
	{
		perror_exit("fcntl()");
	}
	else
	{
		while((ret=read(fd1,&new_node->data,sizeof(struct booked_train_data))))
		{
			if(ret==-1)
				perror_exit("read()");

			if((new_node->data.is_cancelled==false) && (new_node->data.account_type==details->acc_type)
					&& (new_node->data.userid==details->userid))
			{
				count++;
				if(!head)
					head=end=new_node;
				else
				{
					end->next=new_node;
					end=end->next;
				}
				new_node=calloc(1,sizeof(struct booked_train_data_node));
				new_node->next=NULL;
			}
		}
	}
	fl_booked.l_type=F_UNLCK;
	if(fcntl(fd1,F_SETLK,&fl_booked)==-1)
	{
		perror_exit("fcntl()");
	}

#ifdef debug
	printf("total booking count found=%d\n",count);
#endif

	int count_htons=htonl(count);
	if((write(conn_fd,&count_htons,sizeof(count_htons)))==-1)
		perror_exit("write()");

	temp=head;
	while(temp)
	{
#ifdef debug
		printf("train name=%s\n",temp->data.train_name);
#endif
		if((write(conn_fd,&temp->data,sizeof(temp->data)))==-1)
			perror_exit("write()");
		temp=temp->next;
	}

	while(head)
	{
		temp=head;
		head=head->next;
		free(temp);
	}
	if(close(fd1)==-1)
		perror_exit("close()");
}
void Update_Booking(int conn_fd,struct account_details *details)
{
	int flags = O_RDWR|O_CREAT;
	int mode = S_IRUSR | S_IWUSR; // 0600
	int train_no,status=FAILURE,tickets;

	if(read(conn_fd, &train_no, sizeof(int)) == -1)
		perror_exit("read()");
	train_no = ntohl(train_no);

	if(read(conn_fd, &tickets, sizeof(int)) == -1)
		perror_exit("read()");
	tickets = ntohl(tickets);

	int fd1,ret;
	if((fd1= open(BOOKED_TRAIN_FILE, flags,mode)) == -1)
		perror_exit("open()");

	struct booked_train_data booking_data;
	struct flock fl_booked;
	fl_booked.l_type=F_WRLCK;
	fl_booked.l_whence=SEEK_CUR;
	fl_booked.l_start=0;
	fl_booked.l_len=0;
	fl_booked.l_pid=getpid();

	if(fcntl(fd1,F_SETLK,&fl_booked)==-1)
	{
		perror_exit("fcntl()");
	}
	else
	{
		while((ret=read(fd1,&booking_data,sizeof(struct booked_train_data))))
		{
			if(ret==-1)
				perror_exit("read()");
			if((booking_data.is_cancelled==false) && (booking_data.account_type=details->acc_type)
					&& (booking_data.userid==details->userid) && (booking_data.train_no==train_no))
			{
				int fd;
				if((fd= open(TRAIN_FILE, flags ,mode)) == -1)
					perror_exit("open()");

				if((lseek(fd,train_no * sizeof(struct train_data),SEEK_CUR))==(off_t) -1)
					perror_exit("lseek()");

				struct flock fl;
				struct train_data td;
				fl.l_type=F_WRLCK;
				fl.l_whence=SEEK_SET;
				fl.l_start=train_no*sizeof(struct train_data);
				fl.l_len=sizeof(struct train_data);
				fl.l_pid=getpid();

				if(fcntl(fd,F_SETLK,&fl)==-1)
				{
					perror_exit("fcntl()");
				}

				if(read(fd,&td,sizeof(td))==-1)
					perror_exit("read()");

				if(tickets>booking_data.seats_booked)
				{
					if(td.booked_seats+(tickets-booking_data.seats_booked)<=td.total_seats)
						status=SUCCESS;
					else
						status=FAILURE;
				}
				else
					status=SUCCESS;
				td.booked_seats+=(tickets-booking_data.seats_booked);

				if(status==SUCCESS)
				{
					booking_data.ticket_price=booking_data.ticket_price/booking_data.seats_booked*tickets;
					booking_data.seats_booked=tickets;
					if((lseek(fd1,-sizeof(struct booked_train_data),SEEK_CUR))==(off_t) -1)
						perror_exit("lseek()");

					if(write(fd1,&booking_data,sizeof(booking_data))==-1)
						perror_exit("write()");

					if((lseek(fd,-sizeof(struct train_data),SEEK_CUR))==(off_t) -1)
						perror_exit("lseek()");

					if(write(fd,&td,sizeof(td))==-1)
						perror_exit("write()");
				}
				fl.l_type=F_UNLCK;
				if(fcntl(fd,F_SETLK,&fl)==-1)
					perror_exit("fcntl()");

				if(close(fd)==-1)
					perror_exit("close()");
				break;
			}
		}
	}
	fl_booked.l_type=F_UNLCK;
	if(fcntl(fd1,F_SETLK,&fl_booked)==-1)
		perror_exit("fcntl()");

	status=htonl(status);
	if((write(conn_fd,&status,sizeof(status)))==-1)
		perror_exit("write()");

	if(close(fd1)==-1)
		perror_exit("close()");
}
void Cancel_Booking(int conn_fd,struct account_details *details)
{
	int flags = O_RDWR|O_CREAT;
	int mode = S_IRUSR | S_IWUSR; // 0600
	int train_no,status=FAILURE;

	if(read(conn_fd, &train_no, sizeof(int)) == -1)
		perror_exit("read()");
	train_no = ntohl(train_no);

	int fd1,ret;
	if((fd1= open(BOOKED_TRAIN_FILE, flags ,mode)) == -1)
		perror_exit("open()");

	struct booked_train_data booking_data;
	struct flock fl_booked;
	fl_booked.l_type=F_WRLCK;
	fl_booked.l_whence=SEEK_CUR;
	fl_booked.l_start=0;
	fl_booked.l_len=0;
	fl_booked.l_pid=getpid();

	if(fcntl(fd1,F_SETLK,&fl_booked)==-1)
	{
		perror_exit("fcntl()");
	}
	else
	{
		while((ret=read(fd1,&booking_data,sizeof(struct booked_train_data))))
		{
			if(ret==-1)
				perror_exit("read()");
			if((booking_data.is_cancelled==false) && (booking_data.account_type=details->acc_type)
					&& (booking_data.userid==details->userid) && (booking_data.train_no==train_no))
			{
				status=SUCCESS;
				booking_data.is_cancelled=true;

				if(lseek(fd1,-sizeof(struct booked_train_data),SEEK_CUR)==-1)
					perror_exit("lseek()");

				if(write(fd1,&booking_data,sizeof(booking_data))==-1)
					perror_exit("write()");

				int fd;
				if((fd= open(TRAIN_FILE, flags,mode)) == -1)
					perror_exit("open()");

				if((lseek(fd,train_no * sizeof(struct train_data),SEEK_SET))==(off_t) -1)
					perror_exit("lseek()");

				struct flock fl;
				struct train_data td;
				fl.l_type=F_WRLCK;
				fl.l_whence=SEEK_SET;
				fl.l_start=train_no*sizeof(struct train_data);
				fl.l_len=sizeof(struct train_data);
				fl.l_pid=getpid();

				if(fcntl(fd,F_SETLK,&fl)==-1)
				{
					perror_exit("fcntl()");
				}

				if(read(fd,&td,sizeof(td))==-1)
					perror_exit("read()");

				td.booked_seats-=booking_data.seats_booked;

				if((lseek(fd,- sizeof(struct train_data),SEEK_CUR))==(off_t) -1)
					perror_exit("lseek()");

				if(write(fd,&td,sizeof(td))==-1)
					perror_exit("write()");

				fl.l_type=F_UNLCK;
				if(fcntl(fd,F_SETLK,&fl)==-1)
				{
					perror_exit("fcntl()");
				}
				if(close(fd)==-1)
					perror_exit("close()");
				break;
			}
		}
	}
	fl_booked.l_type=F_UNLCK;
	if(fcntl(fd1,F_SETLK,&fl_booked)==-1)
	{
		perror_exit("fcntl()");
	}

	status=htonl(status);
	if((write(conn_fd,&status,sizeof(status)))==-1)
		perror_exit("write()");

	if(close(fd1)==-1)
		perror_exit("close()");
}
void customer_menu(int conn_fd,struct account_details* details)
{
	int choice;
	while(TRUE)
	{
#ifdef debug
		printf("waiting for choice...\n");
#endif
		if(read(conn_fd,&choice,sizeof(choice))==-1)
			perror_exit("read()");
		choice=ntohl(choice);

#ifdef debug
		printf("customer choice=%d\n",choice);
#endif

		switch(choice)
		{
		case 1:
			Book_Ticket(conn_fd,details);
			break;
		case 2:
			View_Previous_Bookings(conn_fd,details);
			break;
		case 3:
			Update_Booking(conn_fd,details);
			break;
		case 4:
			Cancel_Booking(conn_fd,details);
			break;
		default:
			if(close(conn_fd)==-1)
				perror_exit("close()");
			exit(EXIT_SUCCESS);
		}
	}
}
void delete_account(int conn_fd,struct account_details* details)
{
	int fd,status=FAILURE,userid,account_type,ret;
	int flags = O_RDWR|O_CREAT;
	int mode = S_IRUSR | S_IWUSR; // 0600
	struct flock fl;
	struct account_details account_details;

	if(read(conn_fd,&userid,sizeof(userid))==-1)
		perror_exit("read()");
	userid=ntohl(userid);

	if(read(conn_fd,&account_type,sizeof(account_type))==-1)
		perror_exit("read()");
	account_type=ntohl(account_type);

	if(account_type==NORMAL_ACCOUNT)
	{
		if((fd= open(NORMAL_ACCOUNT_FILE, flags ,mode)) == -1)
			perror_exit("open()");

		if(lseek(fd,userid*sizeof(struct account_details),SEEK_SET)==-1)
			perror_exit("lseek()");

		fl.l_type=F_WRLCK;
		fl.l_whence=SEEK_SET;
		fl.l_start=userid*sizeof(struct account_details);
		fl.l_len=sizeof(struct account_details);
		fl.l_pid=getpid();

		if(fcntl(fd,F_SETLK,&fl)==-1)
			perror_exit("fcntl()");

		if((read(fd,&account_details,sizeof(account_details)))==-1)
			perror_exit("read()");

		account_details.is_deleted=true;

		if((lseek(fd,-sizeof(account_details),SEEK_CUR))==-1)
			perror_exit("lseek()");

		if(write(fd,&account_details,sizeof(account_details))==-1)
			perror_exit("write()");

		fl.l_type=F_UNLCK;
		if(fcntl(fd,F_SETLK,&fl)==-1)
		{
			perror_exit("fcntl()");
		}
		if(close(fd)==-1)
			perror_exit("close()");
		status=SUCCESS;
	}
	else if(account_type==AGENT_ACCOUNT)
	{
		if((fd= open(AGENT_ACCOUNT_FILE, flags, mode)) == -1)
			perror_exit("open()");

		if(lseek(fd,userid*sizeof(struct account_details),SEEK_SET)==-1)
			perror_exit("lseek()");

		fl.l_type=F_WRLCK;
		fl.l_whence=SEEK_SET;
		fl.l_start=userid*sizeof(struct account_details);
		fl.l_len=sizeof(struct account_details);
		fl.l_pid=getpid();

		if(fcntl(fd,F_SETLK,&fl)==-1)
			perror_exit("fcntl()");

		if((read(fd,&account_details,sizeof(account_details)))==-1)
			perror_exit("read()");

		account_details.is_deleted=true;

		if((lseek(fd,-sizeof(account_details),SEEK_CUR))==-1)
			perror_exit("lseek()");

		if(write(fd,&account_details,sizeof(account_details))==-1)
			perror_exit("write()");

		fl.l_type=F_UNLCK;
		if(fcntl(fd,F_SETLK,&fl)==-1)
		{
			perror_exit("fcntl()");
		}
		status=SUCCESS;
		if(close(fd)==-1)
			perror_exit("close()");
	}
	else if(account_type==ADMINISTRATOR_ACCOUNT)//ADMINISTRATOR_ACCOUNT
	{
		char super_admin_username[USERNAME_LEN];
		char supe_admin_password[PASSWORD_LEN];

		if(read(conn_fd,super_admin_username,sizeof(super_admin_username))==-1)
			perror_exit("read()");
		if(read(conn_fd,supe_admin_password,sizeof(supe_admin_password))==-1)
			perror_exit("read()");

		//only super admin can add new admins
		if(strcmp(super_admin_username,SUPER_ADMIN_USERNAME)==0 && strcmp(supe_admin_password,SUPER_ADMIN_PASSWORD)==0)
		{
			if((fd= open(ADMINISTRATOR_ACCOUNT_FILE, flags, mode)) == -1)
				perror_exit("open()");

			if(lseek(fd,userid*sizeof(struct account_details),SEEK_SET)==-1)
				perror_exit("lseek()");

			fl.l_type=F_WRLCK;
			fl.l_whence=SEEK_SET;
			fl.l_start=userid*sizeof(struct account_details);
			fl.l_len=sizeof(struct account_details);
			fl.l_pid=getpid();

			if(fcntl(fd,F_SETLK,&fl)==-1)
				perror_exit("fcntl()");

			if((read(fd,&account_details,sizeof(account_details)))==-1)
				perror_exit("read()");

			account_details.is_deleted=true;

			if((lseek(fd,-sizeof(account_details),SEEK_CUR))==-1)
				perror_exit("lseek()");

			if(write(fd,&account_details,sizeof(account_details))==-1)
				perror_exit("write()");

			fl.l_type=F_UNLCK;
			if(fcntl(fd,F_SETLK,&fl)==-1)
			{
				perror_exit("fcntl()");
			}
			if(close(fd)==-1)
				perror_exit("close()");
			status=SUCCESS;
		}
		else
			status=FAILURE;
	}
	else
		status=FAILURE;

	struct booked_train_data btd;
	if((fd=open(BOOKED_TRAIN_FILE,flags,mode))==-1)
		perror_exit("open()");

	struct flock fl_booked;
	fl_booked.l_type=F_WRLCK;
	fl_booked.l_whence=SEEK_SET;
	fl_booked.l_start=0;
	fl_booked.l_len=0;
	fl_booked.l_pid=getpid();

	if(fcntl(fd,F_SETLK,&fl_booked)==-1)
		perror_exit("fcntl()");

	while((ret=read(fd,&btd,sizeof(btd))))
	{
		if(ret==-1)
			perror_exit("read()");

		if(btd.userid==userid && btd.account_type==account_type)
		{
			btd.is_cancelled=true;

			if(lseek(fd,-sizeof(btd),SEEK_CUR)==-1)
				perror_exit("lseek()");

			if((write(fd,&btd,sizeof(btd)))==-1)
				perror_exit("write()");
		}
	}

	fl_booked.l_type=F_UNLCK;
	if(fcntl(fd,F_SETLK,&fl_booked)==-1)
		perror_exit("fcntl()");

	status=htonl(status);
	if(write(conn_fd,&status,sizeof(status))==-1)
		perror_exit("write()");
}

void modify_password_of_account(int conn_fd,struct account_details *details)
{
	int fd,status=FAILURE,userid,account_type;
	int flags = O_RDWR|O_CREAT;
	int mode = S_IRUSR | S_IWUSR; // 0600
	struct flock fl;
	struct account_details account_details;
	char new_password[PASSWORD_LEN];

	if(read(conn_fd,&userid,sizeof(userid))==-1)
		perror_exit("read()");
	userid=ntohl(userid);

	if(read(conn_fd,&account_type,sizeof(account_type))==-1)
		perror_exit("read()");
	account_type=ntohl(account_type);

	if(read(conn_fd,new_password,sizeof(new_password))==-1)
		perror_exit("read()");

	if(account_type==NORMAL_ACCOUNT)
	{
		if((fd= open(NORMAL_ACCOUNT_FILE, flags, mode)) == -1)
			perror_exit("open()");

		if(lseek(fd,userid*sizeof(struct account_details),SEEK_SET)==-1)
			perror_exit("lseek()");

		fl.l_type=F_WRLCK;
		fl.l_whence=SEEK_SET;
		fl.l_start=userid*sizeof(struct account_details);
		fl.l_len=sizeof(struct account_details);
		fl.l_pid=getpid();

		if(fcntl(fd,F_SETLK,&fl)==-1)
			perror_exit("fcntl()");

		if((read(fd,&account_details,sizeof(account_details)))==-1)
			perror_exit("read()");

		strcpy(account_details.password,new_password);

		if((lseek(fd,-sizeof(account_details),SEEK_CUR))==-1)
			perror_exit("lseek()");

		if(write(fd,&account_details,sizeof(account_details))==-1)
			perror_exit("write()");

		fl.l_type=F_UNLCK;
		if(fcntl(fd,F_SETLK,&fl)==-1)
			perror_exit("fcntl()");

		if(close(fd)==-1)
			perror_exit("close()");
		status=SUCCESS;
	}
	else if(account_type==AGENT_ACCOUNT)
	{
		if((fd= open(AGENT_ACCOUNT_FILE, flags, mode)) == -1)
			perror_exit("open()");

		if(lseek(fd,userid*sizeof(struct account_details),SEEK_SET)==-1)
			perror_exit("lseek()");

		fl.l_type=F_WRLCK;
		fl.l_whence=SEEK_SET;
		fl.l_start=userid*sizeof(struct account_details);
		fl.l_len=sizeof(struct account_details);
		fl.l_pid=getpid();

		if(fcntl(fd,F_SETLK,&fl)==-1)
			perror_exit("fcntl()");

		if((read(fd,&account_details,sizeof(account_details)))==-1)
			perror_exit("read()");

		strcpy(account_details.password,new_password);

		if((lseek(fd,-sizeof(account_details),SEEK_CUR))==-1)
			perror_exit("lseek()");

		if(write(fd,&account_details,sizeof(account_details))==-1)
			perror_exit("write()");

		fl.l_type=F_UNLCK;
		if(fcntl(fd,F_SETLK,&fl)==-1)
			perror_exit("fcntl()");

		status=SUCCESS;
		if(close(fd)==-1)
			perror_exit("close()");
	}
	else if(account_type==ADMINISTRATOR_ACCOUNT)//ADMINISTRATOR_ACCOUNT
	{
		char super_admin_username[USERNAME_LEN];
		char supe_admin_password[PASSWORD_LEN];

		if(read(conn_fd,super_admin_username,sizeof(super_admin_username))==-1)
			perror_exit("read()");
		if(read(conn_fd,supe_admin_password,sizeof(supe_admin_password))==-1)
			perror_exit("read()");

		//only super admin can add new admins
		if(strcmp(super_admin_username,SUPER_ADMIN_USERNAME)==0 && strcmp(supe_admin_password,SUPER_ADMIN_PASSWORD)==0)
		{
			if((fd= open(ADMINISTRATOR_ACCOUNT_FILE, flags, mode)) == -1)
				perror_exit("open()");

			if(lseek(fd,userid*sizeof(struct account_details),SEEK_SET)==-1)
				perror_exit("lseek()");

			fl.l_type=F_WRLCK;
			fl.l_whence=SEEK_SET;
			fl.l_start=userid*sizeof(struct account_details);
			fl.l_len=sizeof(struct account_details);
			fl.l_pid=getpid();

			if(fcntl(fd,F_SETLK,&fl)==-1)
				perror_exit("fcntl()");

			if((read(fd,&account_details,sizeof(account_details)))==-1)
				perror_exit("read()");

			strcpy(account_details.password,new_password);

			if((lseek(fd,-sizeof(account_details),SEEK_CUR))==-1)
				perror_exit("lseek()");

			if(write(fd,&account_details,sizeof(account_details))==-1)
				perror_exit("write()");

			fl.l_type=F_UNLCK;
			if(fcntl(fd,F_SETLK,&fl)==-1)
				perror_exit("fcntl()");
			if(close(fd)==-1)
				perror_exit("close()");
			status=SUCCESS;
		}
		else
			status=FAILURE;
	}
	else
		status=FAILURE;

	status=htonl(status);
	if(write(conn_fd,&status,sizeof(status))==-1)
		perror_exit("write()");
}

void search_account(int conn_fd, struct account_details *details)
{
	int fd,status=FAILURE,userid,account_type,ret;
	int flags = O_RDWR|O_CREAT;
	int mode = S_IRUSR | S_IWUSR; // 0600
	struct flock fl;
	struct account_details account_details;

	if(read(conn_fd,&userid,sizeof(userid))==-1)
		perror_exit("read()");
	userid=ntohl(userid);

	if(read(conn_fd,&account_type,sizeof(account_type))==-1)
		perror_exit("read()");
	account_type=ntohl(account_type);

	if(account_type==NORMAL_ACCOUNT)
	{
		if((fd= open(NORMAL_ACCOUNT_FILE, flags, mode)) == -1)
			perror_exit("open()");

		if(lseek(fd,userid*sizeof(struct account_details),SEEK_SET)==-1)
			perror_exit("lseek()");

		fl.l_type=F_RDLCK;
		fl.l_whence=SEEK_SET;
		fl.l_start=userid*sizeof(struct account_details);
		fl.l_len=sizeof(struct account_details);
		fl.l_pid=getpid();

		if(fcntl(fd,F_SETLK,&fl)==-1)
			perror_exit("fcntl()");

		if((ret=read(fd,&account_details,sizeof(account_details)))==-1)
			perror_exit("read()");

		if(ret==0)
			status=FAILURE;
		else if(userid==account_details.userid)
			status=SUCCESS;

		fl.l_type=F_UNLCK;
		if(fcntl(fd,F_SETLK,&fl)==-1)
			perror_exit("fcntl()");
		if(close(fd)==-1)
			perror_exit("close()");
	}
	else if(account_type==AGENT_ACCOUNT)
	{
		if((fd= open(AGENT_ACCOUNT_FILE, flags, mode)) == -1)
			perror_exit("open()");

		if(lseek(fd,userid*sizeof(struct account_details),SEEK_SET)==-1)
			perror_exit("lseek()");

		fl.l_type=F_RDLCK;
		fl.l_whence=SEEK_SET;
		fl.l_start=userid*sizeof(struct account_details);
		fl.l_len=sizeof(struct account_details);
		fl.l_pid=getpid();

		if(fcntl(fd,F_SETLK,&fl)==-1)
			perror_exit("fcntl()");

		if((ret=read(fd,&account_details,sizeof(account_details)))==-1)
			perror_exit("read()");

		if(ret==0)
			status=FAILURE;
		else if(userid==account_details.userid)
			status=SUCCESS;

		fl.l_type=F_UNLCK;
		if(fcntl(fd,F_SETLK,&fl)==-1)
			perror_exit("fcntl()");
		if(close(fd)==-1)
			perror_exit("close()");
	}
	else if(account_type==ADMINISTRATOR_ACCOUNT)//ADMINISTRATOR_ACCOUNT
	{
		if((fd= open(ADMINISTRATOR_ACCOUNT_FILE, flags, mode)) == -1)
			perror_exit("open()");

		if(lseek(fd,userid*sizeof(struct account_details),SEEK_SET)==-1)
			perror_exit("lseek()");

		fl.l_type=F_WRLCK;
		fl.l_whence=SEEK_SET;
		fl.l_start=userid*sizeof(struct account_details);
		fl.l_len=sizeof(struct account_details);
		fl.l_pid=getpid();

		if(fcntl(fd,F_SETLK,&fl)==-1)
			perror_exit("fcntl()");

		if((ret=read(fd,&account_details,sizeof(account_details)))==-1)
			perror_exit("read()");

		if(ret==0)
			status=FAILURE;
		else if(userid==account_details.userid)
			status=SUCCESS;

		fl.l_type=F_UNLCK;
		if(fcntl(fd,F_SETLK,&fl)==-1)
			perror_exit("fcntl()");
		if(close(fd)==-1)
			perror_exit("close(");
	}
	else
		status=FAILURE;

	status=htonl(status);
	if(write(conn_fd,&status,sizeof(status))==-1)
		perror_exit("write()");

	if(ntohl(status)==SUCCESS)
	{
		account_details.acc_type=htonl(account_details.acc_type);
		account_details.userid=htonl(account_details.userid);
		if(write(conn_fd,&account_details,sizeof(account_details))==-1)
			perror_exit("write()");
	}
}

void add_train(int conn_fd,struct account_details *details)
{
	int flags = O_RDWR|O_CREAT;
	int mode = S_IRUSR | S_IWUSR; // 0600
	struct train_data td,td_temp;
	if(read(conn_fd,&td,sizeof(td))==-1)
		perror_exit("read()");

	int fd;
	if((fd=open(TRAIN_FILE,flags,mode))==-1)
		perror_exit("open()");

	struct flock fl;
	fl.l_type=F_WRLCK;
	fl.l_whence=SEEK_SET;
	fl.l_start=0;
	fl.l_len=0;
	fl.l_pid=getpid();

	if(fcntl(fd,F_SETLK,&fl)==-1)
		perror_exit("fcntl()");

	off_t offset;
	if((offset=lseek(fd,0,SEEK_END))==-1)
		perror_exit("lseek()");

#ifdef debug
	printf("offset=%ld\n",offset);
#endif

	if(offset==0)//file is empty
		td.train_no=0;
	else
	{
		if((offset=lseek(fd,-sizeof(td),SEEK_CUR))==(off_t) -1)
			perror_exit("lseek()");

		if(read(fd,&td_temp,sizeof(td_temp))==-1)
			perror_exit("read()");

		td.train_no=td_temp.train_no+1;
	}

	td.booked_seats=ntohl(td.booked_seats);
	td.ticket_price=ntohl(td.ticket_price);
	td.total_seats=ntohl(td.total_seats);

	if(write(fd,&td,sizeof(td))==-1)
		perror_exit("write()");

	fl.l_type=F_UNLCK;
	if(fcntl(fd,F_SETLK,&fl)==-1)
		perror_exit("fcntl()");
	if(close(fd)==-1)
		perror_exit("close()");

	td.train_no=htonl(td.train_no);
	if(write(conn_fd,&td.train_no,sizeof(td.train_no))==-1)
		perror_exit("write()");
}

void delete_train(int conn_fd,struct account_details *details)
{
	int train_no,fd,ret,status=SUCCESS;
	int flags = O_RDWR|O_CREAT;
	int mode = S_IRUSR | S_IWUSR; // 0600
	if(read(conn_fd,&train_no,sizeof(train_no))==-1)
		perror_exit("read()");
	train_no=ntohl(train_no);

#ifdef debug
	printf("train number=%d\n",train_no);
#endif

	if((fd=open(TRAIN_FILE,flags,mode))==-1)
		perror_exit("open()");

	if(lseek(fd,train_no*sizeof(struct train_data),SEEK_SET)==(off_t) -1)
		perror_exit("lseek()");

	struct flock fl;
	fl.l_type=F_WRLCK;
	fl.l_whence=SEEK_SET;
	fl.l_start=train_no*sizeof(struct train_data);
	fl.l_len=sizeof(struct train_data);
	fl.l_pid=getpid();

	if(fcntl(fd,F_SETLK,&fl)==-1)
		perror_exit("fcntl()");

	struct train_data td;
	if((ret=read(fd,&td,sizeof(td)))==-1)
		perror_exit("read()");

	if(ret==0)
		status=FAILURE;
	else
	{
#ifdef debug
		printf("old is_deleted=%d\n",td.is_deleted);
#endif

		if(td.train_no==train_no)
		{
			td.is_deleted=true;
			status=SUCCESS;
		}
		else
			status=FAILURE;

#ifdef debug
		printf("new is_deleted=%d status=%d\n",td.is_deleted,status);
#endif

		if(lseek(fd,-sizeof(struct train_data),SEEK_CUR)==(off_t) -1)
			perror_exit("lseek()");

		if(write(fd,&td,sizeof(td))==-1)
			perror_exit("write()");

		fl.l_type=F_UNLCK;
		if(fcntl(fd,F_SETLK,&fl)==-1)
			perror_exit("fcntl()");
		if(close(fd)==-1)
			perror_exit("close()");

		struct booked_train_data btd;
		if((fd=open(BOOKED_TRAIN_FILE,flags,mode))==-1)
			perror_exit("open()");

		struct flock fl_booked;
		fl_booked.l_type=F_WRLCK;
		fl_booked.l_whence=SEEK_SET;
		fl_booked.l_start=0;
		fl_booked.l_len=0;
		fl_booked.l_pid=getpid();

		if(fcntl(fd,F_SETLK,&fl_booked)==-1)
			perror_exit("fcntl()");

		while((ret=read(fd,&btd,sizeof(btd))))
		{
			if(ret==-1)
				perror_exit("read()");

			if(btd.train_no==train_no)
			{
				btd.is_cancelled=true;

				if(lseek(fd,-sizeof(btd),SEEK_CUR)==-1)
					perror_exit("lseek()");

				if((write(fd,&btd,sizeof(btd)))==-1)
					perror_exit("write()");
			}
		}

		fl_booked.l_type=F_UNLCK;
		if(fcntl(fd,F_SETLK,&fl_booked)==-1)
			perror_exit("fcntl()");
	}

	status=htonl(status);
	if(write(conn_fd,&status,sizeof(status))==-1)
		perror_exit("write()");

	if(close(fd)==-1)
		perror_exit("close()");
}

void modify_ticket_price(int conn_fd, struct account_details* details)
{
	int train_no,new_price,fd,status=SUCCESS;
	int flags = O_RDWR|O_CREAT;
	int mode = S_IRUSR | S_IWUSR; // 0600
	if(read(conn_fd,&train_no,sizeof(train_no))==-1)
		perror_exit("read()");
	train_no=ntohl(train_no);

	if(read(conn_fd,&new_price,sizeof(new_price))==-1)
		perror_exit("read()");
	new_price=ntohl(new_price);

	if((fd=open(TRAIN_FILE,flags,mode))==-1)
		perror_exit("open()");

	if(lseek(fd,train_no*sizeof(struct train_data),SEEK_SET)==(off_t) -1)
		perror_exit("lseek()");

	struct flock fl;
	fl.l_type=F_WRLCK;
	fl.l_whence=SEEK_SET;
	fl.l_start=train_no*sizeof(struct train_data);
	fl.l_len=sizeof(struct train_data);
	fl.l_pid=getpid();

	if(fcntl(fd,F_SETLK,&fl)==-1)
		perror_exit("fcntl()");

	struct train_data td;
	if(read(fd,&td,sizeof(td))==-1)
		perror_exit("read()");

	if(td.train_no==train_no)
	{
		td.ticket_price=new_price;
		status=SUCCESS;
	}
	else
		status=FAILURE;

	if(lseek(fd,-sizeof(struct train_data),SEEK_CUR)==(off_t) -1)
		perror_exit("lseek()");

	if(write(fd,&td,sizeof(td))==-1)
		perror_exit("write()");

	fl.l_type=F_UNLCK;
	if(fcntl(fd,F_SETLK,&fl)==-1)
		perror_exit("fcntl()");
	if(close(fd)==-1)
		perror_exit("close()");

	status=htonl(status);
	if(write(conn_fd,&status,sizeof(status))==-1)
		perror_exit("write()");
}

void search_train(int conn_fd, struct account_details *details)
{
	int train_no,fd,status;
	int flags = O_RDWR|O_CREAT;
	int mode = S_IRUSR | S_IWUSR; // 0600
	if(read(conn_fd,&train_no,sizeof(train_no))==-1)
		perror_exit("read()");
	train_no=ntohl(train_no);

#ifdef debug
	printf("train number=%d\n",train_no);
#endif

	if((fd=open(TRAIN_FILE,flags,mode))==-1)
		perror_exit("open()");

	if(lseek(fd,train_no*sizeof(struct train_data),SEEK_SET)==(off_t) -1)
		perror_exit("lseek()");

	struct flock fl;
	fl.l_type=F_RDLCK;
	fl.l_whence=SEEK_SET;
	fl.l_start=train_no*sizeof(struct train_data);
	fl.l_len=sizeof(struct train_data);
	fl.l_pid=getpid();

	if(fcntl(fd,F_SETLK,&fl)==-1)
		perror_exit("fcntl()");

	struct train_data td;
	if(read(fd,&td,sizeof(td))==-1)
		perror_exit("read()");

	if(td.train_no==train_no)
		status=SUCCESS;
	else
		status=FAILURE;

#ifdef debug
	printf("status=%d\n",status);
#endif

	status=htonl(status);
	if(write(conn_fd,&status,sizeof(status))==-1)
		perror_exit("write()");

	if(ntohl(status)==SUCCESS)
	{
		td.train_no=htonl(td.train_no);
		td.total_seats=htonl(td.total_seats);
		td.booked_seats=htonl(td.booked_seats);
		td.ticket_price=htonl(td.ticket_price);
		if(write(conn_fd,&td,sizeof(td))==-1)
			perror_exit("write()");
	}

	fl.l_type=F_UNLCK;
	if(fcntl(fd,F_SETLK,&fl)==-1)
		perror_exit("fcntl()");
	if(close(fd)==-1)
		perror_exit("close()");
}

void admin_menu(int conn_fd,struct account_details* details)
{
	int choice;
	int userid;
	struct account_details acct_details;
	while(TRUE)
	{
#ifdef debug
		printf("waiting for choice...\n");
#endif
		if(read(conn_fd,&choice,sizeof(choice))==-1)
			perror_exit("read()");
		choice=ntohl(choice);

#ifdef debug
		printf("admin choice=%d\n",choice);
#endif

		switch(choice)
		{
		case 1:

			if(read(conn_fd, &acct_details, sizeof(acct_details)) == -1)
				perror_exit("read()");

			acct_details.acc_type=ntohl(acct_details.acc_type);

			if(acct_details.acc_type==ADMINISTRATOR_ACCOUNT)
			{
				char super_admin_username[USERNAME_LEN];
				char super_admin_password[PASSWORD_LEN];

				if(read(conn_fd,super_admin_username,sizeof(super_admin_username))==-1)
					perror_exit("read()");
				if(read(conn_fd,super_admin_password,sizeof(super_admin_password))==-1)
					perror_exit("read()");

				//only super admin can add new admins
				if(strcmp(super_admin_username,"ADMIN")==0 && strcmp(super_admin_password,"ADMIN")==0)
					userid=create_account(&acct_details,acct_details.acc_type);
				else
					userid=-1;
			}
			else
				userid=create_account(&acct_details,acct_details.acc_type);

			userid=htonl(userid);
			if(write(conn_fd,&userid,sizeof(userid))==-1)
				perror_exit("write()");

			break;
		case 2:
			delete_account(conn_fd,details);
			break;
		case 3:
			modify_password_of_account(conn_fd,details);//change password of any account
			break;
		case 4:
			search_account(conn_fd,details);
			break;
		case 5:
			add_train(conn_fd,details);
			break;
		case 6:
			delete_train(conn_fd,details);
			break;
		case 7:
			modify_ticket_price(conn_fd,details);
			break;
		case 8:
			search_train(conn_fd,details);
			break;
		default:
			if(close(conn_fd)==-1)
				perror_exit("close()");
			exit(EXIT_SUCCESS);
		}
	}
}

void service_client(void *conn_fd_ptr)
{
	int choice,conn_fd=*(int*)conn_fd_ptr,userid;

	while(TRUE)
	{
		if(read(conn_fd,&choice,sizeof(choice))==-1)
			perror_exit("read");
		choice=ntohl(choice);

#ifdef debug
		printf("choice=%d\n",choice);
#endif

		if(choice==1)//Sign_in
		{
			struct account_details details;
			if(read(conn_fd, &details, sizeof(details)) == -1)
				perror_exit("read()");

			details.userid=ntohl(details.userid);
			details.acc_type=ntohl(details.acc_type);

#ifdef debug
			printf("before authenticate!\n");
#endif

			int login_response= authenticate(&details);
			int login_response_htonl=htonl(login_response);
#ifdef debug
			printf("before authenticate login response=%d\n",login_response);
#endif
			if(write(conn_fd,&login_response_htonl,sizeof(login_response_htonl))==-1)
				perror_exit("write()");

#ifdef debug
			printf("after write login response=%d\n",login_response);
#endif
			if(login_response==0)
			{
				if(details.acc_type==ADMINISTRATOR_ACCOUNT)
				{
#ifdef debug
					printf("successful admin login! acc_type=%d\n",details.acc_type);
#endif
					admin_menu(conn_fd,&details);
				}
				else
				{
#ifdef debug
					printf("successful client login! acc_type=%d\n",details.acc_type);
#endif
					customer_menu(conn_fd,&details);
				}
			}
		}
		else if(choice==2)//Sign up
		{
			struct account_details details;
			if(read(conn_fd, &details, sizeof(details)) == -1)
				perror_exit("read()");

			details.acc_type=ntohl(details.acc_type);
			details.userid=ntohl(details.userid);

			if(details.acc_type==ADMINISTRATOR_ACCOUNT)
			{
				char super_admin_username[USERNAME_LEN];
				char supe_admin_password[PASSWORD_LEN];

				if(read(conn_fd,super_admin_username,sizeof(super_admin_username))==-1)
					perror_exit("read()");
				if(read(conn_fd,supe_admin_password,sizeof(supe_admin_password))==-1)
					perror_exit("read()");

				//only super admin can add new admins
				if(strcmp(super_admin_username,"ADMIN")==0 && strcmp(supe_admin_password,"ADMIN")==0)
					userid=create_account(&details,details.acc_type);
				else
					userid=-1;
			}
			else
				userid=create_account(&details,details.acc_type);

			userid=htonl(userid);
			if(write(conn_fd,&userid,sizeof(userid))==-1)
				perror_exit("write()");
		}
		else
		{
			if(close(conn_fd)==-1)
				perror_exit("close()");
			exit(EXIT_SUCCESS);
		}
	}
}
