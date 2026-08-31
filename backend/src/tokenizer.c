#include "tokenizer.h"
#include "config/config.h"
#include "normalize.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *default_stopwords[] = {
    "a", "about", "above", "after", "again", "against", "all", "am", "an", "and",
    "any", "are", "aren't", "as", "at", "be", "because", "been", "before", "being",
    "below", "between", "both", "but", "by", "can", "cannot", "could", "couldn't",
    "did", "didn't", "do", "does", "doesn't", "doing", "don't", "down", "during",
    "each", "few", "for", "from", "further", "had", "hadn't", "has", "hasn't",
    "have", "haven't", "having", "he", "he'd", "he'll", "he's", "her", "here",
    "here's", "hers", "herself", "him", "himself", "his", "how", "how's", "i",
    "i'd", "i'll", "i'm", "i've", "if", "in", "into", "is", "isn't", "it", "it's",
    "its", "itself", "let's", "me", "more", "most", "mustn't", "my", "myself",
    "no", "nor", "not", "of", "off", "on", "once", "only", "or", "other", "ought",
    "our", "ours", "ourselves", "out", "over", "own", "same", "shan't", "she",
    "she'd", "she'll", "she's", "should", "shouldn't", "so", "some", "such",
    "than", "that", "that's", "the", "their", "theirs", "them", "themselves",
    "then", "there", "there's", "these", "they", "they'd", "they'll", "they're",
    "they've", "this", "those", "through", "to", "too", "under", "until", "up",
    "very", "was", "wasn't", "we", "we'd", "we'll", "we're", "we've", "were",
    "weren't", "what", "what's", "when", "when's", "where", "where's", "which",
    "while", "who", "who's", "whom", "why", "why's", "with", "won't", "would",
    "wouldn't", "you", "you'd", "you'll", "you're", "you've", "your", "yours",
    "yourself", "yourselves", NULL
};

static char **custom_stopwords = NULL;
static size_t custom_stopword_count = 0;

int tokenizer_is_stopword(const char *token) {
    if (!token) return 0;
    if (custom_stopwords) {
        for (size_t i = 0; i < custom_stopword_count; i++) {
            if (strcmp(custom_stopwords[i], token) == 0) return 1;
        }
        return 0;
    }
    for (size_t i = 0; default_stopwords[i] != NULL; i++) {
        if (strcmp(default_stopwords[i], token) == 0) return 1;
    }
    return 0;
}

int tokenizer_load_stopwords(const char *filepath) {
    if (!filepath) return -1;
    FILE *fp = fopen(filepath, "r");
    if (!fp) return -1;

    char line[128];
    size_t cap = 64;
    char **list = (char **)calloc(cap, sizeof(char *));
    size_t count = 0;

    while (fgets(line, sizeof(line), fp)) {
        /* strip newline */
        char *p = line;
        while (*p && *p != '\r' && *p != '\n') p++;
        *p = '\0';
        p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p) {
            if (count + 1 >= cap) {
                cap *= 2;
                list = (char **)realloc(list, cap * sizeof(char *));
            }
            list[count++] = strdup(p);
        }
    }
    fclose(fp);

    if (custom_stopwords) {
        for (size_t i = 0; i < custom_stopword_count; i++) free(custom_stopwords[i]);
        free(custom_stopwords);
    }
    custom_stopwords = list;
    custom_stopword_count = count;
    return 0;
}

TokenStream *token_stream_create(void) {
    TokenStream *ts = (TokenStream *)calloc(1, sizeof(TokenStream));
    if (!ts) return NULL;
    ts->capacity = 32;
    ts->tokens = (Token *)calloc(ts->capacity, sizeof(Token));
    ts->count = 0;
    return ts;
}

void token_stream_free(TokenStream *stream) {
    if (!stream) return;
    for (size_t i = 0; i < stream->count; i++) {
        free(stream->tokens[i].text);
    }
    free(stream->tokens);
    free(stream);
}

int token_stream_add(TokenStream *stream, const char *text, int position, DocumentField field) {
    if (!stream || !text) return -1;
    if (stream->count >= stream->capacity) {
        size_t new_cap = stream->capacity * 2;
        Token *new_tokens = (Token *)realloc(stream->tokens, new_cap * sizeof(Token));
        if (!new_tokens) return -1;
        stream->tokens = new_tokens;
        stream->capacity = new_cap;
    }
    stream->tokens[stream->count].text = strdup(text);
    stream->tokens[stream->count].position = position;
    stream->tokens[stream->count].field = field;
    stream->count++;
    return 0;
}

static int is_punctuation(char c) {
    return (c == '.' || c == ',' || c == ';' || c == ':' || c == '!' ||
            c == '?' || c == '(' || c == ')' || c == '[' || c == ']' ||
            c == '{' || c == '}' || c == '"' || c == '/' || c == '\\' ||
            c == '<' || c == '>' || c == '=' || c == '+' || c == '*' ||
            c == '&' || c == '|' || c == '^' || c == '%' || c == '$' ||
            c == '#' || c == '@' || c == '~' || c == '`');
}

