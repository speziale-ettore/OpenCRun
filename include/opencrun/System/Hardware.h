
#ifndef OPENCRUN_SYSTEM_HARDWARE_H
#define OPENCRUN_SYSTEM_HARDWARE_H

#include "llvm/Support/Casting.h"
#include "llvm/Support/DOTGraphTraits.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/Path.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringExtras.h"

#include <stack>

namespace opencrun {
namespace sys {

class HardwareComponent {
public:
  enum Type {
    CPU,
    Cache,
    Node
  };

public:
  typedef llvm::SmallPtrSet<HardwareComponent *, 4> LinksContainer;

  typedef LinksContainer::iterator iterator;
  typedef LinksContainer::const_iterator const_iterator;

public:
  iterator begin() { return Links.begin(); }
  iterator end() { return Links.end(); }

  const_iterator begin() const { return Links.begin(); }
  const_iterator end() const { return Links.end(); }

public:
  template <typename Ty>
  class ConstFilteredIterator {
  public:
    // Start constructor. Push all nodes to visit over the stack. Visit cannot
    // pass Bound.
    template <typename Iter>
    ConstFilteredIterator(Iter I, Iter E, const HardwareComponent *Bound) {
      // Initialize visit stack.
      for(; I != E; ++I)
        ToVisit.push(*I);

      // Mark Bound visited, in order to avoid visiting behind nodes.
      Visited.insert(Bound);

      // Force visiting in order to reach the first valid element.
      if(!ToVisit.empty() && !llvm::isa<Ty>(ToVisit.top()))
        Advance();
    }
    
    // End constructor. ToVisit stack is empty.
    ConstFilteredIterator() {}

  public:
    bool operator==(const ConstFilteredIterator &That) const {
      return ToVisit == That.ToVisit;
    }

    bool operator!=(const ConstFilteredIterator &That) const {
      return !(*this == That);
    }

    const Ty &operator*() const { return static_cast<Ty &>(*ToVisit.top()); }
    const Ty *operator->() const { return static_cast<Ty *>(ToVisit.top()); }

    // Pre-increment.
    ConstFilteredIterator<Ty> &operator++() {
      Advance(); return *this;
    }

    // Post-increment.
    ConstFilteredIterator<Ty> operator++(int Ign) {
      ConstFilteredIterator<Ty> That = *this; ++*this; return That;
    }

  private:
    void Advance() {
      if(ToVisit.empty())
        return;

      do {
        const HardwareComponent *Cur = ToVisit.top();
        Visited.insert(Cur);
        ToVisit.pop();

        for(HardwareComponent::iterator I = Cur->begin(),
                                        E = Cur->end();
                                        I != E;
                                        ++I)
          if(!Visited.count(*I))
            ToVisit.push(*I);
      } while(!ToVisit.empty() && !llvm::isa<Ty>(ToVisit.top()));
    }

  private:
    std::stack<const HardwareComponent *> ToVisit;
    std::set<const HardwareComponent *> Visited;
  };

  template <typename Ty>
  class ConstFilteredLinkIterator {
  public:
    ConstFilteredLinkIterator(iterator I, iterator E) : I(I), E(E) {
      // Find first valid position.
      if(I != E && !llvm::isa<Ty>(*this->I))
        Advance();
    }

  public:
    bool operator==(const ConstFilteredLinkIterator &That) const {
      return I == That.I && E == That.E;
    }

    bool operator!=(const ConstFilteredLinkIterator &That) const {
      return !(*this == That);
    }

    const Ty &operator*() const { return static_cast<Ty &>(*I.operator*()); }
    const Ty *operator->() const { return static_cast<Ty *>(&*I.operator*()); }

    // Pre-increment.
    ConstFilteredLinkIterator<Ty> &operator++() {
      Advance(); return *this;
    }

    // Post-increment.
    ConstFilteredLinkIterator<Ty> operator++(int Ign) {
      ConstFilteredLinkIterator<Ty> That = *this; ++*this; return That;
    }

  private:
    void Advance() {
      if(I == E)
        return;

      do { ++I; } while(I != E && !llvm::isa<Ty>(*I));
    }

  private:
    iterator I;
    iterator E;
  };

protected:
  HardwareComponent(Type Ty) : ComponentTy(Ty) { }

public:
  Type GetType() const { return ComponentTy; }

  void AddLink(HardwareComponent *Comp) { Links.insert(Comp); }

  virtual HardwareComponent *GetParent() = 0;
  HardwareComponent *GetAncestor();

private:
  Type ComponentTy;

  LinksContainer Links;
};

class HardwareCache;
class HardwareNode;

class HardwareCPU : public HardwareComponent, public llvm::FoldingSetNode {
public:
  static bool classof(const HardwareComponent *Comp) {
    return Comp->GetType() == HardwareComponent::CPU;
  }

public:
  typedef llvm::SmallVector<unsigned, 4> CPUIDContainer;

