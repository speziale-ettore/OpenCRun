##===- lib/Device/CPULib/Makefile --------------------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL = ../../..

#
# Give the name of a library.
#
MODULE_NAME = opencrunCPULib

#
# The ocldef is in fact tblgen, so declare we want tblgen support to enable .inc
# files handling.
#
TABLEGEN_INC_FILES_COMMON = 1

#
# Auto-generate some libraries.
#
BUILT_SOURCES = Math.inc

#
# The LLVM build system does not known recognize .cl files automatically.
#
SOURCES = WorkItem.cl Synchronization.cl Math.cl

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

$(ObjDir)/Math.inc.tmp: $(PROJ_SRC_ROOT)/include/opencrun/Device/GenLib/Math.td \
                        $(OCLGEN) $(ObjDir)/.dir
	$(Echo) "Building Math library with oclgen"
	$(Verb) $(OCLGen) -gen-ocl-lib-impl -o $(call SYSPATH, $@) $<
