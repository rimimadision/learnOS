; @petrel 2021/3/28 21:50
; @petrel 2021/3/30 19:25
;	Disable get_cursor, print_string
;	Add edit_video_memory to print string
; @petrel 2021/3/31 15:12
;	Add function rd_disk_m_16
; @petrel 2021/4/1 10:08
; 	Complete todolist 1
;----------------------------------------------------------------------------------
; Simple MBR
; TO-DO LIST
; 1. Change rd_disk_m_16 to avoid the alternation of esi and di (DONE) 
;-----------------------------------------------------------------------------------
; Include some constants

%include "boot.inc" 

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
; Print string
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

	mov byte [gs : 0x02], ' '
	mov byte [gs : 0x03], 0xA4

	mov byte [gs : 0x04], 'M'
	mov byte [gs : 0x05], 0xA4

	mov byte [gs : 0x06], 'B'
	mov byte [gs : 0x07], 0xA4

	mov byte [gs : 0x08], 'R'	
	mov byte [gs : 0x09], 0xA4

;-------------------------------------------------------------------------------------
; Load kernel loader into memory
;-------------------------------------------------------------------
	mov eax, LOADER_START_SECTOR ; LAB addr of kernel loader	
	mov bx, LOADER_BASE_ADDR ; base addr in memory of kernel loader
	mov cx, 4 ; num of sector to read

	call rd_disk_m_16

; Get into kernel loader
	jmp LOADER_BASE_ADDR + 0x300
 
;--------------------------------------------------------------------------------------
; function : read n sector from disk
rd_disk_m_16:
;--------------------------------------------------------------------------------------
; para : eax : LBA sector number
;		 bx : addr to save the content
;        cx : num of sector to read
; return : no

; Save esi, di and dx 
	push di
;	push si
	push dx

;	push cx
;	mov cl, 16
;	shr esi, cl
;	pop cx
	push esi

; Back up eax and cx
	mov esi, eax
	mov di, cx
	
; Set the num of sector to read
	mov dx, 0x1f2 ; 0x1f2 port to primary channel - sector count
	mov al, cl
	out dx, al

; Recover eax 
	mov eax, esi

; Set the LBA addr into port 0x1f3~0x1f6
	; 7~0 LBA addr into port 0x1f3
	mov dx, 0x1f3
	out dx, al

	; 15~8 LBA addr into port 0x1f4
	mov dx, 0x1f4
	mov cl, 8
	shr eax, cl
	out dx, al

	; 23~16 LBA addr into port 0x1f5
	mov dx, 0x1f5
	shr eax, cl
	out dx, al

	; 27~24 LBA addr into port 0x1f6
	mov dx, 0x1f6
	shr eax, cl
	and al, 0x0f
	or al, 0xe0 ; 1110 set lba mode and master disk
	out dx, al	

; Write read command 0x20 into port 0x1f7 
	mov dx, 0x1f7
	mov al, 0x20
	out dx, al

; Check disk status from port 0x1f7
.not_ready:
	nop 
	in al, dx	
	and al, 0x88 ; 3 bit = 1 means data transport ready
				 ; 7 bit = 1 means disk busy
	cmp al, 0x08
	jnz .not_ready

; Read data from port 0x1f0
	; calculate times need to read from disk
	; di * 512 / (1 word per times) 
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

; Recover esi, di and dx
	pop esi
;	mov cl, 16
;	shl	esi, cl
	pop dx
;	pop si
	pop di

	ret

;--------------------------------------------------------------------------------------
	times 510-($-$$) db 0
	db 0x55, 0xaa
