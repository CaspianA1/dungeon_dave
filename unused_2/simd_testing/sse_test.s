	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 11, 0	sdk_version 11, 3
	.intel_syntax noprefix
	.section	__TEXT,__literal16,16byte_literals
	.p2align	4                               ## -- Begin function vec_fn
LCPI0_0:
	.quad	0x400b333333333333              ## double 3.3999999999999999
	.quad	0x4198d5d42aaaaaab              ## double 104166666.66666667
LCPI0_1:
	.quad	0x4016000000000000              ## double 5.5
	.quad	0x4023333333333333              ## double 9.5999999999999996
LCPI0_2:
	.quad	0x400999999999999a              ## double 3.2000000000000002
	.quad	0x400999999999999a              ## double 3.2000000000000002
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_vec_fn
	.p2align	4, 0x90
_vec_fn:                                ## @vec_fn
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	sub	rsp, 112
	xorps	xmm0, xmm0
	movapd	xmmword ptr [rbp - 16], xmm0
	mov	qword ptr [rbp - 24], 0
LBB0_1:                                 ## =>This Inner Loop Header: Depth=1
	cmp	qword ptr [rbp - 24], 1000000000
	jae	LBB0_4
## %bb.2:                               ##   in Loop: Header=BB0_1 Depth=1
	movapd	xmm0, xmmword ptr [rip + LCPI0_0] ## xmm0 = [3.3999999999999999E+0,1.0416666666666667E+8]
	movapd	xmmword ptr [rbp - 48], xmm0
	movapd	xmm1, xmmword ptr [rip + LCPI0_1] ## xmm1 = [5.5E+0,9.5999999999999996E+0]
	movapd	xmmword ptr [rbp - 64], xmm1
	movapd	xmm1, xmmword ptr [rbp - 64]
	movaps	xmm2, xmm0
	divpd	xmm2, xmm1
	movapd	xmm1, xmmword ptr [rip + LCPI0_2] ## xmm1 = [3.2000000000000002E+0,3.2000000000000002E+0]
	movapd	xmmword ptr [rbp - 96], xmm1
	movapd	xmm1, xmmword ptr [rbp - 96]
	addpd	xmm2, xmm1
	movapd	xmmword ptr [rbp - 80], xmm2
	movapd	xmm1, xmmword ptr [rbp - 64]
	mulpd	xmm1, xmm0
	mulpd	xmm1, xmmword ptr [rbp - 80]
	movapd	xmmword ptr [rbp - 112], xmm1
	movapd	xmm0, xmmword ptr [rbp - 112]
	addpd	xmm0, xmmword ptr [rbp - 16]
	movapd	xmmword ptr [rbp - 16], xmm0
## %bb.3:                               ##   in Loop: Header=BB0_1 Depth=1
	mov	rax, qword ptr [rbp - 24]
	add	rax, 1
	mov	qword ptr [rbp - 24], rax
	jmp	LBB0_1
LBB0_4:
	movsd	xmm0, qword ptr [rbp - 16]      ## xmm0 = mem[0],zero
	movsd	xmm1, qword ptr [rbp - 8]       ## xmm1 = mem[0],zero
	lea	rdi, [rip + L_.str]
	mov	al, 2
	call	_printf
	add	rsp, 112
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.section	__TEXT,__literal8,8byte_literals
	.p2align	3                               ## -- Begin function slow_vec_fn
LCPI1_0:
	.quad	0x400999999999999a              ## double 3.2000000000000002
LCPI1_1:
	.quad	0x41cdcd6500000000              ## double 1.0E+9
LCPI1_2:
	.quad	0x4016000000000000              ## double 5.5
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_slow_vec_fn
	.p2align	4, 0x90
_slow_vec_fn:                           ## @slow_vec_fn
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	sub	rsp, 208
	xor	esi, esi
	lea	rax, [rbp - 16]
	mov	rdi, rax
	mov	edx, 16
	call	_memset
	mov	qword ptr [rbp - 24], 0
LBB1_1:                                 ## =>This Inner Loop Header: Depth=1
	cmp	qword ptr [rbp - 24], 1000000000
	jae	LBB1_4
