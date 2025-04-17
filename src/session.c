#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <stdlib.h>

#include "session.h"
#include "logger.h"
#include "data_transfer.h"

void init_session(ftp_session *session, int client_sock) 
{
    session->client_sock = client_sock;
    session->authenticated = 0;
    memset(session->username, 0, sizeof(session->username));
    
    const char *default_root = "/home/kirill/ftp_root";         //TODO: replace into config file
    strncpy(session->root_dir, default_root, sizeof(session->root_dir) - 1);
    
    struct stat st;
    if (stat(session->root_dir, &st) != 0 || !S_ISDIR(st.st_mode))
    {
        log_errorf("Root directory %s does not exist or is not a directory", session->root_dir);
        exit(EXIT_FAILURE);
    }
    
    strncpy(session->cwd, "/", sizeof(session->cwd) - 1);

    session->data_sock = -1;
    memset(&session->data_addr, 0, sizeof(session->data_addr));
    session->data_connection = NONE;
    session->transfer_type = TYPE_ASCII;

    log_infof("Session initialized for client %d with root_dir: %s", client_sock, session->root_dir);
}

void send_response(ftp_session *session, const char *response) 
{
    if (send(session->client_sock, response, strlen(response), 0) < 0) 
        log_errorf("Failed to send response to client %d", session->client_sock);
    else
        log_infof("Sent to client %d: %s", session->client_sock, response);
    
}

int authenticate_user(ftp_session *session, const char *username, const char *password) //TODO: fix auth
{
    if (strcmp(username, "test") == 0 && strcmp(password, "test") == 0) 
    {
        session->authenticated = 1;
        strncpy(session->username, username, sizeof(session->username) - 1);
        log_infof("User %s authenticated", username);
        return 1;
    }
    log_infof("Authentication failed for user %s", username);
    return 0;
}

int change_directory(ftp_session *session, const char *new_dir)
{
    char full_path[PATH_MAX];
    char resolved_path[PATH_MAX];

    if (strlen(new_dir) >= PATH_LEN)
    {
        log_infof("Directory path too long: %s (max length: %d)", new_dir, PATH_LEN - 1);
        return -1;
    }

    if (new_dir[0] == '/') 
    {
        snprintf(full_path, sizeof(full_path), "%s%s", session->root_dir, new_dir);
        log_infof("path: %s", full_path);
    } 
    else 
    {
        snprintf(full_path, sizeof(full_path), "%s%s/%s", session->root_dir, session->cwd, new_dir);
        log_infof("path: %s", full_path);
    }

    if (realpath(full_path, resolved_path) == NULL) 
    {
        log_infof("Invalid directory: %s", new_dir);
        return -1;
    }

    if (strncmp(resolved_path, session->root_dir, strlen(session->root_dir)) != 0) 
    {
        log_infof("Access denied: %s is outside root_dir %s", resolved_path, session->root_dir);
        return -1;
    }

    struct stat st;
    if (stat(resolved_path, &st) != 0 || !S_ISDIR(st.st_mode)) 
    {
        log_infof("Not a directory: %s", resolved_path);
        return -1;
    }

    char *relative_path = resolved_path + strlen(session->root_dir);
    if (strlen(relative_path) == 0)
        strcpy(session->cwd, "/");
    else
        strncpy(session->cwd, relative_path, sizeof(session->cwd) - 1);
    
    log_infof("Changed directory to %s", session->cwd);
    return 0;
}

void close_session(ftp_session *session)
{
    if (session->client_sock != -1) 
    {
        close(session->client_sock);
        log_infof("Session closed for client %d", session->client_sock);
        session->client_sock = -1;
    }
    close_data_connection(session);
}