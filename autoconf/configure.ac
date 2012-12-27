dnl===-----------------------------------------------------------------------===
dnl===
dnl    OpenCRun is intended to be integrated into the LLVM building system, so
dnl    the configure script does not contains extensive checks because they have
dnl    been already done by the LLVM configure script.
dnl
dnl    This means that many LLVM configure options are automatically available
dnl    in OpenCRun. For instance, if LLVM was configured for release mode,
dnl    OpenCRun will be built in release mode.
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 1: Initialization & Setup
dnl===
dnl===-----------------------------------------------------------------------===

dnl Initialize autoconf.
AC_INIT([OPENCRUN],[0.1],[speziale.ettore@gmail.com])

dnl Indicate that we require autoconf 2.60 or later.
AC_PREREQ(2.60)

dnl Identify where LLVM source tree is.
LLVM_SRC_ROOT="../.."
LLVM_OBJ_ROOT="../.."

dnl Find absolute paths to LLVM source and object trees.
LLVM_ABS_SRC_ROOT="`cd $srcdir ; cd $LLVM_SRC_ROOT ; pwd`"
LLVM_ABS_OBJ_ROOT="`cd $LLVM_OBJ_ROOT ; pwd`"

dnl Tell autoconf that this is an LLVM project being configured
dnl This provides the --with-llvmsrc and --with-llvmobj options.
LLVM_CONFIG_PROJECT($LLVM_ABS_SRC_ROOT,$LLVM_ABS_OBJ_ROOT)

dnl Try and find an llvm-config in the build directory. We are only using this
dnl to detect the package level LLVM information (currently just the version),
dnl so we just whatever one we find regardless of build mode.
AC_MSG_CHECKING([llvm-config])
llvm_config_path="`ls -1 $llvm_obj/*/bin/llvm-config 2> /dev/null | head -1`"
if ! test -f "$llvm_config_path" ; then
   llvm_config_path="no"
fi
AC_MSG_RESULT([$llvm_config_path])

dnl Determine the LLVM version, which may be required by the current Makefile
dnl rules.
AC_MSG_CHECKING([LLVM package version])
if test "$llvm_config_path" != no ; then
  llvm_package_version=`$llvm_config_path --version`
else
  llvm_package_version="unknown";
fi
AC_MSG_RESULT([$llvm_package_version])
AC_SUBST(LLVM_VERSION, [$llvm_package_version])

dnl Verify that the source directory is valid.
AC_CONFIG_SRCDIR(["Makefile.config.in"])

dnl Place all of the extra autoconf files into the config subdirectory. Tell
dnl various tools where the m4 autoconf macros are.
AC_CONFIG_AUX_DIR([$llvm_src/autoconf])

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 2: Architecture, target, and host checks
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 3: Command line arguments for the configure script.
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 4: Check for programs we need and that they are the right version
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 5: Check for libraries
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 6: Check for header files
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 7: Check for types and structures
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 8: Check for specific functions needed
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 9: Additional checks, variables, etc.
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 10: Specify the output files and generate it
dnl===
dnl===-----------------------------------------------------------------------===

dnl Configure a config Makefile.
AC_CONFIG_FILES(Makefile.config)

dnl Configure project Makefiles.
AC_CONFIG_MAKEFILE(Makefile)
AC_CONFIG_MAKEFILE(Makefile.common)
AC_CONFIG_MAKEFILE(bench/Makefile)
AC_CONFIG_MAKEFILE(lib/Makefile)
AC_CONFIG_MAKEFILE(test/Makefile)
AC_CONFIG_MAKEFILE(test/Makefile.tests)
AC_CONFIG_MAKEFILE(tools/Makefile)
AC_CONFIG_MAKEFILE(unittests/Makefile)
AC_CONFIG_MAKEFILE(utils/Makefile)

dnl This must be last.
AC_OUTPUT
