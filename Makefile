##===- ./Makefile ------------------------------------------*- Makefile -*-===##
#
# This is the main Makefile for the OpenCl runtime implementation.
#
##===----------------------------------------------------------------------===##

#
# Indicates our relative path to the top of the project's root directory.
#
LEVEL = .
DIRS = utils lib tools unittests bench
EXTRA_DIST = include

#
# Include the Master Makefile that knows how to build all.
#
include $(LEVEL)/Makefile.common
