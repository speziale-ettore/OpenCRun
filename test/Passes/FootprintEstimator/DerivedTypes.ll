
; RUN: opt -load %projshlibdir/OpenCRunPasses.so \
; RUN:     -footprint-estimate                   \
; RUN:     -analyze                              \
; RUN:     -S -o - %s | FileCheck %s
; REQUIRES: loadable_module

define void @foo() nounwind {
entry:
  %integer = alloca i32, align 4
  %struct = alloca {i32, i32}, align 4
  %array = alloca [4 x i32], align 4
  %vector = alloca <4 x i32>, align 16
  %pointer = alloca i32*, align 4
  ret void
}

; CHECK: Private memory: 48 bytes (min), 1.00000 (accuracy)

!opencl.kernels = !{!0}
!opencl.global_address_space = !{!2}
!opencl.local_address_space = !{!3}
!opencl.constant_address_space = !{!4}

!0 = metadata !{void ()* @foo, metadata !1}
!1 = metadata !{}
!2 = metadata !{i32 16776960}
!3 = metadata !{i32 16776961}
!4 = metadata !{i32 16776962}
