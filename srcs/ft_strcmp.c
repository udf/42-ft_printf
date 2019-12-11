#include "ft_printf.h"

int			ft_strcmp(const char *s1, const char *s2)
{
	const unsigned char *s1p;
	const unsigned char *s2p;

	s1p = (const unsigned char*)s1;
	s2p = (const unsigned char*)s2;
	while (*s1p && *s2p && *s1p == *s2p)
	{
		s1p++;
		s2p++;
	}
	return (*s1p - *s2p);
}