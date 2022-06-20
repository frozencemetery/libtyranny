/* SPDX-License-Identifier: MPL-2.0 */

#include "tyranny.h"

#include <stdbool.h>
#include <string.h>

/* Uncomment to enable debug printing. */
/* #define DEBUG 1 */
#ifdef DEBUG
#    define dprintf printf
#else
#    define dprintf(...)
#endif

/* Always 0-initialized, unless the compiler disagrees. */
static inline void *xalloc(size_t s) {
    void *ret = calloc(1, s);
    if (ret == NULL) {
        exit(1);
    }
    return ret;
}

/* Note that xrealloc(NULL, ...) works the way you'd want. */
static inline void *xrealloc(void *p, size_t s) {
    void *ret;

    if (p == NULL) {
        return xalloc(s);
    }

    ret = realloc(p, s);
    if (ret == NULL) {
        exit(1);
    }
    return ret;
}

static inline char *xstrndup(const char *s, size_t n) {
    char *ret = strndup(s, n);
    if (ret == NULL) {
        exit(1);
    }
    return ret;
}

#define warn(msg, ...)                                       \
    {                                                        \
        fprintf(stderr, "warn: %s:%u ", __FILE__, __LINE__); \
        fprintf(stderr, msg, ## __VA_ARGS__);                \
        fprintf(stderr, "\n");                               \
    }

/* Not only do they not give a method for this awfulness, and not only do
 * they put enums in their public headers, but they can't even be bothered to
 * initialize them.  Cross your fingers and hope we were built with a
 * compiler close enough to the distro's libyaml! */
const char *y_tok_type_name(yaml_token_type_t t)
{
    switch (t) {
    case YAML_NO_TOKEN: return "YAML_NO_TOKEN";
    case YAML_STREAM_START_TOKEN: return "YAML_STREAM_START_TOKEN";
    case YAML_STREAM_END_TOKEN: return "YAML_STREAM_END_TOKEN";
    case YAML_VERSION_DIRECTIVE_TOKEN: return "YAML_VERSION_DIRECTIVE_TOKEN";
    case YAML_TAG_DIRECTIVE_TOKEN: return "YAML_TAG_DIRECTIVE_TOKEN";
    case YAML_DOCUMENT_START_TOKEN: return "YAML_DOCUMENT_START_TOKEN";
    case YAML_DOCUMENT_END_TOKEN: return "YAML_DOCUMENT_END_TOKEN";
    case YAML_BLOCK_SEQUENCE_START_TOKEN:
        return "YAML_BLOCK_SEQUENCE_START_TOKEN";
    case YAML_BLOCK_MAPPING_START_TOKEN:
        return "YAML_BLOCK_MAPPING_START_TOKEN";
    case YAML_BLOCK_END_TOKEN: return "YAML_BLOCK_END_TOKEN";
    case YAML_FLOW_SEQUENCE_START_TOKEN:
        return "YAML_FLOW_SEQUENCE_START_TOKEN";
    case YAML_FLOW_SEQUENCE_END_TOKEN: return "YAML_FLOW_SEQUENCE_END_TOKEN";
    case YAML_FLOW_MAPPING_START_TOKEN:
        return "YAML_FLOW_MAPPING_START_TOKEN";
    case YAML_FLOW_MAPPING_END_TOKEN: return "YAML_FLOW_MAPPING_END_TOKEN";
    case YAML_BLOCK_ENTRY_TOKEN: return "YAML_BLOCK_ENTRY_TOKEN";
    case YAML_FLOW_ENTRY_TOKEN: return "YAML_FLOW_ENTRY_TOKEN";
    case YAML_KEY_TOKEN: return "YAML_KEY_TOKEN";
    case YAML_VALUE_TOKEN: return "YAML_VALUE_TOKEN";
    case YAML_ALIAS_TOKEN: return "YAML_ALIAS_TOKEN";
    case YAML_ANCHOR_TOKEN: return "YAML_ANCHOR_TOKEN";
    case YAML_TAG_TOKEN: return "YAML_TAG_TOKEN";
    case YAML_SCALAR_TOKEN: return "YAML_SCALAR_TOKEN";
    default: return "uNrEcOgNiZeD tOkEn TyPe (corruption?)";
    }
}
#define tokname y_tok_type_name

void y_dump_tree(y_value *v) {
    size_t i;
    
    if (v == NULL) {
        printf("(null)");
        return;
    }

    if (v->type == Y_UNINITIALIZED) {
        warn("uninitialized!");
    } else if (v->type == Y_STRING) {
        printf("\"%s\"", v->string);
    } else if (v->type == Y_ARRAY) {
        printf("[");
        for (i = 0; v->array[i] != NULL; i++) {
            y_dump_tree(v->array[i]);
            printf(", ");
        }
        printf("]");
    } else if (v->type == Y_DICT) {
        printf("{");
        for (i = 0; v->dict.keys[i]; i++) {
            printf("\"%s\": ", v->dict.keys[i]);
            y_dump_tree(v->dict.values[i]);
            printf(", ");
        }
        if (v->dict.keys[i] != NULL || v->dict.values[i] != NULL) {
            warn("mismatched dict lengths");
        }
        printf("}");
    } else {
        warn("malformed type - freed?");
    }
}

void y_free_tree(y_value *v) {
    if (v == NULL || v->type == Y_UNINITIALIZED) {
        return;
    } else if (v->type == Y_STRING) {
        free(v->string);
    } else if (v->type == Y_ARRAY) {
        for (size_t i = 0; v->array[i] != NULL; i++) {
            y_free_tree(v->array[i]);
        }
        free(v->array);
    } else if (v->type == Y_DICT) {
        for (size_t i = 0; v->dict.keys[i] != NULL; i++) {
            free(v->dict.keys[i]);
            y_free_tree(v->dict.values[i]);
        }
        free(v->dict.keys);
        free(v->dict.values);
    } else {
        warn("malformed type - freed?");
        return;
    }
    v->type = Y_UNINITIALIZED;
    free(v);
}

static void next(yaml_parser_t *parser, yaml_token_t *token) {
    if (yaml_parser_scan(parser, token) == 0) {
        warn("broken document");
        exit(1);
    }
    dprintf("> %s\n", tokname(token->type));
}

static void wait_for(yaml_parser_t *parser, yaml_token_t *token,
                     yaml_token_type_t target) {
    next(parser, token);
    while (token->type != YAML_NO_TOKEN && token->type != target) {
        warn("skipping token type %s", tokname(token->type));
        yaml_token_delete(token);
        next(parser, token);
    }
}

/* Terminology:
 * - "block" is the weird yaml-y way of writing things; "flow" is json
 *  - "sequence" is array; "mapping" is dict
 *
 * I don't keep track of the difference between block and flow because this is
 * a *parser* and *that's the point*.
 */
static y_value *p_value(yaml_parser_t *parser) {
    yaml_token_t token;
    y_value *ret = NULL;
    size_t i = 0;

    while (1) {
        next(parser, &token);

        if (token.type == YAML_BLOCK_END_TOKEN ||
            token.type == YAML_FLOW_SEQUENCE_END_TOKEN ||
            token.type == YAML_FLOW_MAPPING_END_TOKEN ||
            token.type == YAML_NO_TOKEN) {
            /* 4 ways to start a block, but only 3 to end it.  Great job. */
            goto done;
        } else if (token.type == YAML_FLOW_ENTRY_TOKEN) {
            /* Not a helpful token type to have.  Next. */
        } else if (token.type == YAML_SCALAR_TOKEN) {
            ret = xalloc(sizeof(*ret));
            ret->type = Y_STRING;
            ret->string = xstrndup((char *)token.data.scalar.value,
                                   token.data.scalar.length);

            yaml_token_delete(&token);
            goto done;
        } else if (token.type == YAML_BLOCK_MAPPING_START_TOKEN ||
                   token.type == YAML_FLOW_MAPPING_START_TOKEN) {
            ret = xalloc(sizeof(*ret));
            ret->type = Y_DICT;

            while (1) {
                yaml_token_delete(&token);
                next(parser, &token);
                if (token.type == YAML_BLOCK_END_TOKEN ||
                    token.type == YAML_FLOW_MAPPING_END_TOKEN) {
                    /* Friggin YAML, I tell you hwat. */
                    break;
                } else if (token.type != YAML_KEY_TOKEN) {
                    fprintf(stderr, "expected YAML_KEY_TOKEN, got %s\n",
                            tokname(token.type));
                    free(ret);
                    ret = NULL;
                    goto done;
                }

                yaml_token_delete(&token);
                wait_for(parser, &token, YAML_SCALAR_TOKEN);

                /* We're going to write to i. */
                ret->dict.keys = xrealloc(ret->dict.keys,
                                          (i + 2) * sizeof(ret->dict.keys));
                ret->dict.keys[i + 1] = NULL;

                ret->dict.values = xrealloc(
                    ret->dict.values, (i + 2) * sizeof(ret->dict.values));
                ret->dict.values[i + 1] = NULL;

                ret->dict.keys[i] = xstrndup((char *)token.data.scalar.value,
                                             token.data.scalar.length);

                yaml_token_delete(&token);
                wait_for(parser, &token, YAML_VALUE_TOKEN);

                ret->dict.values[i] = p_value(parser);
                if (ret->dict.values[i] == NULL) {
                    /* Failed to parse value, but ate BLOCK_END. */
                    goto done;
                }

                i++;
            }
            goto done;
        } else if (token.type == YAML_BLOCK_SEQUENCE_START_TOKEN ||
                   token.type == YAML_FLOW_SEQUENCE_START_TOKEN) {
            if (token.type == YAML_BLOCK_SEQUENCE_START_TOKEN) {
                /* Only block sequence, not flow, and not no-seq.  Why. */
                yaml_token_delete(&token);
                wait_for(parser, &token, YAML_BLOCK_ENTRY_TOKEN);
            }

            ret = xalloc(sizeof(*ret));
            ret->type = Y_ARRAY;

            while (1) {
                /* Allocation strategy same as dict case. */
                ret->array = xrealloc(ret->array,
                                      (i + 2) * sizeof(ret->array));
                ret->array[i + 1] = NULL;

                ret->array[i] = p_value(parser);
                if (ret->array[i] == NULL) {
                    /* Yes, it's slightly larger.  No, I don't care. */
                    break;
                }

                i++;
            }
            goto done;
        } else {
            warn("skipping token type %s", tokname(token.type));
        }

        yaml_token_delete(&token);
    }

done:
    yaml_token_delete(&token);
    return ret;
}

y_value *y_parse_yaml(const char *filename) {
    FILE *fp = NULL;
    yaml_parser_t parser;
    yaml_token_t token;
    y_value *value;

    if (filename == NULL) {
        warn("NULL filename");
        return NULL;
    }

    /* prepare a YAML parser */
    if (!yaml_parser_initialize(&parser)) {
        warn("yaml_parser_initialize");
        return NULL;
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        warn("fopen");
        return NULL;
    }

    yaml_parser_set_input_file(&parser, fp);

    /* I don't want to handle aliases.  Don't do that. */
    wait_for(&parser, &token, YAML_STREAM_START_TOKEN);
    yaml_token_delete(&token);

    value = p_value(&parser);

    yaml_parser_delete(&parser);
    fclose(fp);
    return value;
}

/* Internal, recursive lookup. */
static const y_value *fetch(y_value *tree, const char *query,
                            uint8_t match_behavior) {
    size_t index = 0, key_len;
    const char *key;
    y_value *match = NULL;

    if (*query == '\0') {
        return tree;
    } else if (tree->type == Y_UNINITIALIZED) {
        warn("internal error - unexpected unitialized node");
        return NULL;
    } else if (tree->type == Y_STRING) {
        warn("reached terminal node, but query is not exhausted");
        return NULL;
    } else if (tree->type == Y_ARRAY) {
        if (*query != '[') {
            warn("type mismatch: expected ARRAY, but query disagreed");
            return NULL;
        }

        for (query++; *query != ']'; query++) {
            index *= 10;
            index += *query - '0';
        }
        query++; /* eat ']' */

        for (size_t i = 0; i < index; i++) {
            if (tree->array[i] == NULL) {
                warn("index %ld is beyond array bounds (%ld elements)",
                     index, i);
                return NULL;
            }
        }
        return fetch(tree->array[index], query, match_behavior);
    }

    /* dict. */
    if (*query != '.') {
        warn("type mismatch: expected DICT, but query disagreed");
        return NULL;
    }
    query++;

    for (key = query; *query != '.' && *query != '[' && *query != '\0';
         query++);
    key_len = (size_t)query - (size_t)key;

    for (size_t i = 0; tree->dict.keys[i] != NULL; i++) {
        if (strncmp(key, tree->dict.keys[i], key_len) ||
            tree->dict.keys[i][key_len] != '\0') {
            continue;
        } else if (match_behavior == Y_MATCH_ERROR && match != NULL) {
            warn("duplicate matching keys in dict for %s",
                 tree->dict.keys[i]);
            return NULL;
        }
        match = tree->dict.values[i];
        if (match_behavior == Y_MATCH_FIRST) {
            break;
        }
    }
    if (match == NULL) {
        return NULL;
    }
    return fetch(match, query, match_behavior);
}

const y_value *y_get(y_value *tree, const char *query,
                     uint8_t match_behavior) {
    bool in_array = false;

    if (tree == NULL || query == NULL || match_behavior > Y_MATCH_ERROR) {
        return NULL;
    }

    /* Quick validity check. */
    for (size_t i = 0; query[i] != '\0'; i++) {
        if (query[i] == '[') {
            if (in_array) {
                warn("query has mismatched delimiters ('[' inside '[')");
                return NULL;
            }
            in_array = true;

            if (query[i + 1] == ']') {
                warn("query contains invalid subsequence []");
                return NULL;
            }
            continue;
        } else if (query[i] == ']') {
            if (!in_array) {
                warn("query has mismatched delimiters (unexpected ']')");
                return NULL;
            }
            in_array = false;
            continue;
        } else if (in_array && (query[i] < '0' || query[i] > '9')) {
            warn("query has invalid character for array access '%c'",
                 query[i]);
            return NULL;
        }
    }
    if (in_array) {
        warn("query is missing closing delimiter for array access");
    }
    return fetch(tree, query, match_behavior);
}

const char *y_getstr(y_value *tree, const char *query,
                     uint8_t match_behavior) {
    const y_value *ret = y_get(tree, query, match_behavior);
    if (ret == NULL || ret->type != Y_STRING) {
        return NULL;
    }
    return ret->string;
}

/* Local variables: */
/* c-basic-offset: 4 */
/* indent-tabs-mode: nil */
/* End: */
