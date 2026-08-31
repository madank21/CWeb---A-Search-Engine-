#include "snippet.h"
#include "tokenizer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



char *snippet_generate(const Document *doc, const char *query_str, size_t max_length) {
    if (!doc || !doc->body_text || max_length == 0) return strdup("");

    if (max_length < 40) max_length = 150;

    int dummy = 1;
    TokenStream *qts = tokenize_text(query_str, FIELD_BODY, &dummy);

    const char *text = doc->body_text;
    size_t text_len = strlen(text);

    /* Find best match window */
    size_t best_start = 0;
    int max_matches_in_window = 0;

    for (size_t i = 0; i < text_len; i += 20) {
        size_t window_end = (i + max_length < text_len) ? (i + max_length) : text_len;
        int matches = 0;
        if (qts) {
            for (size_t q = 0; q < qts->count; q++) {
                const char *qterm = qts->tokens[q].text;
                const char *p = text + i;
                while (p < text + window_end && (p = strstr(p, qterm)) != NULL) {
                    if (p < text + window_end) {
                        matches++;
                        p += strlen(qterm);
                    } else break;
                }
            }
        }
        if (matches > max_matches_in_window) {
            max_matches_in_window = matches;
            best_start = i;
        }
    }

    token_stream_free(qts);

    /* Adjust best_start to beginning of word */
    if (best_start > 0) {
        while (best_start > 0 && !isspace((unsigned char)text[best_start])) {
            best_start--;
        }
        if (isspace((unsigned char)text[best_start])) best_start++;
    }

    size_t snippet_len = (best_start + max_length < text_len) ? max_length : (text_len - best_start);
    char *raw = (char *)malloc(snippet_len + 1);
    if (!raw) return strdup("");
    memcpy(raw, text + best_start, snippet_len);
    raw[snippet_len] = '\0';

    /* Now build highlighted string into output buffer */
    size_t out_cap = snippet_len * 3 + 256;
    char *out = (char *)malloc(out_cap);
    if (!out) { free(raw); return strdup(""); }

    size_t out_pos = 0;
    if (best_start > 0) {
        strcpy(out, "...");
        out_pos = 3;
    } else {
        out[0] = '\0';
    }

    /* Highlight matching terms */
    int q_dummy = 1;
    TokenStream *highlight_terms = tokenize_text(query_str, FIELD_BODY, &q_dummy);

    const char *p = raw;
    while (*p) {
        int matched = 0;
        if (highlight_terms) {
            for (size_t k = 0; k < highlight_terms->count; k++) {
                const char *term = highlight_terms->tokens[k].text;
                size_t tlen = strlen(term);
                if (strncasecmp(p, term, tlen) == 0 &&
                    (!isalnum((unsigned char)p[tlen]) && p[tlen] != '-')) {
                    matched = 1;
                    if (out_pos + tlen + 16 < out_cap) {
                        strcpy(out + out_pos, "<em>");
                        out_pos += 4;
                        memcpy(out + out_pos, p, tlen);
                        out_pos += tlen;
                        strcpy(out + out_pos, "</em>");
                        out_pos += 5;
                    }
                    p += tlen;
                    break;
                }
            }
        }

        if (!matched) {
            if (out_pos + 1 < out_cap) {
                out[out_pos++] = *p;
            }
            p++;
        }
    }

    token_stream_free(highlight_terms);
    free(raw);

    if (best_start + snippet_len < text_len) {
        if (out_pos + 4 < out_cap) {
            strcpy(out + out_pos, "...");
            out_pos += 3;
        }
    }
    out[out_pos] = '\0';

    return out;
}
