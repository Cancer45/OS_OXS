#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include "../headers/socks.h"
#include "../headers/structs.h"
#include "../headers/interdb.h"
#include "../headers/handlers.h"

int create_user(struct ClientHandlerParams params)
{
    struct User *user = (struct User *) params.data;

    struct DbIODetails io_details =
    {
        .connection = params.connection,
        .data       = user
    };

    setUser(&io_details);

    if (write(params.cfd, (void *) user, sizeof(struct User)) == -1)
        perror("write create_user");

    return CREATED;
}

int create_exam(struct ClientHandlerParams params)
{
    struct Exam *exam = (struct Exam *) params.data;

    struct DbIODetails io_details =
    {
        .connection = params.connection,
        .data       = exam
    };

    setExam(&io_details);

    if (write(params.cfd, (void *) exam, sizeof(struct Exam)) == -1)
        perror("write create_exam");

    return CREATED;
}

int create_session(struct ClientHandlerParams params)
{
    struct ExamSession *session = (struct ExamSession *) params.data;

    struct DbIODetails io_details =
    {
        .connection = params.connection,
        .data       = session
    };

    setExamSessions(&io_details);

    if (write(params.cfd, (void *) session, sizeof(struct ExamSession)) == -1)
        perror("write create_session");

    return CREATED;
}

int admit_candidate(struct ClientHandlerParams params)
{
    struct SessionCandidate *candidate = (struct SessionCandidate *) params.data;

    struct DbIODetails io_details =
    {
        .connection = params.connection,
        .data       = candidate
    };

    setSessionCandidates(&io_details);

    if (write(params.cfd, (void *) candidate, sizeof(struct SessionCandidate)) == -1)
        perror("write admit_candidate");

    return CREATED;
}

int collect_candidate(struct ClientHandlerParams params)
{
    struct Selected *selected = (struct Selected *) params.data;

    struct DbIODetails io_details =
    {
        .connection = params.connection,
        .data       = selected
    };

    setSelected(&io_details);

    if (write(params.cfd, (void *) selected, sizeof(struct Selected)) == -1)
        perror("write collect_candidate");

    return CREATED;
}

int get_user(struct ClientHandlerParams params)
{
    struct User *user = (struct User *) params.data;

    struct DbIODetails io_details =
    {
        .connection = params.connection,
        .data       = user
    };

    getUser(&io_details);

    if (write(params.cfd, (void *) user, sizeof(struct User)) == -1)
        perror("write get_user");

    return OK;
}

int get_exam(struct ClientHandlerParams params)
{
    struct Exam *exam = (struct Exam *) params.data;

    struct DbIODetails io_details =
    {
        .connection = params.connection,
        .data       = exam
    };

    getExam(&io_details);

    if (write(params.cfd, (void *) exam, sizeof(struct Exam)) == -1)
        perror("write get_exam");

    return OK;
}

int get_question(struct ClientHandlerParams params)
{
    struct Question *q = (struct Question *) params.data;

    struct DbIODetails io_details =
    {
        .connection = params.connection,
        .data       = q
    };

    getQuestion(&io_details);

    if (write(params.cfd, (void *) q, sizeof(struct Question)) == -1)
        perror("write get_question");

    return OK;
}

int get_option(struct ClientHandlerParams params)
{
    struct Option *o = (struct Option *) params.data;

    struct DbIODetails io_details =
    {
        .connection = params.connection,
        .data       = o
    };

    getOption(&io_details);

    if (write(params.cfd, (void *) o, sizeof(struct Option)) == -1)
        perror("write get_option");

    return OK;
}

int create_question(struct ClientHandlerParams params)
{
    struct Question *q = (struct Question *) params.data;

    struct DbIODetails io_details =
    {
        .connection = params.connection,
        .data       = q
    };

    setQuestions(&io_details);

    if (write(params.cfd, (void *) q, sizeof(struct Question)) == -1)
        perror("write create_question");

    return CREATED;
}

int create_option(struct ClientHandlerParams params)
{
    struct Option *o = (struct Option *) params.data;

    struct DbIODetails io_details =
    {
        .connection = params.connection,
        .data       = o
    };

    setOptions(&io_details);

    if (write(params.cfd, (void *) o, sizeof(struct Option)) == -1)
        perror("write create_option");

    return CREATED;
}

typedef int (*Handler)(struct ClientHandlerParams);
static Handler router[] =
{
    create_user,        /* 0  CMD_CREATE_USER */
    create_exam,        /* 1  CMD_CREATE_EXAM */
    create_session,     /* 2  CMD_CREATE_SESSION */
    admit_candidate,    /* 3  CMD_ADMIT_CANDIDATE */
    collect_candidate,  /* 4  CMD_COLLECT_CANDIDATE */
    get_user,           /* 5  CMD_GET_USER */
    get_exam,           /* 6  CMD_GET_EXAM */
    get_question,       /* 7  CMD_GET_QUESTION */
    get_option,         /* 8  CMD_GET_OPTION */
    create_question,    /* 9  CMD_CREATE_QUESTION */
    create_option       /* 10 CMD_CREATE_OPTION */
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

    int result = router[rh.cmd_id](chp);
    free(payload);
    return result;
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
