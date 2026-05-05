#include <mysql/mysql.h>
#include "../headers/dbh.h"

struct ConnectionDetails connection_details =
{
    .server   = MYSQL_SERVER,
    .user     = MYSQL_USER,
    .password = MYSQL_PASSWD,
    .database = MYSQL_DB
};
