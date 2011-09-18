
#define IN(ii, jj) \
  in[(ii) * size + (jj)]

#define OUT(ii, jj) \
  out[(ii) * size + (jj)]

#define TMP0(i, j) \
  tmp0[(i) * block_size + (j)]

#define TMP1(i, j) \
  tmp1[(i) * block_size + (j)]

kernel void msquare(global float *out,
                    global float *in,
                    local float *tmp0,
                    local float *tmp1) {
  size_t size = get_global_size(0),
         block_size = get_local_size(0);

  uint ii = get_global_id(0),
       jj = get_global_id(1);

  uint i = get_local_id(0),
       j = get_local_id(1);

  float val = 0.0f;

  // Cycle through all blocks.
  for(uint kk = 0; kk < size; kk += block_size) {
    // Bring current block into local memory.
    TMP0(i, j) = IN(ii, kk + j);
    TMP1(i, j) = IN(kk + i, jj);

    // Wait for all loads to end.
    barrier(CLK_LOCAL_MEM_FENCE);

    // Perform dot product.
    for(uint k = 0; k < block_size; ++k)
      val += TMP0(i, k) * TMP1(k, j);

    // Wait for all dot products to end.
    barrier(CLK_LOCAL_MEM_FENCE);
  }

  // Save to global.
  OUT(ii, jj) = val;
}
