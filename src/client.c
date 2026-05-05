#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../headers/structs.h"
#include "../headers/socks.h"

void flush()
{
    while(getchar() != '\n'); 
}

int login(int server_fd, struct User *user)
{
    //input username
    printf("username: ");
    scanf("%s", user->username);

    //send request
    struct RequestHeader user_info_request =
    {
        .cmd_id = CMD_GET_USER,
        .payload_size = sizeof(struct User)
    };

    //write RequestHeader & User
    write(server_fd, &user_info_request, sizeof(struct RequestHeader));
    write(server_fd, user, sizeof(struct User));

    //process response
    struct ResponseHeader user_info_response;

    //read response header
    read(server_fd, &user_info_response, sizeof(struct ResponseHeader));
    read(server_fd, user, sizeof(struct User));

    //if no errors, read into struct User
    if (user_info_response.status_code == OK)
    {
        return 1;
    }
    return 0;
}

int client_create_exam(int server_fd, struct ExamPaper *exam_paper)
{
    //input title
    do
    {
        printf("\nCREATE EXAM\ntitle: ");

        flush();
        fgets(exam_paper->title, sizeof(exam_paper->title), stdin);
        exam_paper->title[strcspn(exam_paper->title, "\n")] = 0;

    }while(strlen(exam_paper->title) > 100);

    //input n_questions
    printf("\nnumber of questions: ");

    scanf("%d", &exam_paper->n_questions);
    flush();

    //allocate memory for exam_paper questions
    exam_paper->questions = malloc(sizeof(struct FullQuestion) * exam_paper->n_questions);

    for (int i = 0; i < exam_paper->n_questions; i++)
    {
        //input question statement
        do
        {
            printf("Q%d. ", i + 1);
            fgets(exam_paper->questions[i].question_statement,
                    sizeof(exam_paper->questions[i].question_statement),
                    stdin);
            exam_paper->questions[i].question_statement[strcspn(exam_paper->questions[i].question_statement, "\n")] = 0;

        }while(strlen(exam_paper->questions[i].question_statement) > 500);

        //init options character and allocate memory for options
        char opt = 'A';

        //input options
        exam_paper->questions[i].options = malloc(sizeof(struct FullOption) * 4);
        for (int j = 0; j < 4; j++)
        {

            do
            {
                printf("option %c. ", opt);
                fgets(exam_paper->questions[i].options[j].option_statement,
                        sizeof(exam_paper->questions[i].options[j].option_statement),
                        stdin);
                exam_paper->questions[i].options[j].option_statement[strcspn(exam_paper->questions[i].options[j].option_statement, "\n")] = 0;

            }while(strlen(exam_paper->questions[i].options[j].option_statement) > 50);
            exam_paper->questions[i].options[j].is_correct = 0;
            ++opt;
        }

        //input correct option
        int correct_option;
        do {
            printf("correct option? (A, B, C or D): ");

            correct_option = (int)getchar();

        } while(correct_option < 'A' || correct_option > 'D');

        exam_paper->questions[i].options[correct_option - 'A'].is_correct = 1;

        //no use for this rn - maybe when variable number of options
        exam_paper->questions[i].size = sizeof(exam_paper->questions[i]);
    }

    //construct exam paper request header
    struct RequestHeader create_exam_request = 
    {
        //all questions(number of options) are the same size - for now
        .cmd_id = CMD_CREATE_EXAM,
        .payload_size = sizeof(struct ExamPaper) 
            + sizeof(struct FullQuestion) * exam_paper->n_questions
            + sizeof(struct FullOption) * 4 * exam_paper->n_questions
    };

    //send request header
    write(server_fd, &create_exam_request, sizeof(struct RequestHeader));

    //send data
    write(server_fd, exam_paper, sizeof(struct ExamPaper));
    for (int i = 0; i < exam_paper->n_questions; i++)
    {
        //write question
        write(server_fd, &exam_paper->questions[i], sizeof(struct FullQuestion));
    }

    //write options AFTER the questions
    for (int i = 0; i < exam_paper->n_questions; i++)
    {
        for (int j = 0; j < 4; j++)
            write(server_fd, &exam_paper->questions[i].options[j], sizeof(struct FullOption));

        free(exam_paper->questions[i].options);
    }
    free(exam_paper->questions);

    //receive response header
    struct ResponseHeader create_exam_response;
    read(server_fd, &create_exam_response, sizeof(struct ResponseHeader));

    if (create_exam_response.status_code == CREATED)
        return 1;
    else
        return 0;
}

int main()
{
    //connect to server
    int server_fd = uds_socket_init();
    uds_connect(server_fd, SOCK_PATH);

    struct User user = {0};
    int logged_in = login(server_fd, &user);

    if (logged_in)
    {
        printf("\nLOGGED IN!\nUSER DETAILS\nName: %s\nUsername: %s\nID: %d\nRegistration Date: %s\n", 
                user.name, user.username, user.user_id, user.registration_date);
        if (user.status)
            printf("Status: Candidate\n");
        else
            printf("Status: Admin\n");
    }
    else
        printf("no such user\n");

    //create exam
    if (logged_in && !user.status)
    {
        struct ExamPaper exam_paper = {0};
        exam_paper.user_id = user.user_id;

        if (client_create_exam(server_fd, &exam_paper))
            printf("exam created\n");
        else
            printf("exam creation failed\n");
    }
}
