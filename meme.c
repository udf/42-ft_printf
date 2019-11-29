#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <mach-o/dyld.h>
#include <mach-o/nlist.h>
#include <mach-o/dyld_images.h>
#include <mach/mach_vm.h>

#include <stdio.h>
#include <sys/errno.h>

// Dyld shared cache structures
typedef struct {
    char     magic[16];
    uint32_t mappingOffset;
    uint32_t mappingCount;
    uint32_t imagesOffset;
    uint32_t imagesCount;
    uint64_t dyldBaseAddress;
    uint64_t codeSignatureOffset;
    uint64_t codeSignatureSize;
    uint64_t slideInfoOffset;
    uint64_t slideInfoSize;
    uint64_t localSymbolsOffset;
    uint64_t localSymbolsSize;
    char     uuid[16];
    char       sierra_reserved[0x30];
} dyld_cache_header;

typedef struct {
    uint64_t       address;
    uint64_t       size;
    uint64_t       file_offset;
    uint32_t       max_prot;
    uint32_t       init_prot;
} shared_file_mapping_np;

typedef int		(*t_vdprintf)(int fd, const char *fmt, va_list ap);

static char *find_libc_and_cache_base(char *program_name, char **cache_rx_base)
{
    syscall(294, cache_rx_base);

    // Get DYLD task infos
    struct task_dyld_info dyld_info;
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
    kern_return_t ret;
    ret = task_info(mach_task_self_,
            TASK_DYLD_INFO,
            (task_info_t)&dyld_info,
            &count);
    if (ret != KERN_SUCCESS) {
        return NULL;
    }

    // Get image array's size and address
    mach_vm_address_t image_infos = dyld_info.all_image_info_addr;
    struct dyld_all_image_infos *infos = (struct dyld_all_image_infos *)image_infos;
    uint32_t image_count = infos->infoArrayCount;
    const struct dyld_image_info *image_array = infos->infoArray;

    // Find libsystem_c.dylib among them
    char *libc = NULL;
    for (int i = 0; i < image_count; ++i) {
        const struct dyld_image_info *image = image_array + i;

        // Find libsystem_c.dylib's load address
        printf("lib:%s %p\n", image->imageFilePath, image->imageLoadAddress);
        if (strstr(image->imageFilePath, "libsystem_c.dylib")) {
            libc = (char*)image->imageLoadAddress;
        }
    }

    return libc;
}

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
			if (cmd_id == 0x4b4e494c) // LINK
				*linkedit = (struct segment_command_64 *)lc;
			else if (cmd_id == 0x54584554) // TEXT
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
    printf("%p %p %p\n", symtab, linkedit, text);
	if (!linkedit || !symtab || !text)
		return (0);
	file_slide = linkedit->vmaddr - text->vmaddr - linkedit->fileoff;
	strtab = (char *)(base + file_slide + symtab->stroff);
	nl = (struct nlist_64 *)(base + file_slide + symtab->symoff);
	i = -1;
	while (++i < (int)symtab->nsyms)
	{
        printf("%s %p\n", strtab + nl[i].n_un.n_strx, base + nl[i].n_value);
		if (strcmp(name, strtab + nl[i].n_un.n_strx) == 0)
			return (base + nl[i].n_value);
	}
	return (0);
}

int					va_func(void *p, const char *format, ...)
{
	int					ret;
	va_list				args;

    t_vdprintf ptr = (t_vdprintf)p;
	va_start(args, format);
	ret = ptr(1, format, args);
	va_end(args);
	return (ret);
}

int					find_macho(uint64_t ptr, uint64_t *base)
{
	ssize_t r;

	*base = 0;
	while (1)
	{
		r = chmod((char *)ptr, 0777);
		if (errno == 2 && ((unsigned int *)ptr)[0] == 0xfeedfacf)
		{
			*base = ptr;
			return (0);
		}
		ptr += 0x1000;
	}
	return (1);
}

int main (int argc, char *argv[])
{
    printf("target:%p\n", &printf);

    char *shared_cache_rx_base = NULL;
    char *libc = find_libc_and_cache_base(argv[0], &shared_cache_rx_base);
    if (!libc) {
        printf("libsystem_c not found - aborting\n");
        return 1;
    }
    if (!shared_cache_rx_base) {
        printf("shared cache base not found - aborting\n");
        return 1;
    }
    return 2;
    printf("libc:%p\n", libc);
    printf("rx:%p\n", shared_cache_rx_base);
    void *p = resolve_symbol(libc, "_printf", 0, 0);
    printf("lib:%p res:%p\n", &printf, p);

    void (*pf)(const char *fmt, ...) = p;
    pf("%s\n", "hello world");
}