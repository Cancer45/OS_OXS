#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include "../headers/raygui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#include "../headers/structs.h"
#include "../headers/socks.h"  


#define MN                    0xEA75u   

#define CMD_CREATE_USER           0
#define CMD_CREATE_EXAM           1
#define CMD_CREATE_SESSION        2
#define CMD_ADMIT_CANDIDATE       3
#define CMD_COLLECT_CANDIDATE     4
#define CMD_GET_USER              5
#define CMD_GET_EXAM              6
#define CMD_GET_QUESTION          7
#define CMD_GET_OPTION            8
#define CMD_CREATE_QUESTION       9
#define CMD_CREATE_OPTION        10

#define SCREEN_W      800
#define SCREEN_H      600
#define INFO_BAR_H     72   
#define CONTENT_Y     (INFO_BAR_H + 16)
#define MAX_QUESTIONS  50
#define MAX_OPTIONS     4  

#define PAGE_LOGIN         0
#define PAGE_CANDIDATE     1   
#define PAGE_EXAM          2   
#define PAGE_SUBMIT        3   
#define PAGE_ADMIN         4   
#define PAGE_CREATE_SETUP  5   
#define PAGE_CREATE_Q      6   
#define PAGE_SESSION       7   

static int g_sfd = -1;

static int g_page = PAGE_LOGIN;

static char g_err[128] = "";

static struct User g_user;

static char           g_exam_id_str[12]  = "";   
static struct Exam    g_exam;
static struct Question g_questions[MAX_QUESTIONS];
static struct Option   g_options[MAX_QUESTIONS][MAX_OPTIONS];
static int             g_opt_counts[MAX_QUESTIONS];
static int             g_selected[MAX_QUESTIONS]; 
static int             g_q_count   = 0;
static int             g_current_q = 0;

static char g_new_title[100]  = "";
static char g_new_numq_str[8] = "";
static int  g_new_num_q       = 0;
static int  g_create_q        = 0;
static char g_new_q_text   [MAX_QUESTIONS][500];
static char g_new_opt_text [MAX_QUESTIONS][MAX_OPTIONS][50];
static int  g_new_correct  [MAX_QUESTIONS];   

static struct ExamSession g_session;
static bool g_session_active = false;
static int  g_active_combo   = 0;  

static bool eExamId   = false;
static bool eTitle    = false;
static bool eNumQ     = false;
static bool eQText    = false;
static bool eOptA     = false;
static bool eOptB     = false;
static bool eOptC     = false;

static bool g_hover = false;

static bool DrawButton(Rectangle b, const char *t)
{
    if (CheckCollisionPointRec(GetMousePosition(), b)) g_hover = true;
    return GuiButton(b, t);
}
static bool DrawTextBox(Rectangle b, char *t, int sz, bool edit)
{
    if (CheckCollisionPointRec(GetMousePosition(), b)) g_hover = true;
    return GuiTextBox(b, t, sz, edit);
}
static void DrawToggleGroup(Rectangle b, const char *t, int *a)
{
    if (CheckCollisionPointRec(GetMousePosition(), b)) g_hover = true;
    GuiToggleGroup(b, t, a);
}
static void DrawComboBox(Rectangle b, const char *t, int *a)
{
    if (CheckCollisionPointRec(GetMousePosition(), b)) g_hover = true;
    GuiComboBox(b, t, a);
}

static int server_connect(void)
{
    g_sfd = uds_socket_init();
    if (g_sfd < 0) return -1;
    if (uds_connect(g_sfd, SOCK_PATH) < 0)
    {
        close(g_sfd);
        g_sfd = -1;
        return -1;
    }
    return 0;
}

static void server_disconnect(void)
{
    if (g_sfd >= 0) { close(g_sfd); g_sfd = -1; }
}

static int write_all(int fd, const void *buf, size_t n)
{
    size_t sent = 0;
    while (sent < n)
    {
        ssize_t w = write(fd, (const char *)buf + sent, n - sent);
        if (w <= 0) return -1;
        sent += (size_t)w;
    }
    return 0;
}

static int read_all(int fd, void *buf, size_t n)
{
    size_t got = 0;
    while (got < n)
    {
        ssize_t r = read(fd, (char *)buf + got, n - got);
        if (r <= 0) return -1;
        got += (size_t)r;
    }
    return 0;
}

