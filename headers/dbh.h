#ifndef DBH_H
#define DBH_H
#include <mysql/mysql.h>

#define MYSQL_SERVER "localhost"
#define MYSQL_USER   "os_dev"
#define MYSQL_PASSWD "123"
#define MYSQL_DB     "online_exam_system"

struct ConnectionDetails
{
    const char *server, *user, *password, *database;
};

MYSQL* mysql_connection_setup (struct ConnectionDetails);
MYSQL_RES* mysql_execute_query(MYSQL*, const char*);

#endif
