##===- utils/OCLGen/Makefile -------------------------------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
# This tool is just a customization of tblgen in order to add some OpenCL
# specific back-ends.
##===----------------------------------------------------------------------===##

LEVEL = ../..

TOOLNAME = oclgen

LINK_COMPONENTS = support

REQUIRES_EH = 1
REQUIRES_RTTI = 1

#
# List all sources here, in order to force looking files not stored in current
# directory.
#
SOURCES = Error.cpp Record.cpp SetTheory.cpp TGLexer.cpp TGParser.cpp \
          TableGenBackend.cpp \
          OCLGen.cpp OCLLibBuiltin.cpp OCLLibEmitter.cpp

#
# This tool has no plugins, optimize startup time.
#
TOOL_NO_EXPORTS = 1

#
# Include files must be also searched in the TableGen directory.
#
CPP.Flags += -I$(LLVM_SRC_ROOT)/utils/TableGen

include $(LEVEL)/Makefile.common

#
# Add TableGen source dir to the VPATH, in order to find original TableGen
# implementation.
#
VPATH += :$(LLVM_SRC_ROOT)/utils/TableGen
