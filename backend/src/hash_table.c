#include "hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t fnv1a_hash(const char *key) {
    uint64_t hash = 14695981039346656037ULL;
    const unsigned char *ptr = (const unsigned char *)key;
    while (*ptr) {
        hash ^= *ptr++;
        hash *= 1099511628211ULL;
    }
    return hash;
}

HashTable *hash_table_create(size_t initial_buckets) {
    if (initial_buckets == 0) initial_buckets = 1024;
    HashTable *ht = (HashTable *)calloc(1, sizeof(HashTable));
    if (!ht) return NULL;

    ht->num_buckets = initial_buckets;
    ht->buckets = (HashNode **)calloc(ht->num_buckets, sizeof(HashNode *));
    if (!ht->buckets) {
        free(ht);
        return NULL;
    }
    ht->size = 0;
    ht->collisions = 0;
    return ht;
}

void hash_table_free(HashTable *ht, HashFreeValueFn free_fn) {
    if (!ht) return;
    for (size_t i = 0; i < ht->num_buckets; i++) {
        HashNode *node = ht->buckets[i];
        while (node) {
            HashNode *tmp = node;
            node = node->next;
            free(tmp->key);
            if (free_fn && tmp->value) {
                free_fn(tmp->value);
            }
            free(tmp);
        }
    }
    free(ht->buckets);
    free(ht);
}

static void hash_table_resize(HashTable *ht) {
    size_t new_num_buckets = ht->num_buckets * 2;
    HashNode **new_buckets = (HashNode **)calloc(new_num_buckets, sizeof(HashNode *));
    if (!new_buckets) return;

    size_t new_collisions = 0;
    for (size_t i = 0; i < ht->num_buckets; i++) {
        HashNode *node = ht->buckets[i];
        while (node) {
            HashNode *next = node->next;
            uint64_t hash = fnv1a_hash(node->key);
            size_t idx = (size_t)(hash % new_num_buckets);
            if (new_buckets[idx] != NULL) new_collisions++;
            node->next = new_buckets[idx];
            new_buckets[idx] = node;
            node = next;
        }
    }
    free(ht->buckets);
    ht->buckets = new_buckets;
    ht->num_buckets = new_num_buckets;
    ht->collisions = new_collisions;
}

int hash_table_put(HashTable *ht, const char *key, void *value) {
    if (!ht || !key) return -1;

    if ((double)ht->size / (double)ht->num_buckets > 0.75) {
        hash_table_resize(ht);
    }

    uint64_t hash = fnv1a_hash(key);
    size_t idx = (size_t)(hash % ht->num_buckets);

    HashNode *node = ht->buckets[idx];
    if (node != NULL) {
        ht->collisions++;
    }

    while (node) {
        if (strcmp(node->key, key) == 0) {
            node->value = value;
            return 0;
        }
        node = node->next;
    }

    HashNode *new_node = (HashNode *)calloc(1, sizeof(HashNode));
    if (!new_node) return -1;
    new_node->key = strdup(key);
    new_node->value = value;
    new_node->next = ht->buckets[idx];
    ht->buckets[idx] = new_node;
    ht->size++;
    return 0;
}

void *hash_table_get(const HashTable *ht, const char *key) {
    if (!ht || !key) return NULL;
    uint64_t hash = fnv1a_hash(key);
    size_t idx = (size_t)(hash % ht->num_buckets);
    HashNode *node = ht->buckets[idx];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            return node->value;
        }
        node = node->next;
    }
    return NULL;
}

int hash_table_remove(HashTable *ht, const char *key, HashFreeValueFn free_fn) {
    if (!ht || !key) return -1;
    uint64_t hash = fnv1a_hash(key);
    size_t idx = (size_t)(hash % ht->num_buckets);
    HashNode *node = ht->buckets[idx];
    HashNode *prev = NULL;

    while (node) {
        if (strcmp(node->key, key) == 0) {
            if (prev) prev->next = node->next;
            else ht->buckets[idx] = node->next;

            free(node->key);
            if (free_fn && node->value) {
                free_fn(node->value);
            }
            free(node);
            ht->size--;
            return 0;
        }
        prev = node;
        node = node->next;
    }
    return -1;
}

double hash_table_load_factor(const HashTable *ht) {
    if (!ht || ht->num_buckets == 0) return 0.0;
    return (double)ht->size / (double)ht->num_buckets;
}

size_t hash_table_collisions(const HashTable *ht) {
    if (!ht) return 0;
    return ht->collisions;
}
