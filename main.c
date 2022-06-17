/* SPDX-License-Identifier: MPL-2.0 */

#include <assert.h>
#include <tyranny.h>

int main(int argc, char *argv[]) {
    y_value *val;

    assert(argc >= 2);

    val = y_parse_yaml(argv[1]);

    printf("\n\n");

    printf("Parsed structure as:\n");
    y_dump_tree(val);
    printf("\n");

    y_free_tree(val);
    return 0;
}

/* Local variables: */
/* c-basic-offset: 4 */
/* indent-tabs-mode: nil */
/* End: */
