[bits 32]
%define ERROR_CODE nop ; CPU will push error_code automatically
%define ZERO push 0 ; if no error_code, push 0 for uniform

extern put_str ; declare external function
extern idt_table ; interrupt handler written in C 

section .data

global intr_entry_table 
intr_entry_table:

%macro VECTOR 2
section .text 

; No.%1 interrupt
intr%1entry:	
	
	%2
	; save register state
	push ds
	push es
	push fs
	push gs
	pushad

	; send EOI message
	mov al, 0x20 ; normal EOI (R,SL = 0 EOI = 1)
	; if interrupt comes from slave PIC, then need to 
	; send EOI to both master and slave PIC
	out 0xa0, al
	out 0x20, al 

	; call handler in idt_table
	push %1 ; push vec_nr
	call [idt_table + %1 * 4]
	jmp intr_exit

section .data
	dd intr%1entry
%endmacro
section .text
global intr_exit
intr_exit:
	add esp, 4 ; skip vec_nr
	popad
	pop gs	
	pop fs
	pop es
	pop ds
	add esp, 4 ; skip error_code
	iretd
	
VECTOR 0X00, ZERO
VECTOR 0X01, ZERO
VECTOR 0X02, ZERO
VECTOR 0X03, ZERO
VECTOR 0X04, ZERO
VECTOR 0X05, ZERO
VECTOR 0X06, ZERO
VECTOR 0X07, ZERO
VECTOR 0X08, ZERO
VECTOR 0X09, ZERO
VECTOR 0X0a, ZERO
VECTOR 0X0b, ZERO
VECTOR 0X0c, ZERO
VECTOR 0X0d, ZERO
VECTOR 0X0e, ZERO
VECTOR 0X0f, ZERO
VECTOR 0X10, ZERO
VECTOR 0X11, ZERO
VECTOR 0X12, ZERO
VECTOR 0X13, ZERO
VECTOR 0X14, ZERO
VECTOR 0X15, ZERO
VECTOR 0X16, ZERO
VECTOR 0X17, ZERO
VECTOR 0X18, ZERO
VECTOR 0X19, ZERO
VECTOR 0X1a, ZERO
VECTOR 0X1b, ZERO
VECTOR 0X1c, ZERO
VECTOR 0X1d, ZERO
VECTOR 0X1e, ERROR_CODE
VECTOR 0X1f, ZERO
VECTOR 0X20, ZERO
