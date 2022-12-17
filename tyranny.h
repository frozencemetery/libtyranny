/* SPDX-License-Identifier: MPL-2.0 */
/* Copyright the libtyranny contributors */

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

/* Retrieve a specific string (y_getstr) or value (y_get) from the parsed
 * tree.  This is a shortcut method for traversing the tree by hand.  Returned
 * values are owned by the tree; do not free them.  Returns NULL on failure
 * (i.e., not present, match error, or internal error).
 *
 * match_behavior indicates what to do on encountering duplicate keys in a
 * dict (mapping).  The options are to return the FIRST result from the input
 * (i.e., no overriding), the LAST result (i.e., support overriding), or to
 * ERROR (i.e., duplicate keys are not permitted).
 *
 * query syntax uses [] for array (sequence) access and . for dict (mapping)
 * access, and is \0-terminated.  An example valid query (excluding quotes)
 * is: ".alpha[3].beta", while something like "[3{alpha}][2]" ould be invalud,
 * as would "[alpha].beta[2]".  Note that arrays are zero-indexed, and strings
 * cannot be quoted (they'll be treated as part of the string).  Behavior when
 * strings contain formatting characters is undefined (suggest you don't do
 * that), and strings cannot contain any of "[].". */
#define Y_MATCH_FIRST 0
#define Y_MATCH_LAST 1
#define Y_MATCH_ERROR 2
const char *y_getstr(y_value *tree, const char *query,
                     uint8_t match_behavior);
const y_value *y_get(y_value *tree, const char *query,
                     uint8_t match_behavior);

/* Convenience method to get a type name from a libyaml token, because the
 * library doesn't have one. */
const char *y_tok_type_name(yaml_token_type_t t);

/* Read a file and yield a parsed tree.  NULL indicates failure. */
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
