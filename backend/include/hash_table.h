#ifndef CWEB_HASH_TABLE_H
#define CWEB_HASH_TABLE_H

#include <stddef.h>
#include <stdint.h>

typedef struct HashNode {
    char            *key;
    void            *value;
    struct HashNode *next;
} HashNode;

typedef struct {
    HashNode **buckets;
    size_t     num_buckets;
    size_t     size;
    size_t     collisions;
} HashTable;

typedef void (*HashFreeValueFn)(void *value);

HashTable *hash_table_create(size_t initial_buckets);
void hash_table_free(HashTable *ht, HashFreeValueFn free_fn);

int hash_table_put(HashTable *ht, const char *key, void *value);
void *hash_table_get(const HashTable *ht, const char *key);
int hash_table_remove(HashTable *ht, const char *key, HashFreeValueFn free_fn);

double hash_table_load_factor(const HashTable *ht);
size_t hash_table_collisions(const HashTable *ht);

#endif /* CWEB_HASH_TABLE_H */
