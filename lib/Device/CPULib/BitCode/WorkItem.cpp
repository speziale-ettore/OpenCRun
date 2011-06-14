
#include <ocldef.h>

uint get_work_dim() {
  return 0;
}

size_t get_global_size(uint dimindx) {
  return 0;
}

// Implemented with an internal call.
// size_t get_global_id(uint dimindx);

size_t get_local_size(uint dimindx) {
  return 0;
}

size_t get_local_id(uint dimindx) {
  return 0;
}

size_t get_num_groups(uint dimindx) {
  return 0;
}

size_t get_group_id(uint dimindx) {
  return 0;
}

size_t get_global_offset(uint dimindx) {
  return 0;
}
