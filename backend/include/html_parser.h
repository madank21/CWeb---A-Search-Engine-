#ifndef CWEB_HTML_PARSER_H
#define CWEB_HTML_PARSER_H

#include "document.h"

/* Parses an HTML string into a Document struct. 
   doc_id, path, file_size, modified_time must be set by caller. */
int html_parse_document(const char *html_content, size_t length, Document *doc);

#endif /* CWEB_HTML_PARSER_H */
