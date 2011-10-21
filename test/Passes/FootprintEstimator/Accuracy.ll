
; RUN: opt -load %projshlibdir/OpenCRunPasses.so \
; RUN:     -footprint-estimate                   \
; RUN:     -analyze                              \
; RUN:     -S -o - %s | FileCheck %s
; REQUIRES: loadable_module

define void @foo(i32* %out) nounwind {
entry:
  %0 = alloca i32*, align 4
  store i32* %out, i32** %0, align 4
  %1 = load i32** %0, align 4
  %2 = icmp ne i32* %1, null
  br i1 %2, label %then, label %else

then:
  %3 = load i32** %0, align 4
  store i32 0, i32* %3
  br label %else

else:
  ret void
}

; CHECK: Private memory: 4 bytes (min), 0.30769 (accuracy)

!opencl.kernels = !{!0}
!opencl.global_address_space = !{!2}
!opencl.local_address_space = !{!3}
!opencl.constant_address_space = !{!4}

!0 = metadata !{void (i32*)* @foo, metadata !1}
!1 = metadata !{}
!2 = metadata !{i32 16776960}
!3 = metadata !{i32 16776961}
!4 = metadata !{i32 16776962}
