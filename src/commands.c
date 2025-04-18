#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <libgen.h>

#include "commands.h"
#include "session.h"
#include "data_transfer.h"
#include "logger.h"
#include "auth.h"

static ftp_command command_table[] = 
{
    {"USER", cmd_user},
    {"PASS", cmd_pass},
    {"QUIT", cmd_quit},
    {"PWD",  cmd_pwd },
    {"CWD",  cmd_cwd },
    {"PORT", cmd_port},
    {"LIST", cmd_list},
    {"PASV", cmd_pasv},
    {"TYPE", cmd_type},
    {"STOR", cmd_stor},
    {"RETR", cmd_retr},
    {NULL, NULL}
};

static int is_path_safe(const char *root_dir, const char *path) 
{
    // Копируем путь, чтобы не изменять оригинал
    char path_copy[PATH_MAX];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    // Получаем родительскую директорию
    char *parent_dir = dirname(path_copy);
    if (!parent_dir) 
    {
        log_errorf("Failed to get parent directory for path: %s", path);
        return 0;
    }

    // Проверяем, существует ли родительская директория
    char resolved_path[PATH_MAX];
    if (realpath(parent_dir, resolved_path) == NULL) 
    {
        log_errorf("Parent directory does not exist or is inaccessible: %s", parent_dir);
        return 0;
    }

    // Проверяем, что родительская директория находится в пределах root_dir
    if (strncmp(root_dir, resolved_path, strlen(root_dir)) != 0) 
    {
        log_errorf("Path outside root directory: %s", resolved_path);
        return 0;
    }

    return 1;
}

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

int cmd_user(ftp_session *session, parsed_command *parsed_cmd) 
{
    if (!parsed_cmd->args) 
    {
        log_errorf("No username provided for USER command");
        send_response(session, "501 Syntax error in parameters.\r\n");
        return 0;
    }

    if (strlen(parsed_cmd->args) >= sizeof(session->username)) 
    {
        log_errorf("Username too long: %s", parsed_cmd->args);
        send_response(session, "501 Username too long.\r\n");
        return 0;
    }

    if (!user_exists(parsed_cmd->args)) 
    {
        log_infof("Unknown user: %s", parsed_cmd->args);
        send_response(session, "530 Not logged in.\r\n");
        return 0;
    }

    strncpy(session->username, parsed_cmd->args, sizeof(session->username) - 1);
    session->username[sizeof(session->username) - 1] = '\0';
    log_infof("User %s provided, awaiting password", session->username);
    send_response(session, "331 User name okay, need password.\r\n");
    return 0;
}

int cmd_pass(ftp_session *session, parsed_command *parsed_cmd) 
{
    if (!parsed_cmd->args) 
    {
        log_errorf("No password provided for PASS command");
        send_response(session, "501 Syntax error in parameters.\r\n");
        return 0;
    }

    if (strlen(session->username) == 0) 
    {
        log_errorf("No username provided before PASS");
        send_response(session, "503 Bad sequence of commands.\r\n");
        return 0;
    }

    if (authenticate(session, session->username, parsed_cmd->args)) 
    {
        send_response(session, "230 User logged in, proceed.\r\n");
    } 
    else 
    {
        send_response(session, "530 Invalid password.\r\n");
        session->username[0] = '\0';
    }
    
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

int cmd_port(ftp_session *session, parsed_command *parsed) 
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

    unsigned int h1, h2, h3, h4, p1, p2;
    if (sscanf(parsed->args, "%u,%u,%u,%u,%u,%u", &h1, &h2, &h3, &h4, &p1, &p2) != 6) 
    {
        send_response(session, "501 Invalid PORT format.\r\n");
        return 0;
    }

    if (h1 > 255 || h2 > 255 || h3 > 255 || h4 > 255 || p1 > 255 || p2 > 255) 
    {
        send_response(session, "501 Invalid PORT values.\r\n");
        return 0;
    }

    memset(&session->data_addr, 0, sizeof(session->data_addr));
    session->data_addr.sin_family = AF_INET;
    session->data_addr.sin_addr.s_addr = htonl((h1 << 24) | (h2 << 16) | (h3 << 8) | h4);
    session->data_addr.sin_port = htons((p1 * 256) + p2);

    session->data_connection = ACTIVE;
    char ip_str[16];
    inet_ntop(AF_INET, &session->data_addr.sin_addr, ip_str, sizeof(ip_str));
    log_infof("PORT command: IP=%s, Port=%d", ip_str, ntohs(session->data_addr.sin_port));
    send_response(session, "200 PORT command successful.\r\n");
    return 0;
}

