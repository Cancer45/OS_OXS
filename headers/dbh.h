#ifndef DBH_H
#define DBH_H
#include <mysql/mysql.h>

static const char *mysql_server = "localhost";
static const char *mysql_user = "os_dev";
static const char *mysql_password = "123";
static const char *mysql_database = "online_exam_system";

struct ConnectionDetails
{
    const char *server, *user, *password, *database;
};

MYSQL* mysql_connection_setup (struct ConnectionDetails);
MYSQL_RES* mysql_execute_query(MYSQL*, const char*);

#endif
