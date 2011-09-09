
#include "RuntimeFixture.h"

template <typename DevTy>
class WorkGroupTest : public RuntimeFixture<DevTy> { };

#define ALL_DEVICE_TYPES \
  CPUDev

typedef testing::Types<ALL_DEVICE_TYPES> OCLDevicesType;

testing::AssertionResult IsSudokuIterationSpace(cl_uint X[9][9],
                                                cl_uint Y[9][9],
                                                cl_uint LX[9][9],
                                                cl_uint LY[9][9],
                                                cl_uint WGY[9][9],
                                                cl_uint WGX[9][9]);

TYPED_TEST_CASE_P(WorkGroupTest);

TYPED_TEST_P(WorkGroupTest, Single) {
  cl_uint HostOut[4];

  cl::Buffer Out = this->AllocReturnBuffer(4 * sizeof(cl_uint));

  const char *Src = "kernel void writer(global uint *out) {\n"
                    "  size_t id = get_global_id(0);       \n"
                    "                                      \n"
                    "  out[id] = id;                       \n"
                    "}                                     \n";
  cl::Kernel Kern = this->BuildKernel("writer", Src);

  Kern.setArg(0, Out);

  cl::CommandQueue Queue = this->GetQueue();

  Queue.enqueueNDRangeKernel(Kern,
                             cl::NullRange,
                             cl::NDRange(4),
                             cl::NDRange(4));
  Queue.enqueueReadBuffer(Out, true, 0, 4 * sizeof(cl_uint), HostOut);

  // Unroll the verification loop for better error reporting.
  EXPECT_EQ(0u, HostOut[0]);
  EXPECT_EQ(1u, HostOut[1]);
  EXPECT_EQ(2u, HostOut[2]);
  EXPECT_EQ(3u, HostOut[3]);
}

TYPED_TEST_P(WorkGroupTest, Double) {
  cl_uint HostOut[4];

  cl::Buffer Out = this->AllocReturnBuffer(4 * sizeof(cl_uint));

  const char *Src = "kernel void writer(global uint *out) {\n"
                    "  size_t lid = get_local_id(0);       \n"
                    "  size_t gid = get_group_id(0);       \n"
                    "  size_t ws_size = get_local_size(0); \n"
                    "                                      \n"
                    "  out[gid * ws_size + lid] = lid;     \n"
                    "}                                     \n";
  cl::Kernel Kern = this->BuildKernel("writer", Src);

  Kern.setArg(0, Out);

  cl::CommandQueue Queue = this->GetQueue();

  Queue.enqueueNDRangeKernel(Kern,
                             cl::NullRange,
                             cl::NDRange(4),
                             cl::NDRange(2));
  Queue.enqueueReadBuffer(Out, true, 0, 4 * sizeof(cl_uint), HostOut);

  // Unroll the verification loop for better error reporting.
  EXPECT_EQ(0u, HostOut[0]);
  EXPECT_EQ(1u, HostOut[1]);
  EXPECT_EQ(0u, HostOut[2]);
  EXPECT_EQ(1u, HostOut[3]);
}

