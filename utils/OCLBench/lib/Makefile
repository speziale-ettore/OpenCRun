##===- utils/OCLBench/lib/Makefile -------------------------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
# This tool is just a customization of tblgen in order to add some OpenCL
# specific back-ends.
##===----------------------------------------------------------------------===##

LEVEL = ../../..

LIBRARYNAME = OCLBench

BUILD_ARCHIVE = 1

#
# We need exceptions/rtti to catch OpenCL errors.
#
REQUIRES_EH = 1
REQUIRES_RTTI = 1

#
# Do not install this library.
#
NO_INSTALL = 1

include $(LEVEL)/Makefile.common

CPP.Flags := $(CPP.Flags) -I$(PROJ_SRC_ROOT)/utils/OCLBench/include
CXX.Flags := $(CXX.Flags) -Wno-cast-qual