static int send_request(uint16_t cmd_id, void *payload, uint32_t size)
{
    struct RequestHeader hdr =
    {
        .mn           = MN,
        .cmd_id       = cmd_id,
        .payload_size = size
    };
    if (write_all(g_sfd, &hdr, sizeof hdr) < 0) return -1;
    if (size > 0 && write_all(g_sfd, payload, size) < 0) return -1;
    return 0;
}

static int client_get_user(struct User *u)
{
    if (send_request(CMD_GET_USER, u, sizeof *u) < 0) return -1;
    return read_all(g_sfd, u, sizeof *u);
}

static int client_create_user(struct User *u)
{
    if (send_request(CMD_CREATE_USER, u, sizeof *u) < 0) return -1;
    return read_all(g_sfd, u, sizeof *u);
}

static int client_get_exam(struct Exam *e)
{
    if (send_request(CMD_GET_EXAM, e, sizeof *e) < 0) return -1;
    return read_all(g_sfd, e, sizeof *e);
}

static int client_create_exam(struct Exam *e)
{
    if (send_request(CMD_CREATE_EXAM, e, sizeof *e) < 0) return -1;
    return read_all(g_sfd, e, sizeof *e);
}

static int client_get_question(struct Question *q)
{
    if (send_request(CMD_GET_QUESTION, q, sizeof *q) < 0) return -1;
    return read_all(g_sfd, q, sizeof *q);
}

static int client_create_question(struct Question *q)
{
    if (send_request(CMD_CREATE_QUESTION, q, sizeof *q) < 0) return -1;
    return read_all(g_sfd, q, sizeof *q);
}

static int client_get_option(struct Option *o)
{
    if (send_request(CMD_GET_OPTION, o, sizeof *o) < 0) return -1;
    return read_all(g_sfd, o, sizeof *o);
}

static int client_create_option(struct Option *o)
{
    if (send_request(CMD_CREATE_OPTION, o, sizeof *o) < 0) return -1;
    return read_all(g_sfd, o, sizeof *o);
}

static int client_create_session(struct ExamSession *s)
{
    if (send_request(CMD_CREATE_SESSION, s, sizeof *s) < 0) return -1;
    return read_all(g_sfd, s, sizeof *s);
}

static int client_admit_candidate(struct SessionCandidate *sc)
{
    if (send_request(CMD_ADMIT_CANDIDATE, sc, sizeof *sc) < 0) return -1;
    return read_all(g_sfd, sc, sizeof *sc);
}

static int client_collect_candidate(struct Selected *sel)
{
    if (send_request(CMD_COLLECT_CANDIDATE, sel, sizeof *sel) < 0) return -1;
    return read_all(g_sfd, sel, sizeof *sel);
}

static void today_str(char *buf, size_t sz)
{
    time_t t    = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, sz, "%Y-%m-%d", tm);
}

static int do_login(void)
{
    /* Sync name to match username before sending, in case the server's DB matches on 'name' */
    strncpy(g_user.name, g_user.username, sizeof(g_user.name) - 1);

    /* Try to fetch existing user */
    if (client_get_user(&g_user) == 0 && g_user.user_id != 0) {
        return 0; /* Success! */
    }
    return -1; /* Not found */
}

static void do_logout(void)
{
    memset(&g_user, 0, sizeof(g_user));

    g_err[0] = '\0';

    g_page = PAGE_LOGIN;
}

static int do_register(const char* new_username, const char* new_name, int status)
{
    char saved_uname[15];
    char saved_name[50];
    
    strncpy(saved_uname, new_username, sizeof(saved_uname) - 1);
    saved_uname[14] = '\0';
    
    strncpy(saved_name, new_name, sizeof(saved_name) - 1);
    saved_name[49] = '\0';
    
    memset(&g_user, 0, sizeof g_user);
    
    strncpy(g_user.username, saved_uname, sizeof(g_user.username) - 1);
    strncpy(g_user.name,     saved_name,  sizeof(g_user.name)     - 1);
    g_user.status = status; 
    today_str(g_user.registration_date, sizeof(g_user.registration_date));

    if (client_create_user(&g_user) < 0) return -1;

    char saved[15];
    strncpy(saved, g_user.username, sizeof saved - 1);
    memset(&g_user, 0, sizeof g_user);
    
    strncpy(g_user.username, saved, sizeof(g_user.username) - 1);
    strncpy(g_user.name,     saved, sizeof(g_user.name)     - 1); 
    
    client_get_user(&g_user);   
    return 0;
}

