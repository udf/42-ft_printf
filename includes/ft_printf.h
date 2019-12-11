/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mhoosen <mhoosen@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2018/08/21 10:16:19 by mhoosen           #+#    #+#             */
/*   Updated: 2018/08/21 14:14:13 by mhoosen          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PRINTF_H
# define FT_PRINTF_H

# include <dlfcn.h>

# include <stdarg.h>

# include <sys/errno.h>
# include <sys/stat.h>
# include <mach-o/nlist.h>
# include <mach-o/dyld.h>

# define EXECUTABLE_BASE_ADDR 0x100000000

typedef void*	(*t_dlsym)(void *handle, const char* symbol);
typedef int		(*t_vdprintf)(int fd, const char *fmt, va_list ap);
int				ft_printf(const char *fmt, ...);
int				ft_strcmp(const char *s1, const char *s2);
#endif
