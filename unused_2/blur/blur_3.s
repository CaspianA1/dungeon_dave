	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 11, 0	sdk_version 11, 3
	.intel_syntax noprefix
	.globl	_get_pixel                      ## -- Begin function get_pixel
	.p2align	4, 0x90
_get_pixel:                             ## @get_pixel
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	mov	rax, qword ptr [rdi + 8]
	movsxd	rcx, dword ptr [rdi + 24]
	movsxd	rdx, edx
	imul	rdx, rcx
	add	rdx, qword ptr [rdi + 32]
	movzx	eax, byte ptr [rax + 17]
	movsxd	rcx, esi
	imul	rcx, rax
	mov	eax, dword ptr [rcx + rdx]
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.globl	_put_pixel                      ## -- Begin function put_pixel
	.p2align	4, 0x90
_put_pixel:                             ## @put_pixel
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	mov	r8, qword ptr [rdi + 8]
	movsxd	rax, dword ptr [rdi + 24]
	movsxd	rdx, edx
	imul	rdx, rax
	add	rdx, qword ptr [rdi + 32]
	movzx	eax, byte ptr [r8 + 17]
	movsxd	rsi, esi
	imul	rsi, rax
	mov	dword ptr [rsi + rdx], ecx
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.globl	_get_bits                       ## -- Begin function get_bits
	.p2align	4, 0x90
_get_bits:                              ## @get_bits
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	mov	ecx, esi
                                        ## kill: def $cl killed $cl killed $ecx
	shr	edi, cl
	mov	eax, -1
	mov	ecx, edx
	shl	eax, cl
	not	eax
	and	eax, edi
	movzx	eax, al
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.globl	_blurrer                        ## -- Begin function blurrer
	.p2align	4, 0x90
_blurrer:                               ## @blurrer
	.cfi_startproc
## %bb.0:
	push	rbp
	.cfi_def_cfa_offset 16
	.cfi_offset rbp, -16
	mov	rbp, rsp
	.cfi_def_cfa_register rbp
	push	r15
	push	r14
	push	r13
	push	r12
	push	rbx
	sub	rsp, 88
	.cfi_offset rbx, -56
	.cfi_offset r12, -48
	.cfi_offset r13, -40
	.cfi_offset r14, -32
	.cfi_offset r15, -24
	mov	dword ptr [rbp - 56], esi       ## 4-byte Spill
	lea	rsi, [rip + L_.str]
	call	_SDL_RWFromFile
	mov	rdi, rax
	mov	esi, 1
	call	_SDL_LoadBMP_RW
	xor	ecx, ecx
	mov	qword ptr [rbp - 72], rcx       ## 8-byte Spill
	mov	rdi, rax
	mov	esi, 372645892
	xor	edx, edx
	call	_SDL_ConvertSurfaceFormat
	mov	rbx, rax
	mov	esi, dword ptr [rax + 16]
	mov	edx, dword ptr [rax + 20]
	xor	edi, edi
	mov	ecx, 32
	mov	r8d, 372645892
	call	_SDL_CreateRGBSurfaceWithFormat
	mov	qword ptr [rbp - 88], rax       ## 8-byte Spill
	mov	eax, dword ptr [rbx + 20]
	mov	dword ptr [rbp - 60], eax       ## 4-byte Spill
	test	eax, eax
	jle	LBB3_15
## %bb.1:
	mov	eax, dword ptr [rbx + 16]
	mov	dword ptr [rbp - 52], eax       ## 4-byte Spill
	mov	eax, dword ptr [rbp - 56]       ## 4-byte Reload
	mov	ecx, eax
	neg	ecx
	cmp	ecx, eax
	mov	dword ptr [rbp - 48], ecx       ## 4-byte Spill
                                        ## kill: def $ecx killed $ecx def $rcx
	cmovl	ecx, eax
	mov	qword ptr [rbp - 120], rcx      ## 8-byte Spill
	lea	eax, [rcx + 1]
	mov	dword ptr [rbp - 76], eax       ## 4-byte Spill
	jmp	LBB3_2
	.p2align	4, 0x90
