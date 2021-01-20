.text
_start_MAIN:
	sd   ra, 0(sp)
	sd   fp, -8(sp)
	addi fp, sp, -8
	addi sp, sp, -16
	la   ra, _frameSize_MAIN
	lw   ra, 0(ra)
	sub  sp, sp, ra
	sd   s1, 0(sp)
	sd   s2, 8(sp)
	sd   s3, 16(sp)
	sd   s4, 24(sp)
	sd   s5, 32(sp)
	sd   s6, 40(sp)
	sd   s7, 48(sp)
	sd   s8, 56(sp)
	sd   s9, 64(sp)
	sd   s10, 72(sp)
	sd   s11, 80(sp)
	fsw   fs0, 88(sp)
	fsw   fs1, 92(sp)
	fsw   fs2, 96(sp)
	fsw   fs3, 100(sp)
	fsw   fs4, 104(sp)
	fsw   fs5, 108(sp)
	fsw   fs6, 112(sp)
	fsw   fs7, 116(sp)
	fsw   fs8, 120(sp)
	fsw   fs9, 124(sp)
	fsw   fs10, 128(sp)
	fsw   fs11, 132(sp)
	li x6, 1
	li x7, -4
	add x7, fp, x7
	sw x6, 0(x7)
	sw   t0, -8(sp)
	sw   t1, -16(sp)
	sw   t2, -24(sp)
	sw   t3, -32(sp)
	sw   t4, -40(sp)
	sw   t5, -48(sp)
	sw   t6, -56(sp)
	sw   a0, -64(sp)
	sw   a1, -72(sp)
	sw   a2, -80(sp)
	sw   a3, -88(sp)
	sw   a4, -96(sp)
	sw   a5, -104(sp)
	sw   a6, -112(sp)
	sw   a7, -120(sp)
	fsw   ft0, -128(sp)
	fsw   ft1, -136(sp)
	fsw   ft2, -144(sp)
	fsw   ft3, -152(sp)
	fsw   ft4, -160(sp)
	fsw   ft5, -168(sp)
	fsw   ft6, -176(sp)
	fsw   ft7, -184(sp)
	fsw   ft8, -192(sp)
	fsw   ft9, -200(sp)
	fsw   ft10, -208(sp)
	fsw   ft11, -216(sp)
	fsw  fa0, -224(sp)
	fsw  fa1, -232(sp)
	fsw  fa2, -240(sp)
	fsw  fa3, -248(sp)
	fsw  fa4, -256(sp)
	fsw  fa5, -264(sp)
	fsw  fa6, -272(sp)
	fsw  fa7, -280(sp)
	addi sp, sp, -280
	call _start_a
	mv x6, a0
	addi sp, sp, 280
	lw   t0, -8(sp)
	lw   t2, -24(sp)
	lw   t3, -32(sp)
	lw   t4, -40(sp)
	lw   t5, -48(sp)
	lw   t6, -56(sp)
	lw   a0, -64(sp)
	lw   a1, -72(sp)
	lw   a2, -80(sp)
	lw   a3, -88(sp)
	lw   a4, -96(sp)
	lw   a5, -104(sp)
	lw   a6, -112(sp)
	lw   a7, -120(sp)
	flw   ft0, -128(sp)
	flw   ft1, -136(sp)
	flw   ft2, -144(sp)
	flw   ft3, -152(sp)
	flw   ft4, -160(sp)
	flw   ft5, -168(sp)
	flw   ft6, -176(sp)
	flw   ft7, -184(sp)
	flw   ft8, -192(sp)
	flw   ft9, -200(sp)
	flw   ft10, -208(sp)
	flw   ft11, -216(sp)
	flw  fa0, -224(sp)
	flw  fa1, -232(sp)
	flw  fa2, -240(sp)
	flw  fa3, -248(sp)
	flw  fa4, -256(sp)
	flw  fa5, -264(sp)
	flw  fa6, -272(sp)
	flw  fa7, -280(sp)
	beqz  x6, IF_else0
	.data
_CONSTANT_0: .string "hello\n"
	.align 3
	.text
	la   a0, _CONSTANT_0
	call _write_str
	j IF_exit0
IF_else0:
	.data
_CONSTANT_1: .string "no\n"
	.align 3
	.text
	la   a0, _CONSTANT_1
	call _write_str
IF_exit0:
_end_MAIN:
	ld   s1, 0(sp)
	ld   s2, 8(sp)
	ld   s3, 16(sp)
	ld   s4, 24(sp)
	ld   s5, 32(sp)
	ld   s6, 40(sp)
	ld   s7, 48(sp)
	ld   s8, 56(sp)
	ld   s9, 64(sp)
	ld   s10, 72(sp)
	ld   s11, 80(sp)
	flw   fs0, 88(sp)
	flw   fs1, 92(sp)
	flw   fs2, 96(sp)
	flw   fs3, 100(sp)
	flw   fs4, 104(sp)
	flw   fs5, 108(sp)
	flw   fs6, 112(sp)
	flw   fs7, 116(sp)
	flw   fs8, 120(sp)
	flw   fs9, 124(sp)
	flw   fs10, 128(sp)
	flw   fs11, 132(sp)
	ld   ra, 8(fp)
	addi sp, fp, 8
	ld   fp, 0(fp)
	jr   ra
.data
	_frameSize_MAIN: .word 140
