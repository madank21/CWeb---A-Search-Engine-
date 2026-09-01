# CWeb Indexing & Tokenization Architecture

## 1. HTML Tag Scanner
CWeb includes a hand-rolled HTML tag scanner over an HTML5 subset. It strips `<script>`, `<style>`, HTML comments `<!-- ... -->`, and decodes HTML entities (`&lt;`, `&gt;`, `&amp;`, `&quot;`, `&#39;`, `&nbsp;`).
It extracts structured document fields:
- `<title>` (Weight: 4.0)
- `<h1>..<h6>` (Weight: 3.0)
- `<meta name="keywords">` (Weight: 3.0)
- `<meta name="description">` (Weight: 2.0)
- Body text (Weight: 1.0)

## 2. Tokenization Pipeline
1. UTF-8 multi-byte character safety scan.
2. ASCII case-folding (`A-Z -> a-z`).
3. Delimiter splitting on whitespace and punctuation (`.,;:!?()[]{}"'`), maintaining internal hyphens (`co-worker`).
4. Length filtering (`2 <= length <= 40`).
5. Stop-word filtering (`a`, `an`, `the`, `and`, `or`, etc.).
6. Light suffix-stripping normalization (`normalize.c`).
7. 1-indexed position recording per field.
