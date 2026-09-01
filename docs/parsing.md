# CWeb Query Grammar & AST Parser

## 1. Formal Grammar (EBNF)
```ebnf
query        = or_expr ;
or_expr      = and_expr , { "OR" , and_expr } ;
and_expr     = not_expr , { [ "AND" ] , not_expr } ;
not_expr     = [ "NOT" ] , primary ;
primary      = phrase | field_query | term | "(" , query , ")" ;
phrase       = '"' , token , { token } , '"' ;
field_query  = FIELD , ":" , ( term | phrase ) ;
FIELD        = "title" | "category" | "keywords" | "body" ;
term         = TOKEN ;
```

## 2. Operator Precedence
1. Parentheses `(...)`
2. `NOT`
3. `AND` (explicit or implicit adjacent space)
4. `OR`

## 3. Fuzzy Levenshtein Fallback
When a query yields fewer than 3 exact matches, the query engine computes bounded Levenshtein edit distance (\(\le 2\)) against Trie terms, returning a `did_you_mean` field in the JSON response.
