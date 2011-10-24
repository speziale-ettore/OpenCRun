##===- unittests/Runtime/Makefile --------------------------*- Makefile -*-===##

#
# Relative path to the top of the source tree.
#
LEVEL = ../..

#
# Name of the test.
#
TESTNAME = Runtime

LINK_COMPONENTS = support

include $(LEVEL)/Makefile.config
include $(LLVM_SRC_ROOT)/unittests/Makefile.unittest

#
# Force loading bit-code library from the develop tree.
#
export LLVM_LIB_SEARCH_PATH := $(LibDir)

# Define CXX.Flags after including rules.
CXX.Flags := $(CXX.Flags) -Wno-cast-qual -lOpenCRun \
             $(RPATH) -Wl,$(SharedLibDir)
