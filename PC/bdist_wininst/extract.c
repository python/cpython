/*
  IMPORTANT NOTE: IF THIS FILE IS CHANGED, PCBUILD\BDIST_WININST.VCXPROJ MUST
  BE REBUILT AS WELL.

  IF CHANGES TO THIS FILE ARE CHECKED IN, THE RECOMPILED BINARIES MUST BE
  CHECKED IN AS WELL!
*/

#include <windows.h>

#include "zlib.h"

#include <stdio.h>
#include <stdarg.h>

#include "archive.h"

/* Convert unix-path to dos-path */
static void normpath(char *path)
{
    while (path && *path) {
        if (*path == '/')
            *path = '\\';
        ++path;
    }
}

BOOL ensure_directory(char *pathname, char *new_part, NOTIFYPROC notify)
{
    while (new_part && *new_part && (new_part = strchr(new_part, '\\'))) {
        DWORD attr;
        *new_part = '\0';
        attr = GetFileAttributes(pathname);
        if (attr == -1) {
            /* nothing found */
            if (!CreateDirectory(pathname, NULL) && notify)
                notify(SYSTEM_ERROR,
                       "CreateDirectory (%s)", pathname);
            else
                notify(DIR_CREATED, pathname);
        }
        if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            ;
        } else {
            SetLastError(183);
            if (notify)
                notify(SYSTEM_ERROR,
                       "CreateDirectory (%s)", pathname);
        }
        *new_part = '\\';
        ++new_part;
    }
    return TRUE;
}

/* XXX Should better explicitly specify
 * uncomp_size and file_times instead of pfhdr!
 */
