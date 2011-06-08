
#include "opencrun/System/Hardware.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/system_error.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"

#include <algorithm>
#include <limits>

using namespace opencrun::sys;

namespace {

class SimpleParser {
public:
  typedef llvm::StringMap< llvm::SmallString<8> > KeyValueContainer;

protected:
  // Parse a list of generic integers IntTy.
  template <typename IntTy>
  bool ParseList(llvm::sys::Path &File,
                 llvm::SmallVector<IntTy, 4> &Nums,
                 IntTy Discard = std::numeric_limits<IntTy>::max()) {
    llvm::OwningPtr<llvm::MemoryBuffer> Buf;

    if(llvm::MemoryBuffer::getFile(File.str(), Buf))
      return false;

    // Parse list elements
    for(const char *I = Buf->getBufferStart(),
                   *E = Buf->getBufferEnd(),
                   *J = I;
                   I != E;
                   ++I) {
      // Currently reading a list element.
      if(std::isdigit(*I))
        continue;

      // Comma is the internal separator, the list is ended by '\n'.
      if(*I != ',' && *I != '\n')
        return false;

      // Parse current number.
      llvm::StringRef Num(J, I - J);
      IntTy ParsedNum;
      if(Num.getAsInteger(0, ParsedNum))
        return false;

      // Add only if not poisoned.
      if(ParsedNum != Discard)
        Nums.push_back(ParsedNum);

      // Remember current position, for the next parse.
      J = I + 1;
    }

    return true;
  }

  // Parse a generic integer IntTy.
  template <typename IntTy>
  bool ParseNumber(const llvm::sys::Path &File, IntTy &Num) {
    llvm::OwningPtr<llvm::MemoryBuffer> Buf;
    llvm::StringRef RawNum;

    if(llvm::MemoryBuffer::getFile(File.str(), Buf))
      return false;

    // Find the only number.
    for(const char *I = Buf->getBufferStart(),
                   *E = Buf->getBufferEnd(),
                   *J = I;
                   I != E;
                   ++I) {
      // Still parsing a number;
      if(std::isdigit(*I))
        continue;

      // The file should contains only a number.
      if(*I != '\n')
        return false;

      // Parse the number.
      llvm::StringRef RawNum(J, I - J);
      if(RawNum.getAsInteger(0, Num))
        return false;
    }

    return true;
  }

  // Parse a dimension, from a file.
  bool ParseSize(const llvm::sys::Path &File, size_t &Size) {
    llvm::OwningPtr<llvm::MemoryBuffer> Buf;

    if(llvm::MemoryBuffer::getFile(File.str(), Buf))
      return false;

    return ParseSize(Buf->getBuffer(), Size);
  }

  // Parse a dimension, from a String.
  bool ParseSize(llvm::StringRef Str, size_t &Size) {
    unsigned I, J;

    Str = TrimSpaces(Str);

    for(I = 0; I < Str.size() && std::isdigit(Str[I]); ++I) { }
    llvm::StringRef RawSize = Str.substr(0, I);

    for(J = I; J < Str.size() && std::isspace(Str[J]); ++J) { }
    llvm::StringRef RawMult = Str.substr(J);

    if(RawSize.getAsInteger(0, Size))
      return false;

    Size *= llvm::StringSwitch<unsigned>(RawMult)
              .Cases("K", "kB", 1024)
              .Default(0);

    return Size != 0;
  }

  // Parse a <String,String> dictionary.
  bool ParseKeyValue(const llvm::sys::Path &File, KeyValueContainer &Entries) {
    llvm::OwningPtr<llvm::MemoryBuffer> Buf;

    // Force using a 2K file size; this allows to memory mapping /proc files.
    if(llvm::MemoryBuffer::getFile(File.str(), Buf, 2 * 1024))
      return false;

    llvm::StringRef RawBuffer = Buf->getBuffer();
    std::pair<llvm::StringRef, llvm::StringRef> Lines(RawBuffer.split('\n'));

    // Parse file line per line.
    for(llvm::StringRef Cur = Lines.first,
                        Next = Lines.second;
                        !Cur.empty();
                        Lines = Next.split('\n'),
                        Cur = Lines.first,
                        Next = Lines.second) {
      std::pair<llvm::StringRef, llvm::StringRef> Entry = Cur.split(':');

      llvm::StringRef Key = TrimSpaces(Entry.first);
      llvm::StringRef Value = TrimSpaces(Entry.second);

      if(Key.empty() || Value.empty())
        return false;

      Entries[Key] = Value;
    }

    return true;
  }