int cmd_pasv(ftp_session *session, parsed_command *parsed) 
{
    (void)parsed;
    if (!session->authenticated) 
    {
        send_response(session, "530 Please login with USER and PASS.\r\n");
        return 0;
    }

    close_data_connection(session);

    session->data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (session->data_sock < 0) 
    {
        log_errorf("Failed to create PASV socket");
        send_response(session, "425 Can't open data connection.\r\n");
        return 0;
    }

    struct sockaddr_in pasv_addr;
    pasv_addr.sin_family = AF_INET;
    pasv_addr.sin_addr.s_addr = INADDR_ANY;
    pasv_addr.sin_port = 0;

    if (bind(session->data_sock, (struct sockaddr*)&pasv_addr, sizeof(pasv_addr)) < 0) 
    {
        log_errorf("PASV bind failed: %s", strerror(errno));
        close(session->data_sock);
        session->data_sock = -1;
        send_response(session, "425 Can't open data connection.\r\n");
        return 0;
    }

    if (listen(session->data_sock, 1) < 0) 
    {
        log_errorf("PASV listen failed: %s", strerror(errno));
        close(session->data_sock);
        session->data_sock = -1;
        send_response(session, "425 Can't open data connection.\r\n");
        return 0;
    }

    socklen_t addr_len = sizeof(pasv_addr);
    if (getsockname(session->data_sock, (struct sockaddr*)&pasv_addr, &addr_len) < 0) 
    {
        log_errorf("Failed to get PASV port: %s", strerror(errno));
        close(session->data_sock);
        session->data_sock = -1;
        send_response(session, "425 Can't open data connection.\r\n");
        return 0;
    }

    unsigned int port = ntohs(pasv_addr.sin_port);
    unsigned int p1 = port / 256;
    unsigned int p2 = port % 256;

    char response[BUFFER_SIZE];
    //snprintf(response, sizeof(response), "227 Entering Passive Mode (192,168,119,16,%u,%u).\r\n", p1, p2); //TODO: replace on real ip
    snprintf(response, sizeof(response), "227 Entering Passive Mode (127,0,0,1,%u,%u).\r\n", p1, p2);
    send_response(session, response);

    session->data_connection = PASSIVE;
    log_infof("PASV mode enabled, listening on port %u", port);
    return 0;
}

int cmd_type(ftp_session *session, parsed_command *parsed) 
{
    if (!session->authenticated) 
    {
        send_response(session, "530 Please login with USER and PASS.\r\n"); //TODO: divide into funcs
        return 0;
    }
    if (!parsed->args) 
    {
        send_response(session, "501 Syntax error in parameters.\r\n");
        return 0;
    }

    if (strcmp(parsed->args, "A") == 0) 
    {
        session->transfer_type = TYPE_ASCII;
        log_infof("Transfer type set to ASCII");
        send_response(session, "200 Type set to A.\r\n");
    }
    else if (strcmp(parsed->args, "I") == 0) 
    {
        session->transfer_type = TYPE_BINARY;
        log_infof("Transfer type set to Binary");
        send_response(session, "200 Type set to I.\r\n");
    }
    else 
    {
        log_errorf("Invalid TYPE parameter: %s", parsed->args);
        send_response(session, "504 Command not implemented for that parameter.\r\n");
    }
    return 0;
}

static void format_file_entry(char *line, size_t line_size, const char *name, const struct stat *st) 
{
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", localtime(&st->st_mtime));
    snprintf(line, line_size, "%s %8ld %s %s\r\n",
             (S_ISDIR(st->st_mode) ? "drwxr-xr-x" : "-rw-r--r--"), // Упрощённые права
             (long)st->st_size, time_buf, name);
}

