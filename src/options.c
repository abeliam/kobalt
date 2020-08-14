#include "kobalt/options.h"
#include "kobalt/source.h"
#include "kobalt/memory.h"
#include "kobalt/fs.h"
#include "kobalt/uid.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#if WINDOWS
#include <direct.h>
#else
#include <unistd.h>
#endif

void kbopts_new(int argc, char* argv[], struct kbopts* opts) {
    opts->stage = CODEGEN;
    opts->output = stdout;
    opts->numsrcs = 0;
    int srcs_capacity = 1;
    opts->srcs = kbmalloc(sizeof(opts->srcs[0]) * srcs_capacity);
    opts->verbosity = 1;
    unsigned int ii = 1;
    while (ii<(unsigned int)argc) {
        unsigned int jj = ii;
        if (argv[jj][0] == '-') {
            if (strlen(argv[jj]) == 2) {
                char optopt = argv[jj][1];
                char * optarg = NULL;
                if (jj+1<(unsigned int)argc && argv[jj+1][0] != '-') {
                    optarg = argv[jj+1];
                }
                switch (optopt) {
                    case 'L':
                        opts->stage = LEX;
                        break;
                    case 'T':
                        opts->stage = PARSE;
                        break;
                    case 'V':
                        opts->verbosity = 2;
                        break;
                    case 'h':
                        printf("Kobalt Language Compiler v%s\n\n", KBVERSION);
                        break;
                    case 'o': {
                        ii++;
                        if (optarg == NULL) {
                            fprintf(stderr, "Argument to '-o' is missing\n");
                            exit(1);
                        }
                        opts->output = fopen(optarg, "w");
                        if (opts->output == NULL) {
                            perror("kbc: error: can't output file");
                            exit(1);
                        }
                    }
                        break;
                    default: {
                        if (isprint (optopt)) {
                            fprintf (stderr, "kbc: error: unknown option '-%c'\n", optopt);
                        }
                        else {
                            fprintf(stderr, "kbc: error: unknown option character '\\x%x'\n", optopt);
                        }
                        exit(1);
                    }
                        break;
                }
            }
            else {
                fprintf (stderr, "kbc: error: unknown option '%s'\n", argv[jj]);
                exit(1);
            }
        }
        else {
            if (opts->numsrcs == srcs_capacity) {
                srcs_capacity = 2 * srcs_capacity;
                opts->srcs = kbrealloc(opts->srcs, sizeof(opts->srcs[0]) * srcs_capacity);
            }
            opts->srcs[opts->numsrcs] = argv[jj];
            ++ opts->numsrcs;
        }
        ii++;
    }

#if DEBUG
    if(!opts->numsrcs) {
        opts->srcs[opts->numsrcs] = "doc/examples/hello_world.kb";
        ++ opts->numsrcs;
    }
#endif

    if (!opts->numsrcs) {
        printf("error: no input file\n");
        exit(1);
    }

    size_t cwdsize = 32;
    opts->cwd = kbmalloc(cwdsize);
    int maxiter = 4;
    while(maxiter-- && (getcwd(opts->cwd, cwdsize) == NULL)) {
        cwdsize = 2 * cwdsize;
        opts->cwd = kbrealloc(opts->cwd, cwdsize);    
    } 
    if (maxiter == 0) {
        fprintf(stderr, "kbc: error: allocation");
        exit(1);
    }
    
#if WINDOWS
    for(char* c = opts->cwd; *c != '\0'; c++) {
        if (*c == '\\') {
            *c = '/';
        }
    }
#endif
   
    seed(42);
    int cachedirsize = strlen("kbcache") + 1 + 8 + 1;
    opts->cachedir = kbmalloc(sizeof(char) * cachedirsize);
    opts->cachedir[cachedirsize - 1] = '\0';
    strcpy(opts->cachedir, "kbcache/");
    genuid(opts->cachedir + strlen("kbcache") + 1);
    
    ensuredir("kbcache");
    ensuredir(opts->cachedir);
}

char * kbstage_string(enum kbstage stage) {
    switch (stage) {
        case LEX:
            return "LEX";
        case PARSE:
            return "PARSE";
        case CODEGEN:
            return "CODEGEN";
    }
    return "UNDEFINED";
}

void kbopts_display(struct kbopts * opts) {
    fprintf(stderr, "kbopts {\n");
    fprintf(stderr, "  stage = %s\n", kbstage_string(opts->stage));
    fprintf(stderr, "  cwd = %s\n", opts->cwd);
    fprintf(stderr, "  numsrcs = %d\n", opts->numsrcs);
    fprintf(stderr, "  srcs[%d] = [array]\n", opts->numsrcs);
    fprintf(stderr, "}\n");
}

void kbopts_del(struct kbopts * opts) {
    kbfree(opts->cwd);
    kbfree(opts->srcs);
    kbfree(opts->cachedir);
}