  // Detect string bounds.
  llvm::StringRef TrimSpaces(llvm::StringRef Str) {
    unsigned I, J;

    for(I = 0; I < Str.size() && std::isspace(Str[I]); ++I) { }
    for(J = Str.size(); J > 0 && std::isspace(Str[J - 1]); --J) { }

    return Str.slice(I, J);
  }
};

class CPUVisitor : public SimpleParser {
public:
  CPUVisitor(Hardware::CPUComponents &CPUs,
             Hardware::CacheComponents &Caches,
             Hardware::NodeComponents &Nodes) : CPUs(CPUs),
                                                Caches(Caches),
                                                Nodes(Nodes) { }

public:
  HardwareCPU *operator()(const llvm::sys::Path &CPUPath);

private:
  HardwareCPU *ParseCPU(const llvm::sys::Path &CPUPath);
  void ParseCaches(const llvm::sys::Path &CPUPath);
  void ParseNode(const llvm::sys::Path &CPUPath);

private:
  Hardware::CPUComponents &CPUs;
  Hardware::CacheComponents &Caches;
  Hardware::NodeComponents &Nodes;

private:
  class IsNodeDirectory {
  public:
    bool operator()(const llvm::sys::Path &NodePath) {
      llvm::StringRef Name = NodePath.getLast();
      unsigned ID;

      if(!NodePath.isDirectory())
        return false;

      if(!Name.startswith("node"))
        return false;

      return !Name.substr(4).getAsInteger(0, ID);
    }
  };
};

class CacheVisitor : public SimpleParser {
public:
  typedef llvm::SmallPtrSet<HardwareCache *, 4> CacheContainer;
  typedef llvm::DenseMap<unsigned, CacheContainer> CacheByLevelIndex;

public:
  CacheVisitor(Hardware::CPUComponents &CPUs,
               Hardware::CacheComponents &Caches,
               Hardware::NodeComponents &Nodes) : CPUs(CPUs),
                                                  Caches(Caches),
                                                  Nodes(Nodes) { }

public:
  HardwareCache *operator()(const llvm::sys::Path &CachePath);

private:
  bool ParseKind(const llvm::sys::Path &File, HardwareCache::Kind &Kind);

private:
  Hardware::CPUComponents &CPUs;
  Hardware::CacheComponents &Caches;
  Hardware::NodeComponents &Nodes;

