# CWeb — Search Engine Built from Scratch in C (v2.0)

[![C17](https://img.shields.io/badge/C-17-blue.svg)](https://en.cppreference.com/w/c/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)]()
[![Benchmark](https://img.shields.io/badge/Latency-p50%20%3C%200.1ms-success.svg)]()

CWeb is a self-hosted, concurrent, high-performance search engine built completely from scratch in C17. It features a hand-rolled HTML tag scanner, tokenizer with stop-word filtering and light normalization, FNV-1a custom hash table, inverted index with postings list position tracking, prefix autocomplete Trie, EBNF AST query parser, dual field-weighted ranking engines (TF-IDF & BM25), custom versioned binary file persistence with CRC32 integrity verification, and a multi-threaded POSIX/Winsock HTTP REST API server paired with a React Native / Web frontend application.

---

## Key Architecture & Features

- **Custom HTML Tag Scanner**: Parses document structure, strips `<script>`, `<style>`, and HTML comments, decodes HTML entities, and extracts field weights for `<title>` (4.0), `<h1>..<h6>` (3.0), `<meta name="keywords">` (3.0), `<meta name="description">` (2.0), and body text (1.0).
- **Tokenization & Normalization Pipeline**: UTF-8 byte-safe scanning, ASCII case-folding, stop-word filter, internal hyphen preservation (`co-worker`), and light suffix-stripping normalization.
- **Core Data Structures**: FNV-1a Hash Table with chaining and collision tracking, Inverted Index with posting lists (`document_id`, `frequency`, `positions`, `field_weight_sum`), and Autocomplete Trie (< 5 ms prefix suggestions).
- **AST Query Engine**: Formal EBNF grammar parser building an Abstract Syntax Tree. Supports boolean logic (`AND`, `OR`, `NOT`), phrase matching `"..."`, field-specific queries (`title:`, `category:`, `keywords:`), parentheses grouping `(...)`, and bounded Levenshtein fuzzy matching (\(\le 2\)).
- **Dual Ranking Engine**: Selectable TF-IDF and BM25 (\(k_1=1.2, b=0.75\)) with multi-term coordination multipliers and stable tie-breaking rules.
- **Custom Binary Storage (`CWIX`)**: Portable binary file persistence with magic numbers, format version headers, payload truncation checks, and CRC32 verification footers.
- **Concurrent HTTP Server & REST API**: Multi-threaded socket server (`pthread` pool), RCU-style write lock for index pointer swapping, LRU query cache, token bucket rate limiter (20 RPS / 40 burst), and enumerated JSON error codes.
- **React Native / Web Client**: Built with React, TanStack React Query, Zustand, and Lucide Icons. Features zero client-side search logic — acts as a thin typed API consumer.

---

## Directory Structure

```text
SEARCH/
├── backend/
│   ├── CMakeLists.txt
│   ├── Makefile
│   ├── config/
│   │   └── config.h              # Central system configuration macros
│   ├── include/                  # One header per module
│   │   ├── crc32.h, document.h, fuzzy.h, hash_table.h, html_parser.h,
│   │   ├── http_server.h, index.h, logger.h, normalize.h, persistence.h,
│   │   └── query_parser.h, ranking.h, snippet.h, tokenizer.h, trie.h
│   ├── src/                      # C source code implementation
│   │   ├── crc32.c, document.c, fuzzy.c, hash_table.c, html_parser.c,
│   │   ├── http_server.c, index.c, logger.c, main.c, normalize.c,
│   │   └── persistence.c, query_parser.c, ranking.c, snippet.c, tokenizer.c, trie.c
│   ├── tests/
│   │   ├── unit/test_suite.c     # Unit & persistence round-trip test suite
│   │   └── integration/test_api.c# REST API integration test client
│   └── data/
│       ├── pages/                # 52 seeded rich HTML document corpus
│       └── index/                # Binary index files (index.dat)
├── mobile/                       # React Native / Expo Web Client
│   ├── src/
│   │   ├── api/                  # Typed API client with backoff retry
│   │   ├── store/                # Zustand local state store
│   │   ├── components/           # Navbar, SearchBar, ResultCard, SnippetViewer, etc.
│   │   ├── screens/              # SearchScreen, DocumentViewerScreen, BookmarksScreen, AdminStatsScreen
│   │   └── App.tsx
│   ├── app.config.ts             # API URL abstraction
│   ├── package.json
│   └── vite.config.ts
├── scripts/
│   ├── seed_corpus.c             # Seeder generating 52 HTML corpus files
│   ├── benchmark.c               # High-precision benchmark runner (NFR compliance)
│   └── benchmark.sh              # POSIX benchmark script
├── docs/                         # Architecture, API, Indexing, Ranking, Security docs
├── implementation_plan.md
└── README.md
```

---

## Quick Start & Build Instructions

### Prerequisites
- GCC compiler supporting C17 (`gcc --version`)
- Node.js (v18+) and npm

### 1. Build and Seed Corpus
Compile the seeder to populate the 52 HTML pages in `backend/data/pages/`:
```bash
gcc -std=c17 -Wall -Wextra -Werror -I./backend/config -I./backend/include scripts/seed_corpus.c -o scripts/seed_corpus.exe
.\scripts\seed_corpus.exe
```

### 2. Compile and Run C Backend Server
```bash
gcc -std=c17 -Wall -Wextra -Werror -I./backend -I./backend/config -I./backend/include backend/src/*.c -o cweb_server.exe -lws2_32
.\cweb_server.exe
```
*The server will start listening on `http://0.0.0.0:8080`.*

### 3. Run Unit Tests
```bash
gcc -std=c17 -Wall -Wextra -Werror -I./backend -I./backend/config -I./backend/include backend/src/crc32.c backend/src/document.c backend/src/fuzzy.c backend/src/hash_table.c backend/src/html_parser.c backend/src/http_server.c backend/src/index.c backend/src/logger.c backend/src/normalize.c backend/src/persistence.c backend/src/query_parser.c backend/src/ranking.c backend/src/snippet.c backend/src/tokenizer.c backend/src/trie.c backend/tests/unit/test_suite.c -o test_suite.exe -lws2_32
.\test_suite.exe
```

### 4. Run Benchmark Suite (NFR Compliance Verification)
```bash
gcc -std=c17 -Wall -Wextra -Werror -O2 -I./backend -I./backend/config -I./backend/include backend/src/crc32.c backend/src/document.c backend/src/fuzzy.c backend/src/hash_table.c backend/src/html_parser.c backend/src/http_server.c backend/src/index.c backend/src/logger.c backend/src/normalize.c backend/src/persistence.c backend/src/query_parser.c backend/src/ranking.c backend/src/snippet.c backend/src/tokenizer.c backend/src/trie.c scripts/benchmark.c -o benchmark.exe -lws2_32
.\benchmark.exe
```

#### NFR Benchmark Results (§3 Compliance Report)
```text
========================================
 CWEB BENCHMARK — PASS/FAIL vs NFR (§3)
========================================
Documents:            52        
Unique terms:         1003      
Index build time:     105.5  ms   [PASS < 500ms]
Queries evaluated:    1000      
p50 latency (warm):    0.05   ms   [PASS < 2ms]
p95 latency (warm):    7.94   ms   [PASS < 8ms]
p99 latency (warm):    10.58  ms   [PASS < 20ms]
Peak RSS:               12     MB   [PASS < 64MB]
========================================
```

### 5. Launch React Native / Web Client
```bash
cd mobile
npm install
npm run dev
```
Open `http://localhost:3000` in your browser.

---

## QA Demo Script (§25 Verification)

1. **Start Backend**: Launch `cweb_server.exe` and confirm `/health` returns `index_loaded: true`.
2. **Admin Telemetry**: Open app -> navigate to **Admin Stats** tab to observe live `/stats` metrics (indexed docs, unique terms, hash table load factor, LRU cache hit ratio).
3. **Search & Ranking**: Search `compiler optimization` -> observe autocomplete dropdown suggestions, then examine results ranked by BM25 vs TF-IDF with exact `search_time_ms`.
4. **Snippet & Detail Viewer**: Click a result -> verify `<em>...</em>` term highlighting, full document text rendering, and internal link cross-referencing.
5. **Bookmarking**: Click the bookmark icon -> navigate to **Bookmarks** screen to verify local persistence.
6. **Typo Search & Did You Mean**: Search `compilor` -> observe the bounded Levenshtein `did_you_mean` banner suggesting `"compiler"`.
7. **Boolean & Phrase Search**: Search `"compiler construction"` (phrase) and `(compiler OR parser) AND optimization` (boolean query) -> verify AST grammar evaluation.
8. **Concurrent Index Rebuild**: Click **Trigger Index Rebuild** in Admin Stats -> observe `202 Accepted` status and `409` conflict handling under concurrent triggers.

---

## Documentation Deliverables (`docs/`)

- [docs/architecture.md](file:///c:/Users/mehun/OneDrive/Desktop/Project/rough/SEARCH/docs/architecture.md) — Container diagram, RCU pointer swap, thread pool, LRU cache.
- [docs/api.md](file:///c:/Users/mehun/OneDrive/Desktop/Project/rough/SEARCH/docs/api.md) — REST API endpoint schemas and enumerated error codes.
- [docs/indexing.md](file:///c:/Users/mehun/OneDrive/Desktop/Project/rough/SEARCH/docs/indexing.md) — HTML scanner rules, tokenization pipeline, field weights.
- [docs/ranking.md](file:///c:/Users/mehun/OneDrive/Desktop/Project/rough/SEARCH/docs/ranking.md) — Exact TF-IDF and BM25 formulas, coordination multiplier, tie-breaking.
- [docs/parsing.md](file:///c:/Users/mehun/OneDrive/Desktop/Project/rough/SEARCH/docs/parsing.md) — EBNF query grammar, AST evaluation rules, fuzzy Levenshtein.
- [docs/security.md](file:///c:/Users/mehun/OneDrive/Desktop/Project/rough/SEARCH/docs/security.md) — STRIDE threat model, rate limiting, memory safety mitigations.
- [docs/presentation.md](file:///c:/Users/mehun/OneDrive/Desktop/Project/rough/SEARCH/docs/presentation.md) — Executive architecture summary and engineering highlights.

---

## License

MIT License. Built for real systems programming demonstration.
#   C W e b - - - A - S e a r c h - E n g i n e -  
 