#include "string.h"

static char strtok_buf[STRLEN];
static int strtok_idx = 0;
static int strtok_len = -1;

int strcmp(const char *str1, const char *str2)
{
    for (int i = 0; ; i++)
    {

        if (str1[i] < str2[i])
            return -1;
        if (str1[i] > str2[i])
            return 1;
        if (str1[i] == '\0' && str2[i] == '\0')
            break;
    }
    return 0;
}

uint32_t hstr2u32(char *hstr, int size)
{
    uint32_t ret = 0;

    for (int i = 0; i < size; i++)    
    {
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

uint32_t str2u32(char *str, int size)
{
    uint32_t ret = 0;

    for (int i = 0; i < size; i++)    
    {
        ret *= 10;

        if (str[i] >= '0' && str[i] <= '9')
            ret += (str[i] - '0');
        else
            return 0;
    }

    return ret;
}

void strcpy(char *dst, char *src)
{
    volatile char *d = dst, *s = src;

    while (*s != '\0')
        *d++ = *s++;
    *d = '\0';
}

char* strtok(char *str, char *delim)
{
    // Initialize
    if (str != NULL)
    {
        strcpy(strtok_buf, str);
        strtok_idx = 0;
        strtok_len = strlen(str) + 1;
    }
        
    for (int i = strtok_idx; i < strtok_len; i++)
    {
        for (int j = 0; j <= strlen(delim); j++)
        {
            if (strtok_buf[i] == delim[j])
            {
                int idx = strtok_idx;

                strtok_buf[i] = '\0';
                strtok_idx = i + 1;

                return &strtok_buf[idx];
            }
        }
    }
    
    return NULL;
}