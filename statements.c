/*
 * statements.c - BASIC statement implementations
 *
 * Microsoft BASIC 6502 C Port
 * K&R C v2 compatible
 */

#include "m6502basic.h"

/*
 * PRINT statement
 */
void
do_print()
{
    double num;
    string_t *str;
    int c;
    int newline;
    int tabpos;
    unsigned char *save;
    char name[NAMLEN+3];
    int type, i;
    int indices[11];
    int nindices;
    string_t **elem;

    newline = 1;

    while (1) {
        skip_spaces();
        c = peek_char();

        /* End of statement */
        if (c == '\0' || c == ':') {
            break;
        }

        /* Semicolon - suppress spacing */
        if (c == ';') {
            get_next_char();
            newline = 0;
            continue;
        }

        /* Comma - tab to next zone */
        if (c == ',') {
            get_next_char();
            tabpos = ((g_state->trmpos / CLMWID) + 1) * CLMWID;
            while (g_state->trmpos < tabpos) {
                putchar(' ');
                g_state->trmpos++;
            }
            newline = 0;
            continue;
        }

        /* TAB function */
        if ((c & 0xFF) == TOK_TAB) {
            get_next_char();
            skip_spaces();
            if (peek_char() == '(') {
                get_next_char();
            }
            tabpos = eval_integer();
            skip_spaces();
            if (peek_char() == ')') {
                get_next_char();
            }
            while (g_state->trmpos < tabpos - 1) {
                putchar(' ');
                g_state->trmpos++;
            }
            newline = 0;
            continue;
        }

        /* SPC function */
        if ((c & 0xFF) == TOK_SPC) {
            get_next_char();
            skip_spaces();
            if (peek_char() == '(') {
                get_next_char();
            }
            tabpos = eval_integer();
            skip_spaces();
            if (peek_char() == ')') {
                get_next_char();
            }
            while (tabpos-- > 0) {
                putchar(' ');
                g_state->trmpos++;
            }
            newline = 0;
            continue;
        }

        /* String functions - CHR$, STR$, LEFT$, RIGHT$, MID$ */
        if ((c & 0xFF) == TOK_CHR || (c & 0xFF) == TOK_STR ||
            (c & 0xFF) == TOK_LEFT || (c & 0xFF) == TOK_RIGHT ||
            (c & 0xFF) == TOK_MID) {
            str = eval_string();
            if (str && str->ptr) {
                printf("%s", str->ptr);
                g_state->trmpos += str->len;
            }
            if (str) free_string(str);
            newline = 1;
            continue;
        }

        /* String literal */
        if (c == '"') {
            str = parse_string_literal();
            if (str && str->ptr) {
                printf("%s", str->ptr);
                g_state->trmpos += str->len;
            }
            if (str) free_string(str);
            newline = 1;
            continue;
        }

        /* String variable */
        if (IS_ALPHA(c)) {
            /* Look ahead for $ */
            save = g_state->txtptr;

            i = 0;
            while (IS_ALNUM(peek_char()) && i < NAMLEN) {
                name[i++] = get_next_char();
            }
            name[i] = '\0';

            if (peek_char() == '$') {
                get_next_char();
                /* String variable */
                name[i++] = '$';
                name[i] = '\0';

                /* Check for array */
                skip_spaces();
                if (peek_char() == '(') {
                    nindices = 0;

                    get_next_char();
                    while (nindices < 11) {
                        indices[nindices++] = eval_integer();
                        skip_spaces();
                        if (peek_char() == ',') {
                            get_next_char();
                        } else {
                            break;
                        }
                    }
                    skip_spaces();
                    if (peek_char() == ')') get_next_char();

                    elem = array_str_element(name, indices, nindices);
                    if (elem && *elem && (*elem)->ptr) {
                        printf("%s", (*elem)->ptr);
                        g_state->trmpos += (*elem)->len;
                    }
                } else {
                    str = get_str_variable(name);
                    if (str && str->ptr) {
                        printf("%s", str->ptr);
                        g_state->trmpos += str->len;
                    }
                }
                newline = 1;
                continue;
            }

            /* Restore position - it's a numeric expression */
            g_state->txtptr = save;
        }

        /* Numeric expression */
        num = eval_expr();
        printf("%g", num);
        g_state->trmpos += 10;  /* Approximate */
        newline = 1;
    }

    if (newline) {
        printf("\n");
        g_state->trmpos = 0;
    }
}

