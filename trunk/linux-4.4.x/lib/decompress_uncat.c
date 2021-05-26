#include <linux/types.h>
#include <linux/compiler.h>

#ifdef STATIC

STATIC int __decompress(unsigned char *buf, long in_len,
			long (*fill)(void*, unsigned long),
			long (*flush)(void*, unsigned long),
			unsigned char *output, long out_len,
			long *posp,
			void (*error)(char *x))
{
	memmove(output, buf, in_len);
	return 0;
}

#endif
