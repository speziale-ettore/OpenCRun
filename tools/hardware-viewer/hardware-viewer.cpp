
#include <cstdlib>
#include <iostream>

#include "opencrun/System/Hardware.h"

int main(int argc, char *argv[]) {
  opencrun::sys::Hardware &HW = opencrun::sys::GetHardware();

  std::cout << "Building hardware graph...  done." << std::endl;
  HW.View();
  std::cout << " done." << std::endl;

  return EXIT_SUCCESS;
}