/*
 * INPUT statement
 */
void
do_input()
{
    char varname[NAMLEN+3];
    char *line;
    char *p;
    int type, i;
    double num;
    var_t *var;
    string_t *prompt;

    /* Check for prompt string */
    skip_spaces();
    if (peek_char() == '"') {
        prompt = parse_string_literal();
        if (prompt && prompt->ptr) {
            printf("%s", prompt->ptr);
        }
        if (prompt) free_string(prompt);

        skip_spaces();
        if (peek_char() == ';') {
            get_next_char();
        }
    } else {
        printf("? ");
    }
    fflush(stdout);

    /* Read input line */
    if (fgets(g_state->inputbuf, BUFLEN, stdin) == NULL) {
        return;
    }

    line = g_state->inputbuf;
    i = strlen(line);
    if (i > 0 && line[i-1] == '\n') line[i-1] = '\0';

    /* Parse variable names and assign values */
    while (1) {
        skip_spaces();

        /* Get variable name */
        i = 0;
        type = TYPE_NUM;
        while (IS_ALNUM(peek_char()) && i < NAMLEN) {
            varname[i++] = get_next_char();
        }
        if (peek_char() == '$') {
            get_next_char();
            type = TYPE_STR;
            varname[i++] = '$';
        }
        varname[i] = '\0';

        if (i == 0) break;

        /* Skip whitespace in input */
        while (*line == ' ' || *line == '\t') line++;

        /* Get value */
        if (type == TYPE_STR) {
            /* Find end of string value */
            p = line;
            while (*p && *p != ',') p++;
            *p = '\0';
            set_str_variable(varname, string_from_cstr(line));
            line = (*p) ? p + 1 : p;
        } else {
            num = atof(line);
            set_num_variable(varname, num);
            while (*line && *line != ',') line++;
            if (*line == ',') line++;
        }

        /* Check for more variables */
        skip_spaces();
        if (peek_char() == ',') {
            get_next_char();
        } else {
            break;
        }
    }
}

/*
 * LET statement
 */
void
do_let()
{
    char varname[NAMLEN+3];
    int type, i;
    double num;
    string_t *str;
    int indices[11];
    int nindices;
    char c;
    string_t **str_elem;
    double *num_elem;

    skip_spaces();

    /* Get variable name */
    i = 0;
    type = TYPE_NUM;
    while (IS_ALNUM(peek_char()) && i < NAMLEN) {
        c = get_next_char();
        if (c >= 'a' && c <= 'z') {
            varname[i++] = c - 'a' + 'A';
        } else {
            varname[i++] = c;
        }
    }
    if (peek_char() == '$') {
        get_next_char();
        type = TYPE_STR;
        varname[i++] = '$';
    } else if (peek_char() == '%') {
        get_next_char();
        type = TYPE_INT;
    }
    varname[i] = '\0';

    /* Check for array subscript */
    skip_spaces();
    if (peek_char() == '(') {
        get_next_char();
        nindices = 0;

        while (nindices < 11) {
            indices[nindices++] = eval_integer();
            skip_spaces();
            if (peek_char() == ',') {
                get_next_char();
            } else {
                break;
            }
        }

        skip_spaces();
        if (peek_char() == ')') {
            get_next_char();
        }

        /* Expect = */
        skip_spaces();
        if (peek_char() == '=' || match_token(TOK_EQ)) {
            if (peek_char() == '=') get_next_char();
        } else {
            syntax_error();
            return;
        }

        /* Assign to array element */
        if (type == TYPE_STR) {
            str_elem = array_str_element(varname, indices, nindices);
            str = eval_string();
            if (str_elem) {
                if (*str_elem) free_string(*str_elem);
                *str_elem = str;
            } else if (str) {
                free_string(str);
            }
        } else {
            num_elem = array_num_element(varname, indices, nindices);
            num = eval_expr();
            if (num_elem) *num_elem = num;
        }
    } else {
        /* Simple variable */

        /* Expect = */
        skip_spaces();
        if (peek_char() == '=' || match_token(TOK_EQ)) {
            if (peek_char() == '=') get_next_char();
        } else {
            syntax_error();
            return;
        }

        if (type == TYPE_STR) {
            str = eval_string();
            set_str_variable(varname, str);
            if (str) free_string(str);
        } else {
            num = eval_expr();
            set_num_variable(varname, num);
        }
    }
}

