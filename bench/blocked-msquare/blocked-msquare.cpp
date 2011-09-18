
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

class BlockedMSquare : public Benchmark {
public:
  static const cl_uint ASide = 256;
  static const cl_uint BSide = 512;
  static const cl_uint CSide = 1024;

public:
  BlockedMSquare(int argc, char *argv[]) : Benchmark(argc, argv) { }

public:
  virtual BenchmarkResult *Run(Benchmark::Class Cls);

private:
  cl_uint GetSizeForClass(Benchmark::Class Cls);
  cl_uint GetBlockSize(cl_uint Size);

  void HostMSquare(BiManagedMatrix<cl_float> &Out,
                   BiManagedMatrix<cl_float> &In);
};

BenchmarkResult *BlockedMSquare::Run(Benchmark::Class Cls) {
  cl_uint Sz = GetSizeForClass(Cls),
          BlockSz = GetBlockSize(Sz);

  llvm::outs() << "Matrix size is " << Sz      << "x" << Sz      << " "
               << "blocked at "     << BlockSz << "x" << BlockSz << "\n";

  // Get the program.
  cl::Program Prog = LoadProgramFromFile("blocked-msquare.cl");

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
  MSquareKern.setArg(2, cl::__local(BlockSz * BlockSz * sizeof(cl_float)));
  MSquareKern.setArg(3, cl::__local(BlockSz * BlockSz * sizeof(cl_float)));

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
                             cl::NDRange(BlockSz, BlockSz));

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

cl_uint BlockedMSquare::GetSizeForClass(Benchmark::Class Cls) {
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

cl_uint BlockedMSquare::GetBlockSize(cl_uint Size) {
  size_t DevMaxSz = Dev.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>()[0];
  size_t WgMaxSz = std::sqrt(Dev.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>());
  size_t LocalSz = Dev.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();

  cl_uint BlockSz;

  // Limited by the number of "hardware contexts" and by the amount of available
  // local memory.
  for(BlockSz = std::min(DevMaxSz, WgMaxSz); BlockSz != 0; --BlockSz)
    if(Size % BlockSz == 0 && 2 * BlockSz * BlockSz * sizeof(cl_float) < LocalSz)
      break;

  assert(BlockSz != 0 && "Invalid block size");

  return BlockSz;
}

void BlockedMSquare::HostMSquare(BiManagedMatrix<cl_float> &Out,
                                 BiManagedMatrix<cl_float> &In) {
  size_t Sz = Out.GetSize(0),
         BlockSz;

  // Try to block to 32K, it is the smallest cache I have.
  for(BlockSz = std::sqrt(32 / 2 * 1024 / sizeof(cl_float));
      BlockSz != 0;
      --BlockSz)
    if(Sz % BlockSz == 0)
      break;

  assert(BlockSz != 0 && "Invalid block size");

  // Fill with zeros.
  Out.Reset();

  llvm::outs() << "Oracle matrix size is " << Sz      << "x" << Sz      << " "
               << "blocked at "            << BlockSz << "x" << BlockSz << "\n";

  // From Dragon book, chapter 11.
  for(cl_uint II = 0; II < Sz; II += BlockSz)
    for(cl_uint JJ = 0; JJ < Sz; JJ += BlockSz)
      for(cl_uint KK = 0; KK < Sz; KK += BlockSz)
        for(cl_uint I = II; I < II + BlockSz; ++I)
          for(cl_uint J = JJ; J < JJ + BlockSz; ++J)
            for(cl_uint K = KK; K < KK + BlockSz; ++K)
              Out(I, J) += In(I, K) * In(K, J);
}

} // End anonymous namespace.

PROVIDE_BENCHMARK_LAUNCHER(BlockedMSquare)
