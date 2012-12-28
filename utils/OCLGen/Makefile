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

LINK_COMPONENTS = tablegen support

#
# This tool has no plugins, optimize startup time.
#
TOOL_NO_EXPORTS = 1

include $(LEVEL)/Makefile.common
