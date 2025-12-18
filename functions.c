/*
 * functions.c - Built-in functions
 *
 * Microsoft BASIC 6502 C Port
 * K&R C v2 compatible
 */

#include "m6502basic.h"

/* Random number generator state */
static unsigned long rnd_state = 12345L;

/*
 * SGN function
 */
double
fn_sgn(x)
double x;
{
    if (x > 0.0) return 1.0;
    if (x < 0.0) return -1.0;
    return 0.0;
}

/*
 * INT function
 */
double
fn_int(x)
double x;
{
    return floor(x);
}

/*
 * ABS function
 */
double
fn_abs(x)
double x;
{
    return fabs(x);
}

/*
 * SQR function
 */
double
fn_sqr(x)
double x;
{
    if (x < 0.0) {
        error(ERR_ILLEGAL_FUNC);
        return 0.0;
    }
    return sqrt(x);
}

/*
 * RND function
 */
double
fn_rnd(x)
double x;
{
    if (x < 0.0) {
        /* Seed with x */
        rnd_state = (unsigned long)(-x * 65536.0);
    } else if (x == 0.0) {
        /* Return same number */
    } else {
        /* Generate next random number - Linear Congruential Generator */
        rnd_state = rnd_state * 1103515245L + 12345L;
    }

    return (double)(rnd_state & 0x7FFFFFFFL) / 2147483648.0;
}

/*
 * SIN function
 */
double
fn_sin(x)
double x;
{
    return sin(x);
}

/*
 * COS function
 */
double
fn_cos(x)
double x;
{
    return cos(x);
}

/*
 * TAN function
 */
double
fn_tan(x)
double x;
{
    return tan(x);
}

/*
 * ATN function (arctan)
 */
double
fn_atn(x)
double x;
{
    return atan(x);
}

/*
 * LOG function (natural log)
 */
double
fn_log(x)
double x;
{
    if (x <= 0.0) {
        error(ERR_ILLEGAL_FUNC);
        return 0.0;
    }
    return log(x);
}

/*
 * EXP function
 */
double
fn_exp(x)
double x;
{
    double result;

    result = exp(x);
    if (result == HUGE_VAL) {
        error(ERR_OVERFLOW);
        return 0.0;
    }
    return result;
}

/*
 * PEEK function - return 0 (no memory access in portable version)
 */
double
fn_peek(addr)
double addr;
{
    /* In a real implementation, this would read memory */
    /* For portability, just return 0 */
    addr = addr;  /* suppress unused warning */
    return 0.0;
}

/*
 * FRE function - return free memory
 */
double
fn_fre(x)
double x;
{
    x = x;  /* suppress unused warning */
    return (double)(g_state->fretop - g_state->strend);
}

/*
 * POS function - return cursor position
 */
double
fn_pos(x)
double x;
{
    x = x;  /* suppress unused warning */
    return (double)g_state->trmpos;
}

/*
 * LEN function
 */
int
fn_len(s)
string_t *s;
{
    if (!s) return 0;
    return s->len;
}

/*
 * ASC function
 */
int
fn_asc(s)
string_t *s;
{
    if (!s || s->len == 0 || !s->ptr) {
        error(ERR_ILLEGAL_FUNC);
        return 0;
    }
    return (unsigned char)s->ptr[0];
}

/*
 * CHR$ function
 */
string_t *
fn_chr(n)
int n;
{
    string_t *s;

    if (n < 0 || n > 255) {
        error(ERR_ILLEGAL_FUNC);
        return alloc_string(0);
    }

    s = alloc_string(1);
    if (s && s->ptr) {
        s->ptr[0] = (char)n;
        s->ptr[1] = '\0';
    }
    return s;
}

/*
 * STR$ function
 */
string_t *
fn_str(x)
double x;
{
    char buf[32];

    sprintf(buf, "%g", x);
    return string_from_cstr(buf);
}

/*
 * VAL function
 */
double
fn_val(s)
string_t *s;
{
    if (!s || !s->ptr || s->len == 0) {
        return 0.0;
    }
    return atof(s->ptr);
}

/*
 * LEFT$ function
 */
string_t *
fn_left(s, n)
string_t *s;
int n;
{
    string_t *result;

    if (!s || n <= 0) {
        return alloc_string(0);
    }

    if (n > s->len) {
        n = s->len;
    }

    result = alloc_string(n);
    if (result && result->ptr && s->ptr) {
        memcpy(result->ptr, s->ptr, n);
        result->ptr[n] = '\0';
    }
    return result;
}

/*
 * RIGHT$ function
 */
string_t *
fn_right(s, n)
string_t *s;
int n;
{
    string_t *result;
    int start;

    if (!s || n <= 0) {
        return alloc_string(0);
    }

    if (n > s->len) {
        n = s->len;
    }

    start = s->len - n;
    result = alloc_string(n);
    if (result && result->ptr && s->ptr) {
        memcpy(result->ptr, s->ptr + start, n);
        result->ptr[n] = '\0';
    }
    return result;
}

/*
 * MID$ function
 */
string_t *
fn_mid(s, start, len)
string_t *s;
int start;
int len;
{
    string_t *result;

    if (!s || start < 1 || len < 0) {
        if (start < 1 && s) {
            error(ERR_ILLEGAL_FUNC);
        }
        return alloc_string(0);
    }

    start--;  /* Convert to 0-based */

    if (start >= s->len) {
        return alloc_string(0);
    }

    if (start + len > s->len) {
        len = s->len - start;
    }

    result = alloc_string(len);
    if (result && result->ptr && s->ptr) {
        memcpy(result->ptr, s->ptr + start, len);
        result->ptr[len] = '\0';
    }
    return result;
}
