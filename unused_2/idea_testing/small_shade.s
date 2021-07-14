	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 11, 0	sdk_version 11, 3
	.intel_syntax noprefix
	.section	__TEXT,__literal8,8byte_literals
	.p2align	3                               ## -- Begin function orig_shade
LCPI0_0:
	.quad	0x3fe999999999999a              ## double 0.80000000000000004
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_orig_shade
	.p2align	4, 0x90
_orig_shade:                            ## @orig_shade
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	mov	dword ptr [rbp - 4], edi
	movzx	eax, byte ptr [rbp - 2]
	cvtsi2sd	xmm0, eax
	movsd	xmm1, qword ptr [rip + LCPI0_0] ## xmm1 = mem[0],zero
	mulsd	xmm0, xmm1
	cvttsd2si	eax, xmm0
                                        ## kill: def $al killed $al killed $eax
	mov	byte ptr [rbp - 5], al
	movzx	ecx, byte ptr [rbp - 3]
	cvtsi2sd	xmm0, ecx
	mulsd	xmm0, xmm1
	cvttsd2si	ecx, xmm0
                                        ## kill: def $cl killed $cl killed $ecx
	mov	byte ptr [rbp - 6], cl
	movzx	edx, byte ptr [rbp - 4]
	cvtsi2sd	xmm0, edx
	mulsd	xmm0, xmm1
	cvttsd2si	edx, xmm0
                                        ## kill: def $dl killed $dl killed $edx
	mov	byte ptr [rbp - 7], dl
	movzx	esi, byte ptr [rbp - 5]
	shl	esi, 16
	or	esi, 4278190080
	movzx	edi, byte ptr [rbp - 6]
	shl	edi, 8
	or	esi, edi
	movzx	edi, byte ptr [rbp - 7]
	or	esi, edi
	mov	eax, esi
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
	xor	eax, eax
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.section	__TEXT,__const
	.globl	_shade                          ## @shade
	.p2align	3
_shade:
	.quad	0x3fe999999999999a              ## double 0.80000000000000004

.subsections_via_symbols
