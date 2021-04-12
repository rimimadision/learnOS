; @petrel 2021/3/31 16:36
; 	Create the loader file
; @petrel 2021/4/6 16:36
; 	Get into Protect Mode
; @petrel 2021/4/7 18:46
; 	Add detect_mem
; @petrel 2021/4/8 20:33
;	Open PG
; @petrel 2021/4/11 22:14
;  	Load kernel
;-------------------------------------------------------
; Kernel Loader
;-------------------------------------------------------

%include "boot.inc"

;-------------------------------------------------------
SECTION loader vstart=LOADER_BASE_ADDR

LOADER_STACK_TOP equ LOADER_BASE_ADDR
;	jmp loader_start

;-------------------------------------------------------
; Create GDT 
;-------------------------------------------------------
	GDT_BASE: dd 0x00000000
	 		  dd 0x00000000

	CODE_DESC: dd 0x0000FFFF
			   dd DESC_CODE_HIGH4

	DATA_STACK_DESC: dd 0x0000FFFF
					 dd DESC_DATA_HIGH4
	
	VIDEO_DESC: dd 0x80000007; limit=(0xbffff-0xb8000)/4k=0x7
				dd DESC_VIDEO_HIGH4 
	
	GDT_SIZE equ $ - GDT_BASE
	GDT_LIMIT equ GDT_SIZE - 1
	
	; leave space for 60 gdt
	times 60 dq 0

;-------------------------------------------------------
; SEGMENT SELECTOR
;-------------------------------------------------------
	SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0
	SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0
	SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

;-------------------------------------------------------
; Loader Data
;-------------------------------------------------------
	; address of total_mem_bytes -- 0xb00
	total_mem_bytes dd 0

	gdt_ptr dw GDT_LIMIT
			dd GDT_BASE
	ards_buf times 244 db 0
	ards_nr dw 0 ; count of ARDS
	
;	loadermsg db '2 loader in real.' 

;-------------------------------------------------------
; Loader Code	
;-------------------------------------------------------
loader_start:

;-------------------------------------------------------
; Detect memory 
; int 0x15h
; function code : 0xe820
;-------------------------------------------------------
; para: eax=0x0000e820 : function code
; 		ebx=0          : initialize ebx 0, ebx will save
;                        the position of next ards
; 		edx=0x534d150  : signature certificate, unchange
; 		es:di          : buff for ards
; 		ecx=20         : ards size
; ret:  cf			: call success for cf=0
;		eax=edx		 
;   	[es:di]		: ards struct
;		ecx			: count bytes of ards struct
;		ebx			: next ards

	xor ebx, ebx
	mov edx, 0x534d4150
	mov di, ards_buf

.e820_mem_get_loop:
	mov eax, 0x0000e820
	mov ecx, 20 	
	int 0x15
	jc .e820_failed_so_try_e801

	add di, cx
	inc word [ards_nr]
	
	; if ebx=0 and cf=0, ards all return 
	cmp ebx, 0
	jnz .e820_mem_get_loop

; go through to find max ards
	xor ecx, ecx
	mov cx, [ards_nr]
	mov ebx, ards_buf
	xor edx, edx

.find_max_mem_area: 
	mov eax, [ebx]
	add eax, [ebx + 8]
	add ebx, 20
	cmp edx, eax
	jge .next_ards
	mov edx, eax

.next_ards:
	loop .find_max_mem_area
	jmp .mem_get_ok
;-------------------------------------------------------
; Detect memory (lower than 4gb)
; int 0x15h
; function code : 0xe801
;-------------------------------------------------------
; para: ax=0xe801 : function code
; ret:  cf			: call success for cf=0
;		ax          		 
;   	bx           
;		cx=ax
;		dx=bx	
;		mem_size = (ax*1kb + 1 ) + bx*64kb		

.e820_failed_so_try_e801:
	mov ax, 0xe801
	int 0x15
	jc .e801_failed_so_try_88

