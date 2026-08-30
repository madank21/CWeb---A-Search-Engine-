#include "html_parser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char **data;
    size_t count;
    size_t capacity;
} StringList;

static void list_init(StringList *list) {
    list->capacity = 8;
    list->count = 0;
    list->data = (char **)calloc(list->capacity, sizeof(char *));
}

static void list_append(StringList *list, const char *str) {
    if (!str || !*str) return;
    if (list->count + 1 >= list->capacity) {
        size_t new_cap = list->capacity * 2;
        char **new_data = (char **)realloc(list->data, new_cap * sizeof(char *));
        if (!new_data) return;
        list->data = new_data;
        list->capacity = new_cap;
    }
    list->data[list->count++] = strdup(str);
    list->data[list->count] = NULL;
}

static char **list_to_null_terminated(StringList *list) {
    if (!list->data) return NULL;
    char **result = list->data;
    return result;
}

static void decode_html_entities(char *str) {
    if (!str) return;
    char *src = str;
    char *dst = str;
    while (*src) {
        if (*src == '&') {
            if (strncmp(src, "&lt;", 4) == 0) { *dst++ = '<'; src += 4; }
            else if (strncmp(src, "&gt;", 4) == 0) { *dst++ = '>'; src += 4; }
            else if (strncmp(src, "&amp;", 5) == 0) { *dst++ = '&'; src += 5; }
            else if (strncmp(src, "&quot;", 6) == 0) { *dst++ = '"'; src += 6; }
            else if (strncmp(src, "&#39;", 5) == 0) { *dst++ = '\''; src += 5; }
            else if (strncmp(src, "&nbsp;", 6) == 0) { *dst++ = ' '; src += 6; }
            else { *dst++ = *src++; }
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

static void trim_whitespace(char *str) {
    if (!str || !*str) return;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

static int strcasecmp_custom(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0) return diff;
        s1++; s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

static char *get_attr_value(const char *tag_str, const char *attr_name) {
    char attr_pattern[128];
    snprintf(attr_pattern, sizeof(attr_pattern), "%s=", attr_name);
    
    const char *p = tag_str;
    while ((p = strstr(p, attr_name)) != NULL) {
        if (p > tag_str && !isspace((unsigned char)*(p - 1))) {
            p += strlen(attr_name);
            continue;
        }
        const char *eq = p + strlen(attr_name);
        while (*eq && isspace((unsigned char)*eq)) eq++;
        if (*eq == '=') {
            eq++;
            while (*eq && isspace((unsigned char)*eq)) eq++;
            char quote = '\0';
            if (*eq == '"' || *eq == '\'') {
                quote = *eq++;
            }
            const char *val_start = eq;
            const char *val_end = val_start;
            if (quote != '\0') {
                while (*val_end && *val_end != quote) val_end++;
            } else {
                while (*val_end && !isspace((unsigned char)*val_end) && *val_end != '>') val_end++;
            }
            size_t len = val_end - val_start;
            char *val = (char *)malloc(len + 1);
            if (val) {
                memcpy(val, val_start, len);
                val[len] = '\0';
                decode_html_entities(val);
                trim_whitespace(val);
                return val;
            }
        }
        p += strlen(attr_name);
    }
    return NULL;
}

int html_parse_document(const char *html_content, size_t length, Document *doc) {
    if (!html_content || !doc) return -1;

    StringList keywords_list, headings_list, links_list;
    list_init(&keywords_list);
    list_init(&headings_list);
    list_init(&links_list);

    size_t body_buf_cap = length + 256;
    char *body_buf = (char *)malloc(body_buf_cap);
    if (!body_buf) return -1;
    size_t body_len = 0;

    const char *p = html_content;
    const char *end = html_content + length;

    int in_script = 0;
    int in_style = 0;

    while (p < end) {
        /* Handle HTML Comments <!-- ... --> */
        if (p + 4 <= end && strncmp(p, "<!--", 4) == 0) {
            p += 4;
            const char *comment_end = strstr(p, "-->");
            if (comment_end) p = comment_end + 3;
            else p = end;
            continue;
        }

        /* Handle Tags <...> */
        if (*p == '<') {
            const char *tag_start = p;
            p++;
            int is_closing = 0;
            if (p < end && *p == '/') {
                is_closing = 1;
                p++;
            }
            const char *name_start = p;
            while (p < end && isalnum((unsigned char)*p)) p++;
            size_t name_len = p - name_start;
            char tag_name[64];
            if (name_len >= sizeof(tag_name)) name_len = sizeof(tag_name) - 1;
            memcpy(tag_name, name_start, name_len);
            tag_name[name_len] = '\0';

            /* Find end of tag '>' */
            const char *tag_end = strchr(tag_start, '>');
            if (!tag_end || tag_end >= end) {
                break;
            }
            size_t tag_full_len = tag_end - tag_start + 1;
            char *tag_full = (char *)malloc(tag_full_len + 1);
            if (tag_full) {
                memcpy(tag_full, tag_start, tag_full_len);
                tag_full[tag_full_len] = '\0';
            }

            p = tag_end + 1;

            if (strcasecmp_custom(tag_name, "script") == 0) {
                in_script = !is_closing;
            } else if (strcasecmp_custom(tag_name, "style") == 0) {
                in_style = !is_closing;
            } else if (!is_closing && strcasecmp_custom(tag_name, "title") == 0) {
                const char *title_start = p;
                const char *title_end = strstr(title_start, "</title>");
                if (!title_end) title_end = strstr(title_start, "</TITLE>");
                if (title_end && title_end < end) {
                    size_t tlen = title_end - title_start;
                    char *tval = (char *)malloc(tlen + 1);
                    if (tval) {
                        memcpy(tval, title_start, tlen);
                        tval[tlen] = '\0';
                        decode_html_entities(tval);
                        trim_whitespace(tval);
                        doc->title = tval;
                    }
                    p = title_end + 8;
                }
            } else if (!is_closing && strcasecmp_custom(tag_name, "meta") == 0 && tag_full) {
                char *name_attr = get_attr_value(tag_full, "name");
                char *content_attr = get_attr_value(tag_full, "content");

                if (name_attr && content_attr) {
                    if (strcasecmp_custom(name_attr, "description") == 0) {
                        doc->description = strdup(content_attr);
                    } else if (strcasecmp_custom(name_attr, "category") == 0) {
                        doc->category = strdup(content_attr);
                    } else if (strcasecmp_custom(name_attr, "keywords") == 0) {
                        /* Split by comma */
                        char *token = strtok(content_attr, ",");
                        while (token) {
                            trim_whitespace(token);
                            if (*token) list_append(&keywords_list, token);
                            token = strtok(NULL, ",");
                        }
                    }
                }
                free(name_attr);
                free(content_attr);
            } else if (!is_closing && (tolower((unsigned char)tag_name[0]) == 'h' && isdigit((unsigned char)tag_name[1]))) {
                /* Headings h1..h6 */
                char close_tag[80];
                snprintf(close_tag, sizeof(close_tag), "</%s>", tag_name);
                const char *h_start = p;
                const char *h_end = strstr(h_start, close_tag);
                if (h_end && h_end < end) {
                    size_t hlen = h_end - h_start;
                    char *hval = (char *)malloc(hlen + 1);
                    if (hval) {
                        memcpy(hval, h_start, hlen);
                        hval[hlen] = '\0';
                        decode_html_entities(hval);
                        trim_whitespace(hval);
                        if (*hval) list_append(&headings_list, hval);
                        free(hval);
                    }
                }
            } else if (!is_closing && strcasecmp_custom(tag_name, "a") == 0 && tag_full) {
                char *href = get_attr_value(tag_full, "href");
                if (href) {
                    list_append(&links_list, href);
                    free(href);
                }
            }

            free(tag_full);
            continue;
        }

        /* Non-tag text character */
        if (!in_script && !in_style) {
            char c = *p;
            if (isspace((unsigned char)c)) {
                if (body_len > 0 && body_buf[body_len - 1] != ' ') {
                    body_buf[body_len++] = ' ';
                }
            } else {
                body_buf[body_len++] = c;
            }
        }
        p++;
    }

    body_buf[body_len] = '\0';
    decode_html_entities(body_buf);
    trim_whitespace(body_buf);

    doc->body_text = body_buf;
    doc->keywords = list_to_null_terminated(&keywords_list);
    doc->headings = list_to_null_terminated(&headings_list);
    doc->links = list_to_null_terminated(&links_list);

    if (!doc->title) doc->title = strdup("Untitled Document");
    if (!doc->description) doc->description = strdup("");
    if (!doc->category) doc->category = strdup("General");

    return 0;
}
