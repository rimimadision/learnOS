[bits 32]
section .text
global switch_to

switch_to:
; save context of handler of cur_thread 
	push esi
	push edi
	push ebx
	push ebp

	mov eax, [esp + 20]
	mov [eax], esp

; recover context of handler of next_thread

	mov eax, [esp + 24]
	mov esp, [eax]

	pop ebp
	pop ebx
	pop edi
	pop esi
	ret
