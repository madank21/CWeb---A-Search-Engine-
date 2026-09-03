#!/usr/bin/env bash
set -e

echo "Building CWeb Benchmark Runner..."
gcc -std=c17 -Wall -Wextra -Werror -O2 \
    -I./backend -I./backend/config -I./backend/include \
    backend/src/crc32.c \
    backend/src/document.c \
    backend/src/fuzzy.c \
    backend/src/hash_table.c \
    backend/src/html_parser.c \
    backend/src/http_server.c \
    backend/src/index.c \
    backend/src/logger.c \
    backend/src/normalize.c \
    backend/src/persistence.c \
    backend/src/query_parser.c \
    backend/src/ranking.c \
    backend/src/snippet.c \
    backend/src/tokenizer.c \
    backend/src/trie.c \
    scripts/benchmark.c \
    -o scripts/benchmark_runner -lpthread -lm

echo "Running CWeb Benchmark..."
./scripts/benchmark_runner
