/*
 * strings.c - String handling
 *
 * Microsoft BASIC 6502 C Port
 * K&R C v2 compatible
 */

#include "m6502basic.h"

/*
 * Allocate a new string
 */
string_t *
alloc_string(len)
int len;
{
    string_t *str;

    str = (string_t *)malloc(sizeof(string_t));
    if (!str) {
        error(ERR_OUT_OF_STR);
        return NULL;
    }

    if (len > 0) {
        str->ptr = (char *)malloc(len + 1);
        if (!str->ptr) {
            free(str);
            error(ERR_OUT_OF_STR);
            return NULL;
        }
        str->ptr[0] = '\0';
    } else {
        str->ptr = NULL;
    }

    str->len = len;
    return str;
}

/*
 * Free a string
 */
void
free_string(str)
string_t *str;
{
    if (str) {
        if (str->ptr) {
            free(str->ptr);
        }
        free(str);
    }
}

/*
 * Copy a string
 */
string_t *
copy_string(src)
string_t *src;
{
    string_t *dst;

    if (!src) {
        return alloc_string(0);
    }

    dst = alloc_string(src->len);
    if (dst && src->ptr && src->len > 0) {
        memcpy(dst->ptr, src->ptr, src->len);
        dst->ptr[src->len] = '\0';
    }

    return dst;
}

/*
 * Concatenate two strings
 */
string_t *
concat_strings(s1, s2)
string_t *s1;
string_t *s2;
{
    string_t *result;
    int len1, len2;

    len1 = s1 ? s1->len : 0;
    len2 = s2 ? s2->len : 0;

    if (len1 + len2 > 255) {
        error(ERR_STRING_LONG);
        return NULL;
    }

    result = alloc_string(len1 + len2);
    if (!result) {
        return NULL;
    }

    if (len1 > 0 && s1->ptr) {
        memcpy(result->ptr, s1->ptr, len1);
    }
    if (len2 > 0 && s2->ptr) {
        memcpy(result->ptr + len1, s2->ptr, len2);
    }
    result->ptr[len1 + len2] = '\0';

    return result;
}

/*
 * Compare two strings
 */
int
compare_strings(s1, s2)
string_t *s1;
string_t *s2;
{
    int len1, len2, minlen, result;

    len1 = s1 ? s1->len : 0;
    len2 = s2 ? s2->len : 0;

    if (len1 == 0 && len2 == 0) {
        return 0;
    }
    if (len1 == 0) {
        return -1;
    }
    if (len2 == 0) {
        return 1;
    }

    minlen = (len1 < len2) ? len1 : len2;
    result = memcmp(s1->ptr, s2->ptr, minlen);

    if (result != 0) {
        return result;
    }

    /* Strings equal up to minlen, compare lengths */
    if (len1 < len2) return -1;
    if (len1 > len2) return 1;
    return 0;
}

/*
 * Create string from C string
 */
string_t *
string_from_cstr(cstr)
const char *cstr;
{
    string_t *str;
    int len;

    if (!cstr) {
        return alloc_string(0);
    }

    len = strlen(cstr);
    if (len > 255) {
        len = 255;
    }

    str = alloc_string(len);
    if (str && len > 0) {
        memcpy(str->ptr, cstr, len);
        str->ptr[len] = '\0';
    }

    return str;
}

/*
 * Convert string to C string (caller must free)
 */
char *
string_to_cstr(str)
string_t *str;
{
    char *cstr;
    int len;

    if (!str || str->len == 0) {
        cstr = (char *)malloc(1);
        if (cstr) {
            cstr[0] = '\0';
        }
        return cstr;
    }

    len = str->len;
    cstr = (char *)malloc(len + 1);
    if (cstr) {
        memcpy(cstr, str->ptr, len);
        cstr[len] = '\0';
    }

    return cstr;
}
