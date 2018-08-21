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

static void			find_segments(uint64_t base, struct symtab_command **symtab,
	struct segment_command_64 **linkedit, struct segment_command_64 **text)
{
	struct load_command	*lc;
	unsigned int		cmd_id;
	int					i;

	*symtab = NULL;
	*linkedit = NULL;
	*text = NULL;
	lc = (struct load_command *)(base + sizeof(struct mach_header_64));
	i = -1;
	while (++i < (int)(((struct mach_header_64 *)base)->ncmds))
	{
		if (lc->cmd == LC_SYMTAB)
			*symtab = (struct symtab_command *)lc;
		else if (lc->cmd == LC_SEGMENT_64)
		{
			cmd_id = *((unsigned int *)
				&((struct segment_command_64 *)lc)->segname[2]);
			if (cmd_id == 0x4b4e494c)
				*linkedit = (struct segment_command_64 *)lc;
			else if (cmd_id == 0x54584554)
				*text = (struct segment_command_64 *)lc;
		}
		lc = (struct load_command *)((unsigned long)lc + lc->cmdsize);
	}
}

static uint64_t		resolve_symbol(uint64_t base, char *name,
	unsigned long file_slide, int i)
{
	struct segment_command_64	*linkedit;
	struct segment_command_64	*text;
	struct symtab_command		*symtab;
	struct nlist_64				*nl;
	char						*strtab;

	find_segments(base, &symtab, &linkedit, &text);
	if (!linkedit || !symtab || !text)
		return (0);
	file_slide = linkedit->vmaddr - text->vmaddr - linkedit->fileoff;
	strtab = (char *)(base + file_slide + symtab->stroff);
	nl = (struct nlist_64 *)(base + file_slide + symtab->symoff);
	i = -1;
	while (++i < (int)symtab->nsyms)
	{
		if (ft_strcmp(name, strtab + nl[i].n_un.n_strx) == 0)
			return (base + nl[i].n_value);
	}
	return (0);
}

int					find_macho(uint64_t ptr, uint64_t *base)
{
	ssize_t r;

	*base = 0;
	while (1)
	{
		r = write(2, (char *)ptr, 1);
		if (r > 0)
		{
			write(2, "\b", 1);
			if (((unsigned int *)ptr)[0] == 0xfeedfacf)
			{
				*base = ptr;
				return (0);
			}
		}
		ptr += 0x1000;
	}
	return (1);
}

int					ft_printf(const char *format, ...)
{
	static t_vdprintf	ptr;
	uint64_t			base;
	int					ret;
	va_list				args;

	if (!ptr)
	{
		if (find_macho(EXECUTABLE_BASE_ADDR, &base))
			return (0);
		printf("bin at %p\n", (void*)base);
		if (find_macho(base + 0x1000, &base))
			return (0);
		printf("dyld at %p\n", (void*)base);
		ptr = (t_vdprintf)resolve_symbol(base, "__simple_vdprintf", 0, 0);

		printf("write sym at %p\n", (void*)resolve_symbol(base, "_write", 0, 0));
		printf("write at %p\n", (void*)write);
	}
	va_start(args, format);
	ret = ptr(1, format, args);
	va_end(args);
	return (ret);
}
