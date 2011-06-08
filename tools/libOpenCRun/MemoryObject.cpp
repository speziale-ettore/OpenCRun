
#include "CL/opencl.h"

#include "Utils.h"

#include "opencrun/Core/MemoryObj.h"

#define CL_MEM_FIELD_ALL   \
  (CL_MEM_READ_WRITE |     \
   CL_MEM_WRITE_ONLY |     \
   CL_MEM_READ_ONLY |      \
   CL_MEM_USE_HOST_PTR |   \
   CL_MEM_ALLOC_HOST_PTR | \
   CL_MEM_COPY_HOST_PTR)

static inline bool clValidMemField(cl_mem_flags flags) {
  return !(flags & ~CL_MEM_FIELD_ALL);
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateBuffer(cl_context context,
               cl_mem_flags flags,
               size_t size,
               void *host_ptr,
               cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  if(!context)
    RETURN_WITH_ERROR(errcode_ret, CL_INVALID_CONTEXT);

  opencrun::Context &Ctx = *llvm::cast<opencrun::Context>(context);

  if(!clValidMemField(flags))
    RETURN_WITH_ERROR(errcode_ret, CL_INVALID_VALUE);

  opencrun::BufferBuilder Bld(Ctx, size);
  opencrun::Buffer *Buf;
    
  Buf = Bld.SetUseHostMemory(flags & CL_MEM_USE_HOST_PTR, host_ptr)
           .SetAllocHostMemory(flags & CL_MEM_ALLOC_HOST_PTR)
           .SetCopyHostMemory(flags & CL_MEM_COPY_HOST_PTR, host_ptr)
           .SetReadWrite(flags & CL_MEM_READ_WRITE)
           .SetWriteOnly(flags & CL_MEM_WRITE_ONLY)
           .SetReadOnly(flags & CL_MEM_READ_ONLY)
           .Create(errcode_ret);

  if(Buf)
    Buf->Retain();

  return Buf;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateSubBuffer(cl_mem buffer,
                  cl_mem_flags flags,
                  cl_buffer_create_type buffer_create_type,
                  const void *buffer_create_info,
                  cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_1 {
  return 0;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage2D(cl_context context,
                cl_mem_flags flags,
                const cl_image_format *image_format,
                size_t image_width,
                size_t image_height,
                size_t image_row_pitch,
                void *host_ptr,
                cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage3D(cl_context context,
                cl_mem_flags flags,
                const cl_image_format *image_format,
                size_t image_width,
                size_t image_height,
                size_t image_depth,
                size_t image_row_pitch,
                size_t image_slice_pitch,
                void *host_ptr,
                cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_0 {
  return 0;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0 {
  if(!memobj)
    return CL_INVALID_MEM_OBJECT;

  opencrun::MemoryObj &MemObj = *llvm::cast<opencrun::MemoryObj>(memobj);
  MemObj.Retain();

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0 {
  if(!memobj)
    return CL_INVALID_MEM_OBJECT;

  opencrun::MemoryObj &MemObj = *llvm::cast<opencrun::MemoryObj>(memobj);
  MemObj.Release();

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetSupportedImageFormats(cl_context context,
                           cl_mem_flags flags,
                           cl_mem_object_type image_type,
                           cl_uint num_entries,
                           cl_image_format *image_formats,
                           cl_uint *num_image_formats)
CL_API_SUFFIX__VERSION_1_0 {
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetMemObjectInfo(cl_mem memobj,
                   cl_mem_info param_name,
                   size_t param_value_size,
                   void *param_value,
                   size_t *param_value_size_ret) CL_API_SUFFIX__VERSION_1_0 {
  if(!memobj)
    return CL_INVALID_MEM_OBJECT;

  opencrun::MemoryObj &MemObj = *llvm::cast<opencrun::MemoryObj>(memobj);
  switch(param_name) {
  #define PROPERTY(PARAM, FUN, PARAM_TY, FUN_TY)   \
  case PARAM:                                      \
    return clFillValue<PARAM_TY, FUN_TY>(          \
             static_cast<PARAM_TY *>(param_value), \
             MemObj.FUN(),                         \
             param_value_size,                     \
             param_value_size_ret);
  #include "MemoryObjectProperties.def"
  #undef PROPERTY

  default:
    return CL_INVALID_VALUE;
  }
}

CL_API_ENTRY cl_int CL_API_CALL
clGetImageInfo(cl_mem image,
               cl_image_info param_name,
               size_t param_value_size,
               void *param_value,
               size_t *param_value_size_ret) CL_API_SUFFIX__VERSION_1_0 {
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clSetMemObjectDestructorCallback(cl_mem memobj,
                                 void (CL_CALLBACK *pfn_notify)
                                        (cl_mem memobj, void* user_data),
                                 void *user_data) CL_API_SUFFIX__VERSION_1_1 {
  return CL_SUCCESS;
}
