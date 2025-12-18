/*
 * m6502basic.h - Main header for Microsoft BASIC 6502 C port
 *
 * C Port Copyright (c) 2025 Andy Taylor
 * Original Microsoft BASIC Copyright (c) 1976-1978 Microsoft Corporation
 *
 * K&R C v2 compatible - for 2.11 BSD, MacOS, and Linux
 */

#ifndef M6502BASIC_H
#define M6502BASIC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <setjmp.h>

/* HUGE_VAL compatibility for 2.11 BSD and older systems */
#ifndef HUGE_VAL
#define HUGE_VAL 1.0e38
#endif

/* memmove compatibility for 2.11 BSD - use bcopy instead */
#if defined(__211BSD__) || defined(pdp11)
#define memmove(dest, src, n) bcopy((src), (dest), (n))
#endif

/* K&R C compatible character tests */
#define IS_ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_ALNUM(c) (IS_ALPHA(c) || IS_DIGIT(c))
#define TO_UPPER(c) (((c) >= 'a' && (c) <= 'z') ? ((c) - 'a' + 'A') : (c))

/* Platform detection */
#if defined(__211BSD__) || defined(pdp11)
#define PLATFORM_211BSD 1
#define IS_16BIT 1
#elif defined(__APPLE__) || defined(__MACH__)
#define PLATFORM_MACOS 1
#define IS_16BIT 0
#elif defined(__linux__)
#define PLATFORM_LINUX 1
#define IS_16BIT 0
#else
#define PLATFORM_UNKNOWN 1
#define IS_16BIT 0
#endif

/* Basic constants - matching original 6502 BASIC */
#define LINLEN 72       /* Terminal line length */
#define BUFLEN 72       /* Input buffer size */
#define NAMLEN 2        /* Variable name length (2 chars in original) */
#define CLMWID 14       /* Column width for PRINT commas */

/* Platform-specific limits */
#if IS_16BIT
#define MAXLIN 32767    /* Maximum line number */
#define PROGRAM_SIZE 65536L /* Max program memory - probed at runtime */
#else
#define MAXLIN 63999    /* Maximum line number */
#define PROGRAM_SIZE 65536L /* Program memory size */
#endif

/* Data type indicators */
#define TYPE_NUM    0   /* Numeric (default) */
#define TYPE_STR    1   /* String ($) */
#define TYPE_INT    2   /* Integer (%) - if INTPRC enabled */

/* Token definitions - Statement tokens (MSB set, >= 128) */
#define TOK_END     128
#define TOK_FOR     129
#define TOK_NEXT    130
#define TOK_DATA    131
#define TOK_INPUT   132
#define TOK_DIM     133
#define TOK_READ    134
#define TOK_LET     135
#define TOK_GOTO    136
#define TOK_RUN     137
#define TOK_IF      138
#define TOK_RESTORE 139
#define TOK_GOSUB   140
#define TOK_RETURN  141
#define TOK_REM     142
#define TOK_STOP    143
#define TOK_ON      144
#define TOK_WAIT    145
#define TOK_DEF     146
#define TOK_POKE    147
#define TOK_PRINT   148
#define TOK_CONT    149
#define TOK_LIST    150
#define TOK_CLEAR   151
#define TOK_GET     152
#define TOK_NEW     153
#define TOK_LOAD    154
#define TOK_SAVE    155

/* Keyword tokens */
#define TOK_TAB     156
#define TOK_TO      157
#define TOK_FN      158
#define TOK_SPC     159
#define TOK_THEN    160
#define TOK_NOT     161
#define TOK_STEP    162

/* Operator tokens */
#define TOK_PLUS    163
#define TOK_MINUS   164
#define TOK_MULT    165
#define TOK_DIV     166
#define TOK_POWER   167
#define TOK_AND     168
#define TOK_OR      169
#define TOK_GT      170
#define TOK_EQ      171
#define TOK_LT      172

/* Function tokens */
#define TOK_SGN     173
#define TOK_INT     174
#define TOK_ABS     175
#define TOK_USR     176
#define TOK_FRE     177
#define TOK_POS     178
#define TOK_SQR     179
#define TOK_RND     180
#define TOK_LOG     181
#define TOK_EXP     182
#define TOK_COS     183
#define TOK_SIN     184
#define TOK_TAN     185
#define TOK_ATN     186
#define TOK_PEEK    187
#define TOK_LEN     188
#define TOK_STR     189
#define TOK_VAL     190
#define TOK_ASC     191
#define TOK_CHR     192
#define TOK_LEFT    193
#define TOK_RIGHT   194
#define TOK_MID     195

