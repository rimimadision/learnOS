#include "exec.h"
#include "stdint.h"
#include "memory.h"
#include "global.h"

extern void intr_exit(void);
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

struct Elf32_Ehdr {
	unsigned char e_ident[16];
	Elf32_Half e_type;
	Elf32_Half e_machine;
	Elf32_Word e_version;
	Elf32_Addr e_entry;
	Elf32_Off e_phoff;
	Elf32_Off e_shoff;
	Elf32_Word e_flags;
	Elf32_Half e_ehsize;
	Elf32_Half e_phentsize;
	Elf32_Half e_phnum;
	Elf32_Half e_shensize;
	Elf32_Half e_shnum;
	Elf32_Half e_shstrndx;	 
};

struct Elf32_Phdr {
	Elf32_Word p_type;
	Elf32_Off  p_offset;
	Elf32_Addr p_vaddr;
	Elf32_Addr p_paddr;
	Elf32_Word p_filesz;
	Elf32_Word p_memsz;
	Elf32_Word p_flags;
	Elf32_Word p_align;
};

enum segment_type {
	PT_NULL,
	PT_LOAD,
	PT_DYNAMIC,
	PT_INTERP,
	PT_NOTE,
	PT_SHLIB,
	PT_PHDR
};

static bool segment_load(int32_t fd, uint32_t offset, uint32_t filesz, uint32_t vaddr);

static bool segment_load(int32_t fd, uint32_t offset, uint32_t filesz, uint32_t vaddr) {
	uint32_t vaddr_first_page = vaddr & 0xfffff000;
	uint32_t size_in_first_page = PG_SIZE - (vaddr & 0x00000fff);
	uint32_t occupy_pages = 0;
	if (filesz > size_in_first_page) {
		uint32_t left_size = filesz - size_in_first_page;
		occupy_pages = DIV_ROUND_UP(left_size, PG_SIZE) + 1;
	} else {
		occupy_pages = 1;
	}

	uint32_t page_idx = 0;
	uint32_t vaddr_page = vaddr_first_page;
	while (page_idx < occupy_pages) {
		uint32_t* pde = pde_ptr(vaddr_page);
		uint32_t* pte = pte_ptr(vaddr_page);

		if (!(*pde & 0x00000001) || !(*pte & 0x00000001)) {
			/* need a new page */
			if (get_a_page(PF_USER, vaddr_page) == NULL) {
				printk("get a page in 0x%x failed\n", vaddr_page);
				return false;
			}
		}	
		
		vaddr_page += PG_SIZE;
		page_idx++;
	}

	sys_lseek(fd, offset, SEEK_SET);
	/* Debug */
	printk("\nvaddr_start:0x%x\n", vaddr);
	
	sys_read(fd, (void*)vaddr, filesz);
	return true;
}

static int32_t load(const char* pathname) {
	
} 