  CacheByLevelIndex ByLevelIndex;
};

class NUMAVisitor : public SimpleParser {
protected:
  NUMAVisitor(Hardware::CPUComponents &CPUs,
              Hardware::CacheComponents &Caches,
              Hardware::NodeComponents &Nodes) : CPUs(CPUs),
                                                 Caches(Caches),
                                                 Nodes(Nodes) { }
protected:
  Hardware::CPUComponents &CPUs;
  Hardware::CacheComponents &Caches;
  Hardware::NodeComponents &Nodes;
};

class NUMAAwareVisitor : public NUMAVisitor {
public:
  NUMAAwareVisitor(Hardware::CPUComponents &CPUs,
                   Hardware::CacheComponents &Caches,
                   Hardware::NodeComponents &Nodes)
    : NUMAVisitor(CPUs, Caches, Nodes) { }

public:
  void operator()(const llvm::sys::Path &NodePath);
};

class NUMANotAwareVisitor : public NUMAVisitor {
public:
  NUMANotAwareVisitor(Hardware::CPUComponents &CPUs,
                      Hardware::CacheComponents &Caches,
                      Hardware::NodeComponents &Nodes)
   : NUMAVisitor(CPUs, Caches, Nodes) { }

public:
  void operator()(const llvm::sys::Path &NodePath);
};

//
// CPUVistor implementation.
//

HardwareCPU *CPUVisitor::operator()(const llvm::sys::Path &CPUPath) {
  if(!CPUPath.isDirectory())
    return NULL;

  HardwareCPU *CPU = ParseCPU(CPUPath);
  if(!CPU)
    return NULL;

  ParseCaches(CPUPath);
  ParseNode(CPUPath);

  return CPU;
}

HardwareCPU *CPUVisitor::ParseCPU(const llvm::sys::Path &CPUPath) {
  llvm::FoldingSetNodeID ID;
  void *InsertPoint;

  // The CoreID is the CPU identifier for the operating system.
  unsigned CoreID;
  if(CPUPath.getLast().substr(3).getAsInteger(0, CoreID))
    return NULL;

  // Try looking if CPU has already been added.
  ID.AddInteger(CoreID);
  HardwareCPU *CPU = CPUs.FindNodeOrInsertPos(ID, InsertPoint);
  if(CPU)
    return CPU;

  // Switch to "topology" directory.
  llvm::sys::Path Topology(CPUPath);
  if(!Topology.appendComponent("topology"))
    return NULL;

  // Find other CPUs on the same core.
  llvm::sys::Path CoreSiblingsPath(Topology);
  if(!CoreSiblingsPath.appendComponent("core_siblings_list"))
    return NULL;

  // TODO: Integer to objects.
  HardwareCPU::CPUIDContainer CoreSiblingsID;
  if(!ParseList(CoreSiblingsPath, CoreSiblingsID, CoreID))
    return NULL;

  // Find HyperThreaded sibling, if any.
  llvm::sys::Path HTSiblingsPath(Topology);
  if(!HTSiblingsPath.appendComponent("thread_siblings_list"))
    return NULL;

  // TODO: Integer to objects.
  HardwareCPU::CPUIDContainer HTSiblingsID;
  if(!ParseList(HTSiblingsPath, HTSiblingsID, CoreID))
    return NULL;

  // Mark CPU as visited.
  CPU = new HardwareCPU(CoreID);
  CPUs.InsertNode(CPU, InsertPoint);

  return CPU;
}

void CPUVisitor::ParseCaches(const llvm::sys::Path &CPUPath) {
  std::set<llvm::sys::Path> CachePaths;

  // Switch to "cache" directory.
  llvm::sys::Path Cache(CPUPath);
  if(!Cache.appendComponent("cache"))
    return;

  // Get caches.
  if(Cache.getDirectoryContents(CachePaths, NULL))
    return;

  std::for_each(CachePaths.begin(),
                CachePaths.end(),
                CacheVisitor(CPUs, Caches, Nodes));
}

void CPUVisitor::ParseNode(const llvm::sys::Path &CPUPath) {
  std::set<llvm::sys::Path> Paths;

  if(CPUPath.getDirectoryContents(Paths, NULL))
    return;

  std::set<llvm::sys::Path>::iterator I = Paths.begin(),
                                      E = Paths.end(),
                                      J;
  J = std::find_if(I, E, IsNodeDirectory());

  // The kernel exports nodes about NUMA nodes.
  if(J != E)
    NUMAAwareVisitor(CPUs, Caches, Nodes)(*J);

  // Old kernels, use the old way.
  else
    NUMANotAwareVisitor(CPUs, Caches, Nodes)(llvm::sys::Path("/proc"));
}

//
// CacheVisitor implementation.
//

HardwareCache *CacheVisitor::operator()(const llvm::sys::Path &CachePath) {
  llvm::FoldingSetNodeID ID;
  void *InsertPoint;

  // Each cache has its own directory.
  if(!CachePath.isDirectory())
    return NULL;

  // Cache directory format is "indexN".
  unsigned IndexID;
  if(CachePath.getLast().substr(5).getAsInteger(0, IndexID))
    return NULL;

  // Find level.
  llvm::sys::Path LevelPath(CachePath);
  if(!LevelPath.appendComponent("level"))
    return NULL;
  unsigned Level;
  if(!ParseNumber(LevelPath, Level))
    return NULL;

  // Find cache kind.
  llvm::sys::Path TypePath(CachePath);
  if(!TypePath.appendComponent("type"))
    return NULL;
  HardwareCache::Kind Kind;
  if(!ParseKind(TypePath, Kind))
    return NULL;

  // Find cache size.
  llvm::sys::Path SizePath(CachePath);
  if(!SizePath.appendComponent("size"))
    return NULL;
  size_t Size;
  if(!ParseSize(SizePath, Size))
    return NULL;

  HardwareComponent::LinksContainer ServedComps;

  // First level cache: directly connected to cores.
  if(Level == 1) {
    llvm::sys::Path SharedCPUPath(CachePath);
    if(!SharedCPUPath.appendComponent("shared_cpu_list"))
      return NULL;
    HardwareCPU::CPUIDContainer CoreIDs;
    if(!ParseList(SharedCPUPath, CoreIDs))
      return NULL;

    // Get served cores high level description.
    for(HardwareCPU::CPUIDContainer::iterator I = CoreIDs.begin(),
                                              E = CoreIDs.end();
                                              I != E;
                                              ++I) {
      llvm::FoldingSetNodeID CPUID;

      CPUID.AddInteger(*I);
      HardwareCPU *CPU = CPUs.FindNodeOrInsertPos(CPUID, InsertPoint);

      // CPU not yet built: force building.
      if(!CPU) {
        llvm::sys::Path CPURoot(HardwareCPU::CPUsRoot);
        CPURoot.appendComponent("cpu" + llvm::utostr(*I));

        CPU = CPUVisitor(CPUs, Caches, Nodes)(CPURoot);
      }

      // Remember link to CPU.
      ServedComps.insert(CPU);
    }

  // Second, or higher level cache, connected to lower level caches.
  } else {
    CacheContainer &LLCaches = ByLevelIndex[Level - 1];

    // Get lower level caches.
    for(CacheContainer::iterator I = LLCaches.begin(),
                                 E = LLCaches.end();
                                 I != E;
                                 ++I)
      // Insert only "compatible" caches.
      if((*I)->GetKind() & Kind)
        ServedComps.insert(*I);
  }

  // Try looking for the cache.
  ID.AddInteger(Level);
  ID.AddInteger(Kind);

  for(HardwareComponent::iterator I = ServedComps.begin(),
                                  E = ServedComps.end();
                                  I != E;
                                  ++I)
    ID.AddPointer(*I);

  // Cache already built.
  if(Caches.FindNodeOrInsertPos(ID, InsertPoint))
    return NULL;

  // Get cache-line size.
  llvm::sys::Path LineSizePath(CachePath);
  if(!LineSizePath.appendComponent("coherency_line_size"))
    return NULL;
  size_t LineSize;
  if(!ParseNumber(LineSizePath, LineSize))
    return NULL;

  // Build cache.
  HardwareCache *Cache = new HardwareCache(Level, Kind, Size, ServedComps);
  Caches.InsertNode(Cache, InsertPoint);

  // Connect to lower levels.
  for(HardwareComponent::iterator I = ServedComps.begin(),
                                  E = ServedComps.end();
                                  I != E;
                                  ++I)
    (*I)->AddLink(Cache);

  // Add non-essential infos.
  Cache->SetLineSize(LineSize);

  // Remember this level cache.
  ByLevelIndex[Level].insert(Cache);

  return Cache;
}

bool CacheVisitor::ParseKind(const llvm::sys::Path &File,
                             HardwareCache::Kind &Kind) {
  llvm::OwningPtr<llvm::MemoryBuffer> Buf;

  if(llvm::MemoryBuffer::getFile(File.str(), Buf))
    return false;

  llvm::StringRef RawKind(Buf->getBufferStart(), Buf->getBufferSize() - 1);
  Kind = llvm::StringSwitch<HardwareCache::Kind>(RawKind)
           .Case("Instruction", HardwareCache::Instruction)
           .Case("Data", HardwareCache::Data)
           .Case("Unified", HardwareCache::Unified)
           .Default(HardwareCache::Invalid);

  return Kind != HardwareCache::Invalid;
}

//
// NUMAAwareVisitor implementation.
//

void NUMAAwareVisitor::operator()(const llvm::sys::Path &NodePath) { }

//
// NUMANotAwareVisitor implementation.
//

void NUMANotAwareVisitor::operator()(const llvm::sys::Path &NodePath) {
  llvm::FoldingSetNodeID ID;
  void *InsertPoint;

  // Only the "0" node.
  ID.AddInteger(0);
  HardwareNode *Node = Nodes.FindNodeOrInsertPos(ID, InsertPoint);
  if(Node)
    return;

  // Invalid path.
  if(!NodePath.isDirectory())
    return;

  // We expect to find /proc/meminfo.
  llvm::sys::Path MemInfoPath(NodePath);
  if(!MemInfoPath.appendComponent("meminfo"))
    return;

  // Read it.
  KeyValueContainer Values;
  if(!ParseKeyValue(MemInfoPath, Values))
    return;

  // Find total installed memory size.
  size_t Size;
  if(!ParseSize(Values["MemTotal"], Size))
    return;

  // Register the node.
  Node = new HardwareNode(0, Size);
  Nodes.InsertNode(Node, InsertPoint);

  // Setup links.
  for(Hardware::CPUComponents::iterator I = CPUs.begin(),
                                        E = CPUs.end();
                                        I != E;
                                        ++I) {
    HardwareComponent *LLMem = I->GetLastLevelMemory();

    Node->AddLink(LLMem);
    LLMem->AddLink(Node);
  }
}

} // End anonymous namespace.

