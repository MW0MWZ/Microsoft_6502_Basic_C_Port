/*
 * eval.c - Expression evaluator
 *
 * Microsoft BASIC 6502 C Port
 * K&R C v2 compatible
 */

#include "m6502basic.h"

/* Current value type for expression evaluation */
static int valtype;

/* Forward declarations */
static double expr_or();
static double expr_and();
static double expr_not();
static double expr_compare();
static double expr_add();
static double expr_mult();
static double expr_power();
static double expr_unary();
static double expr_primary();

/*
 * Get next character from text pointer
 */
int
get_next_char()
{
    if (g_state->txtptr && *g_state->txtptr) {
        return *g_state->txtptr++ & 0xFF;
    }
    return '\0';
}

/*
 * Peek at next character
 */
int
peek_char()
{
    if (g_state->txtptr && *g_state->txtptr) {
        return *g_state->txtptr & 0xFF;
    }
    return '\0';
}

/*
 * Skip whitespace
 */
void
skip_spaces()
{
    while (peek_char() == ' ' || peek_char() == '\t') {
        get_next_char();
    }
}

/*
 * Match a token
 */
int
match_token(token)
int token;
{
    int c;

    skip_spaces();
    c = peek_char() & 0xFF;

    if (c == (token & 0xFF)) {
        get_next_char();
        return 1;
    }

    return 0;
}

/*
 * Get current value type
 */
int
get_valtype()
{
    return valtype;
}

/*
 * Parse a number from text
 */
static double
parse_number()
{
    char buf[32];
    int i;

    i = 0;
    while (IS_DIGIT(peek_char()) || peek_char() == '.' ||
           peek_char() == 'E' || peek_char() == 'e' ||
           peek_char() == '+' || peek_char() == '-') {
        if (i < 31) {
            buf[i++] = get_next_char();
        } else {
            get_next_char();
        }
        /* Handle E+/E- specially */
        if (i >= 2 && (buf[i-2] == 'E' || buf[i-2] == 'e')) {
            /* This is part of exponent, continue */
        } else if (buf[i-1] == '+' || buf[i-1] == '-') {
            /* Not after E, put it back */
            i--;
            g_state->txtptr--;
            break;
        }
    }
    buf[i] = '\0';

    return atof(buf);
}

/*
 * Parse a string literal
 */
string_t *
parse_string_literal()
{
    char buf[256];
    int i;

    if (peek_char() != '"') {
        return alloc_string(0);
    }
    get_next_char();  /* Skip opening quote */

    i = 0;
    while (peek_char() != '"' && peek_char() != '\0') {
        if (i < 255) {
            buf[i++] = get_next_char();
        } else {
            get_next_char();
        }
    }
    buf[i] = '\0';

    if (peek_char() == '"') {
        get_next_char();  /* Skip closing quote */
    }

    return string_from_cstr(buf);
}

/*
 * Parse variable name
 */
static void
parse_varname(name, type)
char *name;
int *type;
{
    int i;
    char c;

    i = 0;
    *type = TYPE_NUM;

    /* Get name characters (max 2 for 6502 BASIC) */
    while (IS_ALNUM(peek_char()) && i < NAMLEN) {
        c = get_next_char();
        if (c >= 'a' && c <= 'z') {
            name[i++] = c - 'a' + 'A';
        } else {
            name[i++] = c;
        }
    }

    /* Check for type suffix */
    if (peek_char() == '$') {
        get_next_char();
        *type = TYPE_STR;
        name[i++] = '$';
    } else if (peek_char() == '%') {
        get_next_char();
        *type = TYPE_INT;
    }

    name[i] = '\0';
}

/*
 * Primary expression (number, variable, function, parentheses)
 */