LBB3_14:                                ##   in Loop: Header=BB3_2 Depth=1
	mov	rcx, qword ptr [rbp - 72]       ## 8-byte Reload
	inc	ecx
	mov	rax, rcx
	mov	qword ptr [rbp - 72], rcx       ## 8-byte Spill
	cmp	ecx, dword ptr [rbp - 60]       ## 4-byte Folded Reload
	je	LBB3_15
LBB3_2:                                 ## =>This Loop Header: Depth=1
                                        ##     Child Loop BB3_4 Depth 2
                                        ##       Child Loop BB3_6 Depth 3
                                        ##         Child Loop BB3_8 Depth 4
	cmp	dword ptr [rbp - 52], 0         ## 4-byte Folded Reload
	jle	LBB3_14
## %bb.3:                               ##   in Loop: Header=BB3_2 Depth=1
	xor	r14d, r14d
	jmp	LBB3_4
	.p2align	4, 0x90
LBB3_13:                                ##   in Loop: Header=BB3_4 Depth=2
	movd	eax, xmm0
	cdq
	pextrd	ecx, xmm0, 1
	idiv	r15d
	mov	r8d, eax
	mov	eax, ecx
	cdq
	pextrd	ecx, xmm0, 2
	idiv	r15d
	mov	r9d, eax
	mov	eax, ecx
	cdq
	pextrd	ecx, xmm0, 3
	idiv	r15d
	mov	r10d, eax
	mov	eax, ecx
	cdq
	idiv	r15d
	mov	rdi, qword ptr [rbx + 8]
	movzx	esi, r8b
	movzx	edx, r9b
	movzx	ecx, r10b
	movzx	r8d, al
	call	_SDL_MapRGBA
	mov	rdi, qword ptr [rbp - 88]       ## 8-byte Reload
	mov	rcx, qword ptr [rdi + 8]
	movzx	ecx, byte ptr [rcx + 17]
	movsxd	rdx, dword ptr [rdi + 24]
	movsxd	rsi, dword ptr [rbp - 72]       ## 4-byte Folded Reload
	imul	rsi, rdx
	add	rsi, qword ptr [rdi + 32]
	imul	ecx, r14d
	mov	dword ptr [rcx + rsi], eax
	inc	r14d
	cmp	r14d, dword ptr [rbp - 52]      ## 4-byte Folded Reload
	je	LBB3_14
LBB3_4:                                 ##   Parent Loop BB3_2 Depth=1
                                        ## =>  This Loop Header: Depth=2
                                        ##       Child Loop BB3_6 Depth 3
                                        ##         Child Loop BB3_8 Depth 4
	pxor	xmm0, xmm0
	mov	r15d, 0
	mov	eax, dword ptr [rbp - 48]       ## 4-byte Reload
	cmp	eax, dword ptr [rbp - 56]       ## 4-byte Folded Reload
	jg	LBB3_13
## %bb.5:                               ##   in Loop: Header=BB3_4 Depth=2
	xor	r15d, r15d
	pxor	xmm0, xmm0
	mov	eax, dword ptr [rbp - 48]       ## 4-byte Reload
	mov	ecx, eax
	jmp	LBB3_6
	.p2align	4, 0x90
LBB3_12:                                ##   in Loop: Header=BB3_6 Depth=3
	mov	rcx, qword ptr [rbp - 128]      ## 8-byte Reload
	lea	eax, [rcx + 1]
	cmp	ecx, dword ptr [rbp - 120]      ## 4-byte Folded Reload
	mov	ecx, eax
	je	LBB3_13
