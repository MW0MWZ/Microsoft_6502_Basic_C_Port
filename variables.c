/*
 * variables.c - Variable storage and management
 *
 * Microsoft BASIC 6502 C Port
 * K&R C v2 compatible
 */

#include "m6502basic.h"

/*
 * Normalize variable name (2 chars, uppercase)
 */
static void
normalize_name(dest, src, type)
char *dest;
const char *src;
int *type;
{
    int i;

    *type = TYPE_NUM;
    i = 0;

    /* Copy up to 2 characters, convert to uppercase */
    while (*src && i < NAMLEN) {
        if (IS_ALNUM(*src)) {
            if (*src >= 'a' && *src <= 'z') {
                dest[i++] = *src - 'a' + 'A';
            } else {
                dest[i++] = *src;
            }
        } else if (*src == '$') {
            *type = TYPE_STR;
            break;
        } else if (*src == '%') {
            *type = TYPE_INT;
            break;
        } else {
            break;
        }
        src++;
    }
    dest[i] = '\0';
}

/*
 * Find a variable by name, optionally create
 */
var_t *
find_variable(name, create)
const char *name;
int create;
{
    var_t *var;
    char normname[NAMLEN+1];
    int type;

    normalize_name(normname, name, &type);

    /* Search existing variables */
    for (var = g_state->varlist; var != NULL; var = var->next) {
        if (strcmp(var->name, normname) == 0 && var->type == type) {
            return var;
        }
    }

    /* Not found - create if requested */
    if (create) {
        var = (var_t *)malloc(sizeof(var_t));
        if (!var) {
            error(ERR_OUT_OF_MEM);
            return NULL;
        }

        strcpy(var->name, normname);
        var->type = type;

        if (type == TYPE_STR) {
            var->value.strval = alloc_string(0);
        } else {
            var->value.numval = 0.0;
        }

        /* Add to list */
        var->next = g_state->varlist;
        g_state->varlist = var;

        return var;
    }

    return NULL;
}

/*
 * Set numeric variable value
 */
void
set_num_variable(name, val)
const char *name;
double val;
{
    var_t *var;

    var = find_variable(name, 1);
    if (var && var->type != TYPE_STR) {
        var->value.numval = val;
    }
}

/*
 * Set string variable value
 */
void
set_str_variable(name, val)
const char *name;
string_t *val;
{
    var_t *var;

    var = find_variable(name, 1);
    if (var && var->type == TYPE_STR) {
        if (var->value.strval) {
            free_string(var->value.strval);
        }
        var->value.strval = copy_string(val);
    }
}

/*
 * Get numeric variable value
 */
double
get_num_variable(name)
const char *name;
{
    var_t *var;

    var = find_variable(name, 0);
    if (var && var->type != TYPE_STR) {
        return var->value.numval;
    }

    return 0.0;
}

/*
 * Get string variable value
 */
string_t *
get_str_variable(name)
const char *name;
{
    var_t *var;

    var = find_variable(name, 0);
    if (var && var->type == TYPE_STR) {
        return var->value.strval;
    }

    return NULL;
}

/*
 * Clear all variables
 */
void
clear_variables()
{
    var_t *var;
    var_t *next;

    var = g_state->varlist;
    while (var != NULL) {
        next = var->next;

        if (var->type == TYPE_STR && var->value.strval) {
            free_string(var->value.strval);
        }

        free(var);
        var = next;
    }

    g_state->varlist = NULL;
}