  typedef HardwareComponent::LinksContainer CPUContainer;

public:
  HardwareCPU(unsigned CoreID) : HardwareComponent(HardwareComponent::CPU),
                                 CoreID(CoreID) { }

public:
  void Profile(llvm::FoldingSetNodeID &ID) const { ID.AddInteger(CoreID); }

  virtual HardwareComponent *GetParent() { return GetFirstLevelMemory(); }

  unsigned GetCoreID() const { return CoreID; }

  HardwareComponent *GetFirstLevelMemory() const ;
  HardwareComponent *GetLastLevelMemory() { return GetAncestor(); }

private:
  unsigned CoreID;
};

class HardwareCache : public HardwareComponent, public llvm::FoldingSetNode {
public:
  static bool classof(const HardwareComponent *Comp) {
    return Comp->GetType() == HardwareComponent::Cache;
  }

public:
  enum Kind {
    Instruction = 1 << 0,
    Data        = 1 << 1,
    Unified     = Instruction | Data,
    Invalid     = ~(Unified)
  };

public:
  typedef HardwareComponent::ConstFilteredIterator<const HardwareCPU>
          const_cpu_iterator;

public:
  const_cpu_iterator cpu_begin() const {
    llvm::SmallVector<HardwareComponent *, 4> Comps;

    for(iterator I = begin(), E = end(); I != E; ++I) {
      HardwareComponent *Comp = *I;

      if(!llvm::isa<HardwareNode>(Comp))
        Comps.push_back(Comp);
    }

    return const_cpu_iterator(Comps.begin(), Comps.end(), this);
  }

  const_cpu_iterator cpu_end() const {
    return const_cpu_iterator();
  }

public:
  HardwareCache(unsigned Level,
                Kind CacheKind,
                HardwareComponent::LinksContainer &CPUs)
    : HardwareComponent(HardwareComponent::Cache),
      Level(Level),
      CacheKind(CacheKind),
      CPUs(CPUs) { }

public:
  void Profile(llvm::FoldingSetNodeID &ID) const {
    ID.AddInteger(Level);
    ID.AddInteger(CacheKind);

    for(HardwareComponent::iterator I = CPUs.begin(),
                                    E = CPUs.end();
                                    I != E;
                                    ++I)
      ID.AddPointer(*I);
  }

  virtual HardwareComponent *GetParent() { return GetNextLevelMemory(); }

  unsigned GetLevel() const { return Level; }
  Kind GetKind() const { return CacheKind; }

  void SetSize(size_t Size) { this->Size = Size; }
  size_t GetSize() const { return Size; }

  void SetLineSize(size_t LineSize) { this->LineSize = LineSize; }
  size_t GetLineSize() const { return LineSize; }

  HardwareComponent *GetNextLevelMemory();

  bool IsFirstLevelCache() const { return Level == 1; }

private:
  unsigned Level;
  Kind CacheKind;
  HardwareComponent::LinksContainer CPUs;

  size_t Size;
  size_t LineSize;
};

class HardwareNode : public HardwareComponent, public llvm::FoldingSetNode {
public:
  static bool classof(const HardwareComponent *Comp) {
    return Comp->GetType() == HardwareComponent::Node;
  }

public:
  typedef HardwareComponent::ConstFilteredIterator<HardwareCPU>
          const_cpu_iterator;

  typedef HardwareComponent::ConstFilteredLinkIterator<HardwareCache>
          const_llc_iterator;

public:
  const_cpu_iterator cpu_begin() const {
    llvm::SmallVector<HardwareComponent *, 2> Comps;

    for(iterator I = begin(), E = end(); I != E; ++I) {
      HardwareComponent *Comp = *I;

      if(!llvm::isa<HardwareNode>(Comp))
        Comps.push_back(Comp);
    }

    return const_cpu_iterator(begin(), end(), this);
  }

  const_cpu_iterator cpu_end() const {
    return const_cpu_iterator();
  }

  const_llc_iterator llc_begin() const {
    return const_llc_iterator(begin(), end());
  }
  const_llc_iterator llc_end() const {
    return const_llc_iterator(end(), end());
  }

  const HardwareCache &llc_front() const;
  const HardwareCache &llc_back() const;

public:
  HardwareNode(unsigned NodeID) : HardwareComponent(HardwareComponent::Node),
                                  NodeID(NodeID) { }

public:
  void Profile(llvm::FoldingSetNodeID &ID) const { ID.AddInteger(NodeID); }

  virtual HardwareComponent *GetParent() { return NULL; }

  unsigned GetNodeID() const { return NodeID; }

  void SetMemorySize(size_t Size) { MemorySize = Size; }
  size_t GetMemorySize() const { return MemorySize; }

