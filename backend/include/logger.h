#ifndef CWEB_LOGGER_H
#define CWEB_LOGGER_H

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_ERROR = 3
} LogLevel;

void logger_init(LogLevel min_level, const char *log_file_path);
void logger_close(void);

void log_json(LogLevel level, const char *event, const char *request_id, const char *json_fields);

#endif /* CWEB_LOGGER_H */
