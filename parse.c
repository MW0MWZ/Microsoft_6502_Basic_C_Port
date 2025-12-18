/*
 * parse.c - Program line management
 *
 * Microsoft BASIC 6502 C Port
 * K&R C v2 compatible
 */

#include "m6502basic.h"

/*
 * Find a line by number
 */
line_t *
find_line(linenum)
int linenum;
{
    unsigned char *p;
    line_t *line;

    p = g_state->txttab;

    while (p[0] != 0 || p[1] != 0) {
        line = (line_t *)p;

        if (line->linenum == linenum) {
            return line;
        }
        if (line->linenum > linenum) {
            return NULL;
        }

        p += line->len;
    }

    return NULL;
}

/*
 * Delete a line
 */
void
delete_line(linenum)
int linenum;
{
    unsigned char *p;
    unsigned char *end;
    line_t *line;
    int linelen;

    p = g_state->txttab;

    while (p[0] != 0 || p[1] != 0) {
        line = (line_t *)p;

        if (line->linenum == linenum) {
            /* Found the line - remove it */
            linelen = line->len;
            end = g_state->vartab;

            /* Move everything after this line up */
            memmove(p, p + linelen, end - (p + linelen));

            /* Update memory pointers */
            g_state->vartab -= linelen;
            g_state->arytab -= linelen;
            g_state->strend -= linelen;

            return;
        }

        if (line->linenum > linenum) {
            return;  /* Line not found */
        }

        p += line->len;
    }
}

/*
 * Insert a line
 */
void
insert_line(linenum, tokens, len)
int linenum;
unsigned char *tokens;
int len;
{
    unsigned char *p;
    unsigned char *insert_point;
    line_t *line;
    int header_size;
    int total_len;
    int move_len;

    /* First delete any existing line with this number */
    delete_line(linenum);

    /* Calculate line structure size */
    header_size = sizeof(int) + sizeof(int);  /* linenum + len */

    /* Word-align for PDP-11 */
#if IS_16BIT
    if (header_size & 1) header_size++;
#endif

    total_len = header_size + len;

    /* Word-align total length */
#if IS_16BIT
    if (total_len & 1) total_len++;
#endif

    /* Check if there's room */
    if (g_state->vartab + total_len >= g_state->fretop) {
        error(ERR_OUT_OF_MEM);
        return;
    }

    /* Find insertion point (keep lines sorted) */
    p = g_state->txttab;
    insert_point = p;

    while (p[0] != 0 || p[1] != 0) {
        line = (line_t *)p;

        if (line->linenum > linenum) {
            insert_point = p;
            break;
        }

        p += line->len;
        insert_point = p;
    }

    /* Move everything after insertion point down */
    move_len = g_state->vartab - insert_point;
    if (move_len > 0) {
        memmove(insert_point + total_len, insert_point, move_len);
    }

    /* Insert new line */
    line = (line_t *)insert_point;
    line->linenum = linenum;
    line->len = total_len;
    memcpy(line->text, tokens, len);

    /* Update memory pointers */
    g_state->vartab += total_len;
    g_state->arytab += total_len;
    g_state->strend += total_len;
}

/*
 * List program lines
 */
void
list_program(start, end)
int start;
int end;
{
    unsigned char *p;
    line_t *line;
    char *text;

    p = g_state->txttab;

    while (p[0] != 0 || p[1] != 0) {
        line = (line_t *)p;

        if (line->linenum >= start && line->linenum <= end) {
            text = detokenize_line(line->text);
            if (text) {
                printf("%d %s\n", line->linenum, text);
                free(text);
            }
        }

        if (line->linenum > end) {
            break;
        }

        p += line->len;
    }
}

/*
 * Clear program and variables
 */
void
new_program()
{
    /* Clear variables */
    clear_variables();

    /* Clear arrays */
    clear_arrays();

    /* Reset program area */
    g_state->txttab[0] = 0;
    g_state->txttab[1] = 0;
    g_state->vartab = g_state->txttab;
    g_state->arytab = g_state->txttab;
    g_state->strend = g_state->txttab;
    g_state->fretop = g_state->memsiz;

    /* Reset execution state */
    g_state->forsp = 0;
    g_state->gosubsp = 0;
    g_state->dataptr.linenum = 0;
    g_state->dataptr.ptr = NULL;
    g_state->curlin = -1;
    g_state->running = 0;
}
