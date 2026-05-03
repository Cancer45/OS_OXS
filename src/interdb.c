#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "../headers/structs.h"
#include "../headers/dbh.h"

void* setUser(struct DbIODetails *io_details)
{
    MYSQL       *connection = (MYSQL *)      io_details->connection;
    struct User *user       = (struct User *) io_details->data;

    char query[500];
    snprintf(query, sizeof(query),
        "INSERT INTO Users (status, name, registration_date) "
        "VALUES (%d, '%s', '%s')",
        user->status, user->name, user->registration_date);

    mysql_execute_query(connection, query);
    return NULL;
}

void* setExam(void* params)
{
    struct DbIODetails *io_details = (struct DbIODetails *) params;
    MYSQL       *connection = (MYSQL *)      io_details->connection;
    struct Exam *exam       = (struct Exam *) io_details->data;

    char query[500];
    snprintf(query, sizeof(query),
        "INSERT INTO Exams (user_id, title, creation_date) "
        "VALUES (%d, '%s', '%s')",
        exam->user_id, exam->title, exam->creation_date);

    mysql_execute_query(connection, query);
    return NULL;
}

void* setQuestions(void* params)
{
    struct DbIODetails *io_details = (struct DbIODetails *) params;
    MYSQL           *connection = (MYSQL *)          io_details->connection;
    struct Question *question   = (struct Question *) io_details->data;

    char query[500];
    snprintf(query, sizeof(query),
        "INSERT INTO Questions (exam_id, question_statement) "
        "VALUES (%d, '%s')",
        question->exam_id, question->question_statement);

    mysql_execute_query(connection, query);
    return NULL;
}

void* setOptions(void* params)
{
    struct DbIODetails *io_details = (struct DbIODetails *) params;
    MYSQL         *connection = (MYSQL *)        io_details->connection;
    struct Option *option     = (struct Option *) io_details->data;

    char query[500];
    snprintf(query, sizeof(query),
        "INSERT INTO Options (question_id, option_statement, is_correct) "
        "VALUES (%d, '%s', %d)",
        option->question_id, option->option_statement, option->is_correct);

    mysql_execute_query(connection, query);
    return NULL;
}

void* setExamSessions(void* params)
{
    struct DbIODetails  *io_details  = (struct DbIODetails *)  params;
    MYSQL               *connection  = (MYSQL *)               io_details->connection;
    struct ExamSession  *exam_session = (struct ExamSession *)  io_details->data;

    char query[500];
    snprintf(query, sizeof(query),
        "INSERT INTO ExamSessions "
        "(admin_id, exam_id, session_duration, session_status) "
        "VALUES (%d, %d, %d, %d)",
        exam_session->admin_id, exam_session->exam_id,
        exam_session->session_duration, exam_session->session_status);

    mysql_execute_query(connection, query);
    return NULL;
}

void* setSessionCandidates(void* params)
{
    struct DbIODetails      *io_details        = (struct DbIODetails *)      params;
    MYSQL                   *connection        = (MYSQL *)                   io_details->connection;
    struct SessionCandidate *session_candidate = (struct SessionCandidate *) io_details->data;

    char query[500];
    snprintf(query, sizeof(query),
        "INSERT INTO SessionCandidates (session_id, user_id) "
        "VALUES (%d, %d)",
        session_candidate->session_id, session_candidate->user_id);

    mysql_execute_query(connection, query);
    return NULL;
}

void* setSelected(void* params)
{
    struct DbIODetails *io_details = (struct DbIODetails *) params;
    MYSQL           *connection = (MYSQL *)          io_details->connection;
    struct Selected *selected   = (struct Selected *) io_details->data;

    char query[500];
    snprintf(query, sizeof(query),
        "INSERT INTO Selected (option_id, session_candidate_id) "
        "VALUES (%d, %d)",
        selected->option_id, selected->session_candidate_id);

    mysql_execute_query(connection, query);
    return NULL;
}

void* getUser(void* params)
{
    struct DbIODetails *io_details = (struct DbIODetails *) params;
    MYSQL       *connection = (MYSQL *)      io_details->connection;
    struct User *user       = (struct User *) io_details->data;

    char query[500];
    snprintf(query, sizeof(query),
        "SELECT * FROM Users WHERE username = '%s'",
        user->username);

    printf("executing query: %s\n", query);

    MYSQL_RES *result = mysql_execute_query(connection, query);
    if (!result)
    {
        fprintf(stderr, "getUser: %s\n", mysql_error(connection));
        return NULL;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        return NULL;
    }

    char *end_ptr;
    user->user_id = (int) strtol(row[0], &end_ptr, 10);
    user->status  = atoi(row[2]);
    strncpy(user->name,              row[3], sizeof(user->name)              - 1);
    strncpy(user->registration_date, row[4], sizeof(user->registration_date) - 1);

    mysql_free_result(result);
    return NULL;
}

void* getExam(void* params)
{
    struct DbIODetails *io_details = (struct DbIODetails *) params;
    MYSQL       *connection = (MYSQL *)      io_details->connection;
    struct Exam *exam       = (struct Exam *) io_details->data;

    char query[500];
    snprintf(query, sizeof(query),
        "SELECT * FROM Exams WHERE exam_id = %d",
        exam->exam_id);

    printf("executing query: %s\n", query);

    MYSQL_RES *result = mysql_execute_query(connection, query);
    if (!result)
    {
        fprintf(stderr, "getExam: %s\n", mysql_error(connection));
        return NULL;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        return NULL;
    }

    exam->user_id = atoi(row[1]);
    strncpy(exam->title,         row[2], sizeof(exam->title)         - 1);
    strncpy(exam->creation_date, row[3], sizeof(exam->creation_date) - 1);

    mysql_free_result(result);
    return NULL;
}

void* getQuestion(void* params)
{
    struct DbIODetails *io_details = (struct DbIODetails *) params;
    MYSQL           *connection = (MYSQL *)          io_details->connection;
    struct Question *question   = (struct Question *) io_details->data;

    char query[500];
    snprintf(query, sizeof(query),
        "SELECT * FROM Questions WHERE question_id = %d",
        question->question_id);

    printf("executing query: %s\n", query);

    MYSQL_RES *result = mysql_execute_query(connection, query);
    if (!result)
    {
        fprintf(stderr, "getQuestion: %s\n", mysql_error(connection));
        return NULL;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        return NULL;
    }

    question->exam_id = atoi(row[1]);
    strncpy(question->question_statement, row[2],
            sizeof(question->question_statement) - 1);

    mysql_free_result(result);
    return NULL;
}

void* getOption(void* params)
{
    struct DbIODetails *io_details = (struct DbIODetails *) params;
    MYSQL         *connection = (MYSQL *)        io_details->connection;
    struct Option *option     = (struct Option *) io_details->data;

    char query[500];
    snprintf(query, sizeof(query),
        "SELECT * FROM Options WHERE question_id = %d",
        option->question_id);

    printf("executing query: %s\n", query);

    MYSQL_RES *result = mysql_execute_query(connection, query);
    if (!result)
    {
        fprintf(stderr, "getOption: %s\n", mysql_error(connection));
        return NULL;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        return NULL;
    }

    option->option_id   = atoi(row[0]);
    option->question_id = atoi(row[1]);
    strncpy(option->option_statement, row[2],
            sizeof(option->option_statement) - 1);
    option->is_correct  = atoi(row[3]);

    mysql_free_result(result);
    return NULL;
}

struct ConnectionDetails connection_details =
{
    .server   = MYSQL_SERVER,
    .user     = MYSQL_USER,
    .password = MYSQL_PASSWD,
    .database = MYSQL_DB
};