; get mem_size lower than 15mb
	mov cx, 0x400
	mul cx
	and eax, 0x0000ffff
	shl edx, 16
	or edx, eax
	add edx, 0x100000; add 1mb for no mem_hole
	mov esi, edx; back-up mem_low_15mb

; get mem_size 16mb ~ 4gb
	xor eax, eax
	mov ax, bx
	mov ecx, 0x10000	 
	mul ecx
	mov edx, eax; for mem lower than 4gb
				; eax with 32bits is sufficient
	add edx, esi
	jmp .mem_get_ok
	
;-------------------------------------------------------
; Detect memory (lower than 64mb)
; int 0x15h
; function code : 0x88
;-------------------------------------------------------
; para: ah=0x88 : function code
; ret:  cf	: call success for cf=0
;		ax         		 
;		mem_size = ax*1kb + 1mb		

.e801_failed_so_try_88:	
	mov ah, 0x88
	int 0x15
;undefin for now
;	jc .error_hlt
	
; get mem_size
	mov cx, 0x400
	mul cx
	and eax, 0x0000ffff
	shl edx, 16
	or edx, eax
	add edx, 0x100000

	jmp .mem_get_ok
;-------------------------------------------------------
; Finish detecting memory 
;-------------------------------------------------------
; para:	edx : mem_size detected
; ret:  [total_mem_bytes] : record mem_size

.mem_get_ok:
	mov [total_mem_bytes], edx; edx contain count of mem_size with bytes 

;-------------------------------------------------------
; INT 0X10 function_no 0x13 
; Print String
;-------------------------------------------------------
; para: ah=0x13 : function code
;		bh      : page number
;		bl      : attribute
; 		cx      : length of string
;		(dh, dl): position 
;		es:bp 	: addr of string
; 		al=0x01 : display mode(1 -- string only contains chars, attribute in bl and the pos of cursor)
; return: no
	jmp print_msg
		
	loadermsg db "mem size confirm!"

print_msg:
	mov sp, LOADER_BASE_ADDR
	mov bp, loadermsg
	mov cx, 17
	mov ax, 0x1301
	mov bx, 0x001f
	mov dx, 0x1800
	int 0x10

;-------------------------------------------------------
; Enter Protect Mode
;-------------------------------------------------------
; Open A20
	in al, 0x92
	or al, 0000_0010b
	out 0x92, al

; Load gdt
	lgdt [gdt_ptr]

; set cr0 0bit
	mov eax, cr0
	or eax, 0x00000001
	mov cr0, eax

; Flush pipeline and serialize processor
	jmp dword SELECTOR_CODE:p_mode_start

;-------------------------------------------------------
; Have entered the p_mode
;-------------------------------------------------------
[bits 32]
p_mode_start: 
	mov ax, SELECTOR_DATA
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov esp, LOADER_STACK_TOP
	mov ax, SELECTOR_VIDEO
	mov gs, ax
	
;	mov byte [gs : 160], 'P'

;-------------------------------------------------------
; Load kernel
;-------------------------------------------------------
	mov eax, KERNEL_START_SECTOR
	mov ebx, KERNEL_BIN_BASE_ADDR

	mov ecx, 200

	call rd_disk_m_32
;-------------------------------------------------------
; Open PG
;-------------------------------------------------------
	call setup_page
		
	sgdt [gdt_ptr]
	
	; Reflect video segment to high 3gb	
	mov ebx, [gdt_ptr + 2]
	or dword [ebx + 24 + 4], 0xc0000000
	
	;Reflect gdt to high 3gb
	or dword [gdt_ptr + 2], 0xc0000000
	
	; Reflect stack to high 3gb
	or esp, 0xc0000000	

	; Set cr3 with addr of PDE
	mov eax, PAGE_DIR_TABLE_POS
	mov cr3, eax
	
	; Set cr0 PG
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

	lgdt [gdt_ptr]

	jmp SELECTOR_CODE:enter_kernel

enter_kernel:
	call kernel_init
	mov esp, 08047000
	jmp KERNEL_ENTRY_POINT