static void load_exam_questions(void)
{
    g_q_count   = 0;
    g_current_q = 0;
    memset(g_selected,   -1, sizeof g_selected);
    memset(g_opt_counts,  0, sizeof g_opt_counts);

    int misses = 0;
    for (int qid = 1; qid <= 500 && g_q_count < MAX_QUESTIONS; qid++)
    {
        struct Question q;
        memset(&q, 0, sizeof q);
        q.question_id = qid;

        if (client_get_question(&q) < 0) { if (++misses > 8) break; continue; }
        if (q.question_statement[0] == '\0') { if (++misses > 8) break; continue; }

        if (q.exam_id != g_exam.exam_id) { misses++; continue; }

        g_questions[g_q_count] = q;
        misses = 0;

        int  found  = 0;
        int  omisses = 0;
        for (int oid = 1; oid <= 200 && found < MAX_OPTIONS; oid++)
        {
            struct Option o;
            memset(&o, 0, sizeof o);
            o.option_id   = oid;
            o.question_id = qid;

            if (client_get_option(&o) < 0) { if (++omisses > 6) break; continue; }
            if (o.question_id != qid)      { omisses++; continue; }

            g_options[g_q_count][found++] = o;
            omisses = 0;
        }
        g_opt_counts[g_q_count] = found;
        g_q_count++;
    }
}

static void build_option_str(int qi, char *buf, int bufsz)
{
    static const char prefix[] = {'A','B','C','D'};
    buf[0] = '\0';
    for (int i = 0; i < g_opt_counts[qi]; i++)
    {
        char part[80];
        snprintf(part, sizeof part, "%c. %s", prefix[i],
                 g_options[qi][i].option_statement);
        if (i > 0) strncat(buf, ";", bufsz - (int)strlen(buf) - 1);
        strncat(buf, part,  bufsz - (int)strlen(buf) - 1);
    }
}

static void submit_answers(void)
{
    struct SessionCandidate sc;
    memset(&sc, 0, sizeof sc);
    sc.session_id = g_session.session_id;
    sc.user_id    = g_user.user_id;
    client_admit_candidate(&sc);

    for (int i = 0; i < g_q_count; i++)
    {
        if (g_selected[i] < 0) continue;   
        struct Selected sel;
        memset(&sel, 0, sizeof sel);
        sel.option_id            = g_options[i][g_selected[i]].option_id;
        sel.session_candidate_id = sc.session_candidate_id;
        client_collect_candidate(&sel);
    }
}

static void save_exam(void)
{
    struct Exam exam;
    memset(&exam, 0, sizeof exam);
    exam.user_id = g_user.user_id;
    strncpy(exam.title, g_new_title, sizeof(exam.title) - 1);
    today_str(exam.creation_date, sizeof(exam.creation_date));
    client_create_exam(&exam);

    for (int i = 0; i < g_new_num_q; i++)
    {
        struct Question q;
        memset(&q, 0, sizeof q);
        q.exam_id = exam.exam_id;
        strncpy(q.question_statement, g_new_q_text[i],
                sizeof(q.question_statement) - 1);
        client_create_question(&q);

        for (int j = 0; j < MAX_OPTIONS; j++)
        {
            if (g_new_opt_text[i][j][0] == '\0') continue;
            struct Option o;
            memset(&o, 0, sizeof o);
            o.question_id = q.question_id;
            o.is_correct  = (j == g_new_correct[i]) ? 1 : 0;
            strncpy(o.option_statement, g_new_opt_text[i][j],
                    sizeof(o.option_statement) - 1);
            client_create_option(&o);
        }
    }
}

static void do_start_session(int exam_id)
{
    memset(&g_session, 0, sizeof g_session);
    g_session.admin_id         = g_user.user_id;
    g_session.exam_id          = exam_id;
    g_session.session_duration = 3600;
    g_session.session_status   = 1;
    client_create_session(&g_session);
    g_session_active = true;
}

static void draw_info_bar(void)
{
    DrawRectangle(0, 0, SCREEN_W, INFO_BAR_H - 8, (Color){30, 30, 30, 255});

    const char *role = (g_user.status == 0) ? "Admin" : "Candidate";
    const char *name = (g_user.name[0] != '\0') ? g_user.name : g_user.username;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    GuiLabel((Rectangle){16, 12, 500, 28},
             TextFormat("User: %s", name));
    GuiLabel((Rectangle){16, 40, 300, 24},
             TextFormat("Role: %s  |  ID: %d", role, g_user.user_id));
    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);

    GuiLine((Rectangle){0, INFO_BAR_H, SCREEN_W, 8}, NULL);
}

