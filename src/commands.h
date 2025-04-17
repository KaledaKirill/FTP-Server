#ifndef COMMANDS_H
#define COMMANDS_H

#include "session.h"

typedef struct 
{
    char *command; 
    char *args;
} parsed_command;

typedef int (*command_handler)(ftp_session *session, parsed_command *parsed);

typedef struct 
{
    const char *command;
    command_handler handler;
} ftp_command;

parsed_command parse_command(char *buffer);
void free_parsed_command(parsed_command *parsed);
int process_command(ftp_session *session, char *buffer);

int cmd_user(ftp_session *session, parsed_command *parsed);
int cmd_pass(ftp_session *session, parsed_command *parsed);
int cmd_quit(ftp_session *session, parsed_command *parsed);
int cmd_cwd(ftp_session *session, parsed_command *parsed);
int cmd_pwd(ftp_session *session, parsed_command *parsed);
int cmd_port(ftp_session *session, parsed_command *parsed);
int cmd_list(ftp_session *session, parsed_command *parsed);
int cmd_pasv(ftp_session *session, parsed_command *parsed);
int cmd_type(ftp_session *session, parsed_command *parsed);
int cmd_stor(ftp_session *session, parsed_command *parsed);
int cmd_retr(ftp_session *session, parsed_command *parsed);

#endif