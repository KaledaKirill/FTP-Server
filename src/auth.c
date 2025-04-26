#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "auth.h"
#include "logger.h"

typedef struct 
{
    char *username;
    char *password;
} user_t;

static user_t *users = NULL;
static size_t user_count = 0;

int auth_init(const char *users_file) 
{
    FILE *file = fopen(users_file, "r");
    if (!file) 
    {
        log_errorf("Failed to open users file %s", users_file);
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) 
    {
        line[strcspn(line, "\n")] = '\0'; // Удаляем \n
        if (strlen(line) == 0) continue;

        char *username = strtok(line, ":");
        char *password = strtok(NULL, ":");
        if (!username || !password) 
        {
            log_errorf("Invalid line in users file: %s", line);
            continue;
        }

        users = realloc(users, (user_count + 1) * sizeof(user_t));
        if (!users) 
        {
            log_errorf("Memory allocation failed for users");
            fclose(file);
            return -1;
        }

        users[user_count].username = strdup(username);
        users[user_count].password = strdup(password);
        if (!users[user_count].username || !users[user_count].password) 
        {
            log_errorf("Memory allocation failed for user data");
            fclose(file);
            return -1;
        }
        user_count++;
    }

    fclose(file);
    log_infof("Loaded %zu users from %s", user_count, users_file);
    return 0;
}

int user_exists(const char *username) 
{
    for (size_t i = 0; i < user_count; i++) 
    {
        if (strcmp(users[i].username, username) == 0) 
            return 1;
        
    }
    return 0;
}

int authenticate(ftp_session *session, const char *username, const char *password) 
{
    for (size_t i = 0; i < user_count; i++)
    {
        if (strcmp(users[i].username, username) == 0) 
        {
            if (strcmp(users[i].password, password) == 0) 
            {
                log_infof("Successful login for user %s", username);
                session->authenticated = 1;
                return 1;
            } 
            else 
            {
                log_infof("Failed login attempt for user %s: incorrect password", username);
                return 0;
            }
        }
    }

    log_infof("Failed login attempt for user %s: user not found", username);
    return 0;
}

void auth_cleanup(void) 
{
    for (size_t i = 0; i < user_count; i++) 
    {
        free(users[i].username);
        free(users[i].password);
    }
    free(users);
    users = NULL;
    user_count = 0;
    log_infof("Authentication module cleaned up");
}