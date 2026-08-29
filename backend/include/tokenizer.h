#ifndef CWEB_TOKENIZER_H
#define CWEB_TOKENIZER_H

#include "document.h"
#include <stddef.h>

typedef enum {
    FIELD_TITLE       = 0,
    FIELD_HEADING     = 1,
    FIELD_KEYWORDS    = 2,
    FIELD_DESCRIPTION = 3,
    FIELD_BODY        = 4
} DocumentField;

typedef struct {
    char         *text;
    int           position; /* 1-indexed */
    DocumentField field;
} Token;

typedef struct {
    Token *tokens;
    size_t count;
    size_t capacity;
} TokenStream;

TokenStream *token_stream_create(void);
void token_stream_free(TokenStream *stream);
int token_stream_add(TokenStream *stream, const char *text, int position, DocumentField field);

/* Loads stop words from file or uses default embedded stop word list */
int tokenizer_load_stopwords(const char *filepath);
int tokenizer_is_stopword(const char *token);

/* Tokenizes a raw text string into a TokenStream for a specific field */
TokenStream *tokenize_text(const char *text, DocumentField field, int *position_counter);

/* Tokenizes an entire Document struct into a unified TokenStream */
TokenStream *tokenize_document(const Document *doc);

#endif /* CWEB_TOKENIZER_H */
