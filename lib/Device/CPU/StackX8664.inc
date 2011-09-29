
namespace {

// Very architecture-dependent code. Use __thread instead of
// llvm::sys::ThreadLocal in order to generate more compact code.
__thread uint64_t SavedRBP, SavedRSP;

} // End anonymous namespace.

extern "C" {

// Redo declarations here, I do not want to include any header.
typedef void (*EntryPoint)(void **);

void SwitchWorkItem();
void WorkItemLauncher();

// Stack offsets in bytes.
#define STACK_CONTEXT_OFFSET   (                         0)

#define STACK_NEXT_OFFSET      (STACK_CONTEXT_OFFSET   - 0)
#define STACK_RBX_SAVE_OFFSET  (STACK_NEXT_OFFSET      - 8)
#define STACK_R12_SAVE_OFFSET  (STACK_RBX_SAVE_OFFSET  - 8)
#define STACK_R13_SAVE_OFFSET  (STACK_R12_SAVE_OFFSET  - 8)
#define STACK_R14_SAVE_OFFSET  (STACK_R13_SAVE_OFFSET  - 8)
#define STACK_R15_SAVE_OFFSET  (STACK_R14_SAVE_OFFSET  - 8)
#define STACK_RBP_SAVE_OFFSET  (STACK_R15_SAVE_OFFSET  - 8)
#define STACK_RSP_SAVE_OFFSET  (STACK_RBP_SAVE_OFFSET  - 8)
#define STACK_RET_SAVE_OFFSET  (STACK_RSP_SAVE_OFFSET  - 8)

#define STACK_BASE_OFFSET      (STACK_RET_SAVE_OFFSET  - 8)

#define STACK_KERN_FUNC_OFFSET (STACK_BASE_OFFSET      - 0)
#define STACK_KERN_ARGS_OFFSET (STACK_KERN_FUNC_OFFSET - 8)

#define STACK_PTR_OFFSET       (STACK_KERN_ARGS_OFFSET - 8)

// Layout notes:
//
// 0                 63
// +------------------+
// | Context          |
// | ...              |
// +------------------+
// | Kernel function  |
// | Kernel arguments |
// |                  | <- Padding inserted because RSP must be 16-byte aligned
// +------------------+    before call instruction.
// | Kernel frame     |
// | ...              |
// +------------------+
//
// Context contains next stack pointer and rbx, r12, r13, r14, r15, rbp, rsp.

void InitWorkItemStack(void *Stack,
                       size_t Size,
                       EntryPoint Entry,
                       void **Args) {
  uintptr_t StackAddr = (uintptr_t) Stack;

  // Stack grows downward.
  StackAddr += Size - 8;

  // Save stack info.
  *(uint64_t *) (StackAddr + STACK_RBP_SAVE_OFFSET) =
    (uint64_t) (StackAddr + STACK_BASE_OFFSET);
  *(uint64_t *) (StackAddr + STACK_RSP_SAVE_OFFSET) =
    (uint64_t) (StackAddr + STACK_PTR_OFFSET);

  // Place launcher address as fake return address.
  *(uint64_t *) (StackAddr + STACK_RET_SAVE_OFFSET) =
    (uint64_t) WorkItemLauncher;

  // Prepare arguments for launcher.
  *(uint64_t *) (StackAddr + STACK_KERN_FUNC_OFFSET) = (uint64_t) Entry;
  *(uint64_t *) (StackAddr + STACK_KERN_ARGS_OFFSET) = (uint64_t) Args;
}

void SetWorkItemStackLink(void *Stack, void *PrevStack, size_t Size) {
  uintptr_t StackAddr = (uintptr_t) Stack,
            PrevStackAddr = (uintptr_t) PrevStack;

  // Stack grows downward.
  StackAddr += Size - 8;
  PrevStackAddr += Size - 8;

  // Link stacks.
  *(uint64_t *) (PrevStackAddr + STACK_NEXT_OFFSET) = (uint64_t) StackAddr;
}

void RunWorkItems(void *Stack, size_t Size) {
  uintptr_t StackAddr = (uintptr_t) Stack;
  
  // Stack grows downward.
  StackAddr += Size - 8;

  // Save current stack addresses.
  __asm__ __volatile__(
  "movq %%rbp, %0\n"
  "movq %%rsp, %1\n"
  : "=r" (SavedRBP), "=r" (SavedRSP)
  :
  :
  );

  SetCurrentWorkItemStack((void *) StackAddr);

  // Switch to the work-item stack and directly call the kernel.
  __asm__ __volatile__(
  "movq %0, %%rbp\n"
  "movq %1, %%rsp\n"
  "movq -8(%%rbp), %%rdi\n"
  "callq *(%%rbp)\n"
  :
  : "r" (StackAddr + STACK_BASE_OFFSET),
    "r" (StackAddr + STACK_PTR_OFFSET)
  :
  );

  // Switch back to main thread stack.
  __asm__ __volatile__(
  "movq %0, %%rbp\n"
  "movq %1, %%rsp\n"
  :
  : "r" (SavedRBP), "r" (SavedRSP)
  :
  );
}

void NotReachable() {
  llvm_unreachable("Work-items cannot reach here");
}

}

// void SwitchWorkItem()
__asm__ (
"SwitchWorkItem:\n"
// Get current stack base.
"  callq GetCurrentWorkItemStack\n"

// Save standard callee-saved registers.
"  movq %rbx, -8(%rax)\n"
"  movq %r12, -16(%rax)\n"
"  movq %r13, -24(%rax)\n"
"  movq %r14, -32(%rax)\n"
"  movq %r15, -40(%rax)\n"

// Save stack registers. Since we do not have a frame for this function, saving
// rbp is just a simple copy. Saving rsp is more complex, because we will not
// return from this function using a ret, so we have to manually popping the
// return address from the stack.
"  movq %rbp, -48(%rax)\n"
"  movq %rsp, %rbx\n"
"  addq $8, %rbx\n"
"  movq %rbx, -56(%rax)\n"

// Save return address.
"  movq (%rsp), %rbx\n"
"  movq %rbx, -64(%rax)\n"

// Move to the next stack.
"  movq (%rax), %rbx\n"
"  movq %rbx, %rdi\n"
"  callq SetCurrentWorkItemStack\n"
"  movq %rbx, %rax\n"

// Restore standard callee-saved registers.
"  movq -8(%rax), %rbx\n"
"  movq -16(%rax), %r12\n"
"  movq -24(%rax), %r13\n"
"  movq -32(%rax), %r14\n"
"  movq -40(%rax), %r15\n"

// Restore stack registers.
"  movq -48(%rax), %rbp\n"
"  movq -56(%rax), %rsp\n"

// Jump to return address.
"  jmp *-64(%rax)\n"
"  callq NotReachable\n"
);

// void WorkItemLauncher()
__asm__ (
"WorkItemLauncher:\n"
"  movq -8(%rbp), %rdi\n" // Load args.
"  callq *(%rbp)\n"       // Call kernel.
"  callq NotReachable\n"  // The previous call never returns.
);
