# CWeb REST API Specification v2.0

Base Path: `/api/v1`

## Endpoints Summary

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/health` | Server health, version, uptime, and index load status |
| `GET` | `/stats` | Telemetry: documents indexed, terms count, hash table load factor, cache hits |
| `GET` | `/search` | Query search: supports AST boolean logic, phrases, field search, BM25/TF-IDF |
| `GET` | `/suggest` | Prefix autocomplete suggestions (< 5 ms latency) |
| `GET` | `/page/:id` | Full document text retrieval by numeric ID |
| `POST` | `/index/rebuild` | Triggers background index rebuild and persistence |
| `POST` | `/cache/clear` | Clears LRU query cache |

## Response & Error Code Standard

All error responses return:
```json
{
  "error": true,
  "code": "INVALID_QUERY_SYNTAX",
  "message": "Unbalanced parentheses: missing ')'",
  "status": 400
}
```

### Error Code Enumeration
- `INVALID_QUERY_SYNTAX` (400)
- `QUERY_TOO_LONG` (400)
- `PREFIX_TOO_SHORT` (400)
- `INVALID_ID` (400)
- `DOCUMENT_NOT_FOUND` (404)
- `INDEX_BUILD_IN_PROGRESS` (409)
- `RATE_LIMITED` (429)
- `INTERNAL_ERROR` (500)
