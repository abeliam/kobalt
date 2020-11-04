#ifndef KBCMDCC__H
#define KBCMDCC__H

#include "kobalt/kobalt.h"
#include "kobalt/options.h"
#include "kobalt/str.h"

enum kbcc {
    CCGcc = 0,
    CCClang = 1,
    CCTiny = 2,
    CCNone = 3,
};

struct kbcmdcc {
    enum kbcc cc;
    char* version;
};

void kbcmdcc_new(struct kbcmdcc* cmdcc);

void kbcmdcc_compile(struct kbopts* opts, struct kbcmdcc* cmdcc, struct kbstr* src, struct kbstr* bin);

void kbcmdcc_del(struct kbcmdcc* cmdcc);

#endif
