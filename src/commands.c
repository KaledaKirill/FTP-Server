#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "commands.h"
#include "session.h"

int cmd_user(ftp_session *session, parsed_command *parced_cmd);
int cmd_pass(ftp_session *session, parsed_command *parced_cmd);
int cmd_quit(ftp_session *session, parsed_command *parced_cmd);
int cmd_cwd(ftp_session *session, parsed_command *parsed);
int cmd_pwd(ftp_session *session, parsed_command *parsed);

static ftp_command command_table[] = 
{
    {"USER", cmd_user},
    {"PASS", cmd_pass},
    {"QUIT", cmd_quit},
    {"PWD", cmd_pwd},
    {"CWD", cmd_cwd},
    {NULL, NULL}
};

parsed_command parse_command(char *buffer) 
{
    parsed_command result = 
    { 
        .command = NULL, 
        .args = NULL 
    };

    char *end = strchr(buffer, '\r');
    if (end) 
        *end = '\0';
    end = strchr(buffer, '\n');
    if (end) 
        *end = '\0';

    char *temp = strdup(buffer);
    if (!temp) 
        return result;
    
    char *token = strtok(temp, " ");
    if (token)
    {
        result.command = strdup(token);
        token = strtok(NULL, "");
        if (token) 
            result.args = strdup(token);
        
    }

    free(temp);
    return result;
}

void free_parsed_command(parsed_command *parced_cmd) 
{
    if (parced_cmd->command) free(parced_cmd->command);
    if (parced_cmd->args) free(parced_cmd->args);
}

int process_command(ftp_session *session, char *buffer) 
{
    parsed_command parced_cmd = parse_command(buffer);
    if (!parced_cmd.command) 
    {
        send_response(session, "500 Command not recognized.\r\n");
        return 0;
    }

    for (int i = 0; command_table[i].command != NULL; i++) 
    {
        if (strcmp(parced_cmd.command, command_table[i].command) == 0) 
        {
            int result = command_table[i].handler(session, &parced_cmd);
            free_parsed_command(&parced_cmd);
            return result;
        }
    }

    send_response(session, "500 Command not recognized.\r\n");
    free_parsed_command(&parced_cmd);
    return 0;
}

int cmd_user(ftp_session *session, parsed_command *parced_cmd) 
{
    if (!parced_cmd->args) 
    {
        send_response(session, "501 Syntax error in parameters.\r\n");
        return 0;
    }

    strncpy(session->username, parced_cmd->args, sizeof(session->username) - 1);
    send_response(session, "331 User name okay, need password.\r\n");
    return 0;
}

int cmd_pass(ftp_session *session, parsed_command *parced_cmd) 
{
    if (!parced_cmd->args) 
    {
        send_response(session, "501 Syntax error in parameters.\r\n");
        return 0;
    }

    if (authenticate_user(session, session->username, parced_cmd->args)) 
        send_response(session, "230 User logged in, proceed.\r\n");
    else
        send_response(session, "530 Login incorrect.\r\n");
    
    return 0;
}

int cmd_quit(ftp_session *session, parsed_command *parced_cmd) 
{
    (void)parced_cmd;
    send_response(session, "221 Goodbye.\r\n");
    return 1;
}

int cmd_cwd(ftp_session *session, parsed_command *parsed) 
{
    if (!session->authenticated) 
    {
        send_response(session, "530 Please login with USER and PASS.\r\n");
        return 0;
    }
    if (!parsed->args) 
    {
        send_response(session, "501 Syntax error in parameters.\r\n");
        return 0;
    }
    if (change_directory(session, parsed->args) == 0) 
        send_response(session, "250 Directory successfully changed.\r\n");
    else
        send_response(session, "550 Failed to change directory.\r\n");
    
    return 0;
}

int cmd_pwd(ftp_session *session, parsed_command *parsed) 
{
    (void)parsed;
    if (!session->authenticated) 
    {
        send_response(session, "530 Please login with USER and PASS.\r\n");
        return 0;
    }
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "257 \"%s\" is the current directory.\r\n", session->cwd);
    send_response(session, response);
    return 0;
}