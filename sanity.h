/* $Id: sanity.h,v 1.2 1998/10/21 01:58:09 phelps Exp $ */

#ifndef SANITY_H
#define SANITY_H

/* Sanity checking */
#ifdef SANITY

#define SANITY_CHECK(sanity_state) \
{ \
    if (sanity_state -> sanity_check != sanity_value) \
    { \
	fprintf(stderr, "*** %s failed sanity check at %s:%d!\n", sanity_value, __FILE__, __LINE__); \
	fprintf(stderr, "     (It thinks it's a %s)\n", sanity_state -> sanity_check); \
	exit(1); \
    } \
}

#else /* SANITY */

#define SANITY_CHECK(sanity_state)

#endif /* SANITY */

#endif /* SANITY_H */
