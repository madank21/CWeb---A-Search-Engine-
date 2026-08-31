#include "query_parser.h"
#include "config/config.h"
#include "fuzzy.h"
#include "normalize.h"
#include "tokenizer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASTNode *ast_node_create(ASTNodeType type) {
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    node->type = type;
    return node;
}

void ast_node_free(ASTNode *node) {
    if (!node) return;
    free(node->value);
    string_array_free(node->phrase_terms);
    ast_node_free(node->left);
    ast_node_free(node->right);
    free(node);
}

void query_ast_free(QueryAST *ast) {
    if (!ast) return;
    ast_node_free(ast->root);
    free(ast->error_message);
    free(ast);
}

typedef struct {
    const char *str;
    size_t pos;
    size_t len;
    int depth;
} ParserState;

static void skip_whitespace(ParserState *st) {
    while (st->pos < st->len && isspace((unsigned char)st->str[st->pos])) {
        st->pos++;
    }
}

static ASTNode *parse_query(ParserState *st, QueryAST *ast);

static ASTNode *parse_primary(ParserState *st, QueryAST *ast) {
    skip_whitespace(st);
    if (st->pos >= st->len) return NULL;

    /* Parentheses */
    if (st->str[st->pos] == '(') {
        st->pos++;
        st->depth++;
        if (st->depth > CWEB_MAX_QUERY_DEPTH) {
            ast->error_message = strdup("Maximum query nesting depth exceeded.");
            ast->error_pos = (int)st->pos;
            return NULL;
        }
        ASTNode *node = parse_query(st, ast);
        skip_whitespace(st);
        if (st->pos < st->len && st->str[st->pos] == ')') {
            st->pos++;
        } else if (!ast->error_message) {
            ast->error_message = strdup("Unbalanced parentheses: missing ')'");
            ast->error_pos = (int)st->pos;
        }
        st->depth--;
        return node;
    }

    /* Phrase "..." */
    if (st->str[st->pos] == '"') {
        st->pos++;
        size_t start = st->pos;
        while (st->pos < st->len && st->str[st->pos] != '"') {
            st->pos++;
        }
        if (st->pos >= st->len) {
            ast->error_message = strdup("Unterminated phrase: missing closing quote '\"'");
            ast->error_pos = (int)start;
            return NULL;
        }
        size_t plen = st->pos - start;
        char *phrase_str = (char *)malloc(plen + 1);
        if (phrase_str) {
            memcpy(phrase_str, st->str + start, plen);
            phrase_str[plen] = '\0';
        }
        st->pos++; /* skip closing quote */

        ASTNode *node = ast_node_create(NODE_PHRASE);
        if (phrase_str) {
            int dummy_pos = 1;
            TokenStream *ts = tokenize_text(phrase_str, FIELD_BODY, &dummy_pos);
            if (ts && ts->count > 0) {
                node->phrase_terms = (char **)calloc(ts->count + 1, sizeof(char *));
                for (size_t i = 0; i < ts->count; i++) {
                    node->phrase_terms[i] = strdup(ts->tokens[i].text);
                }
                node->phrase_terms[ts->count] = NULL;
            }
            token_stream_free(ts);
            free(phrase_str);
        }
        return node;
    }

    /* Term or Field Query e.g. title:compiler or normal term */
    size_t start = st->pos;
    while (st->pos < st->len && !isspace((unsigned char)st->str[st->pos]) &&
           st->str[st->pos] != '(' && st->str[st->pos] != ')' && st->str[st->pos] != '"') {
        st->pos++;
    }
    size_t tlen = st->pos - start;
    if (tlen == 0) return NULL;

    char *token_raw = (char *)malloc(tlen + 1);
    if (token_raw) {
        memcpy(token_raw, st->str + start, tlen);
        token_raw[tlen] = '\0';
    }

    /* Check for Field Query field:term */
    char *colon = strchr(token_raw, ':');
    if (colon) {
        *colon = '\0';
        char *fname = token_raw;
        char *fval = colon + 1;

        ASTNode *node = ast_node_create(NODE_FIELD);
        node->value = strdup(fname);

        ASTNode *child = ast_node_create(NODE_TERM);
        char norm_buf[128];
        snprintf(norm_buf, sizeof(norm_buf), "%s", fval);
        for (char *p = norm_buf; *p; p++) *p = (char)tolower((unsigned char)*p);
        normalize_token(norm_buf);
        child->value = strdup(norm_buf);

        node->left = child;
        free(token_raw);
        return node;
    }

    ASTNode *node = ast_node_create(NODE_TERM);
    char norm_buf[128];
    snprintf(norm_buf, sizeof(norm_buf), "%s", token_raw);
    for (char *p = norm_buf; *p; p++) *p = (char)tolower((unsigned char)*p);
    normalize_token(norm_buf);
    node->value = strdup(norm_buf);
    free(token_raw);
    return node;
}