/*
 * IF statement
 */
void
do_if()
{
    double condition;
    int c;
    int depth;
    int ch;

    condition = eval_expr();

    /* Expect THEN or GOTO */
    skip_spaces();
    c = peek_char();

    if ((c & 0xFF) == TOK_THEN) {
        get_next_char();
    } else if ((c & 0xFF) == TOK_GOTO) {
        /* GOTO handled below */
    }

    if (condition == 0.0) {
        /* False - skip to ELSE or end of line */
        depth = 1;
        while (peek_char() != '\0') {
            ch = peek_char();
            if ((ch & 0xFF) == TOK_IF) {
                depth++;
            }
            /* Check for ELSE at same depth - not fully implemented in 6502 BASIC */
            get_next_char();
        }
        return;
    }

    /* True - check for line number or execute statement */
    skip_spaces();
    if (IS_DIGIT(peek_char())) {
        /* Line number - do GOTO */
        do_goto();
    } else {
        /* Execute the statement after THEN */
        execute_statement();
        /* Continue with any additional statements on the line */
        while (peek_char() == ':') {
            get_next_char();
            execute_statement();
        }
    }
}

/*
 * GOTO statement
 */
void
do_goto()
{
    int linenum;
    line_t *line;

    linenum = eval_integer();

    line = find_line(linenum);
    if (!line) {
        error(ERR_UNDEF_STMT);
        return;
    }

    g_state->curlin = line->linenum;
    g_state->txtptr = line->text;
    g_state->curline_ptr = line;
}

/*
 * GOSUB statement
 */
void
do_gosub()
{
    int linenum;
    line_t *line;

    if (g_state->gosubsp >= 26) {
        error(ERR_OUT_OF_MEM);
        return;
    }

    linenum = eval_integer();

    line = find_line(linenum);
    if (!line) {
        error(ERR_UNDEF_STMT);
        return;
    }

    /* Push return address */
    g_state->gosubstack[g_state->gosubsp].linenum = g_state->curlin;
    g_state->gosubstack[g_state->gosubsp].txtptr = g_state->txtptr;
    g_state->gosubstack[g_state->gosubsp].line_ptr = g_state->curline_ptr;
    g_state->gosubsp++;

    /* Jump to subroutine */
    g_state->curlin = line->linenum;
    g_state->txtptr = line->text;
    g_state->curline_ptr = line;
}

/*
 * RETURN statement
 */
void
do_return()
{
    if (g_state->gosubsp == 0) {
        error(ERR_RETURN);
        return;
    }

    g_state->gosubsp--;
    g_state->curlin = g_state->gosubstack[g_state->gosubsp].linenum;
    g_state->txtptr = g_state->gosubstack[g_state->gosubsp].txtptr;
    g_state->curline_ptr = g_state->gosubstack[g_state->gosubsp].line_ptr;
}

/*
 * FOR statement
 */
void
do_for()
{
    char varname[NAMLEN+3];
    int i;
    double start, limit, step;
    char c;

    if (g_state->forsp >= 26) {
        error(ERR_OUT_OF_MEM);
        return;
    }

    /* Get variable name */
    skip_spaces();
    i = 0;
    while (IS_ALNUM(peek_char()) && i < NAMLEN) {
        c = get_next_char();
        if (c >= 'a' && c <= 'z') {
            varname[i++] = c - 'a' + 'A';
        } else {
            varname[i++] = c;
        }
    }
    varname[i] = '\0';

    /* Expect = */
    skip_spaces();
    if (peek_char() == '=' || match_token(TOK_EQ)) {
        if (peek_char() == '=') get_next_char();
    } else {
        syntax_error();
        return;
    }

    /* Get start value */
    start = eval_expr();
    set_num_variable(varname, start);

    /* Expect TO */
    skip_spaces();
    if (!match_token(TOK_TO)) {
        syntax_error();
        return;
    }

    /* Get limit */
    limit = eval_expr();

    /* Check for STEP */
    step = 1.0;
    skip_spaces();
    if (match_token(TOK_STEP)) {
        step = eval_expr();
    }

    /* Push FOR info */
    g_state->forstack[g_state->forsp].linenum = g_state->curlin;
    g_state->forstack[g_state->forsp].txtptr = g_state->txtptr;
    g_state->forstack[g_state->forsp].line_ptr = g_state->curline_ptr;
    strcpy(g_state->forstack[g_state->forsp].varname, varname);
    g_state->forstack[g_state->forsp].limit = limit;
    g_state->forstack[g_state->forsp].step = step;
    g_state->forsp++;
}

