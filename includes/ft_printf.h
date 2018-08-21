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

# include <stdio.h>

# include <unistd.h>
# include <stdarg.h>

# include <mach-o/nlist.h>
# include <mach-o/dyld.h>

# include "libft.h"

# define EXECUTABLE_BASE_ADDR 0x100000000
# define DYLD_BASE 0x00007fff5fc00000

typedef int		(*t_vdprintf)(int fd, const char *fmt, va_list ap);
int ft_printf	(const char *fmt, ...);

#endif