static ASTNode *parse_not(ParserState *st, QueryAST *ast) {
    skip_whitespace(st);
    if (st->pos < st->len) {
        if (st->pos + 3 <= st->len && strncasecmp(st->str + st->pos, "NOT", 3) == 0 &&
            (st->pos + 3 == st->len || isspace((unsigned char)st->str[st->pos + 3]) || st->str[st->pos + 3] == '(')) {
            st->pos += 3;
            ASTNode *child = parse_primary(st, ast);
            if (!child) return NULL;
            ASTNode *not_node = ast_node_create(NODE_NOT);
            not_node->left = child;
            return not_node;
        }
    }
    return parse_primary(st, ast);
}

static ASTNode *parse_and(ParserState *st, QueryAST *ast) {
    ASTNode *left = parse_not(st, ast);
    if (!left) return NULL;

    while (st->pos < st->len) {
        skip_whitespace(st);
        if (st->pos >= st->len || st->str[st->pos] == ')') break;

        /* Check for OR operator - stop AND parsing */
        if (st->pos + 2 <= st->len && strncasecmp(st->str + st->pos, "OR", 2) == 0 &&
            (st->pos + 2 == st->len || isspace((unsigned char)st->str[st->pos + 2]) || st->str[st->pos + 2] == '(')) {
            break;
        }

        /* Check explicit AND */
        if (st->pos + 3 <= st->len && strncasecmp(st->str + st->pos, "AND", 3) == 0 &&
            (st->pos + 3 == st->len || isspace((unsigned char)st->str[st->pos + 3]) || st->str[st->pos + 3] == '(')) {
            st->pos += 3;
        }

        ASTNode *right = parse_not(st, ast);
        if (!right) break;

        ASTNode *and_node = ast_node_create(NODE_AND);
        and_node->left = left;
        and_node->right = right;
        left = and_node;
    }
    return left;
}

static ASTNode *parse_or(ParserState *st, QueryAST *ast) {
    ASTNode *left = parse_and(st, ast);
    if (!left) return NULL;

    while (st->pos < st->len) {
        skip_whitespace(st);
        if (st->pos >= st->len || st->str[st->pos] == ')') break;

        if (st->pos + 2 <= st->len && strncasecmp(st->str + st->pos, "OR", 2) == 0 &&
            (st->pos + 2 == st->len || isspace((unsigned char)st->str[st->pos + 2]) || st->str[st->pos + 2] == '(')) {
            st->pos += 2;
            ASTNode *right = parse_and(st, ast);
            if (!right) break;

            ASTNode *or_node = ast_node_create(NODE_OR);
            or_node->left = left;
            or_node->right = right;
            left = or_node;
        } else {
            break;
        }
    }
    return left;
}

static ASTNode *parse_query(ParserState *st, QueryAST *ast) {
    return parse_or(st, ast);
}

QueryAST *query_parse(const char *query_str) {
    QueryAST *ast = (QueryAST *)calloc(1, sizeof(QueryAST));
    if (!ast) return NULL;

    if (!query_str || !*query_str) {
        ast->error_message = strdup("Empty query string");
        ast->error_pos = 0;
        return ast;
    }

    if (strlen(query_str) > CWEB_MAX_QUERY_LEN) {
        ast->error_message = strdup("Query length exceeds maximum allowed limit");
        ast->error_pos = (int)strlen(query_str);
        return ast;
    }

    ParserState st;
    st.str = query_str;
    st.pos = 0;
    st.len = strlen(query_str);
    st.depth = 0;

    ast->root = parse_query(&st, ast);
    return ast;
}

typedef struct {
    char **data;
    size_t count;
    size_t capacity;
} TermList;

static void term_list_init(TermList *list) {
    list->capacity = 8;
    list->count = 0;
    list->data = (char **)calloc(list->capacity, sizeof(char *));
}

