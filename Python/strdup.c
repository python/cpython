/* strdup() replacement (from stdwin, if you must know) */

char *
strdup(const char *str)
{
    if (str != NULL) {
        size_t len = strlen(str) + 1;
        char *copy = malloc(len);
        if (copy != NULL)
            return memcpy(copy, str, len);
    }
    return NULL;
}
