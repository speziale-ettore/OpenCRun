
; RUN: opt -load %projshlibdir/OpenCRunPasses.so \
; RUN:     -aggressive-inliner                   \
; RUN:     -S -o - %s | FileCheck %s
; REQUIRES: loadable_module

define i32 @foo() nounwind {
entry:
  %baz = call i32 @bar() nounwind
  ret i32 %baz
}

define i32 @bar() nounwind {
  %yez = call i32 @yep() nounwind
  %add = add nsw i32 %yez, 4
  ret i32 %add
}

define i32 @yep() nounwind {
  ret i32 4
}

; CHECK:      define i32 @foo() nounwind {
; CHECK-NEXT: entry:
; CHECK-NEXT:   ret i32 8
; CHECK-NEXT: }

!opencl.kernels = !{!0}
!opencl.global_address_space = !{!2}
!opencl.local_address_space = !{!3}
!opencl.constant_address_space = !{!4}

!0 = metadata !{i32 ()* @foo, metadata !1}
!1 = metadata !{}
!2 = metadata !{i32 16776960}
!3 = metadata !{i32 16776961}
!4 = metadata !{i32 16776962}