int cmd_list(ftp_session *session, parsed_command *parsed) 
{
    if (!session->authenticated) 
    {
        send_response(session, "530 Please login with USER and PASS.\r\n");
        return 0;
    }

    if (session->data_connection == NONE) 
    {
        send_response(session, "503 Bad sequence of commands. Use PORT or PASV first.\r\n");
        return 0;
    }

    // Определяем директорию для листинга
    char full_path[PATH_MAX];
    if (parsed->args) 
    {
        // Проверяем, является ли аргумент корректной директорией
        snprintf(full_path, sizeof(full_path), "%s%s", session->root_dir, parsed->args);
        struct stat st;
        if (stat(full_path, &st) != 0 || !S_ISDIR(st.st_mode)) 
        {
            log_errorf("Invalid directory for LIST: %s", full_path);
            send_response(session, "550 Requested directory not found.\r\n");
            return 0;
        }
    }
    else 
    {
        snprintf(full_path, sizeof(full_path), "%s%s", session->root_dir, session->cwd);
    }

    if (setup_data_connection(session) != 0) 
    {
        log_errorf("Failed to setup data connection for LIST");
        send_response(session, "425 Can't open data connection.\r\n");
        return 0;
    }

    send_response(session, "150 Opening ASCII mode data connection for file list.\r\n");

    // Открываем директорию
    DIR *dir = opendir(full_path);
    if (!dir) 
    {
        log_errorf("Failed to open directory %s: %s", full_path, strerror(errno));
        send_response(session, "550 Failed to open directory.\r\n");
        close_data_connection(session);
        return 0;
    }

    // Читаем записи и отправляем по одной
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
            continue;

        // Формируем полный путь к файлу
        char file_path[PATH_MAX*2];
        snprintf(file_path, sizeof(file_path), "%s/%s", full_path, entry->d_name);

        // Получаем метаданные файла
        struct stat st;
        if (stat(file_path, &st) < 0) 
        {
            log_errorf("Failed to stat file %s: %s", entry->d_name, strerror(errno));
            continue;
        }

        // Формируем строку в стиле ls -l
        char line[BUFFER_SIZE];
        format_file_entry(line, sizeof(line), entry->d_name, &st);

        // Отправляем строку
        if (send_data(session, line) != 0) 
        {
            log_errorf("Failed to send LIST data for %s", entry->d_name);
            send_response(session, "426 Data transfer aborted.\r\n");
            closedir(dir);
            close_data_connection(session);
            return 0;
        }
    }
    closedir(dir);

    close_data_connection(session);
    send_response(session, "226 Transfer complete.\r\n");
    return 0;
}

int cmd_stor(ftp_session *session, parsed_command *parsed) 
{
    if (!session->authenticated) 
    {
        send_response(session, "530 Please login with USER and PASS.\r\n");
        return 0;
    }

    if (session->data_connection == NONE) 
    {
        send_response(session, "503 Bad sequence of commands. Use PORT or PASV first.\r\n");
        return 0;
    }

    if (!parsed->args) 
    {
        send_response(session, "501 Syntax error in parameters.\r\n");
        return 0;
    }

    // Формируем полный путь к файлу
    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s%s/%s", session->root_dir, session->cwd, parsed->args);

    // Проверяем безопасность пути
    if (!is_path_safe(session->root_dir, full_path)) 
    {
        log_errorf("Unsafe file path for STOR: %s", full_path);
        send_response(session, "550 Permission denied.\r\n");
        return 0;
    }

    // Устанавливаем соединение
    if (setup_data_connection(session) != 0) 
    {
        log_errorf("Failed to setup data connection for STOR");
        send_response(session, "425 Can't open data connection.\r\n");
        return 0;
    }

    // Открываем файл для записи
    int fd = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) 
    {
        log_errorf("Failed to open file %s for writing: %s", full_path, strerror(errno));
        send_response(session, "550 Failed to open file.\r\n");
        close_data_connection(session);
        return 0;
    }

    send_response(session, "150 Opening data connection for file transfer.\r\n");

    // Приём данных
    if (session->transfer_type == TYPE_ASCII) 
    {
        char c, prev = 0;
        ssize_t n;
        while ((n = recv(session->data_sock, &c, 1, 0)) > 0) 
        {
            if (c == '\n' && prev == '\r') 
            {
                if (write(fd, "\n", 1) < 0) 
                {
                    log_errorf("Failed to write to file %s: %s", full_path, strerror(errno));
                    send_response(session, "426 Data transfer aborted.\r\n");
                    close(fd);
                    close_data_connection(session);
                    return 0;
                }
            } 
            else if (c != '\r') 
            {
                if (write(fd, &c, 1) < 0) 
                {
                    log_errorf("Failed to write to file %s: %s", full_path, strerror(errno));
                    send_response(session, "426 Data transfer aborted.\r\n");
                    close(fd);
                    close_data_connection(session);
                    return 0;
                }
            }
            prev = c;
        }
        if (n < 0) 
        {
            log_errorf("Failed to receive data for STOR: %s", strerror(errno));
            send_response(session, "426 Data transfer aborted.\r\n");
            close(fd);
            close_data_connection(session);
            return 0;
        }
    } 
    else // TYPE_BINARY
    {
        char buf[4096];
        ssize_t n;
        while ((n = recv(session->data_sock, buf, sizeof(buf), 0)) > 0) 
        {
            if (write(fd, buf, n) < 0) 
            {
                log_errorf("Failed to write to file %s: %s", full_path, strerror(errno));
                send_response(session, "426 Data transfer aborted.\r\n");
                close(fd);
                close_data_connection(session);
                return 0;
            }
        }
        if (n < 0) 
        {
            log_errorf("Failed to receive data for STOR: %s", strerror(errno));
            send_response(session, "426 Data transfer aborted.\r\n");
            close(fd);
            close_data_connection(session);
            return 0;
        }
    }

    close(fd);
    close_data_connection(session);
    send_response(session, "226 Transfer complete.\r\n");
    log_infof("File %s stored successfully", full_path);
    return 0;
}