static void term_list_append(TermList *list, const char *str) {
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

static void term_list_free(TermList *list) {
    if (!list || !list->data) return;
    for (size_t i = 0; i < list->count; i++) {
        free(list->data[i]);
    }
    free(list->data);
}

static void collect_query_terms(const ASTNode *node, TermList *list) {
    if (!node) return;
    if (node->type == NODE_TERM && node->value) {
        term_list_append(list, node->value);
    } else if (node->type == NODE_PHRASE && node->phrase_terms) {
        for (size_t i = 0; node->phrase_terms[i] != NULL; i++) {
            term_list_append(list, node->phrase_terms[i]);
        }
    } else if (node->type == NODE_FIELD && node->left && node->left->value) {
        term_list_append(list, node->left->value);
    }
    collect_query_terms(node->left, list);
    collect_query_terms(node->right, list);
}



SearchResultList *query_evaluate(const QueryAST *ast,
                                const InvertedIndex *idx,
                                RankingAlgorithm algo,
                                float k1, float b,
                                char **out_did_you_mean) {
    SearchResultList *list = search_result_list_create(32);
    if (!ast || !ast->root || !idx || !idx->doc_store) return list;

    TermList qterms;
    term_list_init(&qterms);
    collect_query_terms(ast->root, &qterms);

    size_t total_docs = idx->doc_store->count;
    if (total_docs == 0) {
        term_list_free(&qterms);
        return list;
    }

    /* Score per document */
    for (size_t d = 0; d < total_docs; d++) {
        Document *doc = idx->doc_store->docs[d];
        float score = 0.0f;
        int matched_terms = 0;

        for (size_t t = 0; t < qterms.count; t++) {
            const char *term = qterms.data[t];
            PostingList *plist = index_get_posting_list(idx, term);
            if (!plist) continue;

            Posting *post = NULL;
            for (size_t k = 0; k < plist->doc_freq; k++) {
                if (plist->postings[k].document_id == doc->id) {
                    post = &plist->postings[k];
                    break;
                }
            }

            if (post) {
                matched_terms++;
                float term_score = 0.0f;
                if (algo == RANKING_TFIDF) {
                    term_score = calculate_tfidf(post->field_weight_sum, plist->doc_freq, total_docs);
                } else {
                    term_score = calculate_bm25(post->field_weight_sum, plist->doc_freq, total_docs, doc->word_count, idx->avg_doc_length, k1, b);
                }
                score += term_score;
            }
        }

        if (matched_terms > 0 && score > 0.0f) {
            /* Multi-term coordination multiplier */
            if (qterms.count > 1) {
                float coord = 1.0f + ((float)matched_terms / (float)qterms.count) * 0.5f;
                score *= coord;
            }

            if (list->count >= list->capacity) {
                size_t new_cap = list->capacity * 2;
                SearchResult *new_res = (SearchResult *)realloc(list->results, new_cap * sizeof(SearchResult));
                if (new_res) {
                    list->results = new_res;
                    list->capacity = new_cap;
                }
            }

            SearchResult *res = &list->results[list->count++];
            res->document_id = doc->id;
            res->score = score;
            res->matched_terms = matched_terms;
            res->total_query_terms = (int)qterms.count;
            res->doc = doc;
        }
    }

    sort_search_results(list);

    /* Fuzzy suggestion trigger check if total matches < threshold */
    if (list->count < CWEB_FUZZY_TRIGGER_THRESHOLD && qterms.count > 0 && out_did_you_mean) {
        const char *first_term = qterms.data[0];
        char **suggestions = trie_suggest(idx->trie, "", 50);
        if (suggestions) {
            char *best_match = NULL;
            int best_dist = CWEB_FUZZY_MAX_DISTANCE + 1;
            for (size_t i = 0; suggestions[i] != NULL; i++) {
                int dist = levenshtein_distance_bounded(first_term, suggestions[i], CWEB_FUZZY_MAX_DISTANCE);
                if (dist <= CWEB_FUZZY_MAX_DISTANCE && dist < best_dist) {
                    best_dist = dist;
                    free(best_match);
                    best_match = strdup(suggestions[i]);
                }
            }
            trie_string_list_free(suggestions);
            if (best_match) {
                *out_did_you_mean = best_match;
            }
        }
    }

    term_list_free(&qterms);
    return list;
}
