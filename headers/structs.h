#ifndef EXAM_SYS_STRUCTS
#define EXAM_SYS_STRUCTS

#include <stdint.h>
#include <semaphore.h>
#include <sys/epoll.h>
#include "dbh.h"

#define DATE_SIZE 11
#define DB_CONN_LIMIT 10

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

struct Option
{
    int option_id, question_id;
    char option_statement[50];
    int is_correct;
};

struct Selected
{
    int selection_id, option_id, session_candidate_id;
};

struct ExamSession
{
    int session_id, admin_id, exam_id;
    int session_duration, session_status;
    float average_score, score_SD;
};

struct SessionCandidate
{
    int session_candidate_id, session_id, user_id;
    float exam_score;
};

struct DbIODetails
{
    MYSQL* connection;
    void* data;
};

struct ConnectionSlot
{
    MYSQL* connection;
    int index;
    uint8_t avail;
};

struct RequestHandlerParams
{
    int efd, sfd, rfd;
    int *total_connections;
    sem_t *free_slots;
    pthread_mutex_t *slots_lock;
    struct ConnectionSlot *connection_slots;
};


struct ClientHandlerParams
{
    MYSQL* connection;
    int efd, cfd;
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