## %bb.2:                               ##   in Loop: Header=BB1_1 Depth=1
	movsd	xmm0, qword ptr [rip + LCPI1_0] ## xmm0 = mem[0],zero
	movsd	xmm1, qword ptr [rip + LCPI1_1] ## xmm1 = mem[0],zero
	movsd	xmm2, qword ptr [rip + LCPI1_2] ## xmm2 = mem[0],zero
	mov	rax, qword ptr [rip + L___const.slow_vec_fn.a]
	mov	qword ptr [rbp - 40], rax
	mov	rax, qword ptr [rip + L___const.slow_vec_fn.a+8]
	mov	qword ptr [rbp - 32], rax
	movsd	qword ptr [rbp - 56], xmm2
	divsd	xmm1, qword ptr [rbp - 32]
	movsd	qword ptr [rbp - 48], xmm1
	movsd	xmm1, qword ptr [rbp - 40]      ## xmm1 = mem[0],zero
	divsd	xmm1, qword ptr [rbp - 56]
	movsd	qword ptr [rbp - 88], xmm1
	movsd	xmm1, qword ptr [rbp - 32]      ## xmm1 = mem[0],zero
	divsd	xmm1, qword ptr [rbp - 48]
	movsd	qword ptr [rbp - 80], xmm1
	movsd	xmm1, qword ptr [rbp - 88]      ## xmm1 = mem[0],zero
	movsd	qword ptr [rbp - 104], xmm0
	movsd	qword ptr [rbp - 96], xmm0
	addsd	xmm1, qword ptr [rbp - 104]
	movsd	qword ptr [rbp - 72], xmm1
	movsd	xmm1, qword ptr [rbp - 40]      ## xmm1 = mem[0],zero
	divsd	xmm1, qword ptr [rbp - 56]
	movsd	qword ptr [rbp - 120], xmm1
	movsd	xmm1, qword ptr [rbp - 32]      ## xmm1 = mem[0],zero
	divsd	xmm1, qword ptr [rbp - 48]
	movsd	qword ptr [rbp - 112], xmm1
	movsd	xmm1, qword ptr [rbp - 112]     ## xmm1 = mem[0],zero
	movsd	qword ptr [rbp - 136], xmm0
	movsd	qword ptr [rbp - 128], xmm0
	addsd	xmm1, qword ptr [rbp - 128]
	movsd	qword ptr [rbp - 64], xmm1
	movsd	xmm0, qword ptr [rbp - 40]      ## xmm0 = mem[0],zero
	movsd	xmm1, qword ptr [rbp - 56]      ## xmm1 = mem[0],zero
	mulsd	xmm1, qword ptr [rbp - 72]
	movsd	qword ptr [rbp - 168], xmm1
	movsd	xmm1, qword ptr [rbp - 48]      ## xmm1 = mem[0],zero
	mulsd	xmm1, qword ptr [rbp - 64]
	movsd	qword ptr [rbp - 160], xmm1
	mulsd	xmm0, qword ptr [rbp - 168]
	movsd	qword ptr [rbp - 152], xmm0
	movsd	xmm0, qword ptr [rbp - 32]      ## xmm0 = mem[0],zero
	movsd	xmm1, qword ptr [rbp - 56]      ## xmm1 = mem[0],zero
	mulsd	xmm1, qword ptr [rbp - 72]
	movsd	qword ptr [rbp - 184], xmm1
	movsd	xmm1, qword ptr [rbp - 48]      ## xmm1 = mem[0],zero
	mulsd	xmm1, qword ptr [rbp - 64]
	movsd	qword ptr [rbp - 176], xmm1
	mulsd	xmm0, qword ptr [rbp - 176]
	movsd	qword ptr [rbp - 144], xmm0
	movsd	xmm0, qword ptr [rbp - 16]      ## xmm0 = mem[0],zero
	addsd	xmm0, qword ptr [rbp - 152]
	movsd	qword ptr [rbp - 200], xmm0
	movsd	xmm0, qword ptr [rbp - 8]       ## xmm0 = mem[0],zero
	addsd	xmm0, qword ptr [rbp - 144]
	movsd	qword ptr [rbp - 192], xmm0
	mov	rax, qword ptr [rbp - 200]
	mov	qword ptr [rbp - 16], rax
	mov	rax, qword ptr [rbp - 192]
	mov	qword ptr [rbp - 8], rax
## %bb.3:                               ##   in Loop: Header=BB1_1 Depth=1
	mov	rax, qword ptr [rbp - 24]
	add	rax, 1
	mov	qword ptr [rbp - 24], rax
	jmp	LBB1_1