LBB3_6:                                 ##   Parent Loop BB3_2 Depth=1
                                        ##     Parent Loop BB3_4 Depth=2
                                        ## =>    This Loop Header: Depth=3
                                        ##         Child Loop BB3_8 Depth 4
	mov	rax, qword ptr [rbp - 72]       ## 8-byte Reload
	lea	r13d, [rcx + rax]
	cmp	r13d, dword ptr [rbp - 60]      ## 4-byte Folded Reload
	mov	qword ptr [rbp - 128], rcx      ## 8-byte Spill
	jge	LBB3_12
## %bb.7:                               ##   in Loop: Header=BB3_6 Depth=3
	mov	eax, dword ptr [rbp - 48]       ## 4-byte Reload
	mov	r12d, eax
	jmp	LBB3_8
	.p2align	4, 0x90
LBB3_10:                                ##   in Loop: Header=BB3_8 Depth=4
	mov	rsi, qword ptr [rbx + 8]
	movsxd	rcx, dword ptr [rbx + 24]
	movsxd	rdx, r13d
	imul	rdx, rcx
	add	rdx, qword ptr [rbx + 32]
	movzx	ecx, byte ptr [rsi + 17]
	cdqe
	imul	rax, rcx
	mov	edi, dword ptr [rax + rdx]
	lea	rdx, [rbp - 44]
	lea	rcx, [rbp - 43]
	lea	r8, [rbp - 42]
	lea	r9, [rbp - 41]
	movdqa	xmmword ptr [rbp - 112], xmm0   ## 16-byte Spill
	call	_SDL_GetRGBA
	movzx	eax, byte ptr [rbp - 44]
	movzx	ecx, byte ptr [rbp - 43]
	movzx	edx, byte ptr [rbp - 42]
	movzx	esi, byte ptr [rbp - 41]
	movd	xmm0, eax
	pinsrd	xmm0, ecx, 1
	pinsrd	xmm0, edx, 2
	pinsrd	xmm0, esi, 3
	movdqa	xmm1, xmmword ptr [rbp - 112]   ## 16-byte Reload
	paddd	xmm1, xmm0
	movdqa	xmmword ptr [rbp - 112], xmm1   ## 16-byte Spill
	movdqa	xmm0, xmmword ptr [rbp - 112]   ## 16-byte Reload
	inc	r15d
LBB3_11:                                ##   in Loop: Header=BB3_8 Depth=4
	inc	r12d
	cmp	dword ptr [rbp - 76], r12d      ## 4-byte Folded Reload
	je	LBB3_12
LBB3_8:                                 ##   Parent Loop BB3_2 Depth=1
                                        ##     Parent Loop BB3_4 Depth=2
                                        ##       Parent Loop BB3_6 Depth=3
                                        ## =>      This Inner Loop Header: Depth=4
	lea	eax, [r14 + r12]
	mov	ecx, eax
	or	ecx, r13d
	js	LBB3_11
## %bb.9:                               ##   in Loop: Header=BB3_8 Depth=4
	cmp	eax, dword ptr [rbp - 52]       ## 4-byte Folded Reload
	jl	LBB3_10
	jmp	LBB3_12
LBB3_15:
	lea	rdi, [rip + L_.str.1]
	lea	rsi, [rip + L_.str.2]
	call	_SDL_RWFromFile
	mov	rdi, qword ptr [rbp - 88]       ## 8-byte Reload
	mov	rsi, rax
	mov	edx, 1
	call	_SDL_SaveBMP_RW
	add	rsp, 88
	pop	rbx
	pop	r12
	pop	r13
	pop	r14
	pop	r15
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
	lea	rdi, [rip + L_.str.3]
	mov	esi, 5
	call	_blurrer
	xor	eax, eax
	pop	rbp
	ret
	.cfi_endproc
                                        ## -- End function
	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"rb"

L_.str.1:                               ## @.str.1
	.asciz	"out.bmp"

L_.str.2:                               ## @.str.2
	.asciz	"wb"

L_.str.3:                               ## @.str.3
	.asciz	"../../assets/objects/doomguy.bmp"

.subsections_via_symbols
