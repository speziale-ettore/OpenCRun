
namespace {

// Very architecture-dependent code. Use __thread instead of
// llvm::sys::ThreadLocal in order to generate more compact code.
__thread uint64_t SavedEBP, SavedESP;

} // End anonymous namespace.

extern "C" {

// Redo declarations here, I do not want to include any header.
typedef void (*EntryPoint)(void **);

void SwitchWorkItem();
void WorkItemLauncher();

// Stack offsets in bytes.
#define STACK_CONTEXT_OFFSET   (                         0)

#define STACK_NEXT_OFFSET      (STACK_CONTEXT_OFFSET   - 0)
#define STACK_EBX_SAVE_OFFSET  (STACK_NEXT_OFFSET      - 4)
#define STACK_ESI_SAVE_OFFSET  (STACK_EBX_SAVE_OFFSET  - 4)
#define STACK_EDI_SAVE_OFFSET  (STACK_ESI_SAVE_OFFSET  - 4)
#define STACK_EBP_SAVE_OFFSET  (STACK_EDI_SAVE_OFFSET  - 4)
#define STACK_ESP_SAVE_OFFSET  (STACK_EBP_SAVE_OFFSET  - 4)
#define STACK_RET_SAVE_OFFSET  (STACK_ESP_SAVE_OFFSET  - 4)

#define STACK_BASE_OFFSET      (STACK_RET_SAVE_OFFSET  - 4)

#define STACK_KERN_FUNC_OFFSET (STACK_BASE_OFFSET      - 0)
#define STACK_KERN_ARGS_OFFSET (STACK_KERN_FUNC_OFFSET - 4)

#define STACK_PTR_OFFSET       (STACK_KERN_ARGS_OFFSET - 0)

// Layout notes:
//
// 0
// +------------------+
// | Context          |
// | ...              |
// +------------------+
// | Kernel function  |
// | Kernel arguments |    The i386 calling conventions does not enforce 16-byte
// |                  | <- alignment, but GCC does. I think is for handling
// +------------------+    vectors. Insert padding to do the same.
// | Kernel frame     |
// | ...              |
// +------------------+
//
// Context contains next stack pointer and ebx, esi, edi, ebp, esp.

void InitWorkItemStack(void *Stack,
                       size_t Size,
                       EntryPoint Entry,
                       void **Args) {
  uintptr_t StackAddr = (uintptr_t) Stack;

  // Stack grows downward.
  StackAddr += Size - 4;

  // Save stack info.
  *(uint32_t *) (StackAddr + STACK_EBP_SAVE_OFFSET) =
    (uint32_t) (StackAddr + STACK_BASE_OFFSET);
  *(uint32_t *) (StackAddr + STACK_ESP_SAVE_OFFSET) =
    (uint32_t) (StackAddr + STACK_PTR_OFFSET);

  // Place launcher address as fake return address.
  *(uint32_t *) (StackAddr + STACK_RET_SAVE_OFFSET) =
    (uint32_t) WorkItemLauncher;

  // Prepare arguments for launcher.
  *(uint32_t *) (StackAddr + STACK_KERN_FUNC_OFFSET) = (uint32_t) Entry;
  *(uint32_t *) (StackAddr + STACK_KERN_ARGS_OFFSET) = (uint32_t) Args;
}

void SetWorkItemStackLink(void *Stack, void *PrevStack, size_t Size) {
  uintptr_t StackAddr = (uintptr_t) Stack,
            PrevStackAddr = (uintptr_t) PrevStack;

  // Stack grows downward.
  StackAddr += Size - 4;
  PrevStackAddr += Size - 4;

  // Link stacks.
  *(uint32_t *) (PrevStackAddr + STACK_NEXT_OFFSET) = (uint32_t) StackAddr;
}

void RunWorkItems(void *Stack, size_t Size) {
  uintptr_t StackAddr = (uintptr_t) Stack;

  // Stack grows downward.
  StackAddr += Size - 4;

  // Save current stack address.
  __asm__ __volatile__(
  "movl %%ebp, %0\n"
  "movl %%esp, %1\n"
  : "=r" (SavedEBP), "=r" (SavedESP)
  :
  :
  );

  SetCurrentWorkItemStack((void *) StackAddr);

  // Switch to the work-item stack and directly call the kernel.
  __asm__ __volatile__(
  "movl %0, %%ebp\n"
  "movl %1, %%esp\n"
  "subl $12, %%esp\n"
  "movl -4(%%ebp), %%eax\n"
  "movl %%eax, (%%esp)\n"
  "calll *(%%ebp)\n"
  "addl $12, %%esp\n"
  :
  : "r" (StackAddr + STACK_BASE_OFFSET),
    "r" (StackAddr + STACK_PTR_OFFSET)
  :
  );

  // Switch back to main thread stack.
  __asm__ __volatile__(
  "movl %0, %%ebp\n"
  "movl %1, %%esp\n"
  :
  : "r" (SavedEBP), "r" (SavedESP)
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
"  calll GetCurrentWorkItemStack\n"

// Save standard callee-saved registers.
"  movl %ebx, -4(%eax)\n"
"  movl %esi, -8(%eax)\n"
"  movl %edi, -12(%eax)\n"

// Save stack registers. Since we do not have a frame for this function, saving
// ebp is just a simple copy. Saving esp is more complex, because we will not
// return from this function using a ret, so we have to manually popping the
// return address from the stack.
"  movl %ebp, -16(%eax)\n"
"  movl %esp, %ebx\n"
"  addl $4, %ebx\n"
"  movl %ebx, -20(%eax)\n"

// Save return address.
"  movl (%esp), %ebx\n"
"  movl %ebx, -24(%eax)\n"

// Move to the next stack.
"  subl $12, %esp\n"
"  movl (%eax), %ebx\n"
"  movl %ebx, (%esp)\n"
"  calll SetCurrentWorkItemStack\n"
"  addl $12, %esp\n"
"  movl %ebx, %eax\n"

// Restore standard callee-saved registers.
"  movl -4(%eax), %ebx\n"
"  movl -8(%eax), %esi\n"
"  movl -12(%eax), %edi\n"

// Restore stack registers.
"  movl -16(%eax), %ebp\n"
"  movl -20(%eax), %esp\n"

// Jump to return address.
"  jmp *-24(%eax)\n"
"  calll NotReachable\n"
);

// void WorkItemLauncher()
__asm__ (
"WorkItemLauncher:\n"
"  subl $12, %esp\n"      // Make space for argument.
"  movl -4(%ebp), %eax\n" // Copy argument first in a temporary.
"  movl %eax, (%esp)\n"   // Move the temporary to call stack.
"  calll *(%ebp)\n"       // Call the kernel.
"  addl $12, %esp\n"      // Cleanup kernel parameter.
"  calll NotReachable\n"  // The previous call never returns.
);
