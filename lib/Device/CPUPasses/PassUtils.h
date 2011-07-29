
#ifndef OPENCRUN_DEVICE_CPUPASSES_PASSUTILS_H
#define OPENCRUN_DEVICE_CPUPASSES_PASSUTILS_H

#include "llvm/ADT/SmallString.h"
#include "llvm/Instructions.h"

#include <string>

namespace opencrun {

//
// Environment functions.
//

std::string GetKernelOption(llvm::StringRef Default = "");
bool GetVerifyCPUPassOption();

//
// Block functions.
//

//===----------------------------------------------------------------------===//
/// FindNextInst - Get Inst successor.
/// \param Inst the instructions for which the successor has to be found.
/// \return the successor of Inst.
//===----------------------------------------------------------------------===//
llvm::Instruction *FindNextInst(llvm::Instruction *Inst);

//
// Barrier functions.
//

//===----------------------------------------------------------------------===//
/// GetAsBarrierCall - Try to cast given instruction to an OpenCL C barrier
///  call.
/// \param Inst the instruction to visit.
/// \return Inst casted to CallInst if it is a call to an OpenCL C barrier, NULL
///  otherwise.
//===----------------------------------------------------------------------===//
llvm::CallInst *GetAsBarrierCall(llvm::Instruction *Inst);

//===----------------------------------------------------------------------===//
/// FindBarriers - Find all the barriers in the block.
///  \param Block the block to be analyzed.
///  \param Barriers will contain the list of barriers in the block.
//===----------------------------------------------------------------------===//
void FindBarriers(llvm::BasicBlock *Block,
                  std::vector<llvm::CallInst *> &Barriers);

//===----------------------------------------------------------------------===//
/// FindLastBarrier - Find the last barrier in the given block.
///  \param Block the block to be analyzed.
///  \return the call to the barrier function if present, NULL otherwise.
//===----------------------------------------------------------------------===//
llvm::CallInst *FindLastBarrier(llvm::BasicBlock *Block);

//===----------------------------------------------------------------------===//
/// EndsWithAnUniqueBarrier - Check whether Blocks ends with a barrier, that is
///  also the unique barrier in the block.
///  \param Block the basic block to check.
///  \return true if the block contains only a barrier, at its end.
//===----------------------------------------------------------------------===//
bool EndsWithAnUniqueBarrier(llvm::BasicBlock *Block);

//
// Extra.
//

// Controls the execution of the specified action if enable by the user.
#define VERIFY(X) do { if (GetVerifyCPUPassOption()) { X; } } while (0)

} // End namespace opencrun.

#endif // OPENCRUN_DEVICE_CPUPASSES_PASSUTILS_H
