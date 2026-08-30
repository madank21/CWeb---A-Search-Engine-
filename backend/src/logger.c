#include "logger.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static LogLevel g_min_level = LOG_LEVEL_INFO;
static FILE *g_log_fp = NULL;
static pthread_mutex_t g_log_lock = PTHREAD_MUTEX_INITIALIZER;

static const char *level_names[] = { "DEBUG", "INFO", "WARN", "ERROR" };

void logger_init(LogLevel min_level, const char *log_file_path) {
    pthread_mutex_lock(&g_log_lock);
    g_min_level = min_level;
    if (log_file_path && *log_file_path) {
        g_log_fp = fopen(log_file_path, "a");
    }
    if (!g_log_fp) {
        g_log_fp = stdout;
    }
    pthread_mutex_unlock(&g_log_lock);
}

void logger_close(void) {
    pthread_mutex_lock(&g_log_lock);
    if (g_log_fp && g_log_fp != stdout && g_log_fp != stderr) {
        fclose(g_log_fp);
        g_log_fp = NULL;
    }
    pthread_mutex_unlock(&g_log_lock);
}

void log_json(LogLevel level, const char *event, const char *request_id, const char *json_fields) {
    if (level < g_min_level) return;

    time_t now = time(NULL);
    struct tm tm_info;
#ifdef _WIN32
    gmtime_s(&tm_info, &now);
#else
    gmtime_r(&now, &tm_info);
#endif

    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &tm_info);

    pthread_mutex_lock(&g_log_lock);
    FILE *out = g_log_fp ? g_log_fp : stdout;

    fprintf(out, "{\"ts\":\"%s\",\"level\":\"%s\",\"event\":\"%s\",\"request_id\":\"%s\"",
            ts, level_names[level], event ? event : "generic", request_id ? request_id : "system");

    if (json_fields && *json_fields) {
        fprintf(out, ",%s", json_fields);
    }

    fprintf(out, "}\n");
    fflush(out);
    pthread_mutex_unlock(&g_log_lock);
}