TokenStream *tokenize_text(const char *text, DocumentField field, int *position_counter) {
    TokenStream *ts = token_stream_create();
    if (!text || !ts) return ts;

    const char *p = text;
    char token_buf[128];
    size_t buf_len = 0;

    int pos = (position_counter != NULL) ? *position_counter : 1;

    while (*p) {
        unsigned char c = (unsigned char)*p;

        /* UTF-8 multi-byte pass-through (lead byte >= 0x80) */
        if (c >= 0x80) {
            int utf8_bytes = 1;
            if ((c & 0xE0) == 0xC0) utf8_bytes = 2;
            else if ((c & 0xF0) == 0xE0) utf8_bytes = 3;
            else if ((c & 0xF8) == 0xF0) utf8_bytes = 4;

            for (int b = 0; b < utf8_bytes && *p; b++) {
                if (buf_len + 1 < sizeof(token_buf)) {
                    token_buf[buf_len++] = *p;
                }
                p++;
            }
            continue;
        }

        /* Check for delimiter or whitespace or punctuation */
        if (isspace(c) || is_punctuation(c)) {
            if (buf_len > 0) {
                token_buf[buf_len] = '\0';
                
                /* Length check */
                if (buf_len >= CWEB_MIN_TOKEN_LEN && buf_len <= CWEB_MAX_TOKEN_LEN) {
                    normalize_token(token_buf);
                    if (strlen(token_buf) >= CWEB_MIN_TOKEN_LEN) {
                        token_stream_add(ts, token_buf, pos, field);
                    }
                }
                pos++;
                buf_len = 0;
            }
            p++;
            continue;
        }

        /* Internal hyphen/apostrophe e.g. co-worker, client's */
        if ((c == '-' || c == '\'') && buf_len > 0 && isalnum((unsigned char)*(p + 1))) {
            if (buf_len + 1 < sizeof(token_buf)) {
                token_buf[buf_len++] = c;
            }
            p++;
            continue;
        }

        /* Normal ASCII char */
        if (buf_len + 1 < sizeof(token_buf)) {
            token_buf[buf_len++] = (char)tolower(c);
        }
        p++;
    }

    /* Final token flush */
    if (buf_len > 0) {
        token_buf[buf_len] = '\0';
        if (buf_len >= CWEB_MIN_TOKEN_LEN && buf_len <= CWEB_MAX_TOKEN_LEN) {
            normalize_token(token_buf);
            if (strlen(token_buf) >= CWEB_MIN_TOKEN_LEN) {
                token_stream_add(ts, token_buf, pos, field);
            }
        }
        pos++;
    }

    if (position_counter) *position_counter = pos;
    return ts;
}

TokenStream *tokenize_document(const Document *doc) {
    if (!doc) return NULL;

    TokenStream *unified = token_stream_create();
    int position_counter = 1;

    /* 1. Title */
    if (doc->title) {
        TokenStream *ts = tokenize_text(doc->title, FIELD_TITLE, &position_counter);
        for (size_t i = 0; i < ts->count; i++) {
            token_stream_add(unified, ts->tokens[i].text, ts->tokens[i].position, FIELD_TITLE);
        }
        token_stream_free(ts);
    }

    /* 2. Headings */
    if (doc->headings) {
        for (size_t i = 0; doc->headings[i] != NULL; i++) {
            TokenStream *ts = tokenize_text(doc->headings[i], FIELD_HEADING, &position_counter);
            for (size_t k = 0; k < ts->count; k++) {
                token_stream_add(unified, ts->tokens[k].text, ts->tokens[k].position, FIELD_HEADING);
            }
            token_stream_free(ts);
        }
    }

    /* 3. Keywords */
    if (doc->keywords) {
        for (size_t i = 0; doc->keywords[i] != NULL; i++) {
            TokenStream *ts = tokenize_text(doc->keywords[i], FIELD_KEYWORDS, &position_counter);
            for (size_t k = 0; k < ts->count; k++) {
                token_stream_add(unified, ts->tokens[k].text, ts->tokens[k].position, FIELD_KEYWORDS);
            }
            token_stream_free(ts);
        }
    }

    /* 4. Description */
    if (doc->description) {
        TokenStream *ts = tokenize_text(doc->description, FIELD_DESCRIPTION, &position_counter);
        for (size_t i = 0; i < ts->count; i++) {
            token_stream_add(unified, ts->tokens[i].text, ts->tokens[i].position, FIELD_DESCRIPTION);
        }
        token_stream_free(ts);
    }

    /* 5. Body */
    if (doc->body_text) {
        TokenStream *ts = tokenize_text(doc->body_text, FIELD_BODY, &position_counter);
        for (size_t i = 0; i < ts->count; i++) {
            token_stream_add(unified, ts->tokens[i].text, ts->tokens[i].position, FIELD_BODY);
        }
        token_stream_free(ts);
    }

    return unified;
}
