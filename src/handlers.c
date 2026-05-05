#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <mysql/mysql.h>
#include "../headers/socks.h"
#include "../headers/structs.h"
#include "../headers/handlers.h"
#include "../headers/interdb.h"


void create_user(struct ClientHandlerParams params)
{
}

void create_session(struct ClientHandlerParams params)
{
}

void create_exam(struct ClientHandlerParams params)
{
    //reconstruct exam paper
    struct ExamPaper *exam_paper = (struct ExamPaper*)params.data;
    params.data += sizeof(struct ExamPaper);

    //prepare response header
    struct ResponseHeader create_exam_response;

    //point to questions
    exam_paper->questions = params.data; //this is actually questions & options

    //point to options
    void* options_start_here = params.data + (sizeof(struct FullQuestion) * exam_paper->n_questions);
    for (int i = 0; i < exam_paper->n_questions; i++)
    {
        //skip 4 options for every new set (of options)
        exam_paper->questions[i].options = options_start_here + (sizeof(struct FullOption) * 4 * i);
    }

    //write to database
    //write exam paper & record exam_id
    //for each question:
    //write question & record question_id
    //for each option, write option

    char query[500];
    int exam_id, question_id;

    //insert exam
    snprintf(query, sizeof(query),
            "INSERT INTO Exams(user_id, title) VALUES(%d, '%s')",
            exam_paper->user_id, exam_paper->title);
    printf("executing query: %s\n", query);
    mysql_execute_query(params.connection, query);

    exam_id = mysql_insert_id(params.connection);
    for (int i = 0; i < exam_paper->n_questions; i++)
    {
        //insert question
        snprintf(query, sizeof(query),
                "INSERT INTO Questions(exam_id, question_statement) VALUES(%d, '%s')",
                exam_id,
                exam_paper->questions[i].question_statement);
        printf("executing query: %s\n", query);
        mysql_execute_query(params.connection, query);
        question_id = mysql_insert_id(params.connection);

        for (int j = 0; j < 4; j++)
        {
            snprintf(query, sizeof(query),
                    "INSERT INTO Options(question_id, option_statement, is_correct) VALUES(%d, '%s', %d)",
                    question_id,
                    exam_paper->questions[i].options[j].option_statement,
                    exam_paper->questions[i].options[j].is_correct);
            printf("executing query: %s\n", query);
            mysql_execute_query(params.connection, query);
        }
    }

    //check if error
    if (!mysql_errno(params.connection))
        create_exam_response.status_code = CREATED;
    else
        create_exam_response.status_code = (uint8_t)INTERNAL_SERVER_ERROR;

    //respond
    write(params.cfd, &create_exam_response, sizeof(struct ResponseHeader));
}

void get_user(struct ClientHandlerParams params)
{
    //cast data to struct User
    struct User *user = (struct User *) params.data;

    //prepare response header
    struct ResponseHeader get_user_response = {.status_code = OK};

    //query database
    char query[500];

    //construct query string
    snprintf(query, sizeof(query),
        "SELECT * FROM Users WHERE username = '%s'",
        user->username);

    //execute query
    printf("executing query: %s\n", query);
    MYSQL_RES *result = mysql_execute_query(params.connection, query);

    //fetch result
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        get_user_response.status_code = (uint8_t)NOT_FOUND;
    }
    else
    {
        //re-format and store in struct User
        char *end_ptr;
        user->user_id = (int) strtol(row[0], &end_ptr, 10);
        user->status  = atoi(row[2]);
        strncpy(user->name,              row[3], sizeof(user->name)              - 1);
        strncpy(user->registration_date, row[4], sizeof(user->registration_date) - 1);

        //free result
        mysql_free_result(result);
    }
    
    //respond
    write(params.cfd, &get_user_response, sizeof(struct ResponseHeader));
    if (write(params.cfd, (void *) user, sizeof(struct User)) == -1)
        perror("write get_user");
}

void admit_candidate(struct ClientHandlerParams params)
{
}

void collect_candidate(struct ClientHandlerParams params)
{
}

typedef void (*Handler)(struct ClientHandlerParams);
static Handler router[] =
{
    create_user,        // 0  CMD_CREATE_USER
    create_exam,        // 1  CMD_CREATE_EXAM
    create_session,     // 2  CMD_CREATE_SESSION
    admit_candidate,    // 3  CMD_ADMIT_CANDIDATE
    collect_candidate,   // 4  CMD_COLLECT_CANDIDATE
    get_user,           // 5  CMD_GET_USER
};

static const int ROUTER_SIZE = (int)(sizeof(router) / sizeof(router[0]));

