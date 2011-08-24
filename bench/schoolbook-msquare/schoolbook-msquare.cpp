
#include "oclbench/Benchmark.h"
#include "oclbench/DataTypes.h"

#include "llvm/Support/CommandLine.h"

using namespace oclbench;

static llvm::cl::opt<bool> DeviceOverlap("device-overlap",
                                         llvm::cl::init(false),
                                         llvm::cl::desc("Whether to overlap "
                                                        "benchmarking with "
                                                        "oracle computation"));

namespace {

class SchoolbookMSquare : public Benchmark {
public:
  static const cl_uint ASide = 8;
  static const cl_uint BSide = 16;
  static const cl_uint CSide = 32;

public:
  SchoolbookMSquare(int argc, char *argv[]) : Benchmark(argc, argv) { }

public:
  virtual BenchmarkResult *Run(Benchmark::Class Cls);

private:
  cl_uint GetSizeForClass(Benchmark::Class Cls);

  void HostMSquare(BiManagedMatrix<cl_float> &Out,
                   BiManagedMatrix<cl_float> &In);
};

BenchmarkResult *SchoolbookMSquare::Run(Benchmark::Class Cls) {
  cl_uint Sz = GetSizeForClass(Cls);

  // Get the program.
  cl::Program Prog = LoadProgramFromFile("schoolbook-msquare.cl");

  // Allocate matrices on the host.
  BiManagedMatrix<cl_float> DeviceSquare(Sz, Sz),
                            HostSquare(Sz, Sz),
                            Matrix(Sz, Sz);

  // Generate random matrix.
  Matrix.Randomize();

  // Create buffers to hold matrices.
  cl::Buffer DeviceSquareBuf(Ctx, CL_MEM_WRITE_ONLY, DeviceSquare.GetMemSize());
  cl::Buffer MatrixBuf(Ctx, CL_MEM_READ_ONLY, Matrix.GetMemSize());

  // Setup kernel.
  cl::Kernel MSquareKern(Prog, "msquare");
  MSquareKern.setArg(0, DeviceSquareBuf);
  MSquareKern.setArg(1, MatrixBuf);
  MSquareKern.setArg(2, cl_uint(Sz));

  // Move matrix on accelerator.
  Queue.enqueueWriteBuffer(MatrixBuf,
                           true,
                           0,
                           Matrix.GetMemSize(),
                           Matrix.GetMem());

  // Enqueue square.
  Queue.enqueueNDRangeKernel(MSquareKern,
                             cl::NullRange,
                             cl::NDRange(Sz, Sz),
                             cl::NDRange(Sz, Sz));

  // Compute the oracle in the meanwhile.
  if(DeviceOverlap)
    HostMSquare(HostSquare, Matrix);

  // Read back the square.
  Queue.enqueueReadBuffer(DeviceSquareBuf,
                          true,
                          0,
                          DeviceSquare.GetMemSize(),
                          DeviceSquare.GetMem());

  // Compute now the oracle if not done before.
  if(!DeviceOverlap)
    HostMSquare(HostSquare, Matrix);

  BenchmarkResult *Result = new BenchmarkResult();

  if(HostSquare == DeviceSquare)
    Result->SetSuccess();

  return Result;
}

cl_uint SchoolbookMSquare::GetSizeForClass(Benchmark::Class Cls) {
  switch(Cls) {
  #define CLASS_CASE(C)     \
  case Benchmark::Class ## C: \
    return C ## Side;
  CLASS_CASE(A)
  CLASS_CASE(B)
  CLASS_CASE(C)
  #undef CLASS_CASE

  default:
    throw Error("Unsupported class");
  }
}

void SchoolbookMSquare::HostMSquare(BiManagedMatrix<cl_float> &Out,
                                    BiManagedMatrix<cl_float> &In) {
  size_t Sz = Out.GetSize(0);

  for(cl_uint I = 0; I < Sz; ++I)
    for(cl_uint J = 0; J < Sz; ++J)
      for(cl_uint K = 0; K < Sz; ++K)
        Out(I, J) = In(I, K) * In(K, J);
}

} // End anonymous namespace.

PROVIDE_BENCHMARK_LAUNCHER(SchoolbookMSquare)
