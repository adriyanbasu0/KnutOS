#include "stdlib.h"

long strtol(const char *nptr, char **endptr, int base) {
    long result = 0;
    int sign = 1;
    while (*nptr == ' ' || *nptr == '\t') nptr++;
    if (*nptr == '-') { sign = -1; nptr++; }
    else if (*nptr == '+') nptr++;
    while (*nptr) {
        int digit;
        if (*nptr >= '0' && *nptr <= '9') digit = *nptr - '0';
        else if (*nptr >= 'a' && *nptr <= 'z') digit = *nptr - 'a' + 10;
        else if (*nptr >= 'A' && *nptr <= 'Z') digit = *nptr - 'A' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        nptr++;
    }
    if (endptr) *endptr = (char *)nptr;
    return sign * result;
}

unsigned long strtoul(const char *nptr, char **endptr, int base) {
    unsigned long result = 0;
    while (*nptr == ' ' || *nptr == '\t') nptr++;
    if (*nptr == '+') nptr++;
    while (*nptr) {
        int digit;
        if (*nptr >= '0' && *nptr <= '9') digit = *nptr - '0';
        else if (*nptr >= 'a' && *nptr <= 'z') digit = *nptr - 'a' + 10;
        else if (*nptr >= 'A' && *nptr <= 'Z') digit = *nptr - 'A' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        nptr++;
    }
    if (endptr) *endptr = (char *)nptr;
    return result;
}
