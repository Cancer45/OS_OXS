#ifndef EXAM_SYS_STRUCTS
#define EXAM_SYS_STRUCTS
#include <stdint.h>
#include "dbh.h"

#define DATE_SIZE 11

struct User
{
    int user_id, status;
    char name[50], username[15];
    char registration_date[DATE_SIZE];
};

struct Exam
{
    int exam_id, user_id;
    char creation_date[DATE_SIZE];
    char title[100];
};

struct Question
{
    int question_id, exam_id;
    char question_statement[500];
};

struct Options
{
    int option_id, question_id;
    char option_statement[50];
    int is_correct;
};

struct Selected
{
    int selection_id, option_id, user_id, session_id;
};

struct ExamSessions
{
    int session_id, admin_id, exam_id;
    int session_duration, session_status;
    float average_score, score_SD;
};

struct SessionCandidates
{
    int session_candidate_id, session_id, user_id;
    float exam_score;
};

struct DbIODetails
{
    MYSQL* connection;
    void* data;
};

struct RequestHeader
{
    uint16_t mn;
    uint16_t cmd_id;
    uint32_t payload_size;
};

struct ResponseHeader
{
    uint8_t status_code;
    uint16_t cmd_id;
    uint32_t payload_size;
};

#endif
