#ifndef __IP_SET_COMPILER_H
#define __IP_SET_COMPILER_H

/* Compiler attributes */
#ifndef __has_attribute
# define __has_attribute(x) __GCC4_has_attribute_##x
# define __GCC4_has_attribute___fallthrough__		0
#endif

#if __has_attribute(__fallthrough__)
# define fallthrough			__attribute__((__fallthrough__))
#else
# define fallthrough			do {} while (0)  /* fallthrough */
#endif
#endif /* __IP_SET_COMPILER_H */
