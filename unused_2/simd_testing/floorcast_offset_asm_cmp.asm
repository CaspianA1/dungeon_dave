/*
vec vec_tex_offset(const vec pos, const int tex_size) {
	return vec_fill(tex_size - 1) * (pos - _mm_round_pd(pos, _MM_FROUND_TRUNC));
}

ivec ivec_tex_offset(const vec pos, const int tex_size) {
	const int max_offset = tex_size - 1;
	const ivec floored_hit = ivec_from_vec(pos);
	return (ivec) {
		(pos[0] - floored_hit.x) * max_offset,
		(pos[1] - floored_hit.y) * max_offset
	};
}
*/

_vec_tex_offset: # (10 instructions)
	push rbp
	mov	rbp, rsp
	dec	edi
	vcvtsi2sd xmm1, xmm1, edi
	vmovddup xmm1, xmm1
	vroundpd xmm2, xmm0, 3
	vsubpd xmm0, xmm0, xmm2
	vmulpd xmm0, xmm0, xmm1
	pop	rbp
	ret

_ivec_tex_offset: # (17 instructions)
	push rbp
	mov	rbp, rsp
	dec	edi
	vroundsd xmm1, xmm0, xmm0, 9
	vpermilpd xmm2, xmm0, 1
	vroundsd xmm3, xmm2, xmm2, 9
	vsubsd xmm0, xmm0, xmm1
	vcvtsi2sd xmm1, xmm4, edi
	vmulsd xmm0, xmm0, xmm1
	vcvttsd2si eax, xmm0
	vsubsd xmm0, xmm2, xmm3
	vmulsd xmm0, xmm0, xmm1
	vcvttsd2si ecx, xmm0
	shl	rcx, 32
	or rax, rcx
	pop	rbp
	ret

_draw_from_hit_vec_version:
	.cfi_startproc
	push rbp
	mov	rbp, rsp
	push r15
	push r14
	push r12
	push rbx
	sub	rsp, 32
	mov	rbx, rsi
	mov	r14d, edi
	vmovsd	qword ptr [rbp - 56], xmm1
	mov	rax, qword ptr [rip + _p@GOTPCREL]
	mov	ecx, dword ptr [rax + 12]
	dec	ecx
	vcvtsi2sd xmm1, xmm2, ecx
	vmovddup xmm1, xmm1
	vroundpd xmm2, xmm0, 3
	vsubpd xmm2, xmm0, xmm2
	vmulpd xmm1, xmm1, xmm2
	movsxd rcx, dword ptr [rax + 8]
	vpermilpd xmm2, xmm1, 1
	vcvttsd2si edx, xmm2
	movsxd rdx, edx
	imul rdx, rcx
	add	rdx, qword ptr [rax]
	vcvttsd2si rax, xmm1
	mov	r12d, dword ptr [rdx + 4*rax]
	mov	r15, qword ptr [rip + _settings@GOTPCREL]
	vmovsd xmm1, qword ptr [r15 + 56]
	vmovsd qword ptr [rbp - 48], xmm1
	vcvtsi2sd xmm1, xmm3, dword ptr [r15 + 4]
	vmovsd qword ptr [rbp - 40], xmm1
	mov	rax, qword ptr [rip + _current_level@GOTPCREL]
	call qword ptr [rax + 112]
	cmp	dword ptr [r15 + 24], 0
	jle	LBB56_3
## %bb.1:
	vmulsd xmm0, xmm0, qword ptr [rbp - 48]
	vmovsd xmm1, qword ptr [rbp - 40]
	vmulsd xmm1, xmm1, qword ptr [rbp - 56]
	vdivsd xmm0, xmm0, xmm1
	vminsd xmm0, xmm0, qword ptr [rip + LCPI56_0]
	mov	edx, r12d
	mov	eax, r12d
	shr	eax, 16
	movzx eax, al
	vcvtsi2sd xmm1, xmm3, eax
	vmulsd xmm1, xmm0, xmm1
	vcvttsd2si ecx, xmm1
	movzx eax, dh
	vcvtsi2sd xmm1, xmm3, eax
	vmulsd xmm1, xmm0, xmm1
	vcvttsd2si eax, xmm1
	movzx edx, dl
	vcvtsi2sd xmm1, xmm3, edx
	vmulsd xmm0, xmm0, xmm1
	vcvttsd2si edx, xmm0
	shl	ecx, 16
	movzx esi, al
	shl	esi, 8
	movzx eax, dl
	or eax, esi
	or eax, ecx
	or eax, -16777216
	movsxd rcx, r14d
	mov	rdx, rcx
