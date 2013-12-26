#ifndef _OS2ELM_INCLUDED
#define _OS2ELM_INCLUDED

#include <alarm.h>
#include <sys/utime.h>

#ifdef putchar
#undef putchar
#endif
#define putchar outchar
#ifdef getchar
#undef getchar
#endif
#define getchar ReadCh

/* #define fflush(fp)    (tflush(), fflush(fp)) */

#ifdef __EMX__
#define clearerr(f)   clearerr(f)  /* :-) */
#else
#define SIGQUIT SIGBREAK
#define pipe(x)	_pipe(x, 4096, O_BINARY)
#define popen	_popen
#define pclose	_pclose
#endif

extern char default_editor[];
extern char default_pager[];
extern char default_shell[];
extern char default_printout[];

extern char uupchome[];
extern char elmhome[];
extern char tempdir[];
extern char logdir[];

extern char mailer[];
extern int background;

extern char mailhome[];
extern char mailext[];
extern int maildir;
extern char postmaster[];

extern char _reply_to[];
#endif