/* Error codes - matching original 6502 BASIC */
#define ERR_NONE         0
#define ERR_NEXT_NO_FOR  1   /* NF - NEXT without FOR */
#define ERR_SYNTAX       2   /* SN - Syntax error */
#define ERR_RETURN       3   /* RG - RETURN without GOSUB */
#define ERR_OUT_OF_DATA  4   /* OD - Out of DATA */
#define ERR_ILLEGAL_FUNC 5   /* FC - Illegal function call */
#define ERR_OVERFLOW     6   /* OV - Overflow */
#define ERR_OUT_OF_MEM   7   /* OM - Out of memory */
#define ERR_UNDEF_STMT   8   /* US - Undefined statement */
#define ERR_SUBSCRIPT    9   /* BS - Bad subscript */
#define ERR_REDIM       10   /* DD - Redimensioned array */
#define ERR_DIV_ZERO    11   /* /0 - Division by zero */
#define ERR_ILLEGAL_DIR 12   /* ID - Illegal direct */
#define ERR_TYPE_MISM   13   /* TM - Type mismatch */
#define ERR_OUT_OF_STR  14   /* LS - Out of string space */
#define ERR_STRING_LONG 15   /* ST - String too long */
#define ERR_CANT_CONT   16   /* CN - Can't continue */
#define ERR_UNDEF_FUNC  17   /* UF - Undefined function */

/* Forward declarations */
typedef struct line_s line_t;
typedef struct var_s var_t;
typedef struct array_s array_t;
typedef struct string_s string_t;

/* String descriptor - 3 bytes like original */
struct string_s {
    int len;            /* String length */
    char *ptr;          /* Pointer to string data */
};

/* Variable entry - 6 bytes like original (2 name + 4 value) */
struct var_s {
    char name[NAMLEN+1]; /* Variable name (2 chars + null) */
    int type;            /* Data type (TYPE_NUM or TYPE_STR) */
    union {
        double numval;   /* Numeric value */
        string_t *strval; /* String pointer */
    } value;
    var_t *next;         /* Next in list */
};

/* Array descriptor */
struct array_s {
    char name[NAMLEN+1]; /* Array name */
    int type;            /* Element type */
    int ndims;           /* Number of dimensions */
    int dims[11];        /* Dimension sizes (max 11 in original) */
    int size;            /* Total elements */
    union {
        double *numdata;     /* Numeric array data */
        string_t **strdata;  /* String array data */
    } data;
    array_t *next;       /* Next array */
};

/* Program line structure */
struct line_s {
    int linenum;        /* Line number */
    int len;            /* Line length including header */
    unsigned char text[1]; /* Tokenized text (flexible array) */
};

/* FOR loop stack entry */
typedef struct {
    int linenum;        /* Line number of FOR */
    unsigned char *txtptr; /* Position after FOR */
    char varname[NAMLEN+1]; /* Loop variable name */
    double limit;       /* TO value */
    double step;        /* STEP value */
    line_t *line_ptr;   /* Pointer to line for fast return */
} forstack_t;

/* GOSUB stack entry */
typedef struct {
    int linenum;        /* Return line number */
    unsigned char *txtptr; /* Return position */
    line_t *line_ptr;   /* Pointer to line */
} gosubstack_t;

/* DATA pointer state */
typedef struct {
    int linenum;        /* Current DATA line */
    unsigned char *ptr; /* Current position in DATA */
} dataptr_t;