  unsigned CPUsCount() const;

private:
  unsigned NodeID;
  size_t MemorySize;
};

class Hardware {
public:
  static size_t GetPageSize();
  static size_t GetCacheLineSize();
  static size_t GetMaxNaturalAlignment();

public:
  typedef std::set<HardwareComponent *> ComponentContainer;

  typedef ComponentContainer::iterator component_iterator;
  typedef ComponentContainer::const_iterator const_component_iterator;

  typedef llvm::FoldingSet<HardwareCPU> CPUComponents;
  typedef llvm::FoldingSet<HardwareCache> CacheComponents;
  typedef llvm::FoldingSet<HardwareNode> NodeComponents;

public:
  component_iterator component_begin() { return Components.begin(); }
  component_iterator component_end() { return Components.end(); }

  const_component_iterator component_begin() const { return Components.begin(); }
  const_component_iterator component_end() const { return Components.end(); }

public:
  Hardware();
  ~Hardware();

public:
  void View() { llvm::ViewGraph(*this, "hardware-graph"); }

private:
  ComponentContainer Components;

public:
  template <typename Ty>
  class FilteredComponentIterator {
  public:
    FilteredComponentIterator(component_iterator I,
                              component_iterator E) : I(I),
                                                      E(E) {
      // Find first valid position.
      if(I != E && !llvm::isa<Ty>(*this->I))
        Advance();
    }

  public:
    bool operator==(const FilteredComponentIterator<Ty> &That) const {
      return I == That.I && E == That.E;
    }
    bool operator!=(const FilteredComponentIterator<Ty> &That) const {
      return !(*this == That);
    }

    Ty &operator*() const { return static_cast<Ty &>(*I.operator*()); }
    Ty *operator->() const { return static_cast<Ty *>(*I.operator->()); }

    // Pre-increment.
    FilteredComponentIterator<Ty> &operator++() {
      Advance(); return *this;
    }

    // Post-increment.
    FilteredComponentIterator<Ty> operator++(int Ign) {
      FilteredComponentIterator<Ty> That = *this; ++*this; return That;
    }

  private:
    void Advance() {
      if(I == E)
        return;

      do { ++I; } while(I != E && !llvm::isa<Ty>(*I));
    }

  private:
    component_iterator I;
    component_iterator E;
  };

public:
  typedef FilteredComponentIterator<HardwareNode> node_iterator;
  typedef FilteredComponentIterator<HardwareCache> cache_iterator;

public:
  node_iterator node_begin() {
    return node_iterator(component_begin(), component_end());
  }
  node_iterator node_end() {
    return node_iterator(component_end(), component_end());
  }

  cache_iterator cache_begin() {
    return cache_iterator(component_begin(), component_end());
  }
  cache_iterator cache_end() {
    return cache_iterator(component_end(), component_end());
  }
};

Hardware &GetHardware();

} // End namespace sys.
} // End namespace opencrun.

// *GraphTraits must be specialized into llvm namespace.
namespace llvm {

using namespace opencrun::sys;

template <>
struct GraphTraits<Hardware> {
public:
  typedef HardwareComponent NodeType;

  typedef Hardware::const_component_iterator nodes_iterator;

  typedef HardwareComponent::iterator ChildIteratorType;

public:
  static nodes_iterator nodes_begin(const Hardware &HW) {
    return HW.component_begin();
  }
  static nodes_iterator nodes_end(const Hardware &HW) {
    return HW.component_end();
  }

  static ChildIteratorType child_begin(HardwareComponent *Comp) {
    return Comp->begin();
  }
  static ChildIteratorType child_end(HardwareComponent *Comp) {
    return Comp->end();
  }
};

template <>
struct DOTGraphTraits<Hardware> : public DefaultDOTGraphTraits {
public:
  DOTGraphTraits(bool Simple = false) : DefaultDOTGraphTraits(Simple) { }

public:
  std::string getGraphName(const Hardware &HW) { return "Hardware Topology"; }

  std::string getNodeLabel(HardwareComponent *Node, const Hardware &HW) {
    if(HardwareCPU *HWCPU = llvm::dyn_cast<HardwareCPU>(Node))
      return "CPU-" + llvm::utostr(HWCPU->GetCoreID());
    else if(HardwareCache *HWCache = llvm::dyn_cast<HardwareCache>(Node))
      return "L" + llvm::utostr(HWCache->GetLevel()) + "-Cache";
    else if(HardwareNode *HWNode = llvm::dyn_cast<HardwareNode>(Node)) {
      return "Node-" + llvm::utostr(HWNode->GetNodeID());
    }

    return "";
  }
};

} // End namespace llvm.

#endif // OPENCRUN_SYSTEM_HARDWARE_H
