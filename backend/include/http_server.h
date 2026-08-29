#ifndef CWEB_HTTP_SERVER_H
#define CWEB_HTTP_SERVER_H

#include "index.h"
#include <stddef.h>

typedef struct {
    const char   *host;
    int           port;
    InvertedIndex *index;
    const char   *pages_dir;
    const char   *index_dir;
} HttpServerConfig;

int http_server_start(const HttpServerConfig *config);
void http_server_stop(void);

#endif /* CWEB_HTTP_SERVER_H */
