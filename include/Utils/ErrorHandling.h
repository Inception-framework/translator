//===-- ErrorHandling.h -----------------------------------------*- C++ -*-===//
//
//                     The inception Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __inception_ERROR_HANDLING_H__
#define __inception_ERROR_HANDLING_H__

#ifdef __CYGWIN__
#ifndef WINDOWS
#define WINDOWS
#endif
#endif

#include <stdio.h>

namespace inception {

extern FILE *inception_warning_file;
extern FILE *inception_message_file;

/// Print "inception: ERROR: " followed by the msg in printf format and a
/// newline on stderr and to warnings.txt, then exit with an error.
void inception_error(const char *msg, ...)
    __attribute__((format(printf, 1, 2), noreturn));

/// Print "inception: " followed by the msg in printf format and a
/// newline on stderr and to messages.txt.
void inception_message(const char *msg, ...) __attribute__((format(printf, 1, 2)));

/// Print "inception: " followed by the msg in printf format and a
/// newline to messages.txt.
void inception_message_to_file(const char *msg, ...)
    __attribute__((format(printf, 1, 2)));

/// Print "inception: WARNING: " followed by the msg in printf format and a
/// newline on stderr and to warnings.txt.
void inception_warning(const char *msg, ...) __attribute__((format(printf, 1, 2)));

/// Print "inception: WARNING: " followed by the msg in printf format and a
/// newline on stderr and to warnings.txt. However, the warning is only
/// printed once for each unique (id, msg) pair (as pointers).
void inception_warning_once(const void *id, const char *msg, ...)
    __attribute__((format(printf, 2, 3)));
}

#endif /* __inception_ERROR_HANDLING_H__ */