static void draw_error(void)
{
    if (g_err[0] == '\0') return;
    DrawRectangle(0, SCREEN_H - 36, SCREEN_W, 36, (Color){160, 32, 32, 220});
    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
    GuiLabel((Rectangle){16, SCREEN_H - 32, SCREEN_W - 32, 28}, g_err);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
}

static bool is_registering = false;
static bool eUsername = false;  
static bool eRegName = false;   
static char reg_name_input[50] = {0};
static int reg_status = 1;

int client_request(int cmd, void *data, size_t size) {
    int sfd = uds_socket_init();
    if (sfd == -1) return -1;

    if (uds_connect(sfd, SOCK_PATH) == -1) {
        close(sfd);
        return -1;
    }

    if (write(sfd, &cmd, sizeof(int)) != sizeof(int)) {
        close(sfd);
        return -1;
    }

    if (write(sfd, data, size) != (ssize_t)size) {
        close(sfd);
        return -1;
    }

    read(sfd, data, size);

    close(sfd);
    return 0;
}

static void page_login(void)
{
    DrawText("Exam System Login", 300, 100, 30, DARKGRAY);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        if (!is_registering) {
            if (!CheckCollisionPointRec(mouse, (Rectangle){ 300, 200, 200, 40 })) eUsername = false;
        } else {
            if (!CheckCollisionPointRec(mouse, (Rectangle){ 300, 170, 200, 40 })) eUsername = false;
            if (!CheckCollisionPointRec(mouse, (Rectangle){ 300, 230, 200, 40 })) eRegName = false;
        }
    }

    if (!is_registering) {
        if (GuiTextBox((Rectangle){ 300, 200, 200, 40 }, g_user.username, sizeof(g_user.username), eUsername)) {
            eUsername = !eUsername;
        }
        
        if (GuiButton((Rectangle){ 300, 260, 200, 40 }, "LOGIN")) {
            eUsername = false; // Force unfocus
            if (strlen(g_user.username) > 0) {
                if (do_login() == 0) {
                    if (g_user.status == 0) g_page = PAGE_ADMIN;
                    else g_page = PAGE_CANDIDATE;
                } else {
                    snprintf(g_err, sizeof(g_err), "User not found. Please register.");
                }
            } else {
                snprintf(g_err, sizeof(g_err), "Please enter a username.");
            }
        }

        if (GuiButton((Rectangle){ 300, 320, 200, 30 }, "Create new account")) {
            is_registering = true;
            eUsername = false;
            g_err[0] = '\0'; 
        }

    } else {
        DrawText("Username:", 180, 180, 20, DARKGRAY);
        if (GuiTextBox((Rectangle){ 300, 170, 200, 40 }, g_user.username, sizeof(g_user.username), eUsername)) {
            eUsername = !eUsername;
        }

        DrawText("Full Name:", 180, 240, 20, DARKGRAY);
        if (GuiTextBox((Rectangle){ 300, 230, 200, 40 }, reg_name_input, sizeof(reg_name_input), eRegName)) {
            eRegName = !eRegName;
        }

        DrawText("Role:", 230, 300, 20, DARKGRAY);
        
        bool is_admin = (reg_status == 0);
        bool is_candidate = (reg_status == 1);
        
        GuiToggle((Rectangle){ 300, 290, 95, 30 }, "Admin", &is_admin);
        GuiToggle((Rectangle){ 405, 290, 95, 30 }, "Candidate", &is_candidate);
        
        if (is_admin) reg_status = 0;
        if (is_candidate) reg_status = 1;

        if (GuiButton((Rectangle){ 300, 350, 200, 40 }, "REGISTER & LOGIN")) {
            eUsername = false;
            eRegName = false; 
            
            if (strlen(g_user.username) > 0 && strlen(reg_name_input) > 0) {
                if (do_register(g_user.username, reg_name_input, reg_status) == 0) {
                    is_registering = false; // Reset for future logouts
                    if (g_user.status == 0) g_page = PAGE_ADMIN;
                    else g_page = PAGE_CANDIDATE;
                } else {
                    snprintf(g_err, sizeof(g_err), "Registration failed.");
                }
            } else {
                snprintf(g_err, sizeof(g_err), "Please fill all fields.");
            }
        }

        if (GuiButton((Rectangle){ 300, 410, 200, 30 }, "Back to Login")) {
            is_registering = false;
            eUsername = false;
            eRegName = false;
            g_err[0] = '\0';
        }
    }
}