/*
 * NEXT statement
 */
void
do_next()
{
    char varname[NAMLEN+3];
    int i;
    double current, limit, step;
    int done;
    char c;

    /* Get variable name (optional) */
    skip_spaces();
    i = 0;
    if (IS_ALPHA(peek_char())) {
        while (IS_ALNUM(peek_char()) && i < NAMLEN) {
            c = get_next_char();
            if (c >= 'a' && c <= 'z') {
                varname[i++] = c - 'a' + 'A';
            } else {
                varname[i++] = c;
            }
        }
        varname[i] = '\0';
    } else {
        if (g_state->forsp == 0) {
            error(ERR_NEXT_NO_FOR);
            return;
        }
        strcpy(varname, g_state->forstack[g_state->forsp-1].varname);
    }

    if (g_state->forsp == 0) {
        error(ERR_NEXT_NO_FOR);
        return;
    }

    /* Get loop info */
    limit = g_state->forstack[g_state->forsp-1].limit;
    step = g_state->forstack[g_state->forsp-1].step;

    /* Increment variable */
    current = get_num_variable(varname);
    current += step;
    set_num_variable(varname, current);

    /* Check if done */
    done = 0;
    if (step >= 0) {
        if (current > limit) done = 1;
    } else {
        if (current < limit) done = 1;
    }

    if (done) {
        g_state->forsp--;
    } else {
        /* Loop back */
        g_state->curlin = g_state->forstack[g_state->forsp-1].linenum;
        g_state->txtptr = g_state->forstack[g_state->forsp-1].txtptr;
        g_state->curline_ptr = g_state->forstack[g_state->forsp-1].line_ptr;
    }
}

/*
 * DIM statement
 */
void
do_dim()
{
    char arrname[NAMLEN+3];
    int dims[11];
    int ndims, i;
    int type;
    char c;

    while (1) {
        skip_spaces();
        i = 0;
        type = TYPE_NUM;

        while (IS_ALNUM(peek_char()) && i < NAMLEN) {
            c = get_next_char();
            if (c >= 'a' && c <= 'z') {
                arrname[i++] = c - 'a' + 'A';
            } else {
                arrname[i++] = c;
            }
        }

        if (peek_char() == '$') {
            get_next_char();
            type = TYPE_STR;
        } else if (peek_char() == '%') {
            get_next_char();
            type = TYPE_INT;
        }
        arrname[i] = '\0';

        /* Expect ( */
        skip_spaces();
        if (peek_char() != '(') {
            syntax_error();
            return;
        }
        get_next_char();

        /* Get dimensions */
        ndims = 0;
        while (ndims < 11) {
            dims[ndims++] = eval_integer() + 1;  /* 0 to N = N+1 elements */
            skip_spaces();
            if (peek_char() == ',') {
                get_next_char();
            } else {
                break;
            }
        }

        skip_spaces();
        if (peek_char() != ')') {
            syntax_error();
            return;
        }
        get_next_char();

        dimension_array(arrname, dims, ndims, type);

        /* Check for more arrays */
        skip_spaces();
        if (peek_char() == ',') {
            get_next_char();
        } else {
            break;
        }
    }
}

/*
 * DATA statement - skip during execution
 */
void
do_data()
{
    skip_to_eol();
}

/*
 * Find next DATA item
 * Returns pointer to start of data item, or NULL if no more data
 */
