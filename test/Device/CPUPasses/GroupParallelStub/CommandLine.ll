
; RUN: opt -load %projshlibdir/OpenCRunCPUPasses.so \
; RUN:     -cpu-group-parallel-stub                 \
; RUN:     -kernel foo                              \
; RUN:     -S -o - %s | FileCheck %s
; REQUIRES: loadable_module

declare void @__builtin_ocl_barrier(i64);

define void @foo() nounwind {
entry:
  ret void
}

define void @bar() nounwind {
entry:
  ret void
}

; CHECK:      define void @_GroupParallelStub_foo(i8** %args) {
; CHECK-NEXT: entry:
; CHECK-NEXT:   call void @foo()
; CHECK-NEXT:   tail call void @__builtin_ocl_barrier(i64 0)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; CHECK-NOT: define void @_GroupParallelStub_bar(i8** %args)

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
