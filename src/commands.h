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

#endif