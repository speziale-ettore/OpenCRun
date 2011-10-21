
; RUN: opt -load %projshlibdir/OpenCRunPasses.so \
; RUN:     -footprint-estimate                   \
; RUN:     -analyze                              \
; RUN:     -S -o - %s | FileCheck %s
; REQUIRES: loadable_module

define void @foo() nounwind {
entry:
  %float = alloca float, align 4
  %double = alloca double, align 8
  %x86_fp80 = alloca x86_fp80, align 16
  %fp128 = alloca fp128, align 16
  %ppc_fp128 = alloca ppc_fp128, align 16
  %x86_mmx = alloca x86_mmx, align 16
  ret void
}

; CHECK: Private memory: 62 bytes (min), 1.00000 (accuracy)

!opencl.kernels = !{!0}
!opencl.global_address_space = !{!2}
!opencl.local_address_space = !{!3}
!opencl.constant_address_space = !{!4}

!0 = metadata !{void ()* @foo, metadata !1}
!1 = metadata !{}
!2 = metadata !{i32 16776960}
!3 = metadata !{i32 16776961}
!4 = metadata !{i32 16776962}
