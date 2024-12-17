#include "server.h"
#include "router.h"
#include <microhttpd.h>
#include <stdlib.h>
#include <stdio.h>

static enum MHD_Result request_handler(void *cls, struct MHD_Connection *connection,
                                       const char *url, const char *method,
                                       const char *version, const char *upload_data,
                                       size_t *upload_data_size, void **con_cls) {
    return router_handle_request(cls, connection, url, method, version,
                                 upload_data, upload_data_size, con_cls);
}

struct MHD_Daemon *server_start(unsigned short port) {
    return MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
                            &request_handler, NULL, MHD_OPTION_END);
}

void server_stop(struct MHD_Daemon *daemon) {
    if (daemon)
        MHD_stop_daemon(daemon);
}
