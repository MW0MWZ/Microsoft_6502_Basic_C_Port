/*
 * tokenize.c - Tokenizer (line crunching)
 *
 * Microsoft BASIC 6502 C Port
 * K&R C v2 compatible
 */

#include "m6502basic.h"

/* Keyword table */
typedef struct {
    const char *keyword;
    int token;
} keyword_t;

static keyword_t keywords[] = {
    /* Statements */
    {"END", TOK_END},
    {"FOR", TOK_FOR},
    {"NEXT", TOK_NEXT},
    {"DATA", TOK_DATA},
    {"INPUT", TOK_INPUT},
    {"DIM", TOK_DIM},
    {"READ", TOK_READ},
    {"LET", TOK_LET},
    {"GOTO", TOK_GOTO},
    {"RUN", TOK_RUN},
    {"IF", TOK_IF},
    {"RESTORE", TOK_RESTORE},
    {"GOSUB", TOK_GOSUB},
    {"RETURN", TOK_RETURN},
    {"REM", TOK_REM},
    {"STOP", TOK_STOP},
    {"ON", TOK_ON},
    {"WAIT", TOK_WAIT},
    {"DEF", TOK_DEF},
    {"POKE", TOK_POKE},
    {"PRINT", TOK_PRINT},
    {"CONT", TOK_CONT},
    {"LIST", TOK_LIST},
    {"CLEAR", TOK_CLEAR},
    {"GET", TOK_GET},
    {"NEW", TOK_NEW},
    {"LOAD", TOK_LOAD},
    {"SAVE", TOK_SAVE},
    /* Also support ? for PRINT */
    {"?", TOK_PRINT},
    /* Keywords */
    {"TAB", TOK_TAB},
    {"TO", TOK_TO},
    {"FN", TOK_FN},
    {"SPC", TOK_SPC},
    {"THEN", TOK_THEN},
    {"NOT", TOK_NOT},
    {"STEP", TOK_STEP},
    /* Operators */
    {"AND", TOK_AND},
    {"OR", TOK_OR},
    {"+", TOK_PLUS},
    {"-", TOK_MINUS},
    {"*", TOK_MULT},
    {"/", TOK_DIV},
    {"^", TOK_POWER},
    {">", TOK_GT},
    {"=", TOK_EQ},
    {"<", TOK_LT},
    /* Functions */
    {"SGN", TOK_SGN},
    {"INT", TOK_INT},
    {"ABS", TOK_ABS},
    {"USR", TOK_USR},
    {"FRE", TOK_FRE},
    {"POS", TOK_POS},
    {"SQR", TOK_SQR},
    {"RND", TOK_RND},
    {"LOG", TOK_LOG},
    {"EXP", TOK_EXP},
    {"COS", TOK_COS},
    {"SIN", TOK_SIN},
    {"TAN", TOK_TAN},
    {"ATN", TOK_ATN},
    {"PEEK", TOK_PEEK},
    {"LEN", TOK_LEN},
    {"STR$", TOK_STR},
    {"VAL", TOK_VAL},
    {"ASC", TOK_ASC},
    {"CHR$", TOK_CHR},
    {"LEFT$", TOK_LEFT},
    {"RIGHT$", TOK_RIGHT},
    {"MID$", TOK_MID},
    {NULL, 0}
};

/*
 * Look up keyword in table
 */
int
is_keyword(word)
const char *word;
{
    int i;
    char upword[16];
    char *p;
    const char *q;

    /* Convert to uppercase */
    p = upword;
    q = word;
    while (*q && p < upword + 15) {
        if (*q >= 'a' && *q <= 'z') {
            *p++ = *q - 'a' + 'A';
        } else {
            *p++ = *q;
        }
        q++;
    }
    *p = '\0';

    /* Search table */
    for (i = 0; keywords[i].keyword != NULL; i++) {
        if (strcmp(upword, keywords[i].keyword) == 0) {
            return keywords[i].token;
        }
    }

    return 0;
}

/*
 * Tokenize a BASIC line
 */