/* Global interpreter state */
typedef struct {
    /* Memory pointers - like 6502 page zero */
    unsigned char *txttab;  /* Start of program text */
    unsigned char *vartab;  /* Start of variables (end of program) */
    unsigned char *arytab;  /* Start of arrays */
    unsigned char *strend;  /* End of arrays */
    unsigned char *fretop;  /* Top of string free space */
    unsigned char *memsiz;  /* End of memory */

    /* Execution state */
    int curlin;             /* Current line number (-1 = direct mode) */
    unsigned char *txtptr;  /* Current text pointer */
    line_t *curline_ptr;    /* Pointer to current line */

    /* Variable storage */
    var_t *varlist;         /* Variable list head */
    array_t *arrlist;       /* Array list head */

    /* Control stacks */
    forstack_t forstack[26];   /* FOR loop stack (A-Z variables) */
    int forsp;                  /* FOR stack pointer */

    gosubstack_t gosubstack[26]; /* GOSUB stack */
    int gosubsp;                  /* GOSUB stack pointer */

    /* DATA state */
    dataptr_t dataptr;      /* Current DATA position */

    /* Error handling */
    int errnum;             /* Error number */
    int errlin;             /* Error line */
    jmp_buf errtrap;        /* Error recovery */

    /* Execution flags */
    int running;            /* 1 if program running */
    int tression;           /* 1 if TRON active (not in 6502 BASIC) */

    /* I/O state */
    int trmpos;             /* Terminal position (column) */
    char inputbuf[BUFLEN+1]; /* Input buffer */

    /* Random number state */
    unsigned long rndseed;

    /* Continue state */
    int oldlin;             /* Line number for CONT */
    unsigned char *oldtxt;  /* Text pointer for CONT */

} state_t;

/* Global state pointer */
extern state_t *g_state;

/* Function prototypes */

/* main.c */
int main(int argc, char **argv);
void init_state();
void cleanup();

/* repl.c */
void repl();
void execute_direct(char *line);
int load_file(const char *filename);
int save_file(const char *filename);

/* tokenize.c */
unsigned char *tokenize_line(const char *line, int *len);
char *detokenize_line(unsigned char *tokens);
int is_keyword(const char *word);

/* parse.c */
void insert_line(int linenum, unsigned char *tokens, int len);
void delete_line(int linenum);
line_t *find_line(int linenum);
void list_program(int start, int end);
void new_program();

/* execute.c */
void run_program(int startline);
void execute_statement();
void skip_to_eol();

/* eval.c */
double eval_expr();
double eval_numeric();
string_t *eval_string();
int eval_integer();
int get_next_char();
int peek_char();
void skip_spaces();
int match_token(int token);
string_t *parse_string_literal();
int get_valtype();

/* variables.c */
var_t *find_variable(const char *name, int create);
void set_num_variable(const char *name, double val);
void set_str_variable(const char *name, string_t *val);
double get_num_variable(const char *name);
string_t *get_str_variable(const char *name);
void clear_variables();

/* arrays.c */
array_t *find_array(const char *name, int create);
void dimension_array(const char *name, int *dims, int ndims, int type);
double *array_num_element(const char *name, int *indices, int nindices);
string_t **array_str_element(const char *name, int *indices, int nindices);
void clear_arrays();

/* strings.c */
string_t *alloc_string(int len);
void free_string(string_t *str);
string_t *copy_string(string_t *src);
string_t *concat_strings(string_t *s1, string_t *s2);
int compare_strings(string_t *s1, string_t *s2);
string_t *string_from_cstr(const char *cstr);
char *string_to_cstr(string_t *str);

/* statements.c */
void do_print();
void do_input();
void do_let();
void do_if();
void do_goto();
void do_gosub();
void do_return();
void do_for();
void do_next();
void do_dim();
void do_data();
void do_read();
void do_restore();
void do_end();
void do_stop();
void do_cont();
void do_new();
void do_list();
void do_run();
void do_load();
void do_save();
void do_poke();
void do_wait();
void do_get();
void do_on();
void do_def();
void do_clear();

/* functions.c */
double fn_sgn(double x);
double fn_int(double x);
double fn_abs(double x);
double fn_sqr(double x);
double fn_rnd(double x);
double fn_sin(double x);
double fn_cos(double x);
double fn_tan(double x);
double fn_atn(double x);
double fn_log(double x);
double fn_exp(double x);
double fn_peek(double addr);
double fn_fre(double x);
double fn_pos(double x);

int fn_len(string_t *s);
int fn_asc(string_t *s);
string_t *fn_chr(int n);
string_t *fn_str(double x);
double fn_val(string_t *s);
string_t *fn_left(string_t *s, int n);
string_t *fn_right(string_t *s, int n);
string_t *fn_mid(string_t *s, int start, int len);

/* error.c */
void error(int errnum);
void syntax_error();
const char *error_message(int errnum);

#endif /* M6502BASIC_H */
