#include "kobalt/options.h"
#include "kobalt/compiland.h"
#include "kobalt/memory.h"
#include "kobalt/log.h"
#include "kobalt/fs.h"
#include "kobalt/uid.h"
#include "kobalt/time.h"
#include "kobalt/stdsrc.h"
#include "sha-256.h"
#include "base32.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#if WINDOWS
#include <direct.h>
#include <shlobj_core.h>
#pragma comment(lib,"Shell32.lib")
#else
#include <unistd.h>
#endif

void kbopts_new(struct kbopts* opts, int argc, char* argv[]) {
    opts->stages = LexingStage | ParsingStage | TypeInferStage | TypeCheckStage | CGenStage | CCStage | ExecStage;
    opts->optim = 0;
    kbstr_new(&opts->outpath);
    opts->verbosity = 1;
    kbvec_new(&opts->inputs, sizeof(struct kbcompiland));
    kbvec_new(&opts->exe_argv, sizeof(char*));

    int i = 1;
    bool endopts = false;
    while (i < argc) {
        int j = i;
        if (strcmp(argv[j], "--") == 0) {
            endopts = true;
        }
        else if (argv[j][0] == '-') {
            if (strlen(argv[j]) == 2) {
                char optopt = argv[j][1];
                char* optarg = NULL;
                if (j + 1 < argc && argv[j + 1][0] != '-') {
                    optarg = argv[j+1];
                }
                switch (optopt) {
                    case 'L':
                        opts->stages = LexingStage;
                        break;
                    case 'P':
                        opts->stages = LexingStage | ParsingStage;
                        break;
                    case 'T':
                        opts->stages = LexingStage | ParsingStage | TypeInferStage | TypeCheckStage;
                        break;
                    case 'v':
                        printf("Kobalt Language Compiler v%s\n\n", KBVERSION);
                        exit(0);
                        break;
                    case 'h':
                        printf("Usage: %s [file...]\n", argv[0]);
                        printf("Options:\n");
                        printf("  -o  output file\n");
                        printf("  -v  display version\n");
                        printf("  -L  lexing stage\n");
                        printf("  -P  parsing stage: produce ast\n");
                        printf("  -I  type inference stage: produce typed ast\n");
                        exit(0);
                        break;
                    case 'o':
                        {
                            i++;
                            if (optarg == NULL) {
                                kbelog("argument to '-o' is missing");
                                exit(1);
                            }
                            kbstr_cat(&opts->outpath, optarg);
                        }
                        break;
                    default: {
                        if (isprint (optopt)) {
                            kbelog("unknown option '-%c'", optopt);
                        }
                        else {
                            kbelog("unknown option character '\\x%x'", optopt);
                        }
                        exit(1);
                    }
                        break;
                }
            }
            else {
                kbelog("unknown option '%s'", argv[j]);
                exit(1);
            }
        }
        else {
            if (endopts) {
                kbvec_push(&opts->exe_argv, &argv[j]);
            }
            else {

                struct kbcompiland compiland;
                // kbcompiland_new_virt(&compiland, "@std.kb", (char*)stdkb);
                // kbqueue_enqueue(input, &compiland);

                kbcompiland_new_entry(&compiland, &argv[j][0]);
                kbvec_push(&opts->inputs, &compiland);
            }
        }
        ++ i;
    }

    kbvec_push(&opts->exe_argv, &(void*){NULL});

    size_t cwdsize = 32;
    kbstr_new(&opts->cwd);
    kbstr_resize(&opts->cwd, cwdsize);
    int maxiter = 10;
    while(maxiter-- && (getcwd(opts->cwd.data, cwdsize) == NULL)) {
        cwdsize = 2 * cwdsize;
        kbstr_resize(&opts->cwd, cwdsize);
    } 
    if (maxiter <= 0) {
        kbelog("allocation");
        exit(1);
    }
    kbpath_normalize(&opts->cwd);
   
    // int seednum = kbtime_get();
    // kbilog("random seed is %d", seednum);
    seed(1337);

    kbstr_new(&opts->cachepath);
#if UNIX
    char* home = getenv("HOME");
    if(home == NULL) {
        kbelog("cannot get HOME environment variable");
        exit(1);
    }

    kbstr_cat(&opts->cachepath, home);
    kbpath_push(&opts->cachepath, ".cache");
    ensuredir(opts->cachepath.data);
    kbpath_push(&opts->cachepath, "kobalt");

#else
    PWSTR appdata = NULL;
    if (SHGetKnownFolderPath(&FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &appdata) == S_OK) {
        char dest[MAX_PATH];
        wcstombs(dest, appdata, MAX_PATH);
        kbstr_cat(&opts->cachepath, dest);
    }
    else {
        kbelog("getting AppData path"); 
    }
#endif
    ensuredir(opts->cachepath.data);
    kbpath_push(&opts->cachepath, "build");
    ensuredir(opts->cachepath.data);

    uint8_t data[32];
    int bsize = base32encsize(sizeof(data));
    int cachepathlen = opts->cachepath.len + bsize + 1;

    kbpath_push(&opts->cachepath, "placeholder");
    kbstr_resize(&opts->cachepath, cachepathlen);
    
    calc_sha_256(data, opts->cwd.data, opts->cwd.len);

    base32enc(&opts->cachepath.data[opts->cachepath.len - bsize], data, sizeof(data));
    
    kbpath_normalize(&opts->cachepath);
    ensuredir(opts->cachepath.data);

#if DEBUG
    kbilog("cachepath is %s", opts->cachepath.data);
#endif
}

void kbopts_del(struct kbopts* opts) {
    kbvec_del(&opts->inputs);
    kbstr_del(&opts->cwd);
    kbstr_del(&opts->cachepath);
    kbstr_del(&opts->outpath);
    kbfree(*(char**)kbvec_get(&opts->exe_argv, 0));
    kbvec_del(&opts->exe_argv);
}
