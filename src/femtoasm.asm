
format ELF64 executable
entry start
segment readable writable executable

start:
	mov r13d, read_next_character
	mov r14d, read_write_buffer
	mov r15d, defines_map
	; fallthrough

parse_statement:
	;     .-- read_next_character
	call r13
	cmp al, '='
	jne @f

parse_and_store_define:
.parse:
	call parse_identifier
	;         .-- hash
	mov r12, rbx
	call parse_value

.store:
    ;                    .-- hash
	lea rax, [r15 + 8 * r12]
	;           .----------- number of nibbles
	mov [rax], ebp; .------- value
	mov [rax + 4], ebx
	; The 3 LSB of rax will be zero, so the following jmp is unnecessary
	; ~jmp parse_statement~

@@:
	cmp al, '$'
	jne @f

parse_and_load_and_write_constant:
.parse:
	call parse_identifier

.load:  ;                .-- hash
	lea rax, [r15 + 8 * rbx]
	mov ebp, [rax]
	mov ebx, [rax + 4]
	
.write:
	;     .-------- read_write_buffer
	;     |     .-- data
	mov [r14], rbx
	; SYS_write = 1
	xor eax, eax
	inc eax
	;         .---- number of nibbles
	mov edx, ebp
	; divide by 2 to convert nibbles to bytes
	shr edx, 1
	call read_or_write
	; this jmp didn't exist previously, but since read_or_write returns the character
	; written, we need this or the next test would fail if we write a 0x24
	jmp parse_statement

@@:
	cmp al, '%'
	; if it's none of these, then it's a comment
	jne parse_statement
	; fallthrough

parse_and_write_literal:
.parse:
	call parse_value
.write:
	jmp parse_and_load_and_write_constant.write

read_next_character:
	; SYS_read = 0
	xor eax, eax
	; read 1 byte
	xor edx, edx
	inc edx
	; fallthrough

read_or_write:
	; file descriptor coincides with the syscall number
	; SYS_read = STDIN_FILENO, SYS_write = STDOUT_FILENO
	mov edi, eax
	;          .-- read_write_buffer
	lea rsi, [r14]
	syscall
	; if read, quit on EOF; if write, quit if undefined
	or al, al
	jz exit
	;         .--- read_write_buffer
	mov al, [r14]
	ret

parse_value:
	xor ebp, ebp
.next:
	;     .-- read_next_character
	call r13
	cmp al, ' '
	jbe .end
	call convert_nibble
	;    .--- nibble counter
	inc ebp
	;    .--- accumulator
	shl rbx, 4
	or bl, al
	jmp .next
.end:
	ret

parse_identifier:
	;    .----.-- hash
	xor ebx, ebx
.next:
	;     .------ read_next_character
	call r13
	cmp al, ' '
	jbe .end
	xchg bh, bl
	xor bl, al
	jmp .next
.end:
	ret

convert_nibble:
	add al, -'0'
	cmp al, 10
	jb .done
	add al, 0xA + '0' - 'A'
.done:
	ret

exit:
	xor eax, eax
	; SYS_exit_group
	mov al, 231
	syscall

; needed for some optimizations done above
align 16

read_write_buffer dq ?

;                    .-------- bytes per entry (we only actually use 5 bytes)
;                    |      .- number of entries (16-bit hash)
defines_map       rb 8 * 0x10000
