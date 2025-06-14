#include "string.h"
#include "mem.h"

int strcmp(const char *str1, const char *str2) {
    return strncmp(str1, str2, ~0);
}

int strncmp(const char *str1, const char *str2, size_t count) {
    for (int i = 0; i < count; i++) {
        if (str1[i] < str2[i])
            return -1;
        if (str1[i] > str2[i])
            return 1;
        if (str1[i] == '\0' && str2[i] == '\0')
            break;
    }
    return 0;
}

uint32_t hstr2u32(char *hstr, int size) {
    uint32_t ret = 0;

    for (int i = 0; i < size; i++) {
        ret <<= 4;

        if (hstr[i] >= '0' && hstr[i] <= '9')
            ret += (hstr[i] - '0');
        else if (hstr[i] >= 'A' && hstr[i] <= 'F')
            ret += (hstr[i] - 'A' + 10);
        else if (hstr[i] >= 'a' && hstr[i] <= 'f')
            ret += (hstr[i] - 'a' + 10);
        else
            return 0;
    }

    return ret;
}

uint32_t str2u32(char *str, int size) {
    uint32_t ret = 0;

    for (int i = 0; i < size; i++) {
        ret *= 10;

        if (str[i] >= '0' && str[i] <= '9')
            ret += (str[i] - '0');
        else
            return 0;
    }

    return ret;
}

void strcpy(char *dst, const char *src) {
    strncpy(dst, src, ~0);
}

void strncpy(char *dst, const char *src, size_t count) {
    volatile char *d = dst;
    volatile const char *s = src;

    for (int i = 0; i < count && *s != '\0'; i++, d++, s++)
        *d = *s;

    *d = '\0';
}

char* strtok_r(char *str, char *delim, struct strtok_ctx **ctx_ptr) {
    struct strtok_ctx *ctx;

    // Initialize
    if (str != NULL) {
        if (!(*ctx_ptr = malloc(sizeof(struct strtok_ctx))))
            return NULL;

        ctx = *ctx_ptr;
        strcpy(ctx->strtok_str, str);
        ctx->strtok_idx = 0;
        ctx->strtok_len = strlen(str) + 1;
    } 
    else 
        ctx = *ctx_ptr;

    for (int i = ctx->strtok_idx; i < ctx->strtok_len; i++) {
        for (int j = 0; j <= strlen(delim); j++) {
            if (ctx->strtok_str[i] == delim[j]) {
                int idx = ctx->strtok_idx;

                ctx->strtok_str[i] = '\0';
                ctx->strtok_idx = i + 1;

                return &ctx->strtok_str[idx];
            }
        }
    }
    
    return NULL;
}