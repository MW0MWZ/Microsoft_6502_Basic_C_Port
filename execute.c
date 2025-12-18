/*
 * execute.c - Program execution engine
 *
 * Microsoft BASIC 6502 C Port
 * K&R C v2 compatible
 */

#include "m6502basic.h"

/*
 * Skip to end of line
 */
void
skip_to_eol()
{
    while (peek_char() != '\0') {
        get_next_char();
    }
}

/*
 * Execute a single statement
 */
void
execute_statement()
{
    int token;
    int c;

    skip_spaces();

    c = peek_char();
    if (c == '\0') {
        return;
    }

    /* Statement separator */
    if (c == ':') {
        get_next_char();
        skip_spaces();
        c = peek_char();
    }

    if (c == '\0') {
        return;
    }

    /* Token-based statement */
    if (c & 0x80) {
        token = get_next_char() & 0xFF;

        switch (token) {
            case TOK_PRINT:
                do_print();
                break;

            case TOK_INPUT:
                do_input();
                break;

            case TOK_LET:
                do_let();
                break;

            case TOK_IF:
                do_if();
                break;

            case TOK_GOTO:
                do_goto();
                break;

            case TOK_GOSUB:
                do_gosub();
                break;

            case TOK_RETURN:
                do_return();
                break;

            case TOK_FOR:
                do_for();
                break;

            case TOK_NEXT:
                do_next();
                break;

            case TOK_DIM:
                do_dim();
                break;

            case TOK_DATA:
                do_data();
                break;

            case TOK_READ:
                do_read();
                break;

            case TOK_RESTORE:
                do_restore();
                break;

            case TOK_END:
                do_end();
                break;

            case TOK_STOP:
                do_stop();
                break;

            case TOK_CONT:
                do_cont();
                break;

            case TOK_NEW:
                do_new();
                break;

            case TOK_LIST:
                do_list();
                break;

            case TOK_RUN:
                do_run();
                break;

            case TOK_LOAD:
                do_load();
                break;

            case TOK_SAVE:
                do_save();
                break;

            case TOK_POKE:
                do_poke();
                break;

            case TOK_WAIT:
                do_wait();
                break;

            case TOK_ON:
                do_on();
                break;

            case TOK_DEF:
                do_def();
                break;

            case TOK_CLEAR:
                do_clear();
                break;

            case TOK_GET:
                do_get();
                break;

            case TOK_REM:
                skip_to_eol();
                break;

            default:
                printf("?UNKNOWN TOKEN %d\n", token);
                syntax_error();
                break;
        }
    } else if (IS_ALPHA(c)) {
        /* Implicit LET */
        do_let();
    } else {
        syntax_error();
    }
}

/*
 * Run program from specified line
 */
void
run_program(startline)
int startline;
{
    unsigned char *p;
    line_t *line;
    line_t *next_line;
    int prev_line;

    /* Mark as running */
    g_state->running = 1;

    /* Find starting line */
    if (startline == 0) {
        p = g_state->txttab;
        if (p[0] == 0 && p[1] == 0) {
            g_state->running = 0;
            return;
        }
        line = (line_t *)p;
    } else {
        line = find_line(startline);
        if (!line) {
            error(ERR_UNDEF_STMT);
            g_state->running = 0;
            return;
        }
    }

    /* Set up execution context */
    g_state->curlin = line->linenum;
    g_state->txtptr = line->text;
    g_state->curline_ptr = line;

    /* Set up error handler */
    if (setjmp(g_state->errtrap) != 0) {
        if (g_state->errnum != ERR_NONE) {
            if (g_state->errlin >= 0) {
                printf("?%s IN %d\n", error_message(g_state->errnum), g_state->errlin);
            } else {
                printf("?%s\n", error_message(g_state->errnum));
            }
            g_state->errnum = ERR_NONE;
        }
        g_state->running = 0;
        return;
    }

    /* Main execution loop */
    while (g_state->running) {
        prev_line = g_state->curlin;

        /* Execute statements on current line */
        while (peek_char() != '\0' && g_state->running) {
            prev_line = g_state->curlin;
            execute_statement();

            /* Jump occurred */
            if (g_state->curlin != prev_line) {
                break;
            }

            skip_spaces();
            if (peek_char() == ':') {
                get_next_char();
            } else {
                break;
            }
        }

        /* Move to next line if no jump */
        if (g_state->running && g_state->curlin == prev_line) {
            line = g_state->curline_ptr;
            p = ((unsigned char *)line) + line->len;

            if (p[0] == 0 && p[1] == 0) {
                /* End of program */
                g_state->running = 0;
            } else {
                next_line = (line_t *)p;
                g_state->curlin = next_line->linenum;
                g_state->txtptr = next_line->text;
                g_state->curline_ptr = next_line;
            }
        }
    }
}
