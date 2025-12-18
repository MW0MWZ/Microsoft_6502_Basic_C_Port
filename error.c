/*
 * error.c - Error handling
 *
 * Microsoft BASIC 6502 C Port
 * K&R C v2 compatible
 */

#include "m6502basic.h"

/* Error messages - short form like original 6502 BASIC */
static struct {
    int code;
    const char *msg;
} error_table[] = {
    { ERR_NEXT_NO_FOR,  "NF ERROR" },  /* NEXT without FOR */
    { ERR_SYNTAX,       "SN ERROR" },  /* Syntax error */
    { ERR_RETURN,       "RG ERROR" },  /* RETURN without GOSUB */
    { ERR_OUT_OF_DATA,  "OD ERROR" },  /* Out of DATA */
    { ERR_ILLEGAL_FUNC, "FC ERROR" },  /* Illegal function call */
    { ERR_OVERFLOW,     "OV ERROR" },  /* Overflow */
    { ERR_OUT_OF_MEM,   "OM ERROR" },  /* Out of memory */
    { ERR_UNDEF_STMT,   "US ERROR" },  /* Undefined statement */
    { ERR_SUBSCRIPT,    "BS ERROR" },  /* Bad subscript */
    { ERR_REDIM,        "DD ERROR" },  /* Redimensioned array */
    { ERR_DIV_ZERO,     "/0 ERROR" },  /* Division by zero */
    { ERR_ILLEGAL_DIR,  "ID ERROR" },  /* Illegal direct */
    { ERR_TYPE_MISM,    "TM ERROR" },  /* Type mismatch */
    { ERR_OUT_OF_STR,   "LS ERROR" },  /* Out of string space */
    { ERR_STRING_LONG,  "ST ERROR" },  /* String too long */
    { ERR_CANT_CONT,    "CN ERROR" },  /* Can't continue */
    { ERR_UNDEF_FUNC,   "UF ERROR" },  /* Undefined function */
    { 0, NULL }
};

/*
 * Get error message for error code
 */
const char *
error_message(errnum)
int errnum;
{
    int i;

    for (i = 0; error_table[i].msg != NULL; i++) {
        if (error_table[i].code == errnum) {
            return error_table[i].msg;
        }
    }

    return "ERROR";
}

/*
 * Raise an error
 */
void
error(errnum)
int errnum;
{
    g_state->errnum = errnum;
    g_state->errlin = g_state->curlin;

    /* Save state for CONT */
    if (g_state->running) {
        g_state->oldlin = g_state->curlin;
        g_state->oldtxt = g_state->txtptr;
    }

    /* Stop execution */
    g_state->running = 0;

    /* Jump to error handler */
    longjmp(g_state->errtrap, 1);
}

/*
 * Syntax error
 */
void
syntax_error()
{
    error(ERR_SYNTAX);
}