TYPED_TEST_P(WorkGroupTest, Sudoku) {
  cl_uint HostXOut[9][9],
          HostYOut[9][9],
          HostLXOut[9][9],
          HostLYOut[9][9],
          HostWGXOut[9][9],
          HostWGYOut[9][9];

  size_t BufferSize = 9 * 9 * sizeof(cl_uint);

  cl::Buffer XOut = this->AllocReturnBuffer(BufferSize),
             YOut = this->AllocReturnBuffer(BufferSize),
             LXOut = this->AllocReturnBuffer(BufferSize),
             LYOut = this->AllocReturnBuffer(BufferSize),
             WGXOut = this->AllocReturnBuffer(BufferSize),
             WGYOut = this->AllocReturnBuffer(BufferSize);

  const char *Src = "kernel void sudoku(global uint *x_out,                  \n"
                    "                   global uint *y_out,                  \n"
                    "                   global uint *lx_out,                 \n"
                    "                   global uint *ly_out,                 \n"
                    "                   global uint *wgx_out,                \n"
                    "                   global uint *wgy_out) {              \n"
                    "  size_t global_x_id = get_global_id(0);                \n"
                    "  size_t global_y_id = get_global_id(1);                \n"
                    "                                                        \n"
                    "  size_t global_x_size = get_global_size(0);            \n"
                    "                                                        \n"
                    "  size_t id = global_x_id * global_x_size + global_y_id;\n"
                    "                                                        \n"
                    "  x_out[id] = get_global_id(0);                         \n"
                    "  y_out[id] = get_global_id(1);                         \n"
                    "                                                        \n"
                    "  lx_out[id] = get_local_id(0);                         \n"
                    "  ly_out[id] = get_local_id(1);                         \n"
                    "                                                        \n"
                    "  wgx_out[id] = get_group_id(0);                        \n"
                    "  wgy_out[id] = get_group_id(1);                        \n"
                    "}                                                       \n";
  cl::Kernel Kern = this->BuildKernel("sudoku", Src);

  Kern.setArg(0, XOut);
  Kern.setArg(1, YOut);
  Kern.setArg(2, LXOut);
  Kern.setArg(3, LYOut);
  Kern.setArg(4, WGXOut);
  Kern.setArg(5, WGYOut);

  cl::CommandQueue Queue = this->GetQueue();

  Queue.enqueueNDRangeKernel(Kern,
                             cl::NullRange,
                             cl::NDRange(9, 9),
                             cl::NDRange(3, 3));

  Queue.enqueueReadBuffer(XOut, true, 0, BufferSize, HostXOut);
  Queue.enqueueReadBuffer(YOut, true, 0, BufferSize, HostYOut);
  Queue.enqueueReadBuffer(LXOut, true, 0, BufferSize, HostLXOut);
  Queue.enqueueReadBuffer(LYOut, true, 0, BufferSize, HostLYOut);
  Queue.enqueueReadBuffer(WGXOut, true, 0, BufferSize, HostWGXOut);
  Queue.enqueueReadBuffer(WGYOut, true, 0, BufferSize, HostWGYOut);

  EXPECT_TRUE(IsSudokuIterationSpace(HostXOut,
                                     HostYOut,
                                     HostLXOut,
                                     HostLYOut,
                                     HostWGXOut,
                                     HostWGYOut));
}

REGISTER_TYPED_TEST_CASE_P(WorkGroupTest, Single,
                                          Double,
                                          Sudoku);

INSTANTIATE_TYPED_TEST_CASE_P(OCLDev, WorkGroupTest, OCLDevicesType);

testing::AssertionResult IsSudokuIterationSpace(cl_uint X[9][9],
                                                cl_uint Y[9][9],
                                                cl_uint LX[9][9],
                                                cl_uint LY[9][9],
                                                cl_uint WGX[9][9],
                                                cl_uint WGY[9][9]) {
  for(unsigned I = 0; I < 9; ++I)
    for(unsigned J = 0; J < 9; ++J) {
      // Absolute iteration space.
      if(X[I][J] != I || Y[I][J] != J)
        return testing::AssertionFailure() << "expected "
                                           << "[" << I << ", "
                                                  << J
                                           << "] "
                                           << "got "
                                           << "[" << X[I][J] << ", "
                                                  << Y[I][J]
                                           << "]";

      // Partitioned iteration space.
      if(LX[I][J] != I % 3 ||
         LY[I][J] != J % 3 ||
         WGX[I][J] != I / 3 ||
         WGY[I][J] != J / 3)
        return testing::AssertionFailure() << "expected "
                                           << "[" << I % 3 << ", "
                                                  << J % 3 << "|"
                                                  << I / 3 << ", "
                                                  << J / 3
                                           << "] "
                                           << "got "
                                           << "[" << LX[I][J]  << ", "
                                                  << LY[I][J]  << "|"
                                                  << WGX[I][J] << ", "
                                                  << WGY[I][J]
                                           << "]";
    }

  return testing::AssertionSuccess();
}
