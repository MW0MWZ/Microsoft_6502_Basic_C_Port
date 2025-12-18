/*
 * main.c - Main entry point and initialization
 *
 * Microsoft BASIC 6502 C Port
 * K&R C v2 compatible
 */

#include "m6502basic.h"

/* Global state */
state_t *g_state = NULL;

/*
 * Initialize interpreter state
 */
void
init_state()
{
    unsigned char *mem;
    long memsize;

    /* Allocate state structure */
    g_state = (state_t *)malloc(sizeof(state_t));
    if (!g_state) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }

    /* Clear state */
    memset(g_state, 0, sizeof(state_t));

    /* Allocate program memory - try progressively smaller sizes */
    memsize = PROGRAM_SIZE;
    mem = NULL;
    while (memsize >= 4096L && !mem) {
        mem = (unsigned char *)malloc((size_t)memsize);
        if (!mem) {
            memsize = memsize / 2;
        }
    }
    if (!mem) {
        fprintf(stderr, "Out of memory\n");
        free(g_state);
        exit(1);
    }

    /* Set up memory pointers like 6502 BASIC */
    g_state->txttab = mem;
    g_state->vartab = mem;
    g_state->arytab = mem;
    g_state->strend = mem;
    g_state->memsiz = mem + memsize;
    g_state->fretop = g_state->memsiz;

    /* Mark end of program (two zero bytes) */
    g_state->txttab[0] = 0;
    g_state->txttab[1] = 0;

    /* Initialize execution state */
    g_state->curlin = -1;  /* Direct mode */
    g_state->txtptr = NULL;
    g_state->curline_ptr = NULL;

    /* Initialize stacks */
    g_state->forsp = 0;
    g_state->gosubsp = 0;

    /* Initialize DATA pointer */
    g_state->dataptr.linenum = 0;
    g_state->dataptr.ptr = NULL;

    /* Initialize random seed */
    g_state->rndseed = 12345L;

    /* Clear variables and arrays */
    g_state->varlist = NULL;
    g_state->arrlist = NULL;

    /* Terminal position */
    g_state->trmpos = 0;

    /* Not running */
    g_state->running = 0;
    g_state->errnum = ERR_NONE;
}

/*
 * Cleanup and free resources
 */
void
cleanup()
{
    if (g_state) {
        /* Free variables */
        clear_variables();

        /* Free arrays */
        clear_arrays();

        /* Free program memory */
        if (g_state->txttab) {
            free(g_state->txttab);
        }

        /* Free state */
        free(g_state);
        g_state = NULL;
    }
}

/*
 * Print banner
 */
static void
print_banner()
{
    printf("MICROSOFT BASIC 6502\n");
    printf("VERSION 1.1\n");
    printf("(C) COPYRIGHT 1976-1978 MICROSOFT\n");
    printf("C PORT (C) 2025 ANDY TAYLOR\n");
    printf("%ld BYTES FREE\n\n", (long)(g_state->memsiz - g_state->txttab));
}

/*
 * Main entry point
 */
int
main(argc, argv)
int argc;
char **argv;
{
    /* Initialize interpreter */
    init_state();

    /* Print banner */
    print_banner();

    /* Load file if specified on command line */
    if (argc > 1) {
        if (load_file(argv[1]) == 0) {
            printf("LOADED %s\n", argv[1]);
        }
    }

    /* Enter REPL */
    repl();

    /* Cleanup */
    cleanup();

    return 0;
}
