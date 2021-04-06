; @petrel 2021/3/31 16:36
; 	Create the loader file
; @petrel 2021/4/6 16:36
; 	Get into Protect Mode
;-------------------------------------------------------
; Kernel Loader
;-------------------------------------------------------

%include "boot.inc"

;-------------------------------------------------------
SECTION loader vstart=LOADER_BASE_ADDR
;-------------------------------------------------------
LOADER_STACK_TOP equ LOADER_BASE_ADDR
	jmp loader_start
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
;	times 60 dq 0
;-------------------------------------------------------
; SEGMENT SELECTOR
;-------------------------------------------------------
	SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0
	SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0
	SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0
;-------------------------------------------------------
; Loader Data
;-------------------------------------------------------
	gdt_ptr dw GDT_LIMIT
			dd GDT_BASE
	loadermsg db '2 loader in real.' 
;-------------------------------------------------------
; Loader Code	
;-------------------------------------------------------
loader_start:
	
;-------------------------------------------------------
; INT 0X10 function_no 0x13 
; Print String
;-------------------------------------------------------
; para: ah=0x13 : function number
;		bh      : page number
;		bl      : attribute
; 		cx      : length of string
;		(dh, dl): position 
;		es:bp 	: addr of string
; 		al=0x01 : display mode(1 -- string only contains chars, attribute in bl and the pos of cursor)
; return: no

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
	in al, 0x92; ?????????????
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
	
	mov byte [gs : 160], 'P'

	jmp $