static void page_candidate(void)
{
    draw_info_bar();

    float cy = CONTENT_Y + 50;
    GuiLabel((Rectangle){250, cy, 300, 36}, "Enter Exam ID:");
    cy += 44;
    if (DrawTextBox((Rectangle){250, cy, 300, 52}, g_exam_id_str, 11, eExamId))
        eExamId = !eExamId;
    cy += 72;

    if (DrawButton((Rectangle){300, cy, 200, 52}, "Take Exam"))
    {
        g_err[0] = '\0';
        int eid = atoi(g_exam_id_str);
        if (eid <= 0)
        {
            strncpy(g_err, "Enter a valid numeric exam ID.", sizeof g_err - 1);
        }
        else
        {
            memset(&g_exam, 0, sizeof g_exam);
            g_exam.exam_id = eid;
            if (client_get_exam(&g_exam) < 0 || g_exam.exam_id == 0)
            {
                strncpy(g_err, "Exam not found on server.", sizeof g_err - 1);
            }
            else
            {
                load_exam_questions();
                if (g_q_count == 0)
                    strncpy(g_err, "No questions found for this exam.",
                            sizeof g_err - 1);
                else
                {
                    eExamId = false;
                    g_page  = PAGE_EXAM;
                }
            }
        }
    }

    if (GuiButton((Rectangle){ 650, 20, 100, 30 }, "LOGOUT")) {
        do_logout();
    }
}

static void page_exam(void)
{
    draw_info_bar();

    float cy = CONTENT_Y + 4;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    GuiLabel((Rectangle){20, cy, 760, 32},
             TextFormat("Question %d of %d  —  %s",
                        g_current_q + 1, g_q_count, g_exam.title));
    cy += 36;
    GuiLine((Rectangle){0, cy, SCREEN_W, 8}, NULL);
    cy += 18;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 22);
    GuiLabel((Rectangle){20, cy, 760, 60},
             g_questions[g_current_q].question_statement);
    cy += 76;

    if (g_opt_counts[g_current_q] > 0)
    {
        char opt_str[512];
        build_option_str(g_current_q, opt_str, sizeof opt_str);

        int   n     = g_opt_counts[g_current_q];
        float btn_w = 760.0f / n;

        DrawToggleGroup((Rectangle){20, cy, btn_w, 52},
                        opt_str, &g_selected[g_current_q]);
    }
    else
    {
        GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
        GuiLabel((Rectangle){20, cy, 760, 52}, "(No options loaded for this question)");
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 22);

    bool on_first = (g_current_q == 0);
    bool on_last  = (g_current_q == g_q_count - 1);

    if (on_first) GuiSetState(STATE_DISABLED);
    if (DrawButton((Rectangle){20, 520, 160, 52}, "<- Back") && !on_first)
        g_current_q--;
    GuiSetState(STATE_NORMAL);

    if (DrawButton((Rectangle){620, 520, 160, 52}, on_last ? "Finish ->" : "Next ->"))
    {
        if (on_last)
            g_page = PAGE_SUBMIT;
        else
            g_current_q++;
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
}

static void page_submit(void)
{
    draw_info_bar();

    float cy = CONTENT_Y + 60;
    GuiLabel((Rectangle){100, cy, 600, 40}, "You have answered all questions.");
    cy += 56;
    GuiLabel((Rectangle){200, cy, 400, 40}, "Submit your answers?");
    cy += 80;

    if (DrawButton((Rectangle){140, cy, 180, 52}, "<- Back"))
    {
        g_current_q = g_q_count - 1;
        g_page = PAGE_EXAM;
    }
    if (DrawButton((Rectangle){480, cy, 180, 52}, "Submit"))
    {
        submit_answers();
        g_q_count = 0;
        g_current_q = 0;
        memset(g_exam_id_str, 0, sizeof g_exam_id_str);
        g_page = PAGE_CANDIDATE;
    }
}

