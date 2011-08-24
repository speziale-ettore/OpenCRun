
kernel void msquare(global float *out, global float *in, uint size) {
  size_t i = get_global_id(0),
         j = get_global_id(1),
         id = i * size + j;

  out[id] = 0;
  for(uint k = 0; k < size; ++k)
    out[id] += in[i * size + k] * in[k * size + j];
}
