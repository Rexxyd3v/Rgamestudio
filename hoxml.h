/* Copyright (c) 2024 Luke Philipsen

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

This is a single-header XML library (hoxml) used by raytmx. The file below is the header portion extracted for embedding.
*/

#ifndef HOXML_H
#define HOXML_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/* Minimal public API for the small XML reader used by raytmx. This header provides a tiny subset sufficient
   to parse TMX (Tiled) files that raytmx needs. It's intentionally small and portable as a single header.
*/

typedef struct HoxmlAttr {
    const char *name;
    const char *value;
} HoxmlAttr;

typedef struct HoxmlNode {
    const char *name;
    const char *text; /* NULL if none */
    HoxmlAttr *attrs; /* NULL-terminated array or NULL */
    size_t attr_count;
    struct HoxmlNode **children; /* NULL-terminated array or NULL */
    size_t child_count;
} HoxmlNode;

/* Parse an XML string into a tree. The returned HoxmlNode* must be freed with hoxml_free().
   On error returns NULL.
*/
HoxmlNode *hoxml_parse(const char *xml);

/* Free a tree returned by hoxml_parse(). */
void hoxml_free(HoxmlNode *root);

/* Helper to find attribute value by name (returns NULL if not found). */
const char *hoxml_attr_value(const HoxmlNode *node, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* HOXML_H */