int cmd_retr(ftp_session *session, parsed_command *parsed) 
{
    if (!session->authenticated) 
    {
        send_response(session, "530 Please login with USER and PASS.\r\n");
        return 0;
    }

    if (session->data_connection == NONE) 
    {
        send_response(session, "503 Bad sequence of commands. Use PORT or PASV first.\r\n");
        return 0;
    }

    if (!parsed->args) 
    {
        send_response(session, "501 Syntax error in parameters.\r\n");
        return 0;
    }

    // Формируем полный путь к файлу
    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s%s/%s", session->root_dir, session->cwd, parsed->args);

    // Проверяем безопасность пути
    if (!is_path_safe(session->root_dir, full_path)) 
    {
        log_errorf("Unsafe file path for RETR: %s", full_path);
        send_response(session, "550 Permission denied.\r\n");
        return 0;
    }

    // Проверяем существование файла
    struct stat st;
    if (stat(full_path, &st) != 0 || !S_ISREG(st.st_mode)) 
    {
        log_errorf("File %s does not exist or is not a regular file", full_path);
        send_response(session, "550 File not found.\r\n");
        return 0;
    }

    // Устанавливаем соединение
    if (setup_data_connection(session) != 0) 
    {
        log_errorf("Failed to setup data connection for RETR");
        send_response(session, "425 Can't open data connection.\r\n");
        return 0;
    }

    // Открываем файл для чтения
    int fd = open(full_path, O_RDONLY);
    if (fd < 0) 
    {
        log_errorf("Failed to open file %s for reading: %s", full_path, strerror(errno));
        send_response(session, "550 Failed to open file.\r\n");
        close_data_connection(session);
        return 0;
    }

    send_response(session, "150 Opening data connection for file transfer.\r\n");

    // Отправка данных
    if (session->transfer_type == TYPE_ASCII) 
    {
        char c;
        ssize_t n;
        while ((n = read(fd, &c, 1)) > 0) 
        {
            if (c == '\n') 
            {
                if (send(session->data_sock, "\r\n", 2, 0) < 0) 
                {
                    log_errorf("Failed to send data for RETR: %s", strerror(errno));
                    send_response(session, "426 Data transfer aborted.\r\n");
                    close(fd);
                    close_data_connection(session);
                    return 0;
                }
            } 
            else 
            {
                if (send(session->data_sock, &c, 1, 0) < 0) 
                {
                    log_errorf("Failed to send data for RETR: %s", strerror(errno));
                    send_response(session, "426 Data transfer aborted.\r\n");
                    close(fd);
                    close_data_connection(session);
                    return 0;
                }
            }
        }
        if (n < 0) 
        {
            log_errorf("Failed to read file %s: %s", full_path, strerror(errno));
            send_response(session, "426 Data transfer aborted.\r\n");
            close(fd);
            close_data_connection(session);
            return 0;
        }
    } 
    else if (session->transfer_type == TYPE_BINARY)
    {
        char buf[4096];
        ssize_t n;
        while ((n = read(fd, buf, sizeof(buf))) > 0) 
        {
            if (send(session->data_sock, buf, n, 0) < 0) 
            {
                log_errorf("Failed to send data for RETR: %s", strerror(errno));
                send_response(session, "426 Data transfer aborted.\r\n");
                close(fd);
                close_data_connection(session);
                return 0;
            }
        }
        if (n < 0) 
        {
            log_errorf("Failed to read file %s: %s", full_path, strerror(errno));
            send_response(session, "426 Data transfer aborted.\r\n");
            close(fd);
            close_data_connection(session);
            return 0;
        }
    }

    close(fd);
    close_data_connection(session);
    send_response(session, "226 Transfer complete.\r\n");
    log_infof("File %s retrieved successfully", full_path);
    return 0;
}