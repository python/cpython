
size_t
UNIOP(strlen)(const UNIOP_t *u)
{
    int res = 0;
    while(*u++)
        res++;
    return res;
}

UNIOP_t*
UNIOP(strcpy)(UNIOP_t *s1, const UNIOP_t *s2)
{
    UNIOP_t *u = s1;
    while ((*u++ = *s2++));
    return s1;
}

UNIOP_t*
UNIOP(strncpy)(UNIOP_t *s1, const UNIOP_t *s2, size_t n)
{
    UNIOP_t *u = s1;
    while ((*u++ = *s2++))
        if (n-- == 0)
            break;
    return s1;
}

UNIOP_t*
UNIOP(strcat)(UNIOP_t *s1, const UNIOP_t *s2)
{
    UNIOP_t *u1 = s1;
    u1 += UNIOP(strlen(u1));
    UNIOP(strcpy(u1, s2));
    return s1;
}

int
UNIOP(strcmp)(const UNIOP_t *s1, const UNIOP_t *s2)
{
    while (*s1 && *s2 && *s1 == *s2)
        s1++, s2++;
    if (*s1 && *s2)
        return (*s1 < *s2) ? -1 : +1;
    if (*s1)
        return 1;
    if (*s2)
        return -1;
    return 0;
}

int
UNIOP(strncmp)(const UNIOP_t *s1, const UNIOP_t *s2, size_t n)
{
    register UNIOP_t u1, u2;
    for (; n != 0; n--) {
        u1 = *s1;
        u2 = *s2;
        if (u1 != u2)
            return (u1 < u2) ? -1 : +1;
        if (u1 == '\0')
            return 0;
        s1++;
        s2++;
    }
    return 0;
}

UNIOP_t*
UNIOP(strchr)(const UNIOP_t *s, UNIOP_t c)
{
    const UNIOP_t *p;
    for (p = s; *p; p++)
        if (*p == c)
            return (UNIOP_t*)p;
    return NULL;
}

UNIOP_t*
UNIOP(strrchr)(const UNIOP_t *s, UNIOP_t c)
{
    const UNIOP_t *p;
    p = s + UNIOP(strlen)(s);
    while (p != s) {
        p--;
        if (*p == c)
            return (UNIOP_t*)p;
    }
    return NULL;
}

