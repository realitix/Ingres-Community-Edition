#ifndef _BSDFUNC_H_
#define _BSDFUNC_H_

#include <grp.h>
#include <pwd.h>

/*
 * Prototypes for the included BSD functions.
 */
size_t strlcpy (char *dst, const char *src, size_t siz);
void strmode (mode_t mode, char *p);

/*
 * Emulate BSD's setgroupent().
 */
#define setgroupent(close) setgrent()

/*
 * Emulate BSD's setpassent().
 */
#define setpassent(close) setpwent()

/*
 * These are really for MAKE_BOOTSTRAP but harmless.
 * XXX - Why not just use malloc in here, anyway?
 */
#ifndef	ALIGNBYTES
#define	ALIGNBYTES 3
#endif
#ifndef ALIGN
#define	ALIGN(p)	(((long)(p) + ALIGNBYTES) &~ ALIGNBYTES)
#endif

#endif /* _BSDFUNC_H_ */
