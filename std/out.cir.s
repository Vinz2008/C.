	.text
	.file	"print.cpoint"
	.globl	printd                          # -- Begin function printd
	.p2align	4, 0x90
	.type	printd,@function
printd:                                 # @printd
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rax
	.cfi_def_cfa_offset 16
	movsd	%xmm0, (%rsp)
	callq	printd_internal@PLT
	popq	%rax
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end0:
	.size	printd, .Lfunc_end0-printd
	.cfi_endproc
                                        # -- End function
	.globl	printi                          # -- Begin function printi
	.p2align	4, 0x90
	.type	printi,@function
printi:                                 # @printi
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rax
	.cfi_def_cfa_offset 16
	movl	%edi, 4(%rsp)
	callq	printi_internal@PLT
	popq	%rax
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end1:
	.size	printi, .Lfunc_end1-printi
	.cfi_endproc
                                        # -- End function
	.globl	putchard                        # -- Begin function putchard
	.p2align	4, 0x90
	.type	putchard,@function
putchard:                               # @putchard
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rax
	.cfi_def_cfa_offset 16
	movsd	%xmm0, (%rsp)
	callq	putchard_internal@PLT
	popq	%rax
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end2:
	.size	putchard, .Lfunc_end2-putchard
	.cfi_endproc
                                        # -- End function
	.globl	printstr                        # -- Begin function printstr
	.p2align	4, 0x90
	.type	printstr,@function
printstr:                               # @printstr
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rax
	.cfi_def_cfa_offset 16
	movq	%rdi, (%rsp)
	callq	puts@PLT
	popq	%rax
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end3:
	.size	printstr, .Lfunc_end3-printstr
	.cfi_endproc
                                        # -- End function
	.globl	printc                          # -- Begin function printc
	.p2align	4, 0x90
	.type	printc,@function
printc:                                 # @printc
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rax
	.cfi_def_cfa_offset 16
	movl	%edi, %esi
	movb	%sil, 7(%rsp)
	movl	$.L__unnamed_1, %edi
	xorl	%eax, %eax
	callq	printf@PLT
	popq	%rax
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end4:
	.size	printc, .Lfunc_end4-printc
	.cfi_endproc
                                        # -- End function
	.type	.L__unnamed_1,@object           # @0
	.section	.rodata.str1.1,"aMS",@progbits,1
.L__unnamed_1:
	.asciz	"%c"
	.size	.L__unnamed_1, 3

	.section	".note.GNU-stack","",@progbits
