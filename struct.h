#include <stdbool.h>

#define TRUE    1
#define FALSE   0
#define SUCCESS 1
#define FAILURE 0
#define CONFIRM 1
#define CANCEL 	0

#define NORMAL_ACCOUNT          1
#define AGENT_ACCOUNT           2
#define ADMINISTRATOR_ACCOUNT   3
#define USERNAME_LEN 		20
#define PASSWORD_LEN 		20
#define TRAIN_NAME_LEN 		20
#define STATION_NAME_LEN 	20
#define DATE_LEN			10

#define debug 1
//#undef debug

struct account_details
{
	int acc_type;
	int userid;
	char username[USERNAME_LEN];
	char password[PASSWORD_LEN];
	bool is_deleted;
};

struct train_data
{
	int train_no;
	char train_name[TRAIN_NAME_LEN];
	char boarding_point[STATION_NAME_LEN];
	char destination_point[STATION_NAME_LEN];
	int total_seats;
	int booked_seats;
	int ticket_price;
	char date[DATE_LEN];
	bool is_deleted;
};

struct train_data_node
{
	struct train_data data;
	struct train_data_node *next;
};

struct booked_train_data
{
	int account_type;
	int userid;
	int train_no;
	char train_name[TRAIN_NAME_LEN];
	char boarding_point[STATION_NAME_LEN];
	char destination_point[STATION_NAME_LEN];
	int seats_booked;
	int ticket_price;
	char date[DATE_LEN];
	bool is_cancelled;
};

struct booked_train_data_node
{
	struct booked_train_data data;
	struct booked_train_data_node *next;
};