static unsigned char *
find_next_data()
{
    unsigned char *p;
    unsigned char *text;
    line_t *line;

    /* If we have a current position, try to continue from there */
    if (g_state->dataptr.ptr) {
        p = g_state->dataptr.ptr;

        /* Skip whitespace */
        while (*p == ' ' || *p == '\t') p++;

        /* Check for comma (more data on this line) */
        if (*p == ',') {
            p++;
            while (*p == ' ' || *p == '\t') p++;
            return p;
        }

        /* End of this DATA line - find next DATA statement */
        if (*p == '\0') {
            /* Move to next line */
            line = (line_t *)((unsigned char *)g_state->dataptr.ptr -
                   ((unsigned char *)g_state->dataptr.ptr - g_state->txttab) %
                   sizeof(line_t));
            /* This is complex - let's restart search from current line */
            g_state->dataptr.linenum++;
            g_state->dataptr.ptr = NULL;
        }
    }

    /* Search for next DATA statement */
    p = g_state->txttab;
    while (p[0] != 0 || p[1] != 0) {
        line = (line_t *)p;

        if (line->linenum > g_state->dataptr.linenum ||
            (line->linenum == g_state->dataptr.linenum && g_state->dataptr.ptr == NULL)) {
            /* Check if this line has DATA */
            text = line->text;
            while (*text) {
                if ((*text & 0xFF) == TOK_DATA) {
                    text++;
                    while (*text == ' ' || *text == '\t') text++;
                    g_state->dataptr.linenum = line->linenum;
                    g_state->dataptr.ptr = text;
                    return text;
                }
                text++;
            }
        }

        p += line->len;
    }

    return NULL;  /* No more DATA */
}

/*
 * READ statement
 */
void
do_read()
{
    char varname[NAMLEN+3];
    int type;
    unsigned char *datapos;
    double numval;
    char strbuf[256];
    char numbuf[32];
    int i;

    while (1) {
        /* Get variable name */
        skip_spaces();
        if (!IS_ALPHA(peek_char())) {
            break;
        }

        i = 0;
        while (IS_ALNUM(peek_char()) && i < NAMLEN) {
            varname[i++] = get_next_char();
        }
        varname[i] = '\0';

        type = TYPE_NUM;
        if (peek_char() == '$') {
            get_next_char();
            varname[i++] = '$';
            varname[i] = '\0';
            type = TYPE_STR;
        }

        /* Find next DATA item */
        datapos = find_next_data();
        if (!datapos) {
            error(ERR_OUT_OF_DATA);
            return;
        }

        /* Parse the data value */
        if (type == TYPE_STR) {
            /* String data */
            i = 0;
            if (*datapos == '"') {
                datapos++;
                while (*datapos && *datapos != '"' && i < 255) {
                    strbuf[i++] = *datapos++;
                }
                if (*datapos == '"') datapos++;
            } else {
                while (*datapos && *datapos != ',' && *datapos != '\0' && i < 255) {
                    strbuf[i++] = *datapos++;
                }
            }
            strbuf[i] = '\0';
            set_str_variable(varname, string_from_cstr(strbuf));
        } else {
            /* Numeric data */
            i = 0;
            /* Skip leading whitespace */
            while (*datapos == ' ' || *datapos == '\t') datapos++;
            /* Parse number */
            while (*datapos && *datapos != ',' && *datapos != ' ' &&
                   *datapos != '\t' && *datapos != '\0' && i < 31) {
                numbuf[i++] = *datapos++;
            }
            numbuf[i] = '\0';
            numval = atof(numbuf);
            set_num_variable(varname, numval);
        }

        /* Update data pointer */
        g_state->dataptr.ptr = datapos;

        /* Check for more variables */
        skip_spaces();
        if (peek_char() == ',') {
            get_next_char();
        } else {
            break;
        }
    }
}

/*
 * RESTORE statement
 */
void
do_restore()
{
    g_state->dataptr.linenum = 0;
    g_state->dataptr.ptr = NULL;
}

/*
 * END statement
 */
void
do_end()
{
    g_state->running = 0;
    g_state->curlin = -1;
}

/*
 * STOP statement
 */
void
do_stop()
{
    g_state->running = 0;
    g_state->oldlin = g_state->curlin;
    g_state->oldtxt = g_state->txtptr;
    printf("BREAK IN %d\n", g_state->curlin);
}

/*
 * CONT statement
 */
void
do_cont()
{
    line_t *line;

    if (g_state->oldlin <= 0) {
        error(ERR_CANT_CONT);
        return;
    }

    line = find_line(g_state->oldlin);
    if (!line) {
        error(ERR_CANT_CONT);
        return;
    }

    g_state->curlin = g_state->oldlin;
    g_state->txtptr = g_state->oldtxt;
    g_state->curline_ptr = line;
    g_state->running = 1;
    run_program(0);  /* Continue from current position */
}

/*
 * NEW statement
 */
void
do_new()
{
    new_program();
}

/*
 * LIST statement
 */
