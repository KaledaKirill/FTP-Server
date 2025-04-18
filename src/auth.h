#ifndef AUTH_H
#define AUTH_H

#include "session.h"

int init(const char *users_file);

int user_exists(const char *username);

int authenticate(ftp_session *session, const char *username, const char *password);

void auth_cleanup(void);

#endif