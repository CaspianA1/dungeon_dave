	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 11, 0	sdk_version 11, 0
	.intel_syntax noprefix
	.section	__TEXT,__literal8,8byte_literals
	.p2align	3               ## -- Begin function init_trig_table
LCPI0_0:
	.quad	4614256656552045848     ## double 3.1415926535897931
LCPI0_1:
	.quad	4618760256376274944     ## double 6.2831854820251465
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_init_trig_table
	.p2align	4, 0x90
_init_trig_table:                       ## @init_trig_table
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	sub	rsp, 80
	mov	rax, rdi
	mov	dword ptr [rbp - 4], esi
	mov	dword ptr [rbp - 8], edx
	mov	ecx, dword ptr [rbp - 4]
	test	ecx, ecx
	mov	qword ptr [rbp - 40], rdi ## 8-byte Spill
	mov	qword ptr [rbp - 48], rax ## 8-byte Spill
	mov	dword ptr [rbp - 52], ecx ## 4-byte Spill
	je	LBB0_1
	jmp	LBB0_9
LBB0_9:
	mov	eax, dword ptr [rbp - 52] ## 4-byte Reload
	sub	eax, 1
	je	LBB0_2
	jmp	LBB0_10
LBB0_10:
	mov	eax, dword ptr [rbp - 52] ## 4-byte Reload
	sub	eax, 2
	je	LBB0_3
	jmp	LBB0_4
LBB0_1:
	movsd	xmm0, qword ptr [rip + LCPI0_1] ## xmm0 = mem[0],zero
	mov	rax, qword ptr [rip + _sin@GOTPCREL]
	mov	qword ptr [rbp - 16], rax
	movsd	qword ptr [rbp - 24], xmm0
	jmp	LBB0_4
LBB0_2:
	movsd	xmm0, qword ptr [rip + LCPI0_1] ## xmm0 = mem[0],zero
	mov	rax, qword ptr [rip + _cos@GOTPCREL]
	mov	qword ptr [rbp - 16], rax
	movsd	qword ptr [rbp - 24], xmm0
	jmp	LBB0_4
LBB0_3:
	movsd	xmm0, qword ptr [rip + LCPI0_0] ## xmm0 = mem[0],zero
	mov	rax, qword ptr [rip + _tan@GOTPCREL]
	mov	qword ptr [rbp - 16], rax
	movsd	qword ptr [rbp - 24], xmm0
LBB0_4:
	mov	eax, dword ptr [rbp - 4]
	mov	rcx, qword ptr [rbp - 40] ## 8-byte Reload
	mov	dword ptr [rcx], eax
	mov	eax, dword ptr [rbp - 8]
	mov	dword ptr [rcx + 4], eax
	movsxd	rdi, dword ptr [rbp - 8]
	mov	esi, 8
	call	_calloc
	mov	rcx, qword ptr [rbp - 40] ## 8-byte Reload
	mov	qword ptr [rcx + 8], rax
	movsd	xmm0, qword ptr [rbp - 24] ## xmm0 = mem[0],zero
	cvtsi2sd	xmm1, dword ptr [rbp - 8]
	divsd	xmm0, xmm1
	movsd	qword ptr [rcx + 16], xmm0
	xorps	xmm0, xmm0
	movsd	qword ptr [rbp - 32], xmm0
LBB0_5:                                 ## =>This Inner Loop Header: Depth=1
	movsd	xmm0, qword ptr [rbp - 32] ## xmm0 = mem[0],zero
	movsd	xmm1, qword ptr [rbp - 24] ## xmm1 = mem[0],zero
	ucomisd	xmm1, xmm0
	jbe	LBB0_8
## %bb.6:                               ##   in Loop: Header=BB0_5 Depth=1
	mov	rax, qword ptr [rbp - 16]
	movsd	xmm0, qword ptr [rbp - 32] ## xmm0 = mem[0],zero
	call	rax
	mov	rax, qword ptr [rbp - 40] ## 8-byte Reload
	mov	rcx, qword ptr [rax + 8]
	movsd	xmm1, qword ptr [rbp - 32] ## xmm1 = mem[0],zero
	divsd	xmm1, qword ptr [rax + 16]
	movsd	qword ptr [rbp - 64], xmm0 ## 8-byte Spill
	movaps	xmm0, xmm1
	mov	qword ptr [rbp - 72], rcx ## 8-byte Spill
	call	_round
	cvttsd2si	edx, xmm0
	movsxd	rax, edx
	mov	rcx, qword ptr [rbp - 72] ## 8-byte Reload
	movsd	xmm0, qword ptr [rbp - 64] ## 8-byte Reload
                                        ## xmm0 = mem[0],zero
	movsd	qword ptr [rcx + 8*rax], xmm0
## %bb.7:                               ##   in Loop: Header=BB0_5 Depth=1
	mov	rax, qword ptr [rbp - 40] ## 8-byte Reload
	movsd	xmm0, qword ptr [rax + 16] ## xmm0 = mem[0],zero
	addsd	xmm0, qword ptr [rbp - 32]
	movsd	qword ptr [rbp - 32], xmm0
	jmp	LBB0_5