void handle_accept(int efd, int sfd)
{
    for (;;)
    {
        int cfd = accept(sfd, NULL, NULL);
        if (cfd == -1)
        {
            if (errno == EAGAIN) break;
            perror("accept");
            break;
        }
        set_nonblocking(cfd);

        struct epoll_event ev = { .events = EPOLLIN | EPOLLET, .data.fd = cfd };
        if (epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev) == -1)
        {
            perror("epoll_ctl: cfd");
            close(cfd);
            return;
        }
        printf("connected to client: %d\n", cfd);
    }
}

int handle_client(int efd, int cfd, MYSQL *connection)
{
    struct RequestHeader rh;
    ssize_t n = read(cfd, &rh, sizeof(rh));

    if (n == 0)
    {
        printf("client disconnected on: %d\n", cfd);
        epoll_ctl(efd, EPOLL_CTL_DEL, cfd, NULL);
        close(cfd);
        return -1;
    }

    if (n < (ssize_t)sizeof(rh))
        return BAD_REQUEST;

    if (rh.payload_size > MAX_PACKET)
        return BAD_REQUEST;

    if (rh.cmd_id >= (uint16_t)ROUTER_SIZE)
        return BAD_REQUEST;

    void *payload = malloc(rh.payload_size);
    if (!payload) return INTERNAL_SERVER_ERROR;

    size_t total_read = 0;
    while (total_read < rh.payload_size)
    {
        ssize_t curr_read = read(cfd,
                                 (char *)payload + total_read,
                                 rh.payload_size - total_read);

        if (curr_read > 0)
        {
            total_read += (size_t) curr_read;
        }
        else if (curr_read == 0)
        {
            printf("client disconnected on: %d\n", cfd);
            epoll_ctl(efd, EPOLL_CTL_DEL, cfd, NULL);
            close(cfd);
            free(payload);
            return -1;
        }
        else if (errno == EAGAIN)
        {
            free(payload);
            return BAD_REQUEST;
        }
    }

    if (total_read != rh.payload_size)
    {
        free(payload);
        return BAD_REQUEST;
    }

    struct ClientHandlerParams chp =
    {
        .connection = connection,
        .efd        = efd,
        .cfd        = cfd,
        .data       = payload
    };

    router[rh.cmd_id](chp);
    free(payload);
    return 0;
}

void* request_handler(void* args)
{
    struct RequestHandlerParams *params = (struct RequestHandlerParams *) args;

    if (params->rfd == params->sfd)
    {
        printf("new connection attempted\n");
        handle_accept(params->efd, params->sfd);
        return NULL;
    }

    struct ConnectionSlot *assigned_slot = NULL;

    if (sem_trywait(params->free_slots) == -1)
    {
        pthread_mutex_lock(params->slots_lock);

        if (*params->total_connections < DB_CONN_LIMIT)
        {
            MYSQL *new_connection = mysql_connection_setup(connection_details);

            params->connection_slots[*params->total_connections] = (struct ConnectionSlot)
            {
                .connection = new_connection,
                .avail      = 0,
                .index      = *params->total_connections
            };

            assigned_slot = &params->connection_slots[*params->total_connections];
            ++*params->total_connections;

            pthread_mutex_unlock(params->slots_lock);
        }
        else
        {
            pthread_mutex_unlock(params->slots_lock);
            sem_wait(params->free_slots);
            pthread_mutex_lock(params->slots_lock);

            for (int i = 0; i < *params->total_connections; i++)
            {
                if (params->connection_slots[i].avail)
                {
                    assigned_slot = &params->connection_slots[i];
                    params->connection_slots[i].avail = 0;
                    break;
                }
            }
            pthread_mutex_unlock(params->slots_lock);
        }
    }
    else
    {
        pthread_mutex_lock(params->slots_lock);

        for (int i = 0; i < *params->total_connections; i++)
        {
            if (params->connection_slots[i].avail)
            {
                assigned_slot = &params->connection_slots[i];
                params->connection_slots[i].avail = 0;
                break;
            }
        }
        pthread_mutex_unlock(params->slots_lock);
    }

    if (!assigned_slot)
    {
        fprintf(stderr, "request_handler: failed to acquire a connection slot\n");
        return NULL;
    }

    printf("client interaction on fd: %d\n", params->rfd);
    handle_client(params->efd, params->rfd, assigned_slot->connection);

    pthread_mutex_lock(params->slots_lock);
    assigned_slot->avail = 1;
    pthread_mutex_unlock(params->slots_lock);
    sem_post(params->free_slots);

    return NULL;
}
