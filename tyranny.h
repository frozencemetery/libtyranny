/* SPDX-License-Identifier: MPL-2.0 */

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif

#pragma once

#include <stdint.h>
#include <yaml.h>

/* Types modeled after cdson's IR.  Note that it's not an enum in a public
 * header (libyaml, I'm looking at you). */
#define Y_UNINITIALIZED 0
#define Y_STRING 1
#define Y_ARRAY 2 /* "sequence", in YAML speak */
#define Y_DICT 3 /* "mapping", in YAML parlance */
typedef uint8_t y_type; /* Can only take the above values. */

/* This is a public type.  Feel free to traverse it yourself, if you want. */
typedef struct y_value {
    y_type type;
    union {
        char *string; /* \0-terminated. */
        struct y_value **array; /* NULL-terminated. */

        /* keys is NULL-terminated.  values might contain NULLs because YAML
         * allows you to just not map a dict entry to anything. */
        struct {
            char **keys;
            struct y_value **values;
        } dict;
    };
} y_value;

/* Convenience method to get a type name from a libyaml token, because the
 * library doesn't have one. */
const char *y_tok_type_name(yaml_token_type_t t);

/* Read a file and yield a parsed tree.  NULL indicates failure, but
 * malformed documents may just call exit. */
y_value *y_parse_yaml(const char *filename);

/* Print out a parsed tree to stdout in a json-esque format. */
void y_dump_tree(y_value *v);

/* Free a parsed tree. */
void y_free_tree(y_value *v);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

/* Local variables: */
/* c-basic-offset: 4 */
/* indent-tabs-mode: nil */
/* End: */
