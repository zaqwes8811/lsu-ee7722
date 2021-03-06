

#include <gp/cuda-gpuinfo.h>

struct CBall {
  pCoor *position;
  pVect4 *velocity;
  pVect4 *force;
  pQuat *orientation;
  pMatrix3x3p *omatrix;
  pVect4 *omega;
  pVect4 *torque;
  float *mass, *fdt_to_do;
  bool *locked;
};

struct CLink {
  int *ball1_idx, *ball2_idx;
  pVect4 *cb1, *cb2;
  pVect4 *torque1, *torque2;
  pVect4 *spring_force_12;
  float *distance_relaxed;
  bool *is_simulatable;
  bool *is_surface_connection;
};


class CPU_GPU_Common
{
public:
  float platform_xmin, platform_xmax, platform_zmin, platform_zmax;

  CBall balls, h_balls;
  CLink links, h_links;

  int alloc_n_balls, alloc_n_links;
  unsigned int n_balls;
  unsigned int n_links;

  bool opt_tryout1; // For ad-hoc use.
  bool opt_tryout2; // For ad-hoc use.

  bool opt_head_lock, opt_tail_lock;
  float opt_spring_constant;
  float opt_air_resistance;
  pVect gravity_accel;
};

void data_cpu_to_gpu_common(CPU_GPU_Common *host_c);
void launch_time_step(float delta_t, int gsize, int blksize);
__host__ cudaError_t cuda_setup(GPU_Info *gpu_info);
