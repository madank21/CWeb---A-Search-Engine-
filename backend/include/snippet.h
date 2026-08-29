#ifndef CWEB_SNIPPET_H
#define CWEB_SNIPPET_H

#include "document.h"

/* Generates an HTML snippet with <em>...</em> term highlighting */
char *snippet_generate(const Document *doc, const char *query_str, size_t max_length);

#endif /* CWEB_SNIPPET_H */
