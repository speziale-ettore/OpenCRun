##===- tools/platform-query/Makefile -----------------------*- Makefile -*-===##

LEVEL = ../..

TOOLNAME = platform-query

include $(LEVEL)/Makefile.common

# Define CXX.Flags after including rules, to enable rtti and exceptions.
CXX.Flags := $(filter-out -fno-rtti -fno-exceptions, $(CXX.Flags)) \
             -Wno-cast-qual -lOpenCRun