static void page_admin(void)
{
    draw_info_bar();

    float cy = CONTENT_Y + 80;
    if (DrawButton((Rectangle){250, cy, 300, 60}, "Create Exam"))
    {
        memset(g_new_title,    0, sizeof g_new_title);
        memset(g_new_numq_str, 0, sizeof g_new_numq_str);
        memset(g_new_q_text,   0, sizeof g_new_q_text);
        memset(g_new_opt_text, 0, sizeof g_new_opt_text);
        memset(g_new_correct,  0, sizeof g_new_correct);
        g_new_num_q = 0;
        g_create_q  = 0;
        g_page = PAGE_CREATE_SETUP;
    }
    cy += 100;
    if (DrawButton((Rectangle){250, cy, 300, 60}, "Start Session"))
    {
        g_session_active = false;
        g_active_combo   = 0;
        g_page = PAGE_SESSION;
    }

    if (GuiButton((Rectangle){ 650, 20, 100, 30 }, "LOGOUT")) {
        do_logout();
    }
}

static void page_create_setup(void)
{
    draw_info_bar();

    float cy = CONTENT_Y + 24;
    GuiLabel((Rectangle){50, cy, 180, 40}, "Exam Title:");
    if (DrawTextBox((Rectangle){240, cy, 510, 48}, g_new_title, 99, eTitle))
        eTitle = !eTitle;
    cy += 68;

    GuiLabel((Rectangle){50, cy, 280, 40}, "Number of Questions:");
    if (DrawTextBox((Rectangle){340, cy, 110, 48}, g_new_numq_str, 7, eNumQ))
        eNumQ = !eNumQ;
    cy += 80;

    if (DrawButton((Rectangle){200, cy, 160, 52}, "<- Back"))
        g_page = PAGE_ADMIN;

    if (DrawButton((Rectangle){440, cy, 160, 52}, "Next ->"))
    {
        g_err[0] = '\0';
        int n = atoi(g_new_numq_str);
        if (g_new_title[0] == '\0')
            strncpy(g_err, "Please enter a title.", sizeof g_err - 1);
        else if (n <= 0 || n > MAX_QUESTIONS)
            snprintf(g_err, sizeof g_err,
                     "Number of questions must be 1-%d.", MAX_QUESTIONS);
        else
        {
            g_new_num_q = n;
            g_create_q  = 0;
            eTitle = eNumQ = false;
            g_page = PAGE_CREATE_Q;
        }
    }
}

