/*
 * arrays.c - Array storage and management
 *
 * Microsoft BASIC 6502 C Port
 * K&R C v2 compatible
 */

#include "m6502basic.h"

/*
 * Find an array by name
 */
array_t *
find_array(name, create)
const char *name;
int create;
{
    array_t *arr;
    char normname[NAMLEN+1];
    const char *s;
    char *d;
    int type;

    /* Normalize name */
    d = normname;
    s = name;
    type = TYPE_NUM;

    while (*s && d < normname + NAMLEN) {
        if (IS_ALNUM(*s)) {
            if (*s >= 'a' && *s <= 'z') {
                *d++ = *s - 'a' + 'A';
            } else {
                *d++ = *s;
            }
        } else if (*s == '$') {
            type = TYPE_STR;
            break;
        } else if (*s == '%') {
            type = TYPE_INT;
            break;
        } else {
            break;
        }
        s++;
    }
    *d = '\0';

    /* Search existing arrays */
    for (arr = g_state->arrlist; arr != NULL; arr = arr->next) {
        if (strcmp(arr->name, normname) == 0 && arr->type == type) {
            return arr;
        }
    }

    /* Not found */
    if (!create) {
        return NULL;
    }

    /* Create with default dimension (10) */
    arr = (array_t *)malloc(sizeof(array_t));
    if (!arr) {
        error(ERR_OUT_OF_MEM);
        return NULL;
    }

    strcpy(arr->name, normname);
    arr->type = type;
    arr->ndims = 1;
    arr->dims[0] = 11;  /* 0-10 = 11 elements */
    arr->size = 11;

    if (type == TYPE_STR) {
        arr->data.strdata = (string_t **)calloc(arr->size, sizeof(string_t *));
    } else {
        arr->data.numdata = (double *)calloc(arr->size, sizeof(double));
    }

    if ((type == TYPE_STR && !arr->data.strdata) ||
        (type != TYPE_STR && !arr->data.numdata)) {
        free(arr);
        error(ERR_OUT_OF_MEM);
        return NULL;
    }

    /* Add to list */
    arr->next = g_state->arrlist;
    g_state->arrlist = arr;

    return arr;
}

/*
 * Dimension an array
 */
void
dimension_array(name, dims, ndims, type)
const char *name;
int *dims;
int ndims;
int type;
{
    array_t *arr;
    char normname[NAMLEN+1];
    const char *s;
    char *d;
    int size, i;

    /* Normalize name */
    d = normname;
    s = name;

    while (*s && d < normname + NAMLEN) {
        if (IS_ALNUM(*s)) {
            if (*s >= 'a' && *s <= 'z') {
                *d++ = *s - 'a' + 'A';
            } else {
                *d++ = *s;
            }
        } else {
            break;
        }
        s++;
    }
    *d = '\0';

    /* Check if already exists */
    for (arr = g_state->arrlist; arr != NULL; arr = arr->next) {
        if (strcmp(arr->name, normname) == 0 && arr->type == type) {
            error(ERR_REDIM);
            return;
        }
    }

    /* Calculate total size */
    size = 1;
    for (i = 0; i < ndims; i++) {
        size *= dims[i];
    }

    /* Allocate array */
    arr = (array_t *)malloc(sizeof(array_t));
    if (!arr) {
        error(ERR_OUT_OF_MEM);
        return;
    }

    strcpy(arr->name, normname);
    arr->type = type;
    arr->ndims = ndims;
    arr->size = size;

    for (i = 0; i < ndims; i++) {
        arr->dims[i] = dims[i];
    }

    if (type == TYPE_STR) {
        arr->data.strdata = (string_t **)calloc(size, sizeof(string_t *));
        if (!arr->data.strdata) {
            free(arr);
            error(ERR_OUT_OF_MEM);
            return;
        }
    } else {
        arr->data.numdata = (double *)calloc(size, sizeof(double));
        if (!arr->data.numdata) {
            free(arr);
            error(ERR_OUT_OF_MEM);
            return;
        }
    }

    /* Add to list */
    arr->next = g_state->arrlist;
    g_state->arrlist = arr;
}

/*
 * Get pointer to numeric array element
 */
double *
array_num_element(name, indices, nindices)
const char *name;
int *indices;
int nindices;
{
    array_t *arr;
    int offset, i, mult;

    arr = find_array(name, 1);
    if (!arr) {
        return NULL;
    }

    if (arr->type == TYPE_STR) {
        error(ERR_TYPE_MISM);
        return NULL;
    }

    if (nindices != arr->ndims) {
        error(ERR_SUBSCRIPT);
        return NULL;
    }

    /* Calculate offset */
    offset = 0;
    mult = 1;
    for (i = arr->ndims - 1; i >= 0; i--) {
        if (indices[i] < 0 || indices[i] >= arr->dims[i]) {
            error(ERR_SUBSCRIPT);
            return NULL;
        }
        offset += indices[i] * mult;
        mult *= arr->dims[i];
    }

    return &arr->data.numdata[offset];
}

/*
 * Get pointer to string array element
 */
string_t **
array_str_element(name, indices, nindices)
const char *name;
int *indices;
int nindices;
{
    array_t *arr;
    int offset, i, mult;

    arr = find_array(name, 1);
    if (!arr) {
        return NULL;
    }

    if (arr->type != TYPE_STR) {
        error(ERR_TYPE_MISM);
        return NULL;
    }

    if (nindices != arr->ndims) {
        error(ERR_SUBSCRIPT);
        return NULL;
    }

    /* Calculate offset */
    offset = 0;
    mult = 1;
    for (i = arr->ndims - 1; i >= 0; i--) {
        if (indices[i] < 0 || indices[i] >= arr->dims[i]) {
            error(ERR_SUBSCRIPT);
            return NULL;
        }
        offset += indices[i] * mult;
        mult *= arr->dims[i];
    }

    return &arr->data.strdata[offset];
}

/*
 * Clear all arrays
 */
void
clear_arrays()
{
    array_t *arr;
    array_t *next;
    int i;

    arr = g_state->arrlist;
    while (arr != NULL) {
        next = arr->next;

        if (arr->type == TYPE_STR && arr->data.strdata) {
            for (i = 0; i < arr->size; i++) {
                if (arr->data.strdata[i]) {
                    free_string(arr->data.strdata[i]);
                }
            }
            free(arr->data.strdata);
        } else if (arr->data.numdata) {
            free(arr->data.numdata);
        }

        free(arr);
        arr = next;
    }

    g_state->arrlist = NULL;
}