LBB1_4:
	movsd	xmm0, qword ptr [rbp - 16]      ## xmm0 = mem[0],zero
	movsd	xmm1, qword ptr [rbp - 8]       ## xmm1 = mem[0],zero
	lea	rdi, [rip + L_.str.1]
	mov	al, 2
	call	_printf
	add	rsp, 208
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.section	__TEXT,__literal8,8byte_literals
	.p2align	3                               ## -- Begin function benchmark_fn
LCPI2_0:
	.quad	0x412e848000000000              ## double 1.0E+6
	.section	__TEXT,__literal16,16byte_literals
	.p2align	4
LCPI2_1:
	.long	1127219200                      ## 0x43300000
	.long	1160773632                      ## 0x45300000
	.long	0                               ## 0x0
	.long	0                               ## 0x0
LCPI2_2:
	.quad	0x4330000000000000              ## double 4503599627370496
	.quad	0x4530000000000000              ## double 1.9342813113834067E+25
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_benchmark_fn
	.p2align	4, 0x90
_benchmark_fn:                          ## @benchmark_fn
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	sub	rsp, 48
	movsd	xmm0, qword ptr [rip + LCPI2_0] ## xmm0 = mem[0],zero
	mov	qword ptr [rbp - 8], rdi
	mov	qword ptr [rbp - 16], rsi
	movsd	qword ptr [rbp - 40], xmm0      ## 8-byte Spill
	call	_clock
	mov	qword ptr [rbp - 24], rax
	mov	rax, qword ptr [rbp - 8]
	xor	ecx, ecx
                                        ## kill: def $cl killed $cl killed $ecx
	mov	qword ptr [rbp - 48], rax       ## 8-byte Spill
	mov	al, cl
	mov	rdx, qword ptr [rbp - 48]       ## 8-byte Reload
	call	rdx
	call	_clock
	mov	rdx, qword ptr [rbp - 24]
	sub	rax, rdx
	movq	xmm0, rax
	movaps	xmm1, xmmword ptr [rip + LCPI2_1] ## xmm1 = [1127219200,1160773632,0,0]
	punpckldq	xmm0, xmm1              ## xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
	movapd	xmm1, xmmword ptr [rip + LCPI2_2] ## xmm1 = [4.503599627370496E+15,1.9342813113834067E+25]
	subpd	xmm0, xmm1
	movaps	xmm1, xmm0
	unpckhpd	xmm0, xmm0                      ## xmm0 = xmm0[1,1]
	addsd	xmm0, xmm1
	movsd	xmm1, qword ptr [rbp - 40]      ## 8-byte Reload
                                        ## xmm1 = mem[0],zero
	divsd	xmm0, xmm1
	movsd	qword ptr [rbp - 32], xmm0
	movsd	xmm0, qword ptr [rbp - 32]      ## xmm0 = mem[0],zero
	mov	rsi, qword ptr [rbp - 16]
	lea	rdi, [rip + L_.str.2]
	mov	al, 1
	call	_printf
	add	rsp, 48
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.globl	_main                           ## -- Begin function main
	.p2align	4, 0x90
_main:                                  ## @main
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	lea	rax, [rip + _vec_fn]
	mov	rdi, rax
	lea	rsi, [rip + L_.str.3]
	call	_benchmark_fn
	lea	rax, [rip + _slow_vec_fn]
	mov	rdi, rax
	lea	rsi, [rip + L_.str.4]
	call	_benchmark_fn
	xor	eax, eax
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.section	__TEXT,__const
	.globl	_iters                          ## @iters
	.p2align	3
_iters:
	.quad	1000000000                      ## 0x3b9aca00

	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"result_1 = {%lf, %lf}\n"

	.section	__TEXT,__literal16,16byte_literals
	.p2align	3                               ## @__const.slow_vec_fn.a
L___const.slow_vec_fn.a:
	.quad	0x400b333333333333              ## double 3.3999999999999999
	.quad	0x4198d5d42aaaaaab              ## double 104166666.66666667

	.section	__TEXT,__cstring,cstring_literals
L_.str.1:                               ## @.str.1
	.asciz	"result_2 = {%lf, %lf}\n"

L_.str.2:                               ## @.str.2
	.asciz	"Took %lf seconds to complete fn %s\n"

L_.str.3:                               ## @.str.3
	.asciz	"simd"

L_.str.4:                               ## @.str.4
	.asciz	"fake_simd"

.subsections_via_symbols
