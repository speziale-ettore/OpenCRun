
#include "opencrun/System/Env.h"

#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Mutex.h"

#include <cstdlib>

using namespace opencrun::sys;

static llvm::ManagedStatic<llvm::sys::Mutex> EnvLock;

bool opencrun::sys::HasEnv(llvm::StringRef Name) {
  llvm::sys::ScopedLock Lock(*EnvLock);

  return std::getenv(Name.begin());
}

std::string opencrun::sys::GetEnv(llvm::StringRef Name) {
  llvm::sys::ScopedLock Lock(*EnvLock);

  std::string Var;

  if(const char *Str = std::getenv(Name.begin()))
    Var = Str;

  return Var;
}