//
// HardwareComponent implementation.
//

HardwareComponent *HardwareComponent::GetAncestor() {
  HardwareComponent *Comp, *Anc;

  for(Comp = this, Anc = NULL; Comp; Anc = Comp, Comp = Comp->GetParent()) { }

  return Anc;
}

//
// HardwareCPU implementation.
//

HardwareComponent *HardwareCPU::GetFirstLevelMemory() {
  for(HardwareComponent::iterator I = begin(), E = end(); I != E; ++I) {
    HardwareComponent *Comp = *I;

    if(!llvm::isa<HardwareCPU>(*I))
      return Comp;

  }

  return NULL;
}

const llvm::sys::Path HardwareCPU::CPUsRoot("/sys/devices/system/cpu");

//
// HardwareCache implementation.
//

HardwareComponent *HardwareCache::GetNextLevelMemory() {
  for(HardwareComponent::iterator I = begin(), E = end(); I != E; ++I) {
    if(HardwareNode *Node = llvm::dyn_cast<HardwareNode>(*I))
      return Node;

    if(HardwareCache *Cache = llvm::dyn_cast<HardwareCache>(*I))
      if(Cache->GetLevel() > Level)
        return Cache;
  }

  return NULL;
}

//
// HardwareNode implementation.
//

const HardwareCache &HardwareNode::llc_front() const {
  return *llc_begin();
}

