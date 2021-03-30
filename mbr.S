; @petrel 2021/3/28 21:50
; @petrel 2021/3/30 19:25
; Simple MBR
;-----------------------------------------------------------------------------------
SECTION MBR vstart=0x7c00
	mov ax, cs 
	mov ds, ax
    mov es, ax
	mov ss, ax
	mov fs, ax
  	mov sp, 0x7c00

;Set the base address of video memory : 0xb800
	mov ax, 0xb800
	mov gs, ax
;------------------------------------------------------------------------------------
; Clear the screen
; Int 0x10 
;-----------------------------------------------------
; para:	ah : fun_id = 0x600 
;		al : num_rows_wrapped_up = 0(mean whole rows)
;		(cl, ch) : begin position
;		(dl, dh) : end position 
; return: no 

	mov ax, 0x600
	mov bx, 0x700
	mov cx, 0
	mov dx, 0x184f

	int 0x10
;------------------------------------------------------------------------------------
; Get the cursor
; Int 0x10
; NOT USED 
;---------------
;	mov ah, 3	
;	mov bh, 0
;	
;	int 0x10
;------------------------------------------------------------------------------------
; Print the string
; Int 0x10 
; NOT USED
;------------------
;	mov ax, message
;	mov bp, ax
;
;	mov cx, 5
;	mov ax, 0x1301
;	mov bx, 0x2
;
;	int 0x10
;------------------------------------------------------------------------------------
; Edit the video memory to print string : 1 MBR 
; --------------------
	mov byte [gs : 0x00], '1'
	mov byte [gs : 0x01], 0xA4

	mov byte [gs : 0x02], 'T'
	mov byte [gs : 0x03], 0xA4

	mov byte [gs : 0x04], 'e'
	mov byte [gs : 0x05], 0xA4

	mov byte [gs : 0x06], 's'
	mov byte [gs : 0x07], 0xA4

	mov byte [gs : 0x08], 'T'	
	mov byte [gs : 0x09], 0xA4
;-------------------------------------------------------------------------------------
	jmp $
	
;	message db "1 MBR"
	times 510-($-$$) db 0
	db 0x55, 0xaa
