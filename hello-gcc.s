	.file	"hello.c"
	.intel_syntax noprefix
	.text
	.type	read_int, @function
read_int:
.LFB0:
	.cfi_startproc
	endbr64
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	mov	QWORD PTR -24[rbp], rdi
	mov	QWORD PTR -8[rbp], 0
.L5:
	mov	rax, QWORD PTR -24[rbp]
	movzx	eax, BYTE PTR [rax]
	sub	eax, 48
	mov	BYTE PTR -9[rbp], al
	cmp	BYTE PTR -9[rbp], 9
	ja	.L7
	mov	rdx, QWORD PTR -8[rbp]
	mov	rax, rdx
	sal	rax, 2
	add	rax, rdx
	add	rax, rax
	mov	QWORD PTR -8[rbp], rax
	movzx	eax, BYTE PTR -9[rbp]
	add	QWORD PTR -8[rbp], rax
	jmp	.L5
.L7:
	nop
	mov	rax, QWORD PTR -8[rbp]
	pop	rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	read_int, .-read_int
	.globl	main
	.type	main, @function
main:
.LFB1:
	.cfi_startproc
	endbr64
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp
	.cfi_def_cfa_register 6
	sub	rsp, 32
	mov	DWORD PTR -20[rbp], edi
	mov	QWORD PTR -32[rbp], rsi
	mov	DWORD PTR -8[rbp], 0
	mov	DWORD PTR -4[rbp], 0
	jmp	.L9
.L10:
	mov	eax, DWORD PTR -4[rbp]
	cdqe
	lea	rdx, 0[0+rax*8]
	mov	rax, QWORD PTR -32[rbp]
	add	rax, rdx
	mov	rax, QWORD PTR [rax]
	mov	rdi, rax
	call	read_int
	mov	edx, eax
	mov	eax, DWORD PTR -8[rbp]
	add	eax, edx
	mov	DWORD PTR -8[rbp], eax
	add	DWORD PTR -4[rbp], 1
.L9:
	mov	eax, DWORD PTR -4[rbp]
	cmp	eax, DWORD PTR -20[rbp]
	jl	.L10
	mov	eax, DWORD PTR -8[rbp]
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 9.4.0-1ubuntu1~20.04.1) 9.4.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	 1f - 0f
	.long	 4f - 1f
	.long	 5
0:
	.string	 "GNU"
1:
	.align 8
	.long	 0xc0000002
	.long	 3f - 2f
2:
	.long	 0x3
3:
	.align 8
4:
