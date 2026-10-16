/*
   capture.h

   Test helper: run a function that writes to a FILE*, then read back what
   it wrote as a string, so the formatting code can be checked exactly.
 */

#ifndef FRAMESMITH_TESTS_CAPTURE_H
#define FRAMESMITH_TESTS_CAPTURE_H

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Opens a throwaway temporary file for a test to write into.

   On POSIX, tmpfile() is perfect: it creates an unnamed file that vanishes
   when closed. On Windows, the C runtime's tmpfile() tries to create the
   file in the root of the current drive (C:\), which an ordinary, non-admin
   process cannot write to, so it returns NULL. Instead we create a uniquely
   named file in the current directory (where CTest runs each test, and which
   is writable) and open it with the "D" mode flag, a Windows C runtime
   extension meaning "delete this file when it is closed" -- so it cleans
   itself up exactly like tmpfile() and the existing fclose() calls need no
   change. The name includes the process id and a counter so parallel tests
   never collide. */
static FILE *capture_tmpfile(void) {
#ifdef _WIN32
    static unsigned counter = 0;
    char name[64];
    snprintf(name, sizeof(name), "framesmith_test_%lu_%u.tmp", (unsigned long)GetCurrentProcessId(),
             counter++);
    return fopen(name, "wb+D");
#else
    return tmpfile();
#endif
}

/* Returns a malloc'ed, zero-terminated copy of everything written to the
   temporary file so far, or NULL on error. The caller frees it. */
static char *capture_contents(FILE *file) {
    long length = ftell(file);
    if (length < 0) {
        return NULL;
    }
    rewind(file);
    char *text = malloc((size_t)length + 1);
    if (text == NULL) {
        return NULL;
    }
    size_t read = fread(text, 1, (size_t)length, file);
    text[read] = '\0';
    return text;
}

#endif /* FRAMESMITH_TESTS_CAPTURE_H */
