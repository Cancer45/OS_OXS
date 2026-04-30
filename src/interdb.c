#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "../headers/dbh.h"
#include "../headers/structs.h"

void* setUser(void* args)
{
    //construct structs
    struct DbIODetails *io_details = (struct DbIODetails*) args;
    MYSQL *connection = (MYSQL*) io_details->connection;
    struct User *user = (struct User*) io_details->data;
    
    //format query string & execute query
    char query[500];
    snprintf(query, sizeof(query), 
    "INSERT INTO Users (status, name, registration_date) VALUES (%d, '%s', '%s')", 
    user->status, user->name, user->registration_date);
    //mysql_real_escape_string(connection, query, query, sizeof(query));
    mysql_execute_query(connection, query);

    return NULL;
}

void* getUser(void* args)
{
    //construct structs
    struct DbIODetails *io_details = (struct DbIODetails*) args;
    MYSQL *connection = (MYSQL*) io_details->connection;
    struct User *user = (struct User*) io_details->data;
    
    //format query string
    char query[500];
    snprintf(query, sizeof(query), 
    "SELECT * FROM Users WHERE username = '%s'", 
    user->username);
    //mysql_real_escape_string(connection, query, query, sizeof(query));

    //execute query & store result
    printf("executing query: %s\n", query);
    MYSQL_RES *result = mysql_execute_query(connection, query);
    MYSQL_ROW row = mysql_fetch_row(result);
 
    if (row == NULL) {
        printf("getUser: no user found for username '%s'\n", user->username);
        mysql_free_result(result);
        return NULL;
    }   

    //populate User struct
    char* end_ptr;
    user->user_id = strtol(row[0], &end_ptr,10);
    user->status = atoi(row[2]);
    strcpy(user->name, row[3]);
    strcpy(user->registration_date, row[4]);

    mysql_free_result(result);
    return NULL;
}