static double
expr_primary()
{
    double result;
    int c, token;
    char varname[NAMLEN+3];
    int type;
    var_t *var;
    int indices[11];
    int nindices;
    double *numelem;
    string_t *s;

    skip_spaces();
    c = peek_char();

    /* Number */
    if (IS_DIGIT(c) || (c == '.' && IS_DIGIT(g_state->txtptr[1]))) {
        valtype = TYPE_NUM;
        return parse_number();
    }

    /* String literal */
    if (c == '"') {
        /* This shouldn't be called for strings, but handle it */
        valtype = TYPE_STR;
        return 0.0;
    }

    /* Parenthesized expression */
    if (c == '(') {
        get_next_char();
        result = expr_or();
        skip_spaces();
        if (peek_char() == ')') {
            get_next_char();
        }
        return result;
    }

    /* Token-based functions */
    token = c & 0xFF;
    if (token >= 128) {
        get_next_char();

        switch (token) {
            case TOK_SGN:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_sgn(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_INT:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_int(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_ABS:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_abs(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_SQR:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_sqr(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_RND:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_rnd(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_SIN:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_sin(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_COS:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_cos(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_TAN:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_tan(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_ATN:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_atn(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_LOG:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_log(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_EXP:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_exp(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_PEEK:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_peek(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_FRE:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_fre(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_POS:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                result = fn_pos(expr_or());
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return result;

            case TOK_LEN:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                s = eval_string();
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                result = (double)fn_len(s);
                if (s) free_string(s);
                return result;

            case TOK_ASC:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                s = eval_string();
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                result = (double)fn_asc(s);
                if (s) free_string(s);
                return result;

            case TOK_VAL:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                s = eval_string();
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                result = fn_val(s);
                if (s) free_string(s);
                return result;

            default:
                /* Unknown token */
                valtype = TYPE_NUM;
                return 0.0;
        }
    }

    /* Variable or array */
    if (IS_ALPHA(c)) {
        parse_varname(varname, &type);

        if (type == TYPE_STR) {
            valtype = TYPE_STR;
            return 0.0;  /* String handling done separately */
        }

        valtype = TYPE_NUM;

        /* Check for array subscript */
        skip_spaces();
        if (peek_char() == '(') {
            get_next_char();
            nindices = 0;

            while (nindices < 11) {
                indices[nindices++] = (int)expr_or();
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

            numelem = array_num_element(varname, indices, nindices);
            return numelem ? *numelem : 0.0;
        }

        /* Simple variable */
        var = find_variable(varname, 0);
        if (var) {
            return var->value.numval;
        }
        return 0.0;
    }

    valtype = TYPE_NUM;
    return 0.0;
}

/*
 * Unary operators (-, NOT)
 */
static double
expr_unary()
{
    int c;

    skip_spaces();
    c = peek_char();

    if (c == '-' || (c & 0xFF) == TOK_MINUS) {
        get_next_char();
        return -expr_unary();
    }

    if ((c & 0xFF) == TOK_NOT) {
        get_next_char();
        return (expr_unary() == 0.0) ? -1.0 : 0.0;
    }

    if (c == '+' || (c & 0xFF) == TOK_PLUS) {
        get_next_char();
        return expr_unary();
    }

    return expr_primary();
}

/*
 * Power operator (^)
 */
static double
expr_power()
{
    double left, right;

    left = expr_unary();

    skip_spaces();
    while (peek_char() == '^' || (peek_char() & 0xFF) == TOK_POWER) {
        get_next_char();
        right = expr_unary();
        left = pow(left, right);
        skip_spaces();
    }

    return left;
}

/*
 * Multiplication and division
 */
static double
expr_mult()
{
    double left, right;
    int op;

    left = expr_power();

    while (1) {
        skip_spaces();
        op = peek_char();

        if (op == '*' || (op & 0xFF) == TOK_MULT) {
            get_next_char();
            right = expr_power();
            left = left * right;
        } else if (op == '/' || (op & 0xFF) == TOK_DIV) {
            get_next_char();
            right = expr_power();
            if (right == 0.0) {
                error(ERR_DIV_ZERO);
                return 0.0;
            }
            left = left / right;
        } else {
            break;
        }
    }

    return left;
}

/*
 * Addition and subtraction
 */
static double
expr_add()
{
    double left, right;
    int op;

    left = expr_mult();

    while (1) {
        skip_spaces();
        op = peek_char();

        if (op == '+' || (op & 0xFF) == TOK_PLUS) {
            get_next_char();
            right = expr_mult();
            left = left + right;
        } else if (op == '-' || (op & 0xFF) == TOK_MINUS) {
            get_next_char();
            right = expr_mult();
            left = left - right;
        } else {
            break;
        }
    }

    return left;
}

/*
 * Comparison operators
 */
static double
expr_compare()
{
    double left, right;
    int op1, op2;
    int relation;

    left = expr_add();

    skip_spaces();
    op1 = peek_char();

    if (op1 == '<' || op1 == '>' || op1 == '=' ||
        (op1 & 0xFF) == TOK_LT || (op1 & 0xFF) == TOK_GT || (op1 & 0xFF) == TOK_EQ) {

        get_next_char();
        skip_spaces();
        op2 = peek_char();

        /* Check for compound operators */
        relation = 0;
        if (op1 == '<' || (op1 & 0xFF) == TOK_LT) {
            relation = 1;  /* Less than */
            if (op2 == '>' || (op2 & 0xFF) == TOK_GT) {
                get_next_char();
                relation = 5;  /* Not equal */
            } else if (op2 == '=' || (op2 & 0xFF) == TOK_EQ) {
                get_next_char();
                relation = 3;  /* Less or equal */
            }
        } else if (op1 == '>' || (op1 & 0xFF) == TOK_GT) {
            relation = 2;  /* Greater than */
            if (op2 == '=' || (op2 & 0xFF) == TOK_EQ) {
                get_next_char();
                relation = 4;  /* Greater or equal */
            } else if (op2 == '<' || (op2 & 0xFF) == TOK_LT) {
                get_next_char();
                relation = 5;  /* Not equal */
            }
        } else if (op1 == '=' || (op1 & 0xFF) == TOK_EQ) {
            relation = 6;  /* Equal */
            if (op2 == '<' || (op2 & 0xFF) == TOK_LT) {
                get_next_char();
                relation = 3;  /* Less or equal */
            } else if (op2 == '>' || (op2 & 0xFF) == TOK_GT) {
                get_next_char();
                relation = 4;  /* Greater or equal */
            }
        }

        right = expr_add();

        switch (relation) {
            case 1: return (left < right) ? -1.0 : 0.0;
            case 2: return (left > right) ? -1.0 : 0.0;
            case 3: return (left <= right) ? -1.0 : 0.0;
            case 4: return (left >= right) ? -1.0 : 0.0;
            case 5: return (left != right) ? -1.0 : 0.0;
            case 6: return (left == right) ? -1.0 : 0.0;
        }
    }

    return left;
}

/*
 * NOT operator (handled at comparison level)
 */
static double
expr_not()
{
    return expr_compare();
}

/*
 * AND operator
 */
static double
expr_and()
{
    double left, right;

    left = expr_not();

    while (1) {
        skip_spaces();
        if ((peek_char() & 0xFF) == TOK_AND) {
            get_next_char();
            right = expr_not();
            left = (double)((long)left & (long)right);
        } else {
            break;
        }
    }

    return left;
}

/*
 * OR operator (top level)
 */
static double
expr_or()
{
    double left, right;

    left = expr_and();

    while (1) {
        skip_spaces();
        if ((peek_char() & 0xFF) == TOK_OR) {
            get_next_char();
            right = expr_and();
            left = (double)((long)left | (long)right);
        } else {
            break;
        }
    }

    return left;
}

/*
 * Evaluate numeric expression
 */
double
eval_numeric()
{
    valtype = TYPE_NUM;
    return expr_or();
}

/*
 * Evaluate expression (main entry)
 */
double
eval_expr()
{
    return eval_numeric();
}

/*
 * Evaluate integer expression
 */
int
eval_integer()
{
    return (int)eval_numeric();
}

/*
 * Evaluate string expression
 */
string_t *
eval_string()
{
    char varname[NAMLEN+3];
    int type;
    var_t *var;
    int indices[11];
    int nindices;
    string_t **strelem;
    int token;
    string_t *s;
    string_t *str_result;
    int n, start, len;
    double x;

    skip_spaces();

    /* String literal */
    if (peek_char() == '"') {
        return parse_string_literal();
    }

    /* String function tokens */
    if ((peek_char() & 0xFF) >= 128) {
        token = get_next_char() & 0xFF;

        switch (token) {
            case TOK_CHR:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                n = eval_integer();
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return fn_chr(n);

            case TOK_STR:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                x = eval_numeric();
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                return fn_str(x);

            case TOK_LEFT:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                s = eval_string();
                skip_spaces();
                if (peek_char() == ',') get_next_char();
                n = eval_integer();
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                str_result = fn_left(s, n);
                if (s) free_string(s);
                return str_result;

            case TOK_RIGHT:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                s = eval_string();
                skip_spaces();
                if (peek_char() == ',') get_next_char();
                n = eval_integer();
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                str_result = fn_right(s, n);
                if (s) free_string(s);
                return str_result;

            case TOK_MID:
                skip_spaces();
                if (peek_char() == '(') get_next_char();
                s = eval_string();
                skip_spaces();
                if (peek_char() == ',') get_next_char();
                start = eval_integer();
                skip_spaces();
                len = 255;  /* Default to rest of string */
                if (peek_char() == ',') {
                    get_next_char();
                    len = eval_integer();
                }
                skip_spaces();
                if (peek_char() == ')') get_next_char();
                str_result = fn_mid(s, start, len);
                if (s) free_string(s);
                return str_result;

            default:
                /* Put back the token and continue */
                g_state->txtptr--;
                break;
        }
    }

    /* String variable or function */
    if (IS_ALPHA(peek_char())) {
        parse_varname(varname, &type);

        if (type != TYPE_STR) {
            error(ERR_TYPE_MISM);
            return NULL;
        }

        /* Check for array */
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

            strelem = array_str_element(varname, indices, nindices);
            if (strelem && *strelem) {
                return copy_string(*strelem);
            }
            return alloc_string(0);
        }

        /* Simple string variable */
        var = find_variable(varname, 0);
        if (var && var->type == TYPE_STR && var->value.strval) {
            return copy_string(var->value.strval);
        }
        return alloc_string(0);
    }

    /* String functions would be handled here */

    return alloc_string(0);
}
