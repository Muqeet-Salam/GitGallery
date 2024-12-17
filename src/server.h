#ifndef SERVER_H
#define SERVER_H

#include <microhttpd.h>

struct MHD_Daemon *server_start(unsigned short port);
void server_stop(struct MHD_Daemon *daemon);

#endif // SERVER_H