LBB0_8:
	mov	rax, qword ptr [rbp - 48] ## 8-byte Reload
	add	rsp, 80
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.globl	__lookup                ## -- Begin function _lookup
	.p2align	4, 0x90
__lookup:                               ## @_lookup
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	sub	rsp, 16
	lea	rax, [rbp + 16]
	movsd	qword ptr [rbp - 8], xmm0
	mov	rcx, qword ptr [rax + 8]
	movsd	xmm0, qword ptr [rbp - 8] ## xmm0 = mem[0],zero
	divsd	xmm0, qword ptr [rax + 16]
	mov	qword ptr [rbp - 16], rcx ## 8-byte Spill
	call	_round
	cvttsd2si	edx, xmm0
	movsxd	rax, edx
	mov	rcx, qword ptr [rbp - 16] ## 8-byte Reload
	movsd	xmm0, qword ptr [rcx + 8*rax] ## xmm0 = mem[0],zero
	add	rsp, 16
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.section	__TEXT,__literal8,8byte_literals
	.p2align	3               ## -- Begin function lookup_sin
LCPI2_0:
	.quad	4618760256376274944     ## double 6.2831854820251465
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_lookup_sin
	.p2align	4, 0x90
_lookup_sin:                            ## @lookup_sin
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	sub	rsp, 64
	movsd	qword ptr [rbp - 8], xmm0
	movsd	xmm0, qword ptr [rbp - 8] ## xmm0 = mem[0],zero
	movsd	qword ptr [rbp - 16], xmm0
	xorps	xmm0, xmm0
	ucomisd	xmm0, qword ptr [rbp - 8]
	jbe	LBB2_2
## %bb.1:
	movsd	xmm0, qword ptr [rbp - 8] ## xmm0 = mem[0],zero
	movq	rax, xmm0
	movabs	rcx, -9223372036854775808
	xor	rax, rcx
	movq	xmm0, rax
	movsd	qword ptr [rbp - 8], xmm0
LBB2_2:
	movsd	xmm0, qword ptr [rip + LCPI2_0] ## xmm0 = mem[0],zero
	movsd	xmm1, qword ptr [rbp - 8] ## xmm1 = mem[0],zero
	ucomisd	xmm1, xmm0
	jbe	LBB2_4
## %bb.3:
	movsd	xmm0, qword ptr [rip + LCPI2_0] ## xmm0 = mem[0],zero
	movsd	xmm1, qword ptr [rbp - 8] ## xmm1 = mem[0],zero
	movsd	xmm2, qword ptr [rip + LCPI2_0] ## xmm2 = mem[0],zero
	movsd	qword ptr [rbp - 32], xmm0 ## 8-byte Spill
	movaps	xmm0, xmm1
	movaps	xmm1, xmm2
	call	_fmod
	movsd	qword ptr [rbp - 8], xmm0
LBB2_4:
	movsd	xmm0, qword ptr [rbp - 8] ## xmm0 = mem[0],zero
	lea	rax, [rip + _sin_table]
	mov	rcx, qword ptr [rax]
	mov	qword ptr [rsp], rcx
	mov	rcx, qword ptr [rax + 8]
	mov	qword ptr [rsp + 8], rcx
	mov	rax, qword ptr [rax + 16]
	mov	qword ptr [rsp + 16], rax
	call	__lookup
	movsd	qword ptr [rbp - 24], xmm0
	xorps	xmm0, xmm0
	ucomisd	xmm0, qword ptr [rbp - 16]
	jbe	LBB2_6
## %bb.5:
	movsd	xmm0, qword ptr [rbp - 24] ## xmm0 = mem[0],zero
	movq	rax, xmm0
	movabs	rcx, -9223372036854775808
	xor	rax, rcx
	movq	xmm0, rax
	movsd	qword ptr [rbp - 40], xmm0 ## 8-byte Spill
	jmp	LBB2_7
LBB2_6:
	movsd	xmm0, qword ptr [rbp - 24] ## xmm0 = mem[0],zero
	movsd	qword ptr [rbp - 40], xmm0 ## 8-byte Spill
LBB2_7:
	movsd	xmm0, qword ptr [rbp - 40] ## 8-byte Reload
                                        ## xmm0 = mem[0],zero
	add	rsp, 64
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.section	__TEXT,__literal8,8byte_literals
	.p2align	3               ## -- Begin function lookup_cos
LCPI3_0:
	.quad	4609753057121533952     ## double 1.5707963705062866
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_lookup_cos
	.p2align	4, 0x90
_lookup_cos:                            ## @lookup_cos
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	sub	rsp, 16
	movsd	xmm1, qword ptr [rip + LCPI3_0] ## xmm1 = mem[0],zero
	movsd	qword ptr [rbp - 8], xmm0
	subsd	xmm1, qword ptr [rbp - 8]
	movaps	xmm0, xmm1
	call	_lookup_sin
	add	rsp, 16
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.globl	_lookup_tan             ## -- Begin function lookup_tan
	.p2align	4, 0x90
