; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-- -mattr=+sse2 | FileCheck %s

define float @foo(ptr %x, ptr %y, i32 %c) nounwind {
; CHECK-LABEL: foo:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    pushl %esi
; CHECK-NEXT:    pushl %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    testl %eax, %eax
; CHECK-NEXT:    je .LBB0_1
; CHECK-NEXT:  # %bb.2: # %bb18.preheader
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    xorps %xmm0, %xmm0
; CHECK-NEXT:    xorl %esi, %esi
; CHECK-NEXT:    .p2align 4
; CHECK-NEXT:  .LBB0_3: # %bb18
; CHECK-NEXT:    # =>This Inner Loop Header: Depth=1
; CHECK-NEXT:    xorps %xmm1, %xmm1
; CHECK-NEXT:    cvtsi2ssl (%edx,%esi,4), %xmm1
; CHECK-NEXT:    mulss (%ecx,%esi,4), %xmm1
; CHECK-NEXT:    addss %xmm1, %xmm0
; CHECK-NEXT:    incl %esi
; CHECK-NEXT:    cmpl %eax, %esi
; CHECK-NEXT:    jb .LBB0_3
; CHECK-NEXT:    jmp .LBB0_4
; CHECK-NEXT:  .LBB0_1:
; CHECK-NEXT:    xorps %xmm0, %xmm0
; CHECK-NEXT:  .LBB0_4: # %bb23
; CHECK-NEXT:    movss %xmm0, (%esp)
; CHECK-NEXT:    flds (%esp)
; CHECK-NEXT:    addl $4, %esp
; CHECK-NEXT:    popl %esi
; CHECK-NEXT:    retl
entry:
	%tmp2132 = icmp eq i32 %c, 0		; <i1> [#uses=1]
	br i1 %tmp2132, label %bb23, label %bb18

bb18:		; preds = %bb18, %entry
	%i.0.reg2mem.0 = phi i32 [ 0, %entry ], [ %tmp17, %bb18 ]		; <i32> [#uses=3]
	%res.0.reg2mem.0 = phi float [ 0.000000e+00, %entry ], [ %tmp14, %bb18 ]		; <float> [#uses=1]
	%tmp3 = getelementptr i32, ptr %x, i32 %i.0.reg2mem.0		; <ptr> [#uses=1]
	%tmp4 = load i32, ptr %tmp3, align 4		; <i32> [#uses=1]
	%tmp45 = sitofp i32 %tmp4 to float		; <float> [#uses=1]
	%tmp8 = getelementptr float, ptr %y, i32 %i.0.reg2mem.0		; <ptr> [#uses=1]
	%tmp9 = load float, ptr %tmp8, align 4		; <float> [#uses=1]
	%tmp11 = fmul float %tmp9, %tmp45		; <float> [#uses=1]
	%tmp14 = fadd float %tmp11, %res.0.reg2mem.0		; <float> [#uses=2]
	%tmp17 = add i32 %i.0.reg2mem.0, 1		; <i32> [#uses=2]
	%tmp21 = icmp ult i32 %tmp17, %c		; <i1> [#uses=1]
	br i1 %tmp21, label %bb18, label %bb23

bb23:		; preds = %bb18, %entry
	%res.0.reg2mem.1 = phi float [ 0.000000e+00, %entry ], [ %tmp14, %bb18 ]		; <float> [#uses=1]
	ret float %res.0.reg2mem.1
}
