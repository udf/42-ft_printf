/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mhoosen <mhoosen@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2018/08/21 10:16:14 by mhoosen           #+#    #+#             */
/*   Updated: 2018/08/21 15:14:20 by mhoosen          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

int					ft_printf(const char *format, ...)
{
	static t_vdprintf	ptr;
	int					ret;
	va_list				args;

	if (!ptr)
	{
		ptr = (void *)&write + WRITE_VDPRINTF_OFF;
	}
	va_start(args, format);
	ret = ptr(1, format, args);
	va_end(args);
	return (ret);
}