const HardwareCache &HardwareNode::llc_back() const {
  const_llc_iterator I = llc_begin(),
                     E = llc_end(),
                     J = I;

  for(; I != E; ++I) { }

  return *J;
}

unsigned HardwareNode::CPUsCount() const {
  unsigned N = 0;

  for(const_cpu_iterator I = cpu_begin(), E = cpu_end(); I != E; ++I)
    N++;

  return N;
}

//
// Hardware implementation.
//

size_t Hardware::GetPageSize() {
  return getpagesize();
}

size_t Hardware::GetCacheLineSize() {
  Hardware &HW = GetHardware();
  Hardware::cache_iterator I = HW.cache_begin(),
                           E = HW.cache_end();

  return I != E ? I->GetLineSize() : 0;
}

Hardware::Hardware() {
  std::set<llvm::sys::Path> CPUsPath;

  if(HardwareCPU::CPUsRoot.getDirectoryContents(CPUsPath, NULL))
    llvm::report_fatal_error("Cannot read CPUs root directory");

  Hardware::CPUComponents CPUs;
  Hardware::CacheComponents Caches;
  Hardware::NodeComponents Nodes;

  std::for_each(CPUsPath.begin(),
                CPUsPath.end(),
                CPUVisitor(CPUs, Caches, Nodes));
  
  for(Hardware::CPUComponents::iterator I = CPUs.begin(),
                                        E = CPUs.end();
                                        I != E;
                                        ++I)
    Components.insert(&*I);
  for(Hardware::CacheComponents::iterator I = Caches.begin(),
                                          E = Caches.end();
                                          I != E;
                                          ++I)
    Components.insert(&*I);
  for(Hardware::NodeComponents::iterator I = Nodes.begin(),
                                         E = Nodes.end();
                                         I != E;
                                         ++I)
    Components.insert(&*I);
}

Hardware::~Hardware() {
  llvm::DeleteContainerPointers(Components);
}

llvm::ManagedStatic<Hardware> HW;

Hardware &opencrun::sys::GetHardware() {
  return *HW;
}