unsigned char *
tokenize_line(line, len)
const char *line;
int *len;
{
    unsigned char *tokens;
    unsigned char *newtokens;
    unsigned char *p;
    const char *s;
    char word[16];
    int i, token;
    int in_string, in_data, in_rem;
    int allocated;
    int offset;

    allocated = BUFLEN * 2;
    tokens = (unsigned char *)malloc(allocated);
    if (!tokens) {
        return NULL;
    }

    p = tokens;
    s = line;
    in_string = 0;
    in_data = 0;
    in_rem = 0;

    while (*s) {
        /* Check buffer space */
        if (p - tokens >= allocated - 10) {
            offset = p - tokens;
            allocated *= 2;
            newtokens = (unsigned char *)realloc(tokens, allocated);
            if (!newtokens) {
                free(tokens);
                return NULL;
            }
            tokens = newtokens;
            p = tokens + offset;
        }

        /* Skip spaces outside strings/data/rem */
        if (!in_string && !in_data && !in_rem && (*s == ' ' || *s == '\t')) {
            s++;
            continue;
        }

        /* String literal */
        if (*s == '"') {
            *p++ = *s++;
            in_string = !in_string;
            continue;
        }

        /* Copy verbatim in string/data/rem */
        if (in_string || in_data || in_rem) {
            *p++ = *s++;
            continue;
        }

        /* Check for ' comment */
        if (*s == '\'') {
            *p++ = TOK_REM;
            s++;
            in_rem = 1;
            continue;
        }

        /* Check for keywords (alphabetic start) */
        if (IS_ALPHA(*s)) {
            i = 0;
            while ((IS_ALNUM(*s) || *s == '$') && i < 15) {
                word[i++] = *s++;
            }
            word[i] = '\0';

            token = is_keyword(word);
            if (token) {
                *p++ = token;

                if (token == TOK_DATA) {
                    in_data = 1;
                } else if (token == TOK_REM) {
                    in_rem = 1;
                }
            } else {
                /* Not a keyword - copy as identifier */
                for (i = 0; word[i]; i++) {
                    *p++ = word[i];
                }
            }
            continue;
        }

        /* Numbers */
        if (IS_DIGIT(*s) || (*s == '.' && IS_DIGIT(s[1]))) {
            while (IS_DIGIT(*s) || *s == '.' || *s == 'E' || *s == 'e' ||
                   ((*s == '+' || *s == '-') && (s[-1] == 'E' || s[-1] == 'e'))) {
                *p++ = *s++;
            }
            continue;
        }

        /* Single character operators */
        switch (*s) {
            case '>':
            case '<':
            case '=':
            case '+':
            case '-':
            case '*':
            case '/':
            case '^':
                token = is_keyword(word);
                /* Just store the character for now */
                *p++ = *s++;
                break;
            default:
                /* Other characters */
                *p++ = *s++;
                break;
        }
    }

    *p++ = '\0';
    *len = p - tokens;
    return tokens;
}

/*
 * Detokenize a line back to text
 */
char *
detokenize_line(tokens)
unsigned char *tokens;
{
    char *text;
    char *newtext;
    char *p;
    unsigned char *t;
    int i, token;
    int allocated;
    int offset;

    allocated = BUFLEN * 2;
    text = (char *)malloc(allocated);
    if (!text) {
        return NULL;
    }

    p = text;
    t = tokens;

    while (*t) {
        /* Check buffer space */
        if (p - text >= allocated - 20) {
            offset = p - text;
            allocated *= 2;
            newtext = (char *)realloc(text, allocated);
            if (!newtext) {
                free(text);
                return NULL;
            }
            text = newtext;
            p = text + offset;
        }

        token = *t & 0xFF;

        /* Check for token (high bit set) */
        if (token >= 128) {
            t++;

            /* Find keyword */
            for (i = 0; keywords[i].keyword != NULL; i++) {
                if (keywords[i].token == token) {
                    strcpy(p, keywords[i].keyword);
                    p += strlen(keywords[i].keyword);
                    *p++ = ' ';
                    break;
                }
            }
        } else {
            /* Regular character */
            *p++ = *t++;
        }
    }

    *p = '\0';
    return text;
}
