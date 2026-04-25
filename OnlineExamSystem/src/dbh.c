#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include "dbh.h"

MYSQL* mysql_connection_setup(struct connection_details mysql_details)
{
    MYSQL* connection = mysql_init(NULL);

    if (!mysql_real_connect(connection, mysql_details.server, mysql_details.user, mysql_details.password, mysql_details.database, 0, NULL, 0))
    {
        printf("Connection Error: %s\n", mysql_error(connection));
        exit(1);
    }

    return connection;
}

MYSQL_RES* mysql_execute_query(MYSQL* connection, const char *sql_query)
{
    if (mysql_query(connection, sql_query))
    {
        printf("Connection Error: %s\n", mysql_error(connection));
        return NULL;
    }

    return mysql_use_result(connection);
}


/*
int main(int argc, char* const argv[])
{
    MYSQL* con;
    MYSQL_RES* res;
    MYSQL_ROW row;

    struct connection_details mysql_d;
    mysql_d.server = "localhost";
    mysql_d.user = "os_dev";
    mysql_d.password = "123";
    mysql_d.database = "online_exam_system";

    con = mysql_connection_setup(mysql_d);

    int status = atoi(argv[1]);
    char* name = argv[2];

    char input[100];
    snprintf(input, 100, "INSERT INTO Users(status, name) VALUES (%d, '%s')", status, name);
    res = mysql_execute_query(con, input);

    printf("QUERY OUTPUT\n");

    while((row = mysql_fetch_row(res)) != NULL)
            printf("%s | %s | %s | %s\n", row[0], row[1], row[2], row[3]);

    mysql_free_result(res);
    mysql_close(con);

    return 0;
}

*/