_lookup_tan:                            ## @lookup_tan
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	sub	rsp, 16
	movsd	qword ptr [rbp - 8], xmm0
	movsd	xmm0, qword ptr [rbp - 8] ## xmm0 = mem[0],zero
	call	_lookup_sin
	movsd	xmm1, qword ptr [rbp - 8] ## xmm1 = mem[0],zero
	movsd	qword ptr [rbp - 16], xmm0 ## 8-byte Spill
	movaps	xmm0, xmm1
	call	_lookup_cos
	movsd	xmm1, qword ptr [rbp - 16] ## 8-byte Reload
                                        ## xmm1 = mem[0],zero
	divsd	xmm1, xmm0
	movaps	xmm0, xmm1
	add	rsp, 16
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.globl	_millis                 ## -- Begin function millis
	.p2align	4, 0x90
_millis:                                ## @millis
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	sub	rsp, 32
	lea	rdi, [rbp - 16]
	mov	esi, 1
	call	_timespec_get
	imul	rcx, qword ptr [rbp - 16], 1000
	mov	rdx, qword ptr [rbp - 8]
	mov	dword ptr [rbp - 20], eax ## 4-byte Spill
	mov	rax, rdx
	cqo
	mov	edi, 1000000
	idiv	rdi
	add	rcx, rax
	mov	rax, rcx
	add	rsp, 32
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.section	__TEXT,__literal8,8byte_literals
	.p2align	3               ## -- Begin function benchmark
LCPI6_0:
	.quad	4666723172467343360     ## double 1.0E+4
LCPI6_1:
	.quad	4562254508917369340     ## double 0.001
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_benchmark
	.p2align	4, 0x90
_benchmark:                             ## @benchmark
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	sub	rsp, 32
	mov	qword ptr [rbp - 8], rdi
	call	_millis
	mov	qword ptr [rbp - 16], rax
	xorps	xmm0, xmm0
	movsd	qword ptr [rbp - 24], xmm0
LBB6_1:                                 ## =>This Inner Loop Header: Depth=1
	movsd	xmm0, qword ptr [rip + LCPI6_0] ## xmm0 = mem[0],zero
	ucomisd	xmm0, qword ptr [rbp - 24]
	jbe	LBB6_4
## %bb.2:                               ##   in Loop: Header=BB6_1 Depth=1
	mov	rax, qword ptr [rbp - 8]
	movsd	xmm0, qword ptr [rbp - 24] ## xmm0 = mem[0],zero
	call	rax
## %bb.3:                               ##   in Loop: Header=BB6_1 Depth=1
	movsd	xmm0, qword ptr [rip + LCPI6_1] ## xmm0 = mem[0],zero
	addsd	xmm0, qword ptr [rbp - 24]
	movsd	qword ptr [rbp - 24], xmm0
	jmp	LBB6_1
LBB6_4:
	call	_millis
	sub	rax, qword ptr [rbp - 16]
	add	rsp, 32
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.globl	_main                   ## -- Begin function main
	.p2align	4, 0x90
_main:                                  ## @main
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	sub	rsp, 48
	xor	esi, esi
	lea	rdi, [rbp - 24]
	mov	edx, 15000
	call	_init_trig_table
	mov	rax, qword ptr [rbp - 24]
	mov	qword ptr [rip + _sin_table], rax
	mov	rax, qword ptr [rbp - 16]
	mov	qword ptr [rip + _sin_table+8], rax
	mov	rax, qword ptr [rbp - 8]
	mov	qword ptr [rip + _sin_table+16], rax
	lea	rdi, [rip + _lookup_cos]
	call	_benchmark
	mov	rdi, qword ptr [rip + _cos@GOTPCREL]
	mov	qword ptr [rbp - 32], rax
	call	_benchmark
	mov	qword ptr [rbp - 40], rax
	mov	rsi, qword ptr [rbp - 32]
	mov	rdx, qword ptr [rbp - 40]
	lea	rdi, [rip + L_.str]
	mov	al, 0
	call	_printf
	mov	rcx, qword ptr [rip + _sin_table+8]
	mov	rdi, rcx
	mov	dword ptr [rbp - 44], eax ## 4-byte Spill
	call	_free
	xor	eax, eax
	add	rsp, 48
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.section	__TEXT,__const
	.globl	_two_pi                 ## @two_pi
	.p2align	2
_two_pi:
	.long	1086918619              ## float 6.28318548

	.globl	_half_pi                ## @half_pi
	.p2align	2
_half_pi:
	.long	1070141403              ## float 1.57079637

.zerofill __DATA,__bss,_sin_table,24,3  ## @sin_table
	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"Table time vs default: %lld ms, %lld ms\n"

.subsections_via_symbols
