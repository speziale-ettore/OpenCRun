
; RUN: opt -load %projshlibdir/OpenCRunPasses.so \
; RUN:     -footprint-estimator                  \
; RUN:     -analyze                              \
; RUN:     -S -o - %s | FileCheck %s
; REQUIRES: loadable_module

declare float @acosf(float) nounwind readnone

define void @foo() nounwind {
entry:
  %acos = call float @acosf(float 0.000000e+00) nounwind readnone
  ret void
}

; CHECK:      Kernel: foo
; CHECK-NEXT:   Private memory: 0 bytes (min), 0.00000 (accuracy)

!opencl.kernels = !{!0}
!opencl.global_address_space = !{!2}
!opencl.local_address_space = !{!3}
!opencl.constant_address_space = !{!4}

!0 = metadata !{void ()* @foo, metadata !1}
!1 = metadata !{}
!2 = metadata !{i32 16776960}
!3 = metadata !{i32 16776961}
!4 = metadata !{i32 16776962}
