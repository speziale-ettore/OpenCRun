
#ifndef OPENCRUN_SYSTEM_FASTRENDEVOUZ_H
#define OPENCRUN_SYSTEM_FASTRENDEVOUZ_H

namespace opencrun {
namespace sys {

class FastRendevouz {
public:
  FastRendevouz() : Meet(false) { }

  FastRendevouz(const FastRendevouz &That); // Do not implement.
  void operator=(const FastRendevouz &That); // Do not implement.

public:
  void Signal() { Meet = true; }
  void Wait() const { while(!Meet) { } }

private:
  volatile bool Meet;
};

} // End namespace sys.
} // End namespace opencrun.

#endif // OPENCRUN_SYSTEM_FAST_RENDEVOUZ_H
