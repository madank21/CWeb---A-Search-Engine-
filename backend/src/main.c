#include "config/config.h"
#include "document.h"
#include "html_parser.h"
#include "http_server.h"
#include "index.h"
#include "logger.h"
#include "persistence.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir_compat(path) _mkdir(path)
#else
#define mkdir_compat(path) mkdir(path, 0755)
#endif

static void handle_signal(int sig) {
    (void)sig;
    http_server_stop();
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    logger_init(LOG_LEVEL_INFO, "cweb_server.log");
    log_json(LOG_LEVEL_INFO, "startup", "main", "\"message\":\"Starting CWeb v2.0 Search Engine Server...\"");

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    mkdir_compat(CWEB_DATA_DIR);
    mkdir_compat(CWEB_PAGES_DIR);
    mkdir_compat(CWEB_INDEX_DIR);

    /* Try loading binary index from disk first */
    InvertedIndex *idx = persistence_load_index(CWEB_INDEX_DIR);
    if (!idx) {
        log_json(LOG_LEVEL_INFO, "index_load", "main", "\"status\":\"Disk index missing or corrupt; building from corpus...\"");
        idx = index_create();

        /* Index documents in data/pages */
        int indexed_count = 0;
        for (int id = 1; id <= 52; id++) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/page_%d.html", CWEB_PAGES_DIR, id);

            FILE *fp = fopen(filepath, "rb");
            if (fp) {
                fseek(fp, 0, SEEK_END);
                long len = ftell(fp);
                fseek(fp, 0, SEEK_SET);

                char *buf = (char *)malloc(len + 1);
                if (buf) {
                    if (fread(buf, 1, len, fp) == (size_t)len) {
                        buf[len] = '\0';
                        Document *doc = document_create();
                        doc->id = id;
                        doc->path = strdup(filepath);
                        doc->file_size = len;
                        doc->modified_time = time(NULL);

                        html_parse_document(buf, len, doc);
                        index_add_document(idx, doc);
                        indexed_count++;
                    }
                    free(buf);
                }
                fclose(fp);
            }
        }

        if (indexed_count > 0) {
            persistence_save_index(idx, CWEB_INDEX_DIR);
        }
    }

    index_atomic_swap(idx);

    HttpServerConfig cfg;
    cfg.host = CWEB_DEFAULT_HOST;
    cfg.port = CWEB_DEFAULT_PORT;
    cfg.index = idx;
    cfg.pages_dir = CWEB_PAGES_DIR;
    cfg.index_dir = CWEB_INDEX_DIR;

    printf("====================================================\n");
    printf(" CWeb Search Engine Backend v2.0 Listening on :%d\n", CWEB_DEFAULT_PORT);
    printf("====================================================\n");

    http_server_start(&cfg);

    logger_close();
    return 0;
}
