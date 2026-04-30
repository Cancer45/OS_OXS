#include <stdio.h>
#include <mysql/mysql.h>
#include "../headers/dbh.h"
#include <stdlib.h>

MYSQL* mysql_connection_setup(struct ConnectionDetails mysql_details)
{
    MYSQL *connection = mysql_init(NULL);

    if (!mysql_real_connect(connection, mysql_details.server, mysql_details.user, mysql_details.password, mysql_details.database, 0, NULL, 0))
    {
        printf("connection error: %s\n", mysql_error(connection));
        exit(1);
    }

    return connection;
}

MYSQL_RES* mysql_execute_query(MYSQL *connection, const char *sql_query)
{
    if (mysql_query(connection, sql_query))
    {
        printf("MYSQL Query Error: %s\n", mysql_error(connection));
        exit(1);
    }

    return mysql_use_result(connection);
}

