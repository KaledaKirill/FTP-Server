#ifndef DATA_TRANSFER_H
#define DATA_TRANSFER_H

#include "session.h"

int setup_data_connection(ftp_session *session);
int send_data(ftp_session *session, const char *data);
void close_data_connection(ftp_session *session);

#endif