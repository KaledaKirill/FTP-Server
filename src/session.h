#ifndef SESSION_H
#define SESSION_H

#define BUFFER_SIZE 4096
#define USERNAME_LEN 32
#define PATH_MAX 4096
#define PATH_LEN 1024

typedef struct 
{
    int client_sock;
    int authenticated;
    char username[USERNAME_LEN];
    char root_dir[PATH_LEN]; 
    char cwd[PATH_LEN];
} ftp_session;

void init_session(ftp_session *session, int client_sock);
void send_response(ftp_session *session, const char *response);
int authenticate_user(ftp_session *session, const char *username, const char *password);
int change_directory(ftp_session *session, const char *new_dir);
void close_session(ftp_session *session);

#endif