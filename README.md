
OpenCRun -- Yet another OpenCL runtime
======================================

OpenCRun is an OpenCL runtime developed for research purpose at
[Politecnico di Milano][1]. It used [LLVM][2] and [CLANG][3] to compile OpenCL
kernels into executable code.

It can support different devices. Right now, only CPU devices (i386 and X86_64)
are supported.

Integration with LLVM/CLANG
---------------------------

The whole runtime heavily exploit LLVM/CLANG libraries and build system.
LLVM/CLANG must be installed before building OpenCRun.

Currently OpenCRun is going to be realigned with LLVM/CLANG trunk, so there may
be impossible building it -- sorry.

Current quality status
----------------------

The runtime is in prototype phase. No extensive tests have been conducted, nor
the full OpenCL API has been implemented. In the `unittests` directory contains
automated tests using OpenCRun through the OpenCL C++ API. Moreover, in the
`bench` directory there are some more complex examples.

Authors
-------

For any question, please contact [Ettore Speziale][4] or [Alberto Magni][5].

[1]: http://www.polimi.it
[2]: http://www.llvm.org
[3]: http://clang.llvm.org
[4]: mailto:speziale.ettore@gmail.com
[5]: mailto:alberto.magni86@gmail.com
