#ifndef CWEB_QUERY_PARSER_H
#define CWEB_QUERY_PARSER_H

#include "index.h"
#include "ranking.h"
#include <stddef.h>

typedef enum {
    NODE_TERM,
    NODE_PHRASE,
    NODE_FIELD,
    NODE_AND,
    NODE_OR,
    NODE_NOT
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType     type;
    char           *value;        /* term or field name */
    char          **phrase_terms; /* NULL-terminated for phrase */
    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode;

typedef struct {
    ASTNode *root;
    char    *error_message;
    int      error_pos;
} QueryAST;

ASTNode *ast_node_create(ASTNodeType type);
void ast_node_free(ASTNode *node);

QueryAST *query_parse(const char *query_str);
void query_ast_free(QueryAST *ast);

/* Evaluates query AST against inverted index with chosen ranking algorithm */
SearchResultList *query_evaluate(const QueryAST *ast,
                                const InvertedIndex *idx,
                                RankingAlgorithm algo,
                                float k1, float b,
                                char **out_did_you_mean);

#endif /* CWEB_QUERY_PARSER_H */
