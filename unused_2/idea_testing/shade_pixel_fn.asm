/*
Uint32 shade_ARGB_pixel(const Uint32 pixel, const double dist, const vec hit) {
	const double shade = calculate_shade(settings.proj_dist / dist, hit);
	const byte r = (byte) (pixel >> 16) * shade, g = (byte) (pixel >> 8) * shade, b = (byte) pixel * shade;
	return 0xFF000000 | (r << 16) | (g << 8) | b;
}
*/

_shade_ARGB_pixel:
	push rbp
	mov	rbp, rsp
	push rbx
	sub	rsp, 24
	vmovsd qword ptr [rbp - 32], xmm0
	mov	ebx, edi
	mov	rax, qword ptr [rip + _settings@GOTPCREL]
	vmovsd xmm0, qword ptr [rax + 56]
	vmovsd qword ptr [rbp - 24], xmm0
	vcvtsi2sd xmm0, xmm2, dword ptr [rax + 4]
	vmovsd qword ptr [rbp - 16], xmm0
	mov	rax, qword ptr [rip + _current_level@GOTPCREL]
	vmovapd	xmm0, xmm1
	call qword ptr [rax + 112]
	vmulsd xmm0, xmm0, qword ptr [rbp - 24]
	vmovsd xmm1, qword ptr [rbp - 16]
	vmulsd	xmm1, xmm1, qword ptr [rbp - 32]
	vdivsd	xmm0, xmm0, xmm1
	vminsd	xmm0, xmm0, qword ptr [rip + LCPI54_0]
	mov	eax, ebx
	shr	eax, 16
	movzx	eax, al
	vcvtsi2sd	xmm1, xmm2, eax
	vmulsd	xmm1, xmm0, xmm1
	vcvttsd2si	ecx, xmm1
	movzx	eax, bh
	vcvtsi2sd	xmm1, xmm2, eax
	vmulsd	xmm1, xmm0, xmm1
	vcvttsd2si	eax, xmm1
	movzx	edx, bl
	vcvtsi2sd	xmm1, xmm2, edx
	vmulsd	xmm0, xmm0, xmm1
	vcvttsd2si	edx, xmm0
	shl	ecx, 16
	movzx	esi, al
	shl	esi, 8
	movzx	eax, dl
	or	eax, esi
	or	eax, ecx
	or	eax, -16777216
	add	rsp, 24
	pop	rbx
	pop	rbp
	ret