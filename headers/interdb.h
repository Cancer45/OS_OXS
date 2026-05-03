#ifndef INTERDB_H
#define INTERDB_H

#include "dbh.h"
#include "structs.h"

void* setUser            (struct DbIODetails *io_details);
void* setExam            (void *params);
void* setQuestions       (void *params);
void* setOptions         (void *params);
void* setExamSessions    (void *params);
void* setSessionCandidates(void *params);
void* setSelected        (void *params);

void* getUser    (void *params);
void* getExam    (void *params);
void* getQuestion(void *params);
void* getOption  (void *params);

extern struct ConnectionDetails connection_details;

#endif