void
do_list()
{
    int start, end;

    start = 0;
    end = MAXLIN;

    skip_spaces();
    if (IS_DIGIT(peek_char())) {
        start = eval_integer();
        end = start;

        skip_spaces();
        if (peek_char() == '-') {
            get_next_char();
            skip_spaces();
            if (IS_DIGIT(peek_char())) {
                end = eval_integer();
            } else {
                end = MAXLIN;
            }
        }
    }

    list_program(start, end);
}

/*
 * RUN statement
 */
void
do_run()
{
    int startline;

    startline = 0;

    skip_spaces();
    if (IS_DIGIT(peek_char())) {
        startline = eval_integer();
    }

    clear_variables();
    clear_arrays();
    g_state->forsp = 0;
    g_state->gosubsp = 0;

    run_program(startline);
}

/*
 * LOAD statement
 */
void
do_load()
{
    string_t *filename;
    char *fname;

    filename = eval_string();
    if (!filename) return;

    fname = string_to_cstr(filename);
    free_string(filename);

    if (!fname) return;

    if (load_file(fname) != 0) {
        printf("?FILE NOT FOUND\n");
    }

    free(fname);
}

/*
 * SAVE statement
 */
void
do_save()
{
    string_t *filename;
    char *fname;

    filename = eval_string();
    if (!filename) return;

    fname = string_to_cstr(filename);
    free_string(filename);

    if (!fname) return;

    if (save_file(fname) != 0) {
        printf("?FILE ERROR\n");
    }

    free(fname);
}

/*
 * POKE statement - no-op in portable version
 */
void
do_poke()
{
    eval_integer();  /* Address */
    skip_spaces();
    if (peek_char() == ',') get_next_char();
    eval_integer();  /* Value */
    /* No actual memory write for portability */
}

/*
 * WAIT statement - no-op in portable version
 */
void
do_wait()
{
    eval_integer();  /* Address */
    skip_spaces();
    if (peek_char() == ',') get_next_char();
    eval_integer();  /* Mask */
    /* No actual wait for portability */
}

/*
 * GET statement - simplified
 */
void
do_get()
{
    /* Simplified - just skip */
    skip_spaces();
    while (IS_ALNUM(peek_char()) || peek_char() == '$') {
        get_next_char();
    }
}

/*
 * ON statement (ON...GOTO/GOSUB)
 */
void
do_on()
{
    int index;
    int linenum;
    int i;
    int is_gosub;
    line_t *line;

    index = eval_integer();

    skip_spaces();
    is_gosub = 0;

    if (match_token(TOK_GOTO)) {
        is_gosub = 0;
    } else if (match_token(TOK_GOSUB)) {
        is_gosub = 1;
    } else {
        syntax_error();
        return;
    }

    /* Skip to the index-th line number */
    for (i = 1; i < index; i++) {
        skip_spaces();
        while (IS_DIGIT(peek_char())) {
            get_next_char();
        }
        skip_spaces();
        if (peek_char() == ',') {
            get_next_char();
        } else {
            return;  /* Not enough targets */
        }
    }

    skip_spaces();
    if (!IS_DIGIT(peek_char())) {
        return;  /* Index out of range */
    }

    linenum = eval_integer();

    if (is_gosub) {
        line = find_line(linenum);
        if (!line) {
            error(ERR_UNDEF_STMT);
            return;
        }

        if (g_state->gosubsp >= 26) {
            error(ERR_OUT_OF_MEM);
            return;
        }

        g_state->gosubstack[g_state->gosubsp].linenum = g_state->curlin;
        g_state->gosubstack[g_state->gosubsp].txtptr = g_state->txtptr;
        g_state->gosubstack[g_state->gosubsp].line_ptr = g_state->curline_ptr;
        g_state->gosubsp++;

        g_state->curlin = line->linenum;
        g_state->txtptr = line->text;
        g_state->curline_ptr = line;
    } else {
        line = find_line(linenum);
        if (!line) {
            error(ERR_UNDEF_STMT);
            return;
        }

        g_state->curlin = line->linenum;
        g_state->txtptr = line->text;
        g_state->curline_ptr = line;
    }
}

/*
 * DEF statement - simplified (not fully implemented)
 */
void
do_def()
{
    /* Skip DEF FN definition */
    skip_to_eol();
}

/*
 * CLEAR statement
 */
void
do_clear()
{
    clear_variables();
    clear_arrays();
    g_state->forsp = 0;
    g_state->gosubsp = 0;
}