static void page_create_q(void)
{
    draw_info_bar();

    float cy = CONTENT_Y + 4;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    GuiLabel((Rectangle){20, cy, 760, 30},
             TextFormat("Question %d of %d  —  %s",
                        g_create_q + 1, g_new_num_q, g_new_title));
    cy += 34;
    GuiLine((Rectangle){0, cy, SCREEN_W, 8}, NULL);
    cy += 16;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    GuiLabel((Rectangle){20, cy, 200, 32}, "Question:");
    cy += 36;
    if (DrawTextBox((Rectangle){20, cy, 760, 48},
                    g_new_q_text[g_create_q], 499, eQText))
        eQText = !eQText;
    cy += 58;

    static const char *labels[] = {"Option A:", "Option B:", "Option C:"};
    bool *edits[] = {&eOptA, &eOptB, &eOptC};
    for (int j = 0; j < 3; j++)
    {
        GuiLabel((Rectangle){20, cy, 130, 32}, labels[j]);
        if (DrawTextBox((Rectangle){158, cy, 622, 42},
                        g_new_opt_text[g_create_q][j], 49, *edits[j]))
            *edits[j] = !*edits[j];
        cy += 50;
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    GuiLabel((Rectangle){20, cy, 200, 32}, "Correct Answer:");
    DrawToggleGroup((Rectangle){224, cy, 110, 38}, "A;B;C",
                    &g_new_correct[g_create_q]);
    cy += 52;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 22);

    bool is_first = (g_create_q == 0);
    bool is_last  = (g_create_q == g_new_num_q - 1);

    if (is_first) GuiSetState(STATE_DISABLED);
    if (DrawButton((Rectangle){20, 530, 160, 50}, "<- Prev") && !is_first)
    {
        eQText = eOptA = eOptB = eOptC = false;
        g_create_q--;
    }
    GuiSetState(STATE_NORMAL);

    if (DrawButton((Rectangle){SCREEN_W - 220, 530, 200, 50},
                   is_last ? "Save Exam" : "Next ->"))
    {
        g_err[0] = '\0';
        if (g_new_q_text[g_create_q][0] == '\0')
        {
            strncpy(g_err, "Please enter question text before continuing.",
                    sizeof g_err - 1);
        }
        else
        {
            eQText = eOptA = eOptB = eOptC = false;
            if (is_last)
            {
                save_exam();
                g_page = PAGE_ADMIN;
            }
            else
            {
                g_create_q++;
            }
        }
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
}

static void page_session(void)
{
    draw_info_bar();

    float cy = CONTENT_Y + 16;

    if (!g_session_active)
    {
        GuiLabel((Rectangle){50, cy, 180, 40}, "Select Exam:");
        DrawComboBox((Rectangle){240, cy, 510, 48},
                     "Math 101;C Final;History 201", &g_active_combo);
        cy += 80;

        if (DrawButton((Rectangle){280, cy, 240, 56}, "Create Session"))
        {
            int exam_ids[] = {1, 2, 3};
            do_start_session(exam_ids[g_active_combo]);
        }
        cy += 80;

        if (DrawButton((Rectangle){300, cy, 200, 52}, "<- Back"))
            g_page = PAGE_ADMIN;
    }
    else
    {
        GuiSetStyle(DEFAULT, TEXT_SIZE, 28);
        GuiLabel((Rectangle){50, cy, 700, 44}, "Session is live!");
        cy += 56;
        GuiSetStyle(DEFAULT, TEXT_SIZE, 22);

        GuiLabel((Rectangle){50, cy, 700, 36},
                 TextFormat("Session ID : %d", g_session.session_id));
        cy += 46;
        GuiLabel((Rectangle){50, cy, 700, 36},
                 TextFormat("Exam ID    : %d", g_session.exam_id));
        cy += 46;
        GuiLabel((Rectangle){50, cy, 700, 36},
                 TextFormat("Duration   : %d min",
                            g_session.session_duration / 60));
        cy += 46;
        GuiLabel((Rectangle){50, cy, 700, 36},
                 "Share the Session ID with candidates.");
        cy += 70;

        if (DrawButton((Rectangle){250, cy, 300, 56}, "End Session / Back"))
        {
            memset(&g_session, 0, sizeof g_session);
            g_session_active = false;
            g_page = PAGE_ADMIN;
        }
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
}

int main(void)
{
    InitWindow(SCREEN_W, SCREEN_H, "Exam System — Client");
    SetTargetFPS(60);

    Color bgColor = GetColor(0x181818FF);

    GuiSetStyle(DEFAULT, TEXT_SIZE,            24);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL,    ColorToInt(RAYWHITE));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL,    ColorToInt(DARKGRAY));
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR,     ColorToInt(BLACK));
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL,  ColorToInt(LIGHTGRAY));
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, ColorToInt(RAYWHITE));

    GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED, ColorToInt(RAYWHITE));
    GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED, ColorToInt(DARKGRAY));

    GuiSetStyle(TOGGLE, BASE_COLOR_FOCUSED, ColorToInt(RAYWHITE));
    GuiSetStyle(TOGGLE, TEXT_COLOR_FOCUSED, ColorToInt(DARKGRAY));
    GuiSetStyle(TOGGLE, BASE_COLOR_PRESSED, ColorToInt(RAYWHITE));
    GuiSetStyle(TOGGLE, TEXT_COLOR_PRESSED, ColorToInt(DARKGRAY));

    GuiSetStyle(COMBOBOX, BASE_COLOR_FOCUSED, ColorToInt(RAYWHITE));
    GuiSetStyle(COMBOBOX, TEXT_COLOR_FOCUSED, ColorToInt(DARKGRAY));

    memset(&g_user, 0, sizeof g_user);

    while (!WindowShouldClose())
    {
        g_hover = false;

        BeginDrawing();
        ClearBackground(bgColor);

        switch (g_page)
        {
            case PAGE_LOGIN:        page_login();        break;
            case PAGE_CANDIDATE:    page_candidate();    break;
            case PAGE_EXAM:         page_exam();         break;
            case PAGE_SUBMIT:       page_submit();       break;
            case PAGE_ADMIN:        page_admin();        break;
            case PAGE_CREATE_SETUP: page_create_setup(); break;
            case PAGE_CREATE_Q:     page_create_q();     break;
            case PAGE_SESSION:      page_session();      break;
        }

        draw_error();

        SetMouseCursor(g_hover ? MOUSE_CURSOR_POINTING_HAND
                               : MOUSE_CURSOR_DEFAULT);
        EndDrawing();
    }

    server_disconnect();
    CloseWindow();
    return 0;
}