;-------------------------------------------------------
; Init kernel
;-------------------------------------------------------
kernel_init:
	xor eax, eax
	xor ebx, ebx; record ph_addr
	xor ecx, ecx; record num_ph
	xor edx, edx; record ph_size
	
	; Get ph_size
	mov dx, [KERNEL_BIN_BASE_ADDR + 42]
	
	; Get ph_addr
	mov ebx, [KERNEL_BIN_BASE_ADDR + 28]
	add ebx, KERNEL_BIN_BASE_ADDR

	; Get ph_num
	mov cx, [KERNEL_BIN_BASE_ADDR + 44]

	; Judge if segment null
.each_segment:
	cmp byte [ebx], PT_NULL
	je .PTNULL
	
	; Push pflie_size, src, dst
	push dword [ebx + 16]
	mov eax, [ebx + 4]
	add eax, KERNEL_BASE_ADDR
	push eax
	push dword [ebx + 8]
	call mem_cpy
	add esp, 12
.PTNULL:
	add ebx, edx

	loop .each_segment

	ret

;-------------------------------------------------------
; Function: mem_cpy
;-------------------------------------------------------
; para: [esp+8]  dst
;		[esp+12] src
;		[esp+16] byte_num
; return: no
;
mem_cpy:
	cld
	push ebp
	mov ebp, esp
	push ecx
	mov edi, [ebp + 8]
	mov esi, [ebp + 12]
	mov ecx, [ebp + 16]

	rep movsb
	
	pop ecx
	pop ebp
	ret

;-------------------------------------------------------
; Function: setup_page
; Prepare for opening PG
;-------------------------------------------------------
; para: no
; return: no
; note: function will set up PDE and PTE.
; 		eax, ecx, esi, edx.... will be changed
; 		so just take this function as a process 
setup_page:
	
	; Init 4kb PDE to 0
	mov ecx, 4096
	mov esi, 0
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS + esi], 0
	inc esi
	loop .clear_page_dir

	; Create PDE for first entry
	mov eax, PAGE_DIR_TABLE_POS + 0X1000
	or eax, PG_US_U | PG_RW_W | PG_P
	mov [PAGE_DIR_TABLE_POS], eax

	; Set the last entry point PDE itself
	sub eax, 0x1000
	mov [PAGE_DIR_TABLE_POS + 4092], eax
	
	; Set PTE of high_3gb_mem to kernel
	add eax, 0X1000
	mov ecx, 255; 1024/4 -1(last entry)
	mov esi, 768;
	mov ebx, PAGE_DIR_TABLE_POS
.create_kernel_pde:
	mov [ebx + esi * 4], eax
	inc esi
	add eax, 0x1000
	loop .create_kernel_pde

	; Create PTE
	mov ecx, 256; 256 * 4kb = 1mb
	mov esi, 0
	mov ebx, PAGE_DIR_TABLE_POS + 0X1000
	mov edx, PG_US_U | PG_RW_W | PG_P
.create_pte:
	mov [ebx + esi * 4], edx
	inc esi
	add edx, 0x1000
	loop .create_pte

	ret
;--------------------------------------------------------------------------------------
; function : read n sector from disk
rd_disk_m_32:
;--------------------------------------------------------------------------------------
; para : eax : LBA sector number
;		 ebx : addr to save the content
;        ecx : num of sector to read
; return : no
	
    mov esi, eax
    mov di, cx

    mov dx, 0x1f2
    mov al, cl
    out dx, al

    mov eax, esi

    mov dx, 0x1f3
    out dx, al

    mov cl, 8
    shr eax, cl
    mov dx, 0x1f4
    out dx, al

    shr eax, cl
    mov dx, 0x1f5
    out dx, al

    shr eax, cl
    and al, 0x0f
    or al, 0xe0
    mov dx, 0x1f6
    out dx, al

    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

.not_ready:
    nop
    in al, dx
    and al, 0x88
    cmp al, 0x08
    jnz .not_ready

    mov ax, di
    mov dx, 256
    mul dx
    mov cx, ax
    mov dx, 0x1f0

.go_on_read:
    in ax, dx
    mov [bx], ax
    add bx, 2
    loop .go_on_read
    ret
