/*
 * repl.c - Read-Eval-Print Loop
 *
 * Microsoft BASIC 6502 C Port
 * K&R C v2 compatible
 */

#include "m6502basic.h"

/*
 * Check if line starts with line number
 */
static int
has_linenum(line)
const char *line;
{
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    return IS_DIGIT(*line);
}

/*
 * Extract line number
 */
static int
extract_linenum(line)
const char *line;
{
    int num;

    num = 0;
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    while (IS_DIGIT(*line)) {
        num = num * 10 + (*line - '0');
        line++;
    }
    return num;
}

/*
 * Skip past line number
 */
static const char *
skip_linenum(line)
const char *line;
{
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    while (IS_DIGIT(*line)) {
        line++;
    }
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    return line;
}

/*
 * Execute a direct command
 */
void
execute_direct(line)
char *line;
{
    unsigned char *tokens;
    unsigned char *saved_txtptr;
    int saved_curlin;
    int len;

    tokens = tokenize_line(line, &len);
    if (!tokens) {
        return;
    }

    saved_txtptr = g_state->txtptr;
    saved_curlin = g_state->curlin;

    g_state->txtptr = tokens;
    g_state->curlin = -1;
    g_state->running = 0;

    if (setjmp(g_state->errtrap) == 0) {
        execute_statement();
    } else {
        if (g_state->errnum != ERR_NONE) {
            printf("?%s\n", error_message(g_state->errnum));
            g_state->errnum = ERR_NONE;
        }
    }

    g_state->txtptr = saved_txtptr;
    g_state->curlin = saved_curlin;

    free(tokens);
}

/*
 * Main REPL loop
 */
void
repl()
{
    char *line;
    int linenum;
    const char *text;
    unsigned char *tokens;
    int len;

    while (1) {
        /* Print prompt */
        if (g_state->curlin == -1 || !g_state->running) {
            printf("READY.\n");
        }

        /* Read line */
        if (fgets(g_state->inputbuf, BUFLEN, stdin) == NULL) {
            break;
        }

        /* Remove trailing newline/CR */
        line = g_state->inputbuf;
        len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        if (len > 0 && line[len-1] == '\r') {
            line[len-1] = '\0';
            len--;
        }

        /* Skip leading whitespace */
        while (*line == ' ' || *line == '\t') {
            line++;
        }

        /* Skip empty lines */
        if (*line == '\0') {
            continue;
        }

        /* Check for line number */
        if (has_linenum(line)) {
            linenum = extract_linenum(line);
            text = skip_linenum(line);

            if (*text == '\0') {
                /* Delete line */
                delete_line(linenum);
            } else {
                /* Add/replace line */
                tokens = tokenize_line(text, &len);
                if (tokens) {
                    insert_line(linenum, tokens, len);
                    free(tokens);
                }
            }
        } else {
            /* Direct command */
            execute_direct(line);
        }
    }
}

/*
 * Load program from file
 */
int
load_file(filename)
const char *filename;
{
    FILE *fp;
    char line[BUFLEN+1];
    int linenum;
    const char *text;
    unsigned char *tokens;
    int len;

    fp = fopen(filename, "r");
    if (!fp) {
        return -1;
    }

    new_program();

    while (fgets(line, BUFLEN, fp) != NULL) {
        /* Remove trailing newline/CR */
        len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        if (len > 0 && line[len-1] == '\r') {
            line[len-1] = '\0';
            len--;
        }

        /* Skip empty lines */
        text = line;
        while (*text == ' ' || *text == '\t') {
            text++;
        }
        if (*text == '\0') {
            continue;
        }

        /* Parse line */
        if (has_linenum(line)) {
            linenum = extract_linenum(line);
            text = skip_linenum(line);

            if (*text != '\0') {
                tokens = tokenize_line(text, &len);
                if (tokens) {
                    insert_line(linenum, tokens, len);
                    free(tokens);
                }
            }
        }
    }

    fclose(fp);
    return 0;
}

/*
 * Save program to file
 */
int
save_file(filename)
const char *filename;
{
    FILE *fp;
    unsigned char *p;
    line_t *line;
    char *text;

    fp = fopen(filename, "w");
    if (!fp) {
        return -1;
    }

    p = g_state->txttab;
    while (p[0] != 0 || p[1] != 0) {
        line = (line_t *)p;

        text = detokenize_line(line->text);
        if (text) {
            fprintf(fp, "%d %s\n", line->linenum, text);
            free(text);
        }

        p += line->len;
    }

    fclose(fp);
    return 0;
}
