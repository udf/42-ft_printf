#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <mach-o/dyld.h>
#include <mach-o/nlist.h>
#include <mach-o/dyld_images.h>
#include <mach/mach_vm.h>

/* Dyld is the OSX Dynamic Linker
 * /usr/include//mach-o/loader.h
 *
 * Tested on OSX 10.10.5 (Yosemite)
 * and macOS 10.12.1 (Sierra)
 *
 * build with:
 * gcc -D KERNEL_VERSION=$(uname -r | cut -d. -f1)  call_printf.c
 *
 */

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
#if KERNEL_VERSION >= 16
        // New addition for macOS Sierra
        char       sierra_reserved[0x30];
#endif
} dyld_cache_header;

typedef struct {
        uint64_t       address;
        uint64_t       size;
        uint64_t       file_offset;
        uint32_t       max_prot;
        uint32_t       init_prot;
} shared_file_mapping_np;

static char *find_libc_and_cache_base(char *program_name, char **cache_rx_base)
{
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
        struct dyld_image_info *image_array = infos->infoArray;

        // Find libsystem_c.dylib among them
        struct dyld_image_info *image;
        char *libc = NULL;
        char *first_lib = NULL;
        for (int i = 0; i < image_count; ++i) {
                image = image_array + i;

                // Find libsystem_c.dylib's load address
                if (strstr(image->imageFilePath, "libsystem_c.dylib")) {
                        libc = (char*)image->imageLoadAddress;
                        //printf("libc base @ %p\n", libc);
                        // Thanks Sierra
                        *cache_rx_base = (char*)infos->sharedCacheBaseAddress;
                        return libc;
                }

                // Look for first dynamic library
                if (!strstr(image->imageFilePath, program_name)) {
                        char *load_addr = (char*)image->imageLoadAddress;
                        if (!first_lib || first_lib > load_addr) {
                                first_lib = load_addr;
                        }
                }
        }

        // find beginning of RX mapping
        // this method sucks
        while (memcmp(first_lib, "dyld_v1 x86_64h", 16)) {
                --first_lib;
        }

        *cache_rx_base = first_lib;

        return libc;
}

static char *find_printf(char *libc, char *shared_cache_rx_base)
{
        struct mach_header_64 *libc_header = (struct mach_header_64 *)libc;
        uint32_t ncmds = libc_header->ncmds;
        struct symtab_command *symcmd = NULL;
        struct segment_command_64 *cmd;
        cmd = (struct segment_command_64*)(libc + sizeof(struct mach_header_64));

        // Get symtab and dysymtab
        for (uint32_t i = 0; i < ncmds; ++i) {
                if (cmd->cmd == LC_SYMTAB) {
                        symcmd = (struct symtab_command*)cmd;
                        break;
                }
                cmd = (struct segment_command_64*)((char *)cmd + cmd->cmdsize);
        }

        dyld_cache_header *h = (dyld_cache_header*) shared_cache_rx_base;
        shared_file_mapping_np *mapping = (void*)(h + 1);

        /*
         *    DYLD SHARED CACHE MAPPINGS*
         *    ===========================
         *
         *    (*): Without ASLR slide
         *
         *   ## OSX Yosemite
         *
         *    ----------------------  0x7fff70000000
         *   |                      |
         *   |                      |
         *   |                      |
         *   |                      |
         *   |         RW-          |
         *   |                      |
         *   |                      |
         *   |                      |
         *   |----------------------| 0x7fff70000000 + [RW-].size
         *   |         Junk         |
         *   |----------------------| 0x7fff80000000
         *   |     Cache Header     |
         *   |----------------------|
         *   |                      |
         *   |         R-X          |
         *   |                      |
         *   |         ...          |
         *   |  libsystem_c.dylib   |
         *   |         ...          |
         *   |                      |
         *   |                      |
         *   |----------------------| 0x7fff80000000 + [R-X].size
         *   |                      |
         *   |                      |
         *   |                      |
         *   |         R--          |
         *   |                      |
         *   |                      |
         *   |                      |
         *   |                      |
         *    ----------------------  0x7fff80000000 + [R-X].size + [R--].size
         *
         *    cache.base = [R-X].address + [R-X].size - [R--].offset
         *
         *
         *
         *   ## macOS Sierra
         *
         *   Memory mapping follows file layout, so nothing strange to
         *   take into account. The only thing to take into account is
         *   presence of junk between each mapping, so offsets have to be
         *   handled cleverly.
         *
         */

        char *shared_cache_base = shared_cache_rx_base;

        size_t rx_size;
        size_t rw_size;
        uint64_t rx_addr;
        uint64_t ro_addr;
        off_t ro_off;

        for (int i = 0; i < h->mappingCount; ++i) {
                if (mapping[i].init_prot & VM_PROT_EXECUTE) {
                        // Get size and address of [R-X] mapping
                        rx_size = mapping[i].size;
                        rx_addr = mapping[i].address;
                } else if (mapping[i].init_prot & VM_PROT_WRITE) {
                        // Get size of [RW-] mapping
                        rw_size = mapping[i].size;
                } else if (mapping[i].init_prot == VM_PROT_READ) {
                        // Get file offset of [R--] mapping
                        ro_off = mapping[i].file_offset;
                        ro_addr = mapping[i].address;
                }
        }

        // Can be determined by dyld_all_image_info->sharedCacheSlide but meh.
        uint64_t aslr_slide = (uint64_t)shared_cache_rx_base - rx_addr;

        /*
         * Previously 'shared_cache_base + symcmd->XXXX', but since there is some
         * gap between each mapping, it would only work on Yosemite out of luck and
         * segfault in Sierra. Uglier, but it works on both versions.
         */
        char *shared_cache_ro = (char*)(ro_addr + aslr_slide);
        uint64_t stroff_from_ro = symcmd->stroff - rx_size - rw_size;
        uint64_t symoff_from_ro = symcmd->symoff - rx_size - rw_size;

        struct nlist_64 *symtab;
        char *strtab = shared_cache_ro + stroff_from_ro;
        symtab = (struct nlist_64 *)(shared_cache_ro + symoff_from_ro);

        for(uint32_t i = 0; i < symcmd->nsyms; ++i){
                uint32_t strtab_off = symtab[i].n_un.n_strx;
                uint64_t func       = symtab[i].n_value;

                if(strcmp(&strtab[strtab_off], "_printf") == 0) {
                        return (char*)(func + aslr_slide);
                }
        }

        return NULL;
}


int main (int argc, char *argv[])
{

        char *rx = NULL;
        syscall(294, &rx);
        printf("rx:%p\n", rx);

        char *shared_cache_rx_base = NULL;
        char *libc = find_libc_and_cache_base(argv[0], &shared_cache_rx_base);
        if (!libc) {
                //printf("libsystem_c not found - aborting\n");
                return 1;
        }
        if (!shared_cache_rx_base) {
                //printf("shared cache base not found - aborting\n");
                return 1;
        }
        printf("rx:%p\n", shared_cache_rx_base);

        char *printf_addr = find_printf(libc, shared_cache_rx_base);
        if (!printf_addr) {
                //printf("printf not found - aborting\n");
                return 1;
        }

        void (*print)(const char *fmt, ...) = (void *)printf_addr;
        print("Gotcha\n");

        return 0;
}