LBB56_2:
	mov	dword ptr [rbx + 4*rdx], eax
	inc	rdx
	movsxd rsi, dword ptr [r15 + 24]
	add	rsi, rcx
	cmp	rdx, rsi
	jl LBB56_2
LBB56_3:
	add	rsp, 32
	pop	rbx
	pop	r12
	pop	r14
	pop	r15
	pop	rbp
	ret

_draw_from_hit_ivec_version:
	push rbp
	mov	rbp, rsp
	push r15
	push r14
	push r12
	push rbx
	sub	rsp, 32
	mov	rax, qword ptr [rip + _p@GOTPCREL]
	mov	ecx, dword ptr [rax + 12]
	dec	ecx
	vroundsd xmm2, xmm0, xmm0, 9
	vpermilpd xmm3, xmm0, 1
	vroundsd xmm4, xmm3, xmm3, 9
	vsubsd xmm2, xmm0, xmm2
	vcvtsi2sd xmm5, xmm5, ecx
	vmulsd xmm2, xmm2, xmm5
	vcvttsd2si ecx, xmm2
	mov	rbx, rsi
	mov	r14d, edi
	vmovsd qword ptr [rbp - 56], xmm1
	vsubsd xmm1, xmm3, xmm4
	vmulsd xmm1, xmm1, xmm5
	vcvttsd2si edx, xmm1
	movsxd rsi, dword ptr [rax + 8]
	movsxd rdx, edx
	imul rdx, rsi
	add	rdx, qword ptr [rax]
	movsxd	rax, ecx
	mov	r12d, dword ptr [rdx + 4*rax]
	mov	r15, qword ptr [rip + _settings@GOTPCREL]
	vmovsd xmm1, qword ptr [r15 + 56]
	vmovsd qword ptr [rbp - 48], xmm1
	vcvtsi2sd xmm1, xmm6, dword ptr [r15 + 4]
	vmovsd qword ptr [rbp - 40], xmm1
	mov	rax, qword ptr [rip + _current_level@GOTPCREL]
	call qword ptr [rax + 112]
	cmp	dword ptr [r15 + 24], 0
	jle	LBB56_3
## %bb.1:
	vmulsd	xmm0, xmm0, qword ptr [rbp - 48]
	vmovsd	xmm1, qword ptr [rbp - 40]
	vmulsd	xmm1, xmm1, qword ptr [rbp - 56]
	vdivsd	xmm0, xmm0, xmm1
	vminsd	xmm0, xmm0, qword ptr [rip + LCPI56_0]
	mov	edx, r12d
	mov	eax, r12d
	shr	eax, 16
	movzx	eax, al
	vcvtsi2sd	xmm1, xmm6, eax
	vmulsd	xmm1, xmm0, xmm1
	vcvttsd2si	ecx, xmm1
	movzx	eax, dh
	vcvtsi2sd	xmm1, xmm6, eax
	vmulsd	xmm1, xmm0, xmm1
	vcvttsd2si	eax, xmm1
	movzx	edx, dl
	vcvtsi2sd	xmm1, xmm6, edx
	vmulsd	xmm0, xmm0, xmm1
	vcvttsd2si	edx, xmm0
	shl	ecx, 16
	movzx	esi, al
	shl	esi, 8
	movzx	eax, dl
	or	eax, esi
	or	eax, ecx
	or	eax, -16777216
	movsxd	rcx, r14d
	mov	rdx, rcx
LBB56_2:
	mov	dword ptr [rbx + 4*rdx], eax
	inc	rdx
	movsxd rsi, dword ptr [r15 + 24]
	add	rsi, rcx
	cmp	rdx, rsi
	jl	LBB56_2
LBB56_3:
	add	rsp, 32
	pop	rbx
	pop	r12
	pop	r14
	pop	r15
	pop	rbp
	ret
