
; RUN: opt -load %projshlibdir/OpenCRunPasses.so \
; RUn:     -aggressive-inliner                   \
; RUN:     -S -o - %s | FileCheck %s
; REQUIRES: loadable_module

define void @foo() nounwind {
entry:
  ret void
}

define void @bar() nounwind {
entry:
  ret void
}

; CHECK:      define void @foo() nounwind {
; CHECK-NEXT: entry:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; CHECK:      define void @bar() nounwind {
; CHECK-NEXT: entry:
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

!opencl.kernels = !{!0, !2}
!opencl.global_address_space = !{!3}
!opencl.local_address_space = !{!4}
!opencl.constant_address_space = !{!5}

!0 = metadata !{void ()* @foo, metadata !1}
!1 = metadata !{}
!2 = metadata !{void ()* @bar, metadata !1}
!3 = metadata !{i32 16776960}
!4 = metadata !{i32 16776961}
!5 = metadata !{i32 16776962}
