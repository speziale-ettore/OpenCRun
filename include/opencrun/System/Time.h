
#ifndef OPENCRUN_SYSTEM_TIME_H
#define OPENCRUN_SYSTEM_TIME_H

#include "opencrun/Util/Table.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/DataTypes.h"

namespace opencrun {
namespace sys {

class Time {
public:
  static const int64_t NanoInSeconds;

public:
  static Time GetWallClock();

public:
  enum {
    InvalidTime = INT64_MIN
  };

public:
  Time() : Seconds(InvalidTime), NanoSeconds(InvalidTime) { }
  Time(int64_t Seconds,
       int64_t NanoSeconds) : Seconds(Seconds + NanoSeconds / NanoInSeconds),
                              NanoSeconds(NanoSeconds % NanoInSeconds) { }

public:
  bool IsValid() const { return Seconds >= 0 && NanoSeconds >= 0; }

  double AsDouble() const {
    return Seconds + (double) NanoSeconds / NanoInSeconds;
  }

private:
  int64_t Seconds;
  int64_t NanoSeconds;

  friend Time operator-(const Time &RHS1, const Time &RHS2);
};

inline Time operator-(const Time &RHS1, const Time &RHS2) {
  return Time(RHS1.Seconds - RHS2.Seconds, RHS1.NanoSeconds - RHS2.NanoSeconds);
}

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const Time& TM) {
  if(TM.IsValid())
    return OS << TM.AsDouble();
  else
    return OS << "N\\A";
}

inline opencrun::util::Table &operator<<(opencrun::util::Table &Tab,
                                         const Time &TM) {
  if(TM.IsValid())
    return Tab << TM.AsDouble();
  else
    return Tab << "N\\A";
}

} // End namespace sys.
} // End namespace opencrun.

#endif // OPENCRUN_SYSTEM_TIME_H
