/* $Id: sanity.h,v 1.1 1997/05/31 03:42:31 phelps Exp $ */

#ifndef SANITY_H
#define SANITY_H

/* Sanity checking */
#ifdef SANITY

#define SANITY_CHECK(sanity_state) \
{ \
    if (sanity_state -> sanity_check != sanity_value) \
    { \
	fprintf(stderr, "*** %s failed sanity check at %s:%d!\n", sanity_value, __FILE__, __LINE__); \
	exit(1); \
    } \
}

#else /* SANITY */

#define SANITY_CHECK(sanity_state)

#endif /* SANITY */

#endif /* SANITY_H */
