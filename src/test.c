#include <stdio.h>
#include <string.h>
#include "../headers/dbh.h"
#include "../headers/structs.h"
#include "../headers/interdb.h"

int main()
{
    struct ConnectionDetails connection_details = 
    {
        .server = mysql_server,
        .user = mysql_user,
        .password = mysql_password,
        .database = mysql_database
    };
    MYSQL* connection = mysql_connection_setup(connection_details);

    struct User user = {0};
    strcpy(user.username, "lobaloba");

    struct DbIODetails args = {connection, &user};
    printf("sending username: %s\n", user.username);
    getUser(&args);
    printf("USER DETAILS\n%d\n%s\n%d\n%s\n%s\n",
            user.user_id,
            user.username,
            user.status,
            user.name,
            user.registration_date);
}