char *map_new_file(DWORD flags, char *filename,
                   char *pathname_part, int size,
                   WORD wFatDate, WORD wFatTime,
                   NOTIFYPROC notify)
{
    HANDLE hFile, hFileMapping;
    char *dst;
    FILETIME ft;

  try_again:
    if (!flags)
        flags = CREATE_NEW;
    hFile = CreateFile(filename,
                       GENERIC_WRITE | GENERIC_READ,
                       0, NULL,
                       flags,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD x = GetLastError();
        switch (x) {
        case ERROR_FILE_EXISTS:
            if (notify && notify(CAN_OVERWRITE, filename))
                hFile = CreateFile(filename,
                                   GENERIC_WRITE|GENERIC_READ,
                                   0, NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
            else {
                if (notify)
                    notify(FILE_OVERWRITTEN, filename);
                return NULL;
            }
            break;
        case ERROR_PATH_NOT_FOUND:
            if (ensure_directory(filename, pathname_part, notify))
                goto try_again;
            else
                return FALSE;
            break;
        default:
            SetLastError(x);
            break;
        }
    }
    if (hFile == INVALID_HANDLE_VALUE) {
        if (notify)
            notify (SYSTEM_ERROR, "CreateFile (%s)", filename);
        return NULL;
    }

    if (notify)
        notify(FILE_CREATED, filename);

    DosDateTimeToFileTime(wFatDate, wFatTime, &ft);
    SetFileTime(hFile, &ft, &ft, &ft);


    if (size == 0) {
        /* We cannot map a zero-length file (Also it makes
           no sense */
        CloseHandle(hFile);
        return NULL;
    }

    hFileMapping = CreateFileMapping(hFile,
                                     NULL, PAGE_READWRITE, 0, size, NULL);

    CloseHandle(hFile);

    if (hFileMapping == NULL) {
        if (notify)
            notify(SYSTEM_ERROR,
                   "CreateFileMapping (%s)", filename);
        return NULL;
    }

    dst = MapViewOfFile(hFileMapping,
                        FILE_MAP_WRITE, 0, 0, 0);

    CloseHandle(hFileMapping);

    if (!dst) {
        if (notify)
            notify(SYSTEM_ERROR, "MapViewOfFile (%s)", filename);
        return NULL;
    }
    return dst;
}


BOOL
extract_file(char *dst, char *src, int method, int comp_size,
             int uncomp_size, NOTIFYPROC notify)
{
    z_stream zstream;
    int result;

    if (method == Z_DEFLATED) {
        int x;
        memset(&zstream, 0, sizeof(zstream));
        zstream.next_in = src;
        zstream.avail_in = comp_size+1;
        zstream.next_out = dst;
        zstream.avail_out = uncomp_size;

/* Apparently an undocumented feature of zlib: Set windowsize
   to negative values to suppress the gzip header and be compatible with
   zip! */
        result = TRUE;
        if (Z_OK != (x = inflateInit2(&zstream, -15))) {
            if (notify)
                notify(ZLIB_ERROR,
                       "inflateInit2 returns %d", x);
            result = FALSE;
            goto cleanup;
        }
        if (Z_STREAM_END != (x = inflate(&zstream, Z_FINISH))) {
            if (notify)
                notify(ZLIB_ERROR,
                       "inflate returns %d", x);
            result = FALSE;
        }
      cleanup:
        if (Z_OK != (x = inflateEnd(&zstream))) {
            if (notify)
                notify (ZLIB_ERROR,
                    "inflateEnd returns %d", x);
            result = FALSE;
        }
    } else if (method == 0) {
        memcpy(dst, src, uncomp_size);
        result = TRUE;
    } else
        result = FALSE;
    UnmapViewOfFile(dst);
    return result;
}

/* Open a zip-compatible archive and extract all files
 * into the specified directory (which is assumed to exist)
 */
BOOL
unzip_archive(SCHEME *scheme, char *dirname, char *data, DWORD size,
              NOTIFYPROC notify)
{
    int n;
    char pathname[MAX_PATH];
    char *new_part;

    /* read the end of central directory record */
    struct eof_cdir *pe = (struct eof_cdir *)&data[size - sizeof
                                                   (struct eof_cdir)];

    int arc_start = size - sizeof (struct eof_cdir) - pe->nBytesCDir -
        pe->ofsCDir;

    /* set position to start of central directory */
    int pos = arc_start + pe->ofsCDir;

    /* make sure this is a zip file */
    if (pe->tag != 0x06054b50)
        return FALSE;

    /* Loop through the central directory, reading all entries */
    for (n = 0; n < pe->nTotalCDir; ++n) {
        int i;
        char *fname;
        char *pcomp;
        char *dst;
        struct cdir *pcdir;
        struct fhdr *pfhdr;

        pcdir = (struct cdir *)&data[pos];
        pfhdr = (struct fhdr *)&data[pcdir->ofs_local_header +
                                     arc_start];

        if (pcdir->tag != 0x02014b50)
            return FALSE;
        if (pfhdr->tag != 0x04034b50)
            return FALSE;
        pos += sizeof(struct cdir);
        fname = (char *)&data[pos]; /* This is not null terminated! */
        pos += pcdir->fname_length + pcdir->extra_length +
            pcdir->comment_length;

        pcomp = &data[pcdir->ofs_local_header
                      + sizeof(struct fhdr)
                      + arc_start
                      + pfhdr->fname_length
                      + pfhdr->extra_length];

        /* dirname is the Python home directory (prefix) */
        strcpy(pathname, dirname);
        if (pathname[strlen(pathname)-1] != '\\')
            strcat(pathname, "\\");
        new_part = &pathname[lstrlen(pathname)];
        /* we must now match the first part of the pathname
         * in the archive to a component in the installation
         * scheme (PURELIB, PLATLIB, HEADERS, SCRIPTS, or DATA)
         * and replace this part by the one in the scheme to use
         */
        for (i = 0; scheme[i].name; ++i) {
            if (0 == strnicmp(scheme[i].name, fname,
                              strlen(scheme[i].name))) {
                char *rest;
                int len;

                /* length of the replaced part */
                int namelen = strlen(scheme[i].name);

                strcat(pathname, scheme[i].prefix);

                rest = fname + namelen;
                len = pfhdr->fname_length - namelen;

                if ((pathname[strlen(pathname)-1] != '\\')
                    && (pathname[strlen(pathname)-1] != '/'))
                    strcat(pathname, "\\");
                /* Now that pathname ends with a separator,
                 * we must make sure rest does not start with
                 * an additional one.
                 */
                if ((rest[0] == '\\') || (rest[0] == '/')) {
                    ++rest;
                    --len;
                }

                strncat(pathname, rest, len);
                goto Done;
            }
        }
        /* no prefix to replace found, go unchanged */
        strncat(pathname, fname, pfhdr->fname_length);
      Done:
        normpath(pathname);
        if (pathname[strlen(pathname)-1] != '\\') {
            /*
             * The local file header (pfhdr) does not always
             * contain the compressed and uncompressed sizes of
             * the data depending on bit 3 of the flags field.  So
             * it seems better to use the data from the central
             * directory (pcdir).
             */
            dst = map_new_file(0, pathname, new_part,
                               pcdir->uncomp_size,
                               pcdir->last_mod_file_date,
                               pcdir->last_mod_file_time, notify);
            if (dst) {
                if (!extract_file(dst, pcomp, pfhdr->method,
                                  pcdir->comp_size,
                                  pcdir->uncomp_size,
                                  notify))
                    return FALSE;
            } /* else ??? */
        }
        if (notify)
            notify(NUM_FILES, new_part, (int)pe->nTotalCDir,
                   (int)n+1);
    }
    return TRUE;
}
