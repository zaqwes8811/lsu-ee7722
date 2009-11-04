/// LSU EE 4702-1 (Fall 2009), GPU Programming
//
 /// Demo of Dynamic Simulation, Multiple Balls on Curved Platform

// $Id:$

/// Purpose
//
//   Demonstrate Several Graphical and Simulation Techniques.


/// What Code Does

//  Simulates balls bouncing on a half-cylinder platform.

//  Features

//    Ball friction and angular momentum modeled.

//    Balls cast shadows on platform:
//      Demonstrates use of stencils and vertex shader.
//      A vertex shader is used to compute shadow locations.
//      Stencils are used to mark the shadow locations.

//    Ball reflections visible on mirrored tiles.
//      Demonstrates stencils, blending, vertex, and geometry shaders.
//      Vertex shader computes reflection locations (> 1 per vertex).
//      Geometry shader emits triangles for all reflection points.
//      Later, tile image is blended over reflected image of balls.

//    Occlusion queries used to limit number of balls rendered.

//    Two-color specular lighting used for balls.




///  Keyboard Commands
 //
 /// Object (Eye, Light, Ball) Location or Push
 //   Arrows, Page Up, Page Down
 //   Will move object or push ball, depending on mode:
 //   'e': Move eye.
 //   'l': Move light.
 //   'b': Move ball. (Change position but not velocity.)
 //   'B': Push ball. (Add velocity.)
 //
 /// Eye Direction
 //   Home, End, Delete, Insert
 //   Turn the eye direction.
 //   Home should rotate eye direction up, End should rotate eye
 //   down, Delete should rotate eye left, Insert should rotate eye
 //   right.  The eye direction vector is displayed in the upper left.

 /// Simulation Options
 //  (Also see variables below.)
 //
 //  'p'    Pause simulation. (Press again to resume.)
 //  'a'    Switch physics method (CPU to GPU).

 //  'd'    Toggle dripping of balls.
 //  'x'    Toggle shower of balls.
 //  'X'    Release one pair of balls.
 //  't'    Run 5-tier-of-balls benchmark.
 //  'T'    Run 1-tier-of-balls benchmark.
 //  'R'    Remove all but one ball.

 //  'm'    Toggle reflections.
 //  'w'    Toggle shadows.
 //  'n'    Toggle visibility of platform normals.

 //  's'    Stop balls linear motion.
 //  'S'    Stop balls rotational motion.

 //  'c'    Use colors to show number of reflected points, and other info.
 //  'M'    Switch between different shortcuts in computing reflections.

 //  'g'    Turn gravity on and off.
 //  'F12'  Write screenshot to file.

 /// Variables
 //   Selected program variables can be modified using the keyboard.
 //   Use "Tab" to cycle through the variable to be modified, the
 //   name of the variable is displayed next to "VAR" on the bottom
 //   line of green text.

 //  'Tab' Cycle to next variable.
 //  '`'   Cycle to previous variable.
 //  '+'   Increase variable value.
 //  '-'   Decrease variable value.
 //
 //  VAR Light Intensity 
 //              - The light intensity.
 //  VAR Gravity - Gravitational acceleration. (Turn on/off using 'g'.)
 //  VAR Ball Mass
 //  VAR Ball Radius
 //  VAR Elasticity 
 //              - Softness of balls. (Inverse of spring constant
 //                used to compute repulsion forces when balls touch.)
 //  VAR Sliding Friction 
 //              - Dynamic friction coefficient. Used for ball/ball
 //                and ball/platform contact.  The standard friction model
 //                is used, frictional force is proportional to force
 //                between two sliding surfaces.
 //  VAR Rolling Friction
 //              - Rolling friction coefficient. Used both for
 //                ball/ball and ball/platform rolling. Frictional
 //                model is ad-hoc, with force proportional to ball
 //                rotation with respect to contact point and ball
 //                deformation.
 //  VAR Bounce Energy Loss
 //              - Amount of energy lost in contact.


#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES

#include <math.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

#include <gp/util.h>
#include <gp/glextfuncs.h>
#include <gp/coord.h>
#include <gp/shader.h>
#include <gp/pstring.h>
#include <gp/misc.h>
#include <gp/gl-buffer.h>
#include <gp/texture-util.h>
#include <gp/cuda-util.h>
#include "shapes.h"
#include "demo-x.cuh"



void vec_set(float4& a, pQuat b)
{a.x = b.v.x; a.y = b.v.y; a.z = b.v.z;  a.w = b.w; }
void vec_set(pQuat&a, float4 b)
{a.v.x = b.x; a.v.y = b.y; a.v.z = b.z;  a.w = b.w; }

void vec_set(float3& a, pCoor b) {a.x = b.x; a.y = b.y; a.z = b.z;}
void vec_sets(pCoor& a, float3 b) {a.x = b.x; a.y = b.y; a.z = b.z;}
void vec_sets(pCoor& a, float4 b) {a.x = b.x; a.y = b.y; a.z = b.z; a.w = b.w;}
void vec_set(pVect& a, float3 b) {a.x = b.x; a.y = b.y; a.z = b.z; }
void vec_sets3(pCoor& a, float4 b) {a.x = b.x; a.y = b.y; a.z = b.z; }
void vec_sets3(float4& a, pCoor b) {a.x = b.x; a.y = b.y; a.z = b.z; }
void vec_sets4(pVect& a, float4 b) {a.x = b.x; a.y = b.y; a.z = b.z; }


template <typename T>
void to_dev_ds(const char* const dst_name, int idx, T src)
{
  T cpy = src;
  const int offset = sizeof(void*) * idx;
  CE(cudaMemcpyToSymbol(dst_name,&cpy,sizeof(T),offset,cudaMemcpyHostToDevice));
}

void to_dev_ds(const char* const dst_name, int idx, double& src)
{
  ASSERTS( false );
}

#define TO_DEV_DS(dst,src) to_dev_ds(#dst,0,src);
#define TO_DEV_DSS(dst,src1,src2) \
        to_dev_ds(#dst,0,src1); to_dev_ds(#dst,1,src2);
#define TO_DEV(var) to_dev_ds<typeof var>(#var,0,var)
#define TO_DEVF(var) to_dev_ds<float>(#var,0,float(var))
#define TO_DEV_OM(obj,memb) to_dev_ds(#memb,0,obj.memb)
#define TO_DEV_OM_F(obj,memb) to_dev_ds(#memb,0,float(obj.memb))



///
/// Main Data Structures
///
//
// class World: All data about scene.


class World;
class Contact;


class Pass {
public:
  Pass(){ block_cnt = 0; round_cnt = 0; }
  dim3 dim_grid, dim_block;
  int block_cnt;
  int round_cnt;
  int prefetch_offset;
  int schedule_offset;
};


// Object Holding Ball State
//
class Ball {
public:
  Ball(World *w);
  Ball(Ball &b):w(b.w),original(&b)
  {
#define C(m) m=b.m
    C(serial);C(idx);
    C(position);C(velocity);C(orientation);C(angular_momentum);
    C(prev_velocity);
    C(proximity);
    C(collision_count); C(debug_pair_calls);
    query_occlusion_id = 0;
#undef C
  }
  ~Ball();
  World& w;
  Ball* const original;
  int serial;
  int idx;

  pCoor position;
  pVect velocity;
  pQuat orientation;
  pVect angular_momentum;

  pVect prev_velocity;
  pVect prev_angular_momentum;

  GLuint query_occlusion_id;
  bool occlusion_query_active;
  bool occluded;
  int occluded_run;
  int occlusion_countdown;

  pColor color_event;
  pColor color_natural;

  int collision_count;
  int contact_count;
  uint32_t debug_pair_calls;

  pVect point_rot_vel(pNorm tact_dir);
  void apply_deltao(pNorm tact_dir, pNorm force_dir, double deltao);
  void apply_tan_do(pNorm tact_dir, pVect force);
  void apply_tan_dv(pNorm tact_dir, pVect force);
  void push(pVect amt);
  void translate(pVect amt);
  void stop();
  void freeze();

  PStack<int> proximity;
  int pass;
  int pass_iterated;
  int pass_todo;
  int rounds;
  bool placed;
  int block;
  int sm_idx;

};

const pColor red(0.8,0.1,0.1);
const pColor green(0.1,0.8,0.1);
const pColor blue(0.1,0.1,0.8);
const pColor cyan(0.1,0.8,0.8);
const pColor dark(0);
const pColor light_gray(0.8,0.8,0.8);
const pColor dark_gray(0.15,0.15,0.15);
const pColor white(0xffffff);
const pColor lsu_business_purple(0x7f5ca2);
const pColor lsu_spirit_purple(0x580da6);
const pColor lsu_spirit_gold(0xf9b237);
const pColor lsu_official_purple(0x2f0462);
const pColor* const colors[8] =
  { &lsu_spirit_gold, &lsu_spirit_purple, &green, &blue,
    &red, &cyan, &light_gray, &dark_gray };
const int colors_mask = 0x7;

enum Data_Location
  { 
    DL_PO  = 0x1, // Position, Orientation
    DL_CV  = 0x2, // Collision Count, Velocity
    DL_OT  = 0x4, // Other.
    DL_ALL = 0x7,
    DL_PO_CPU  = 0x1, // Position, Orientation
    DL_CV_CPU  = 0x2, // Collision Count, Velocity
    DL_OT_CPU  = 0x4, // Other.
    DL_ALL_CPU = 0x7,
    DL_PO_CUDA = 0x10,
    DL_CV_CUDA = 0x20,
    DL_OT_CUDA = 0x40,
    DL_ALL_CUDA = 0x70 };

enum GPU_Physics_Method
  { GP_cpu, GP_cuda, GP_cuda_1_pass, GP_cuda_2_pass, GP_ENUM_SIZE };
const char* const gpu_physics_method_str[] =
  { "CPU", "CUDA", "CUDA 1 Pass", "CUDA 2 Pass" };


class World {
public:
  World(pOpenGL_Helper &fb):
    ogl_helper(fb){init();}
  void init();

  pOpenGL_Helper& ogl_helper;
  pVariable_Control variable_control;
  pFrame_Timer frame_timer;

  static void render_w(void *moi){ ((World*)moi)->render(); }
  void render();
  void cb_keyboard();

  void variables_update();
  void modelview_update();
  void time_step_cpu();


  bool opt_verify;
  bool opt_debug;

  double world_time;

  PStack<Ball*> balls;

  bool opt_spray_on;
  int spray_cnt, spray_run;
  double ball_countdown;

  bool opt_drip;
  int drip_cnt, drip_run;
  Ball *dball;
  Ball* pball;

  int balls_occluded;

  void ball_init();
  void benchmark_setup(int tiers=1);

  bool penetration_balls_resolve
  (Ball *ball1, Ball *ball2, bool b2_real = true);
  void balls_render(bool do_ot);
  void balls_render_simple();
  void balls_remove();
  void balls_stop();
  void balls_rot_stop();


  bool opt_gravity;
  float opt_gravity_accel;      // Value chosen by user.
  float opt_ball_mass;
  float opt_ball_radius;
  float opt_bounce_loss;
  float opt_elasticity;
  float opt_friction_coeff;
  float opt_friction_roll;

  // Pre-computed values.
  //
  double delta_t;
  pVect gravity_accel;          // Set to zero when opt_gravity is false;
  pVect gravity_accel_dt;
  double ball_mass_inv;
  double elasticity_inv_dt;
  double ball_mo_inertia;
  double two_r_sq;
  double two_r;
  double mo_vel_factor;
  double v_to_do;
  double r_inv;
  double short_xrad_sq;

  /// Tiled platform for ball.
  //
  float platform_xmin, platform_xmax, platform_zmin, platform_zmax;
  float platform_xmid, platform_xrad, platform_xrad_inv;
  float delta_theta_inv, tile_size_inv;
  float platform_pi_xwidth_inv;
  float trmin, trmax, tsmin, tsmax; // Platform texture boundary.
  pBuffer_Object<pVect> platform_tile_norms;
  pBuffer_Object<pVect> platform_tile_coords;
  pBuffer_Object<float> platform_tex_coords;
  int platform_even_vtx_cnt, platform_odd_vtx_cnt;

  // Re-Computed Constants
  //
  float platform_xmin_mr;
  float platform_xmax_pr;
  float platform_zmin_mr;
  float platform_zmax_pr;

  void platform_update();
  bool platform_collision_possible(pCoor pos, float ts_mov_max = 0);

  pCoor light_location;
  float opt_light_intensity;
  enum { MI_Eye, MI_Light, MI_Ball, MI_Ball_V, MI_COUNT } opt_move_item;
  bool opt_pause;

  pCoor eye_location;
  pVect eye_direction;
  pMatrix modelview;
  pMatrix modelview_inv;
  pMatrix modelview_projection;


  bool opt_shadows;
  bool opt_mirror;
  int opt_mirror_method;
  bool opt_normals_visible;
  bool opt_color_events;

  pShader *vs_fixed;
  pShader *vs_shadow;
  pShader *vs_reflect;

  GLint sun_axis_e, sun_axis_ne, sun_platform_xrad_sq, sun_light_num;
  GLint sun_platform_xmid, sun_platform_xrad; 
  GLint sun_eye_location, sun_eye_to_world, sun_world_to_clip;
  GLint sun_opt_mirror_method, sun_opt_color_events;


  GLuint texid_plat;
  GLuint texid_ball;

  // Pre-Computed Spheres
  //
  Sphere sphere;
  Sphere* spheres;
  Sphere sphere_lite;
  int sphere_lod_max;
  int sphere_lod_min;
  double sphere_delta_lod, sphere_lod_factor, sphere_lod_offset;
  int sphere_count;
  Sphere* sphere_get(Ball *ball);

  Cone cone;                    // Used to show platform normals.

  int opt_physics_method;
  int data_location;

  void time_step_cuda(bool need_data);

  int cuda_schedule_stale;
  cudaEvent_t frame_start_ce, frame_stop_ce;


  PStack<Pass> passes;
  pCUDA_Memory<SM_Idx2> big_array;
  pCUDA_Memory<int> prefetch_array;

  void cuda_init();
  void cuda_constants_update();
  bool cpu_data_to_cuda();
  void cuda_data_to_cpu(uint which_data);
  int cuda_ball_cnt;
  bool cuda_initialized;
  bool cuda_constants_stale;
  pCUDA_Memory_X<CUDA_Ball,CUDA_Ball_X> cuda_balls;
};

void
World::init()
{
  data_location = DL_ALL_CPU;
  cuda_initialized = false;
  cuda_schedule_stale = 1;
  opt_physics_method = GP_cpu;

  delta_t = 1.0 / 240;

  frame_timer.work_unit_set("Steps / s");
  world_time = 0;
  opt_gravity_accel = 9.8;
  opt_gravity = true;
  gravity_accel = pVect(0,-opt_gravity_accel,0);
  opt_normals_visible = false;
  opt_shadows = true;
  opt_mirror = true;
  opt_mirror_method = 0;
  opt_spray_on = false;
  opt_color_events = false;
  opt_debug = false;

  vs_fixed = new pShader();

  vs_shadow = new pShader("demo-x-shdr.cc","vs_main_shadow();");
  sun_axis_e = vs_shadow->uniform_location("axis_e");
  sun_axis_ne = vs_shadow->uniform_location("axis_ne");
  sun_platform_xrad_sq = vs_shadow->uniform_location("platform_xrad_sq");
  sun_light_num = vs_shadow->uniform_location("light_num");

  vs_reflect =
    new pShader
    ("demo-x-shdr.cc","vs_main_reflect();", "gs_main_reflect();", NULL);
  sun_platform_xrad = vs_reflect->uniform_location("platform_xrad");
  sun_platform_xmid = vs_reflect->uniform_location("platform_xmid");
  sun_eye_location = vs_reflect->uniform_location("eye_location");
  sun_eye_to_world = vs_reflect->uniform_location("eye_to_world");
  sun_world_to_clip = vs_reflect->uniform_location("world_to_clip");
  sun_opt_mirror_method = vs_reflect->uniform_location("opt_mirror_method");
  sun_opt_color_events = vs_reflect->uniform_location("opt_color_events");

  eye_location = pCoor(17.9,-2,117.2);
  eye_direction = pVect(-0.15,-0.06,-0.96);

  platform_xmin = -40; platform_xmax = 40;
  platform_zmin = -80; platform_zmax = 80;

  const bool alternate_graphics = false;

  if ( alternate_graphics )
    {
      trmin = 1;
      trmax = 0;
      tsmin = 1;
      tsmax = 0;
      texid_plat = pBuild_Texture_File( "avatar2.png", false,-1);
      //  texid_ball = pBuild_Texture_File("../gpup/gpup.png", false,255);
    }
  else
    {
      trmax = 0.05;
      trmin = 0.7;
      tsmax = 0;
      tsmin = 0.4;
      texid_plat = pBuild_Texture_File("../gpup/gpup.png", false,255);
    }

  texid_ball = pBuild_Texture_File("../gpgpu/mult.png", false,-1);

  opt_light_intensity = 100.2;
  light_location = pCoor(28.2,25.8,-14.3);

  opt_ball_mass = 0.25;
  opt_ball_radius = 2;
  opt_friction_coeff = 0.1;
  opt_friction_roll = 0.1;
  opt_bounce_loss = 0.55;
  opt_elasticity = 1.0 / 16;
  opt_drip = true;
  drip_cnt = spray_cnt = 0;
  drip_run = 3;
  spray_run = 8;
  dball = NULL;
  opt_verify = true;

  variable_control.insert(opt_ball_mass,"Ball Mass");
  variable_control.insert(opt_elasticity,"Elasticity");
  variable_control.insert(opt_friction_coeff,"Sliding Friction");
  variable_control.insert(opt_friction_roll,"Rolling Friction");
  variable_control.insert(opt_bounce_loss,"Bounce Energy Loss");
  variable_control.insert(opt_gravity_accel,"Gravity");
  variable_control.insert(opt_light_intensity,"Light Intensity");
  variable_control.insert(opt_ball_radius,"Ball Radius");

  opt_move_item = MI_Eye;
  opt_pause = false;

  pball = new Ball(this); 
  pball->prev_velocity = pVect(0,0,0);

  ball_countdown = 0.1;
  sphere.init(40);
  sphere_lite.init(4);

  sphere_lod_max = 40;
  sphere_lod_min = 8;
  sphere_count = 8;
  sphere_delta_lod = ( sphere_lod_max - sphere_lod_min ) / (sphere_count-1);
  spheres = new Sphere[sphere_count];

  for ( int i=0; i<sphere_count; i++ )
    {
      const int lod = sphere_lod_min + int( i * sphere_delta_lod + 0.5 );
      spheres[i].init(lod);
    }

  variables_update();
  platform_update();
  modelview_update();

  /// Initialize Ball Positions
  //
  //  Code below places balls in one of several ways.
  //  Some of the ball arrangements are intended for debugging.

  if ( opt_spray_on )
    {
      balls += new Ball(this);
      return;
    }


  const float r_short = platform_xrad - opt_ball_radius;

  if ( true )
    {
      const double sa = asin( opt_ball_radius / r_short );
      const double ca = 1.5 * M_PI;
      const double a[] = { ca - sa, ca + sa, ca };
      const double r[] =
        { r_short, r_short,
          r_short - sqrt(3) * opt_ball_radius
        };
      for ( int i=0; i<3; i++ )
        {
          Ball* const b = new Ball(this);
          b->position = pCoor( r[i] * cos(a[i]), r[i] * sin(a[i]), 45);
          b->velocity = pVect(0,0,0);
          balls += b;
        }
    }

  if (false){
    {
      Ball* const b = new Ball(this);
      b->position = pCoor(0,-r_short,40);
      b->velocity = pVect(0,0,0);
      balls += b;
    }
    {
      Ball* const b = new Ball(this);
      b->position = pCoor(r_short*cos(1.75*M_PI),r_short*sin(1.75*M_PI),40);
      b->velocity = pVect(0,0,0);
      balls += b;
    }
  }

  if ( false )
    {
      Ball *b = new Ball(this);
      b->position = pCoor(0,-r_short,48);
      b->velocity = pVect(0,0,0);
      b->angular_momentum = pVect(0,6,0);
      balls += b;
      b = new Ball(this);
      b->position = pCoor(0,-r_short+3*opt_ball_radius,48);
      b->velocity = pVect(0,0,0);
      b->angular_momentum = pVect(0,1,0);
      balls += b;
    }
}


void
World::variables_update()
{
  // Updated pre-computed constants.
  //
  // This routine is called after user changes something.

  cuda_constants_stale = true;
  ball_mass_inv = 1 / opt_ball_mass;
  r_inv = 1.0 / opt_ball_radius;
  const double r_sq = opt_ball_radius * opt_ball_radius;
  two_r_sq = 4 * r_sq;
  two_r = 2 * opt_ball_radius;
  gravity_accel.y = opt_gravity ? -opt_gravity_accel : 0;
  gravity_accel_dt = delta_t * gravity_accel;
  sphere.radius = opt_ball_radius;
  sphere_lite.radius = opt_ball_radius;
  for ( int i=0; i<sphere_count; i++ ) spheres[i].radius = opt_ball_radius;
  elasticity_inv_dt = 100 * delta_t / opt_elasticity;
  if ( opt_bounce_loss > 1 ) opt_bounce_loss = 1;
  ball_mo_inertia = 0.4 * opt_ball_mass * r_sq;
  v_to_do = opt_ball_mass * opt_ball_radius / ball_mo_inertia;
  mo_vel_factor = 0.5 / ( 1 + v_to_do * opt_ball_radius );
  short_xrad_sq = pow(platform_xrad - opt_ball_radius,2);
  platform_xmin_mr = platform_xmin - opt_ball_radius;
  platform_zmin_mr = platform_zmin - opt_ball_radius;
  platform_xmax_pr = platform_xmax + opt_ball_radius;
  platform_zmax_pr = platform_zmax + opt_ball_radius;
}

void
World::platform_update()
{
  /// Set up Platform

  const float platform_delta_z = platform_zmax - platform_zmin;
  const int tile_count = int(max(1.0,platform_delta_z * 0.15));
  const float zdelta = platform_delta_z / tile_count;
  const float platform_delta_x = platform_xmax - platform_xmin;

  PStack<pVect> p_tile_coords;
  PStack<pVect> p1_tile_coords;
  PStack<pVect> p_tile_norms;
  PStack<pVect> p1_tile_norms;
  PStack<float> p_tex_coords, p1_tex_coords;
  platform_pi_xwidth_inv = M_PI / platform_delta_x;

  const int tess_bits = 2;
  const int tess_mask = 1 << tess_bits;
  const int th_tile_count = 19 << tess_bits;
  const double delta_theta = M_PI / th_tile_count;
  delta_theta_inv = 1.0 / delta_theta;
  tile_size_inv = 1 / zdelta;
  platform_xmid = 0.5 * ( platform_xmax + platform_xmin );
  platform_xrad = 0.5 * platform_delta_x;
  platform_xrad_inv = 1 / platform_xrad;
  const float trdelta = ( trmax - trmin ) / tess_mask;

  for ( int i = 0; i < th_tile_count; i++ )
    {
      const double theta0 = i * delta_theta;
      const double theta1 = theta0 + delta_theta;
      bool even = i & tess_mask;
      const float x0 = platform_xmid - platform_xrad * cos(theta0);
      const float x1 = platform_xmid - platform_xrad * cos(theta1);
      const float y0 = -0.01 - platform_xrad * sin(theta0);
      const float y1 = -0.01 - platform_xrad * sin(theta1);
      pVect norm0( cos(theta0), sin(theta0), 0);
      pVect norm1( cos(theta1), sin(theta1), 0);

      const float trma = trmax - trdelta * ( i & ( tess_mask - 1 ) );
      const float trmi = trma - trdelta;

      for ( float z = platform_zmin; z < platform_zmax; z += zdelta )
        {
          PStack<pVect>& t_coords = even ? p_tile_coords : p1_tile_coords;
          PStack<pVect>& t_norms = even ? p_tile_norms : p1_tile_norms;
          PStack<float>& tex_coords = even ? p_tex_coords : p1_tex_coords;
          tex_coords += trma; tex_coords += tsmax;
          t_coords += pVect(x0,y0,z);
          t_norms += norm0;  t_norms += norm0;
          tex_coords += trma; tex_coords += tsmin;
          t_coords += pVect(x0,y0,z+zdelta);
          tex_coords += trmi; tex_coords += tsmin;
          t_coords += pVect(x1,y1,z+zdelta);
          t_norms += norm1;  t_norms += norm1;
          tex_coords += trmi; tex_coords += tsmax;
          t_coords += pVect(x1,y1,z);
          even = !even;
        }
    }

  platform_even_vtx_cnt = p_tile_coords.occ();
  platform_odd_vtx_cnt = p1_tile_coords.occ();

  while ( pVect* const v = p1_tile_coords.iterate() ) p_tile_coords += *v;
  while ( pVect* const v = p1_tile_norms.iterate() ) p_tile_norms += *v;
  while ( float* const v = p1_tex_coords.iterate() ) p_tex_coords += *v;

  platform_tile_norms.re_take(p_tile_norms);
  platform_tile_norms.to_gpu();
  platform_tile_coords.re_take(p_tile_coords);
  platform_tile_coords.to_gpu();
  platform_tex_coords.re_take(p_tex_coords);
  platform_tex_coords.to_gpu();
}

void
World::modelview_update()
{
  pMatrix_Translate center_eye(-eye_location);
  pMatrix_Rotation rotate_eye(eye_direction,pVect(0,0,-1));
  modelview = rotate_eye * center_eye;
  modelview_inv = invert(modelview);
}

void
World::benchmark_setup(int tiers)
{
  // Set up an arrangement of balls intended for benchmarking.

  cuda_data_to_cpu(DL_ALL);
  data_location = DL_ALL_CPU;

  while ( balls.occ() ) delete balls.pop();

  const float delta_z = opt_ball_radius * 5;
  const float delta_x = opt_ball_radius * 2.1;
  const float hdeltaz = delta_z / 2;
  const float hdeltax = delta_x / 2;
  const float delta_y = 2.1 * opt_ball_radius;
  const float ymax = delta_y * tiers - 0.001;
  opt_drip = false;
  opt_spray_on = false;
  opt_verify = false;
  //  opt_friction_roll = 0.01; opt_friction_coeff = 0.01;
  for ( float z = platform_zmin + hdeltaz; z < platform_zmax; z+= delta_z )
    for ( float x = platform_xmin + hdeltax; x < platform_xmax; x += delta_x )
      for ( float y = 0; y < ymax; y+= delta_y )
        {
          Ball* const b = new Ball(this);
          b->velocity.z = 0;
          b->position = pCoor(x,y,z);
          balls += b;
        }
}

void
World::cuda_init()
{
  if ( cuda_initialized ) return;
  cuda_ball_cnt = 5;
  cuda_initialized = true;
  cuda_constants_stale = true;
  int device_count;
  cudaGetDeviceCount(&device_count);
  ASSERTS( device_count );
  const int dev = 0;
  cudaDeviceProp prop;
  CE(cudaGetDeviceProperties(&prop,dev));
  CE(cudaGLSetGLDevice(dev));
  printf
    ("GPU: %s @ %.2f GHz WITH %d MiB GLOBAL MEM\n",
     prop.name, prop.clockRate/1e6, int(prop.totalGlobalMem >> 20));
  printf
    ("CAP: %d.%d  NUM MP: %d  TH/BL: %d  SHARED: %d  CONST: %d  "
     "# REGS: %d\n",
     prop.major, prop.minor,
     prop.multiProcessorCount, prop.maxThreadsPerBlock,
     int(prop.sharedMemPerBlock), int(prop.totalConstMem),
     prop.regsPerBlock
     );

  CE(cudaEventCreate(&frame_start_ce));
  CE(cudaEventCreate(&frame_stop_ce));

  cuda_balls.disable_aos = true;
  cuda_balls.use_aos = false;
  CMX_SETUP(cuda_balls,orientation);
  CMX_SETUP(cuda_balls,position);
  CMX_SETUP(cuda_balls,velocity);
  CMX_SETUP(cuda_balls,prev_velocity);
  CMX_SETUP(cuda_balls,angular_momentum);
  CMX_SETUP(cuda_balls,collision_count);
  CMX_SETUP(cuda_balls,contact_count);
  CMX_SETUP(cuda_balls,debug_pair_calls);
  cuda_balls.alloc_locked(cuda_ball_cnt);
  cuda_balls.ptrs_to_cuda_soa("balls_x_0","balls_x_1");

}

void
World::cuda_constants_update()
{
  if ( !cuda_constants_stale ) return;
  cuda_constants_stale = false;
  TO_DEV(gravity_accel_dt);
  TO_DEV(opt_ball_radius); TO_DEV(opt_bounce_loss);
  TO_DEV(opt_debug);
  TO_DEV(platform_xmin); TO_DEV(platform_xmax);
  TO_DEV(platform_zmin); TO_DEV(platform_zmax);
  TO_DEV(platform_xmin_mr); TO_DEV(platform_xmax_pr);
  TO_DEV(platform_zmin_mr); TO_DEV(platform_zmax_pr);
  TO_DEV(platform_xmid); TO_DEV(platform_xrad);
  TO_DEVF(delta_t);
  TO_DEVF(short_xrad_sq);
  TO_DEVF(r_inv); TO_DEVF(two_r); TO_DEVF(two_r_sq);
  TO_DEVF(elasticity_inv_dt); TO_DEVF(ball_mass_inv);
  TO_DEVF(opt_friction_coeff); TO_DEVF(opt_friction_roll);
  TO_DEVF(mo_vel_factor); TO_DEVF(v_to_do);
}

bool
World::cpu_data_to_cuda()
{
  ASSERTS( ( DL_ALL & ( data_location | ( data_location >> 4 ) ) ) == DL_ALL );
  const int cnt = balls.occ();
  if ( cnt > cuda_ball_cnt )
    {
      cuda_balls.realloc(cnt);
      cuda_balls.ptrs_to_cuda_soa("balls_x_0","balls_x_1");
      cuda_ball_cnt = cnt;
    }
  cuda_constants_update();
  if ( ( data_location & DL_ALL_CUDA ) == DL_ALL_CUDA ) return false;
  data_location |= DL_ALL_CUDA;

  for ( int idx=0; idx<cnt; idx++ )
    {
      Ball* const ball = balls[idx];
      vec_set(cuda_balls.soa.orientation[idx],ball->orientation);
      vec_sets3(cuda_balls.soa.position[idx],ball->position);
      vec_set(cuda_balls.soa.prev_velocity[idx],ball->velocity);
      vec_set(cuda_balls.soa.velocity[idx],ball->velocity);
      vec_set(cuda_balls.soa.angular_momentum[idx],ball->angular_momentum);
      cuda_balls.soa.collision_count[idx] = ball->collision_count;
      cuda_balls.soa.contact_count[idx] = ball->contact_count;
      cuda_balls.soa.debug_pair_calls[idx] = ball->debug_pair_calls;
    }

  cuda_balls.to_cuda();
  return true;
}

void
World::cuda_data_to_cpu(uint which_data)
{
  ASSERTS( ( DL_ALL & ( data_location | ( data_location >> 4 ) ) ) == DL_ALL );
  ASSERTS( which_data & DL_ALL );
  const uint mask = which_data & ~data_location;
  if ( !mask ) return;
  data_location |= which_data;
  cuda_balls.from_cuda();

  const int cnt = balls.occ();

  for ( int idx=0; idx<cnt; idx++ )
    {
      Ball* const ball = balls[idx];
      if ( mask & DL_PO_CPU )
        {
          vec_sets3(ball->position,cuda_balls.soa.position[idx]);
          vec_set(ball->orientation,cuda_balls.soa.orientation[idx]);
          ball->contact_count =
            0xff & ( cuda_balls.soa.contact_count[idx] >> 8 );
        }
      if ( mask & DL_CV_CPU )
        {
          vec_set(ball->velocity,cuda_balls.soa.velocity[idx]);
          ball->collision_count = cuda_balls.soa.collision_count[idx];
        }
      if ( mask & DL_OT_CPU )
        {
          vec_set(ball->angular_momentum,cuda_balls.soa.angular_momentum[idx]);
          ball->debug_pair_calls = cuda_balls.soa.debug_pair_calls[idx];
        }
    }
}


///
/// Physical Simulation Code
///

 /// Initialize Simulation
//

int ball_serial_next = 0;

Ball::Ball(World* wp):w(*wp),original(NULL)
{
  w.cuda_data_to_cpu(DL_ALL);
  w.data_location = DL_ALL_CPU;
  w.cuda_schedule_stale = 1;

  occluded = false;
  occlusion_query_active = false;
  occluded_run = 0;
  occlusion_countdown = 0;
  glGenQueries(1,&query_occlusion_id);
  collision_count = 0;
  contact_count = 0;
  color_event = pColor(0.5,0.5,0.5);
  color_natural = pColor(0.5,0.5,0.5);
  serial = ball_serial_next++;
  position = pCoor(30,22,-15.4);
  velocity = pVect(random()/(0.0+RAND_MAX),0,random()/(0.0+RAND_MAX));

  orientation.set(pVect(0,1,0),0);
  angular_momentum = pVect(0,0,0);
  debug_pair_calls = 0;
}

Ball::~Ball()
{ 
  if ( query_occlusion_id )
    glDeleteQueries(1,&query_occlusion_id);

  if ( original ) return;
  ASSERTS( ! ( w.data_location & DL_ALL_CUDA ) );
  w.cuda_schedule_stale = 1;
}

bool
World::platform_collision_possible(pCoor pos, float ts_mov_max)
{
  return pos.y < opt_ball_radius + ts_mov_max
    && pos.x - ts_mov_max >= platform_xmin_mr
    && pos.x + ts_mov_max <= platform_xmax_pr
    && pos.z - ts_mov_max >= platform_zmin_mr
    && pos.z + ts_mov_max <= platform_zmax_pr;
}

 /// External Modifications to State
//
//   These allow the user to play with state while simulation
//   running.

// Move the ball.
//
void Ball::translate(pVect amt) {position += amt;}

// Add velocity to the ball.
//
void Ball::push(pVect amt) {velocity += amt;}

// Set the velocity to zero.
//
void Ball::stop()
{
  velocity = pVect(0,0,0);
}

// Remove all but one ball.
//
void World::balls_remove()
{
  cuda_data_to_cpu(DL_ALL);
  data_location = DL_ALL_CPU;
  Ball* const survivor = balls.pop();
  while ( balls.occ() ) delete balls.pop();
  balls += survivor;
}

void World::balls_stop()
{
  for ( Ball *ball; balls.iterate(ball); ) ball->stop();
}
void World::balls_rot_stop()
{
  for ( Ball *ball; balls.iterate(ball); )
    ball->angular_momentum = pVect(0,0,0);
}

bool
World::penetration_balls_resolve(Ball *ball1, Ball *ball2, bool b2_real)
{
  /// Update velocity and angular momentum for a pair of balls in contact.

  const pNorm dist(ball1->position,ball2->position);

  // Return if balls aren't touching.  Note avoidance of square root.
  //
  if ( dist.mag_sq >= two_r_sq ) return false;

  // Update counters used for optimization (contact_count) and
  // to decide when to release new balls (collision_count).
  //
  ball1->collision_count++; ball1->contact_count++; 
  if ( b2_real ) {ball2->collision_count++; ball2->contact_count++;}

  // Compute relative (approach) velocity.
  //
  pVect prev_appr_vel = ball1->prev_velocity - ball2->prev_velocity;
  const double prev_approach_speed = dot( prev_appr_vel, dist );

  const double loss_factor = 1 - opt_bounce_loss;

  // Compute change in speed based on how close balls touching, ignoring
  // energy loss.
  //
  const double appr_deltas_no_loss =
    ( two_r - dist.magnitude ) * elasticity_inv_dt * ball_mass_inv;

  // Change in speed accounting for energy loss. Only applied when
  // balls separating.
  //
  const double appr_deltas =
    prev_approach_speed > 0
    ? appr_deltas_no_loss : loss_factor * appr_deltas_no_loss;

  /// Update Linear Velocity
  //
  ball1->velocity -= appr_deltas * dist;
  if ( b2_real ) ball2->velocity += appr_deltas * dist;

  // Find speed on surface of balls at point of contact.
  //
  pVect tact1_rot_vel = ball1->point_rot_vel(dist);
  pVect tact2_rot_vel = ball2->point_rot_vel(-dist);

  // Find relative velocity of surfaces at point of contact
  // in the plane formed by their surfaces.
  //
  pVect tan_vel = prev_appr_vel - prev_approach_speed * dist;
  pNorm tact_vel_dir = tact1_rot_vel - tact2_rot_vel + tan_vel;

  // Find change in velocity due to friction.
  //
  const double fric_dv_potential =
    fabs(appr_deltas_no_loss) * opt_friction_coeff;
  const double dv_limit_raw = tact_vel_dir.magnitude * mo_vel_factor;
  const double dv_limit = b2_real ? dv_limit_raw : 2 * dv_limit_raw;

  // If true, surfaces are not sliding or will stop sliding after
  // frictional forces applied. (If a ball surface isn't sliding
  // against another surface than it must be rolling.)
  //
  const bool will_roll = dv_limit <= fric_dv_potential;
  const double sliding_fric_deltav =
    will_roll ? dv_limit : fric_dv_potential;

  const double dv_tolerance = 0.000001;

  if ( sliding_fric_deltav > dv_tolerance )
    {
      // Apply frictional force.

      // Compute change in angular momentum due to friction.
      //
      const double fric_deltao = sliding_fric_deltav * v_to_do;

      // Apply torque (resulting in angular momentum change) and
      // linear force (resulting in velocity change).
      //
      ball1->apply_deltao(dist,tact_vel_dir,-fric_deltao);
      ball1->velocity -= sliding_fric_deltav * tact_vel_dir;

      // Ditto for the other ball, if it's real.
      if ( b2_real )
        {
          ball2->apply_deltao(-dist,tact_vel_dir,fric_deltao);
          ball2->velocity += sliding_fric_deltav * tact_vel_dir;;
        }
    }

  // Check for correctness.
  //
  if ( false && opt_verify && b2_real && will_roll )
    {
      // Check only works if current velocity used for friction calc.
      //
      pVect appr_vel2 = ball1->velocity - ball2->velocity;
      const double approach_speed2 = dot( appr_vel2, dist );

      pVect tact1_rot_vel2 = ball1->point_rot_vel(dist);
      pVect tact2_rot_vel2 = ball2->point_rot_vel(-dist);

      pVect tan_vel2 = appr_vel2 - approach_speed2 * dist;
      pNorm tan_vel_dir2 = tact1_rot_vel2 - tact2_rot_vel2 + tan_vel2;
      ASSERTS( tan_vel_dir2.magnitude <= 0.0001 + 100 * dv_tolerance );
      ball1->color_event = ball2->color_event = pColor(1,1,1);
    }

  {
    /// Torque
    //
    //
    // Account for forces of surfaces twisting against each
    // other. (For example, if one ball is spinning on top of
    // another.)
    //
    const double appr_omega =
      dot(ball1->angular_momentum,dist) - dot(ball2->angular_momentum,dist);
    const double fric_deltao_pot = fric_dv_potential * v_to_do;
    const bool rev = appr_omega < 0;
    const double fric_deltao = min(fabs(appr_omega),fric_deltao_pot);
    pVect delta_am = rev ? -fric_deltao * dist : fric_deltao * dist;
    ball1->angular_momentum -= delta_am;
    if ( b2_real ) ball2->angular_momentum += delta_am;
  }

  {
    /// Rolling Friction
    //

    // The rolling friction model used here is ad-hoc.

    pVect tan_b12_vel = b2_real ? 0.5 * tan_vel : pVect(0,0,0);
    const double torque_limit_sort_of = appr_deltas_no_loss
      * sqrt( opt_ball_radius - 0.25 * dist.mag_sq * r_inv );

    pVect tact1_rot_vel = ball1->point_rot_vel(dist);
    pVect tact1_roll_vel = tact1_rot_vel + tan_b12_vel;
    pNorm tact1_roll_vel_dir = tact1_roll_vel;
    pVect lost_vel(0,0,0);

    const double rfric_loss1 =
      torque_limit_sort_of *
      ( tact1_roll_vel_dir.magnitude * opt_friction_roll /
        ( 1 + tact1_roll_vel_dir.magnitude * opt_friction_roll ) );
    
    pVect lost_vel1 =
      min(tact1_roll_vel_dir.magnitude, rfric_loss1) * tact1_roll_vel_dir;

    lost_vel = -lost_vel1;
    
    if ( b2_real )
      {
        pVect tact2_rot_vel = ball2->point_rot_vel(-dist);
        pVect tact2_roll_vel = tact2_rot_vel - tan_b12_vel;
        pNorm tact2_roll_vel_dir = tact2_roll_vel;
        const double rfric_loss2 =
          torque_limit_sort_of *
          ( tact2_roll_vel_dir.magnitude * opt_friction_roll /
            ( 1 + tact2_roll_vel_dir.magnitude * opt_friction_roll ) );
        pVect lost_vel2 =
          min(tact2_roll_vel_dir.magnitude, rfric_loss2 ) * tact2_roll_vel_dir;

        lost_vel += lost_vel2;
      }

    ball1->apply_tan_dv(dist,lost_vel);
    if ( b2_real ) ball2->apply_tan_dv(dist,lost_vel);

    if ( opt_verify )
      {
        pVect ch_tact1_rot_vel = ball1->point_rot_vel(dist);
        pVect ch_tact1_roll_vel = ch_tact1_rot_vel + tan_b12_vel;
        const double magloss = tact1_roll_vel.mag() - ch_tact1_roll_vel.mag();
        ASSERTS( magloss >= -10.0 );
      }
  }

  return true;
}

pVect
Ball::point_rot_vel(pNorm direction)
{
  /// Return velocity of point on surface of ball.
  //
  return w.opt_ball_radius * cross( angular_momentum, direction );
}

void
Ball::apply_deltao(pNorm tact_dir, pNorm force_dir, double deltao)
{
  /// Change angular momentum due to torque in direction force_dir.
  //
  apply_tan_do(tact_dir, deltao * force_dir );
}

void
Ball::apply_tan_do(pNorm tact_dir, pVect force)
{
  /// Change angular momentum due to force (already accounts for time).
  //
  pVect axis_torque = cross( tact_dir, force );
  angular_momentum += axis_torque;
}

void
Ball::apply_tan_dv(pNorm tact_dir, pVect force)
{
  apply_tan_do(tact_dir, force * w.r_inv );
}



void
World::time_step_cpu()
{
  const float deep = -100;

  if ( data_location & DL_ALL_CUDA )
    {
      cuda_data_to_cpu(DL_ALL);
      data_location = DL_ALL_CPU;
    }

  /// Remove balls that have fallen away from the platform.
  //
  for ( Ball *ball; balls.iterate(ball); )
    if ( ball->position.y < deep ) { balls.iterate_yank(); delete ball; }

  for ( Ball *ball; balls.iterate(ball); )
    {
      ball->prev_angular_momentum = ball->angular_momentum;
      ball->prev_velocity = ball->velocity;
      ball->contact_count = 0;
    }

  /// Sort balls in z in preparation for finding balls that touch.
  //
  const float region_length = two_r;
  PSList<Ball*,double> z;
  for ( Ball *ball; balls.iterate(ball); ) z.insert(ball->position.z,ball);
  z.sort();

  /// Apply forces for balls that are touching.
  //
  for ( int idx0 = 0, idx9 = 0; idx9 < z.occ(); idx9++ )
    {
      Ball* const ball9 = z[idx9];
      const float z_min = ball9->position.z - region_length;

      while ( idx0 < idx9 && z[idx0]->position.z < z_min ) idx0++;

      for ( int i=idx0; i<idx9; i++ )
        penetration_balls_resolve(z[i],ball9);
    }

  /// Apply gravitational force.
  //
  for ( Ball *ball; balls.iterate(ball); ) ball->velocity += gravity_accel_dt;

  /// Apply force for platform contact.
  //
  for ( Ball *ball; balls.iterate(ball); )
    {
      const pCoor pos(ball->position);
      if ( !platform_collision_possible(pos) ) continue;
      pCoor axis(platform_xmid,0,pos.z);

      // Test for different ways ball can touch platform. If contact
      // is found find position of an artificial platform ball (pball)
      // that touches the real ball at the same place and angle as
      // the platform. This pball will be used for the ball-ball penetration
      // routine, penetration_balls_resolve.

      if ( pos.y > 0 )
        {
          // Possible contact with upper edge of platform.
          //
          pCoor tact
            (pos.x > platform_xmid ? platform_xmax : platform_xmin, 0, pos.z);
          pNorm tact_dir(pos,tact);
          if ( tact_dir.mag_sq >= two_r_sq ) continue;
          pball->position = tact + opt_ball_radius * tact_dir;
        }
      else if ( pos.z > platform_zmax || pos.z < platform_zmin )
        {
          // Possible contact with side (curved) edges of platform.
          //
          pNorm ball_dir(axis,pos);
          if ( ball_dir.mag_sq <= short_xrad_sq ) continue;
          const float zedge =
            pos.z > platform_zmax ? platform_zmax : platform_zmin;
          pCoor axis_edge(platform_xmid,0,zedge);
          pCoor tact = axis_edge + platform_xrad * ball_dir;
          pNorm tact_dir(pos,tact);
          if ( tact_dir.mag_sq >= two_r_sq ) continue;
          pball->position = tact + opt_ball_radius * tact_dir;
        }
      else
        {
          // Possible contact with surface of platform.
          //
          pNorm tact_dir(axis,pos);
          if ( tact_dir.mag_sq <= short_xrad_sq ) continue;
          pball->position = axis + (opt_ball_radius+platform_xrad) * tact_dir;
        }

      // Finish initializing platform ball, and call routine to
      // resolve penetration.
      //
      pball->angular_momentum = pVect(0,0,0);
      pball->velocity = pVect(0,0,0);
      penetration_balls_resolve(ball,pball,false);
    }

  /// Based on updated velocity, update ball positions.
  //
  for ( Ball *ball; balls.iterate(ball); ) 
    ball->position += delta_t * ball->velocity;

  float contact_y_max = -platform_xrad;

  /// Update orientation of balls. (Also find highest ball that hit something.)
  //
  for ( Ball *ball; balls.iterate(ball); )
    {
      pNorm axis(ball->angular_momentum);

      // If ball isn't spinning fast skip expensive rotation.
      //
      if ( axis.mag_sq < 0.000001 ) continue;

      // Update ball orientation.
      //
      ball->orientation =
        pQuat(axis,delta_t * axis.magnitude) * ball->orientation;

      // Find position of highest ball that has hit something.
      //
      if ( ball->collision_count && ball->position.y > contact_y_max )
        contact_y_max = ball->position.y;
    }

  /// If dripping is on, release a new ball if last one hit something.
  //
  if ( opt_drip && ( !dball || dball->collision_count ) )
    {
      dball = new Ball(this);
      dball->position =
        pCoor(30,max(20.0f,contact_y_max) + 3 * opt_ball_radius,60);
      dball->velocity = pVect(0,0,0);
      dball->color_natural = *colors[ ( drip_cnt >> drip_run ) & colors_mask ];
      drip_cnt++;
      balls += dball;
    }

  /// If spray is on, release a new ball if it's time.
  //
  ball_countdown -= delta_t;
  if ( opt_spray_on && ball_countdown <= 0 || balls.occ() == 0 )
    {
      Ball* const nball = new Ball(this);
      double r = opt_ball_radius * 5;
      double c = 2 * M_PI * r;
      const double delta_theta =
        0.0001 + 2 * M_PI / ( c / (2 * opt_ball_radius ) );
      static double th = 0;  th += delta_theta;
      nball->position.x = platform_xmax - r - 2 * opt_ball_radius
        + (r+opt_ball_radius) * cos(th);
      nball->position.z = (r+opt_ball_radius) * sin(th);
      nball->position.y = max(20.0f,contact_y_max + 3 * opt_ball_radius);
      nball->color_natural = *colors[ ( spray_cnt>>spray_run ) & colors_mask ];
      spray_cnt++;
      balls += nball;
      ball_countdown = 0.1;
    }
}


class Contact {
public:
  Contact():ball1(NULL),ball2(NULL){};
  Contact(Ball *ball1, Ball *ball2):
    ball1(ball1),ball2(ball2),placed(false){};
  Ball* const ball1;
  Ball* const ball2;
  Ball* other_ball(Ball* ball) { return ball == ball1 ? ball2 : ball1; }
  bool placed;
  int pass;
  int block;
  int col;
  int round;
  int sort_idx;
};

#if 0
void
World::schedule(PStack<Contact>& pairs)
{
  const float region_length = 3 * opt_ball_radius;
  const float rl_inv = 1.0 / region_length;
  const int hash_bits = 24;

  PStack<Ball*> groups[8];

  PStack<Ball*> unplaced;

  for ( Contact *c; pairs.iterate(c); )
    {
      
      if ( !b->proximity.occ() ) continue;
      const int bin_x = b->position.x * rl_inv;
      const int bin_y = b->position.y * rl_inv;
      const int bin_z = b->position.z * rl_inv;
      const int group =
        ( ( bin_x & 1 ) << 2 ) |
        ( ( bin_y & 1 ) << 1 ) | bin_z & 1;
      const int hash = 
        ( ( bin_x & 0xff ) << 16 ) |
        ( ( bin_y & 0xff ) << 8 ) | bin_z & 0xff;
      b->group = group;
      b->hash = hash;
      b->block = -1;
      groups[group] += b;
      b->pass = -1;
      unplaced += b;
    }

  int pass_num = -1;
  int block_num = -1;

  PSList<Ball*,int64_t> pre_sched;

  while ( unplaced.occ() )
    {
      Ball* const b = unplaced.pop();
      if ( b->block >= 0 ) continue;
      block_num++;
      PQueue<Ball*> stack(b);
      PStack<Ball*> block;
      int ball_count = 0;
      for ( Ball *e; stack.pop(e); )
        {
          b->block = block_num;
          b->sort_key =
            block_num << (hash_bits + 3)
            + ( b->group << hash_bits ) + b->hash;
          pre_sched.insert(b->sort_key,b);
          block += e;
          if ( block.occ() >= ball_limit ) break;
          for ( Ball *n = e->proximity.iterate_reset();
                e->proximity.iterate(n); ) stack += n;
        }
    }

  while ( Ball* const b = pre_sched.iterate() )
    {
      
    }


  {
      for ( int g=0; g<8; g++ )
        {
          for ( int col_num = 0; col_num < block_size; col_num++ )
            {
              for ( int round = 0; round < max_rounds; round++ )
                {
                  if ( !groups[g].occ() ) break;
                  Ball* const b = groups[g].pop();
                  
                }
            }
        }
      
    }
}
#endif


void
World::time_step_cuda(bool need_data_p)
{
  const float deep = -100;

  const bool need_data = false || need_data_p;

  pError_Check();
  cuda_init();

  for ( Ball *ball; balls.iterate(ball); )
    if ( ball->position.y < deep )
      {
        cuda_data_to_cpu(DL_ALL);
        data_location = DL_ALL_CPU;
        balls.iterate_yank(); delete ball;
      }

  const int ball_cnt = balls.occ();
  const int block_size = 32;

  dim3 dim_grid, dim_block;
  dim_grid.x = int(ceil(double(ball_cnt)/block_size));
  dim_grid.y = dim_grid.z = 1;
  dim_block.x = block_size;
  dim_block.y = dim_block.z = 1;

  const bool cuda_updated = cuda_schedule_stale > 0;

  if ( cuda_updated )
    {
      cuda_schedule_stale = -10;
      cuda_data_to_cpu(DL_ALL);

      double max_vsq = 0;

      for ( Ball *ball; balls.iterate(ball); )
        {
          const double vsq = dot(ball->velocity,ball->velocity);
          set_max(max_vsq,vsq);
          ball->idx = balls.iterate_get_idx();
          ball->prev_velocity = ball->velocity;
          ball->prev_angular_momentum = ball->angular_momentum;
          ball->debug_pair_calls = 0;
          ball->contact_count = 0;
        }

      const double radii =
        sqrt(max_vsq) * ( -cuda_schedule_stale ) * delta_t / opt_ball_radius;

      static double mradii = 0;
      if ( radii > mradii )
        {
          mradii = radii;
          printf("Fastest ball can travel %.3f radii\n",radii);
        }

      const float region_length =
        ( 2.1 + min(radii,2.0) ) * opt_ball_radius;
      const double prox_dist_sq = pow( region_length, 2 );

      PStack<Contact> pairs;

      PSList<Ball*,double> z;
      for ( Ball *ball; balls.iterate(ball); )
        {
          ball->proximity.reset();
          z.insert(ball->position.z,ball);
        }
      z.sort();
      for ( int idx0 = 0, idx9 = 0; idx9 < z.occ(); idx9++ )
        {
          Ball* const ball9 = z[idx9];
          const float z_min = ball9->position.z - region_length;

          while ( idx0 < idx9 && z[idx0]->position.z < z_min ) idx0++;

          for ( int i=idx0; i<idx9; i++ )
            {
              Ball* const ball1 = z[i];

              pNorm dist(ball1->position,ball9->position);
              if ( dist.mag_sq > prox_dist_sq ) continue;

              const int c_idx = pairs.occ();
              new (pairs.pushi()) Contact(ball1,ball9);

              ball1->proximity += c_idx;
              ball9->proximity += c_idx;
            }
        }

      //  schedule(pairs);

      PQueue<Ball*> work[2];
      int w = 0;
      int n = 1-w;

      for ( Ball *b; balls.iterate(b); )
        if ( b->proximity.occ() )
          {
            b->pass = -1;
            b->pass_iterated = -1;
            b->pass_todo = 0;
            work[w] += b;
          }

      int pass_num = 0;
      int block_num = 0;
      int ball_count = 0;
      int col_num = 0;
      const int max_rounds = 32;
      const int ball_limit = BALLS_PER_BLOCK;
      int total_rounds = 0;
      int max_blocks = 0;
      int pass_max_rounds = 0;
      int iterations = 0;
      const int block_cnt_limit = 2048;

      while ( true )
        {
          if ( !work[w].occ() )
            {
              total_rounds += pass_max_rounds;
              set_max(max_blocks,block_num+1);
              if ( !work[n].occ() ) break;
              pass_max_rounds = 0;

              w = 1-w;  n = 1-n;

              pass_num++;
              ball_count = block_num = col_num = 0;
            }

          ASSERTS( ball_count <= ball_limit );

          if ( col_num == block_size || ball_count == ball_limit )
            {
              ball_count = 0;  col_num = 0;  block_num++;
            }

          Ball* const ball1 = work[w].pop();

          if ( ball1->pass_iterated == pass_num ) continue;

          ball1->pass_iterated = pass_num;

          if ( ball1->pass < pass_num )
            {
              ball1->pass = pass_num;
              ball1->placed = false;
              ball1->rounds = 0;
            }

          if ( ball1->placed && ball1->block != block_num )
            {
              if ( ball1->pass_todo <= pass_num )
                {
                  ball1->pass_todo = pass_num + 1;
                  work[n] += ball1;
                }
              continue;
            }

          int round = 0;
          int placed_count = 0;

          for ( int c_idx = ball1->proximity.iterate_reset();
                ball1->proximity.iterate(c_idx); )
            {
              iterations++;
              Contact* const c = &pairs[c_idx];
              if ( c->placed ) continue;
              Ball* const ball2 = c->other_ball(ball1);

              if ( ball2->pass < pass_num )
                {
                  ball2->pass = pass_num;
                  ball2->placed = false;
                  ball2->rounds = 0;
                }

              if ( ball2->placed && ball2->block != block_num )
                { 
                  if ( ball1->pass_todo <= pass_num )
                    {
                      ball1->pass_todo = pass_num + 1;
                      work[n] += ball1;
                    }
                  continue;
                }

              uint32_t mask = 0;
              while ( round < max_rounds )
                {
                  mask = 1 << round;
                  if ( !( ball1->rounds & mask || ball2->rounds & mask ) )
                    break;
                  iterations++;
                  round++;
                }
              if ( round == max_rounds )
                {
                  work[n] += ball1;
                  break;
                }

              const int next_ball_count =
                ball_count + !ball1->placed + !ball2->placed;
              if ( next_ball_count > ball_limit )
                {
                  work[n] += ball1;
                  break;
                }

              if ( !ball1->placed )
                {
                  ball1->placed = true;
                  ball1->block = block_num;
                }
              if ( !ball2->placed )
                {
                  ball2->placed = true;
                  ball2->block = block_num;
                  work[w].enqueue_at_head(ball2);
                }
              ball_count = next_ball_count;
              placed_count++;
              c->placed = true;
              c->pass = pass_num;  c->block = block_num;
              c->col = col_num; c->round = round;
              ball1->rounds |= mask;
              ball2->rounds |= mask;
              round++;
              set_max(pass_max_rounds,round);
            }

          if ( placed_count ) col_num++;
        }

      static int total_rounds_max = 0;
      if ( total_rounds > total_rounds_max )
        {
          printf("For %d tacts, Number of passes %d, Total Rounds %d, "
                 "Eff %.4f  Max Bl %d\n",
                 pairs.occ(),
                 pass_num,
                 total_rounds,
                 pairs.occ() / double(iterations),
                 max_blocks
                 );
          total_rounds_max = total_rounds;
        }

      PSList<Contact*,int64_t> pair_check;
      while ( Contact* const c = pairs.iterate() )
        pair_check.insert
          ( 
           c->pass * block_cnt_limit * max_rounds * block_size
           + c->block * max_rounds * block_size
           + c->round * block_size
           + int64_t(c->col) , 
           c );
      pair_check.sort();
      for ( Ball *ball; balls.iterate(ball); )
        {
          ball->pass = -1;
        }

      while ( Contact* const c = pair_check.iterate() )  
        {
          const int pass = c->pass;
          const int round = c->round;
          const int block = c->block;
          c->sort_idx = pair_check.iterate_get_idx();
          ASSERTS( c->placed );
          for ( int i=0; i<2; i++ )
            {
              Ball* const b = i ? c->ball2 : c->ball1;
              ASSERTS( b->pass < pass
                       || b->block == block && b->rounds < round );
              b->pass = pass;
              b->block = block;
              b->rounds = round;
            }
        }
  
      const int prefetch_rounds = int( ceil(double(ball_limit)/block_size) );
      const int prefetch_elts_per_block = prefetch_rounds * block_size;

      passes.reset();

      while ( Contact* const c = pair_check.iterate() )
        {
          const int pass = c->pass;
          const int round_cnt = c->round + 1;
          const int block_cnt = c->block + 1;
          if ( pass >= passes.occ() ) passes.pushi();
          Pass& p = passes[pass];
          set_max(p.block_cnt,block_cnt);
          set_max(p.round_cnt,round_cnt);
        }

      const int pass_cnt = passes.occ();

      int ba_size = 0;
      int pa_size = 0;

      for ( int i=0; i<pass_cnt; i++ )
        {
          Pass& p = passes[i];
          p.dim_block.x = block_size;
          p.dim_block.y = p.dim_block.z = 1;
          p.dim_grid.x = p.block_cnt;
          p.dim_grid.y = p.dim_grid.z = 1;
          p.prefetch_offset = pa_size;
          pa_size += prefetch_elts_per_block * p.block_cnt;
          p.schedule_offset = ba_size;
          ba_size += block_size * p.block_cnt * p.round_cnt;
        }

      big_array.realloc(ba_size);
      big_array.ptrs_to_cuda("schedule");
      memset(big_array.data,-1,big_array.chars);

      prefetch_array.realloc(pa_size);
      prefetch_array.ptrs_to_cuda("schedule_inputs");
      memset(prefetch_array.data,-1,prefetch_array.chars);

      int ball_sidx_next = -1;
      int pass_curr = -1;
      int block_curr = -1;

      for ( Ball *ball; balls.iterate(ball); )
        {
          ball->pass = -1;
        }

      while ( Contact* const c = pair_check.iterate() )
        {
          const int pass = c->pass;
          const int round = c->round;
          const int block = c->block;
          Pass& p = passes[pass];

          if ( pass_curr < pass )
            {
              pass_curr = pass;
              block_curr = -1;
            }

          if ( block_curr != block )
            {
              block_curr = block;
              ball_sidx_next = 0;
            }

          for ( int i=0; i<2; i++ )
            {
              Ball* const b = i ? c->ball2 : c->ball1;
              if ( b->pass < pass )
                {
                  ASSERTS( ball_sidx_next < ball_limit );
                  const int idx =
                    p.prefetch_offset
                    + block * prefetch_elts_per_block
                    + ball_sidx_next;
                  ASSERTS( prefetch_array[idx] == -1 );
                  b->sm_idx = ball_sidx_next;
                  b->pass = pass;
                  b->block = block;
                  prefetch_array[ idx ] = b->idx;
                  ball_sidx_next++;
                }
            }
          SM_Idx2 pair = { c->ball1->sm_idx, c->ball2->sm_idx };
          const int ba_idx =
            p.schedule_offset
            + block * p.round_cnt * block_size
            + round * block_size
            + c->col;
          ASSERTS( ba_idx < ba_size && ba_idx >= 0 );
          SM_Idx2 old_pair = big_array[ba_idx];
          ASSERTS( old_pair.x == 0xffff && old_pair.y == 0xffff );
          big_array[ ba_idx ] = pair;

        }
      prefetch_array.to_cuda();
      big_array.to_cuda();
    }

  cuda_schedule_stale++;

  Ball ball_a = *balls[0];
  Ball ball_a1 =*balls[min(1,balls.occ())];

  CE(cudaEventRecord(frame_start_ce,0));

  cpu_data_to_cuda();

  const int read_side = cuda_balls.current;

  data_location = DL_ALL_CUDA;
  while ( Pass* const p = passes.iterate() )
    {
      pass_pairs_launch
        (p->dim_grid,p->dim_block,read_side,
         p->prefetch_offset,p->schedule_offset,p->round_cnt);
    }

  pass_something_launch(dim_grid,dim_block,cuda_balls.current,balls.occ());

  if ( need_data )
    cuda_data_to_cpu(DL_ALL);

  Ball ball_b = *balls[0];

  if ( data_location & DL_OT_CPU )
  for ( Ball *ball; balls.iterate(ball); )
    {
      const int px = ball->proximity.occ();
      const int ca = ball->debug_pair_calls >> 16;
      ASSERTS( true || px == ca );
    }

  CE(cudaEventRecord(frame_stop_ce,0));
  CE(cudaEventSynchronize(frame_stop_ce));
  float cuda_time = -1.1;
  CE(cudaEventElapsedTime(&cuda_time,frame_start_ce,frame_stop_ce));
  frame_timer.cuda_frame_time_set(cuda_time);
  pError_Check();

  ball_countdown -= delta_t;

  if ( ! ( data_location & DL_CV_CPU ) ) return;

  float contact_y_max = -platform_xrad;

  for ( Ball *ball; balls.iterate(ball); )
    if ( ball->collision_count ) set_max(contact_y_max,ball->position.y);

  if ( opt_drip && ( !dball || dball->collision_count ) )
    {
      dball = new Ball(this);
      dball->position =
        pCoor(30,max(20.0f,contact_y_max) + 3 * opt_ball_radius,60);
      dball->velocity = pVect(0,0,0);
      dball->color_natural = *colors[ ( drip_cnt >> drip_run ) & colors_mask ];
      drip_cnt++;
      balls += dball;
    }

  if ( opt_spray_on && ball_countdown <= 0 || balls.occ() == 0 )
    {
      Ball* const nball = new Ball(this);
      double r = opt_ball_radius * 5;
      double c = 2 * M_PI * r;
      const double delta_theta =
        0.0001 + 2 * M_PI / ( c / (2 * opt_ball_radius ) );
      static double th = 0;  th += delta_theta;
      nball->position.x = platform_xmax - r - 2 * opt_ball_radius
        + (r+opt_ball_radius) * cos(th);
      nball->position.z = (r+opt_ball_radius) * sin(th);
      nball->position.y = max(20.0f,contact_y_max + 3 * opt_ball_radius);
      nball->color_natural = *colors[ ( spray_cnt>>spray_run ) & colors_mask ];
      spray_cnt++;
      balls += nball;
      ball_countdown = 0.1;
    }

}

#include <complex>
using std::complex;
typedef complex<double> cdouble;

double
alhazan_check(pCoor center,pCoor eye,pCoor vertex,pCoor mirror)
{
  pNorm me(mirror,eye);
  pNorm mv(mirror,vertex);
  pNorm mc(mirror,center);
  const double de = dot(me,mc);
  const double dv = dot(mv,mc);
  const double err = fabs(dv-de);
  return err;
}

pCoor
alhazan(pCoor center, double radius, pCoor eye, pCoor vertex)
{
  //  http://mathworld.wolfram.com/AlhazensBilliardProblem.html
  //  http://www.math.sjsu.edu/~alperin/Alhazen.pdf

  pVect c_to_e(center,eye);
  pVect c_to_v(center,vertex);

  pNorm dir_a(c_to_e);
  pNorm dir_b(c_to_v);

  const double close = 0.001;

#define RETURN(rvp)                                                           \
  {                                                                           \
    pCoor rv = rvp;                                                           \
    double err = alhazan_check(center,eye,vertex,rv);                         \
    ASSERTS( err < 0.001 );                                                   \
    return rv;                                                                \
  }

  if ( dir_a.magnitude < close ) RETURN(center + radius * dir_b);
  if ( dir_b.magnitude < close ) RETURN(center + radius * dir_a);

  const double ab_dot = dot(dir_a,dir_b);

  pNorm e_to_v(eye,vertex);

  const double rad_inv = 1/radius;

  pNorm dir_ab( dir_a + dir_b );
  pNorm dir_ev( c_to_e + c_to_v );

  pCoor rv1 = center + radius * dir_ab;
  pCoor rv2 = center + radius * ( -dir_ev + 2 * dir_ab );
  pCoor rv3 = center + radius * ( 2 * dir_ev -  dir_ab );
  pCoor rva = center + radius * dir_a;
  pCoor rvb = center + radius * dir_b;
  double err1 = alhazan_check(center,eye,vertex,rv1);
  double err2 = alhazan_check(center,eye,vertex,rv2);
  double err3 = alhazan_check(center,eye,vertex,rv3);
  double erra = alhazan_check(center,eye,vertex,rva);
  double errb = alhazan_check(center,eye,vertex,rvb);

  if ( fabs(ab_dot) > 0.9999 )
    {
      if ( err1 < errb ) RETURN(rv1);
      RETURN(rvb);
    }

  const double cos_th = dir_ab.x;
  const double sin_th = -dir_ab.y;

  pCoor mx(0,0,0,0);

  static int bail_count = 0;

# define BAIL(reason) \
  { bail_count++; \
    if ( bail_count < 10 ) \
      printf("Can't solve, (%6.3f,%6.3f)  (%6.3f,%6.3f)  %s\n", \
             a.x,a.y, b.x, b.y, reason); \
    return mx; \
  } 

  pCoor a;
  a.x = eye.x * cos_th - eye.y * sin_th;
  a.y = eye.x * sin_th + eye.y * cos_th;
  a.z = 0; a.w = 1;
  pCoor b;
  b.x = vertex.x * cos_th - vertex.y * sin_th;
  b.y = vertex.x * sin_th + vertex.y * cos_th;
  b.z = 0; b.w = 1;

  a = a * rad_inv;
  b = b * rad_inv;

  const double p = a.x * b.y + a.y * b.x;
  const double q = a.x * b.x - a.y * b.y;
  const double r = a.x + b.x;
  const double s = a.y + b.y;
  const double ssq = s * s;
  const double qsq = q * q;
  const double rsq = r * r;

  ASSERTS( fabs(p) < 0.001 );

  const double k1 = (-4.0 * qsq + rsq + ssq);
  const double k1cu = k1 * k1 * k1;
  const double k2 = 2.0 * k1cu + 432.0 * qsq * ssq * ( k1 + 4.0 * qsq - ssq );

  const double k4 = 4 * qsq - rsq - ssq;
  const double k4sq = k4 * k4;

  const double k3pre = -4.0 * k1cu * k1cu + k2 * k2;

  const cdouble ck3pre = k3pre;
  const cdouble ck3 = k2 + sqrt( ck3pre );
  const cdouble ck3cr = pow(ck3,1/3.);
  const cdouble ck21 =
    1.5874010519681996 * ck3cr + 2.5198420997897464 * k4sq / ck3cr;

  const bool k3_neg = k3pre < 0;
  const double k3sp = sqrt( fabs(k3pre) );

  const double k3_re = k2 + ( k3_neg ? 0 : k3sp );
  const double k3_imsq = max(-k3pre,0.0);
  const double k3_im = k3_neg ? k3sp : 0;
  const double k3_car = (1/3.) * atan2(k3_im,k3_re);
  const double k3cr_cm = pow( k3_re * k3_re + k3_imsq, 1/6.);
  const double k3_real_n = cos(k3_car);
  const double k21 = 
    k3_real_n *
    ( 1.5874010519681996 * k3cr_cm + 2.5198420997897464 * k4sq / k3cr_cm );

  //  if ( k3pre < 0 ) BAIL("k3pre");

  const double k20 = sqrt( -4.*k1 + k21 + 6.*ssq );

  const double k6 = 8.*k1 + k21 - 12.*ssq;
  const double k7 = 29.393876913398135 * s * (k1 + 8.*qsq - ssq) / k20;

  const double k14 = -k6 - k7;
  const double k15 = -k6 + k7;

  const double k9 = 0.25 * s;
  const double k11b = 2.449489742783178 / 24.0;
  const double k10 = k11b * k20;

  const bool set1 = k14 >= 0;
  const bool set2 = k15 >= 0;

  if ( !set1 && k15 < 0 ) BAIL("both");

  const double qinv = 1/q;

  const double k14sr = k11b * sqrt( k14 );
  const double k15sr = k11b * sqrt( k15 );

  const double ys11 = ( -k9 - k10 - k14sr ) * qinv;
  const double ys12 = ( -k9 - k10 + k14sr ) * qinv;
  const double ys21 = ( -k9 + k10 - k15sr ) * qinv;
  const double ys22 = ( -k9 + k10 + k15sr ) * qinv;

  const double y1 = set1 ? ys11 : ys21;
  const double y2 = set1 ? ys12 : ys22;

  const double x1t = r * y1 / ( 2 * q * y1 + s );
  const double x2t = r * y2 / ( 2 * q * y2 + s );

  const double x1m = sqrt( 1 - y1 * y1 );
  const double x2m = sqrt( 1 - y2 * y2 );
  const double x1 = x1t < 0 ? -x1m : x1m;
  const double x2 = x2t < 0 ? -x2m : x2m;

  //  pCoor m1(x1a * cos_th + y1 * sin_th, -x1a * sin_th + y1 * cos_th, 0, 1);
  //  pCoor m2(x2a * cos_th + y2 * sin_th, -x2a * sin_th + y2 * cos_th, 0, 1);

#if 0
  pNorm m1n(x1,y1,0),m2n(x2,y2,0);

  const bool sol1 = fabs(m1n.magnitude - 1 ) < 0.01;
  const bool sol2 = fabs(m2n.magnitude - 1 ) < 0.01;

  double errs1 = alhazan_check(center,eye,vertex,m1);
  double errs1n = alhazan_check(center,eye,vertex,m1n);
  double errs2 = alhazan_check(center,eye,vertex,m2);
  double errs2n = alhazan_check(center,eye,vertex,m2n);

  ASSERTS( sol1 && sol2 );
#endif

  const double y = y2;
  const double xt = r * y / ( 2 * q * y + s );
  const double xm = sqrt( 1 - y * y );
  const double x = xt < 0 ? -xm : xm;

  pCoor m;
  m.x = x * cos_th + y * sin_th;
  m.y = -x * sin_th + y * cos_th;
  m.z = 0; m.w = 1;
  pNorm mc(-m);
  ASSERTS( fabs( mc.magnitude - 1 ) < 0.01 );

  m = m * radius;
  pNorm ma(m,eye);
  pNorm mb(m,vertex);

  const double da = dot(ma,mc);
  const double db = dot(mb,mc);

  const double err = fabs(da-db);
  const bool okay = err < 0.0001;
  //  ASSERTS( okay );
  static int msg = 10;
  if ( msg-- > 0 )
    printf("Okay %d, set1/2, %d/%d (%.3f,%.3f) (%.3f,%.3f) (%.3f,%.3f) %.3f\n",
           okay, set1,set2, 
           a.x, a.y, b.x, b.y, x,y, m.y);
  
  if ( !okay ) BAIL("not okay");
  RETURN( m );
}


Sphere*
World::sphere_get(Ball *ball)
{
  const float dist = distance(ball->position,eye_location) - opt_ball_radius;
  const int lod_raw =
    int( 0.99 + sphere_lod_factor / dist - sphere_lod_offset );
  const int lod = max(min(lod_raw,sphere_count-1),0);
  return &spheres[lod];
}

void
World::balls_render_simple()
{
  // Render balls without textures. Intended for casting shadows.

  for ( Ball *ball; balls.iterate(ball); )
    {
      const int c = ball->contact_count;

      // Assume that a ball in contact with more than 6 others
      // won't cast a visible shadow.
      //
      if ( c > 6 ) continue;

      Sphere* const s = opt_pause ? &sphere : sphere_get(ball);

      s->render_simple(ball->position);
    }
}

void
World::balls_render(bool regular)
{

  // Sort balls by distance from user's eye.
  // This is needed for the occlusion test.
  //
  PSList<Ball*,double> eye_dist;
  for ( Ball *ball; balls.iterate(ball); )
    {
      pVect ve(ball->position,eye_location);
      eye_dist.insert(dot(ve,ve),ball);
    }
  eye_dist.sort();

  balls_occluded = 0;

  for ( Ball *ball; eye_dist.iterate(ball); )
    {
      pColor color =
        opt_color_events ? ball->color_event : ball->color_natural;

      // Retrieve the result of an occlusion test on this ball.
      //
      while ( ball->occlusion_query_active )
        {
          GLint avail = -1;
          glGetQueryObjectiv
            (ball->query_occlusion_id,GL_QUERY_RESULT_AVAILABLE,&avail);
          if ( !avail ) break;
          GLint samples_passed = -1;
          glGetQueryObjectiv
            (ball->query_occlusion_id,GL_QUERY_RESULT,&samples_passed);
          ball->occlusion_query_active = false;
          ball->occluded = samples_passed == 0;
          if ( ball->occluded ) ball->occluded_run++;
          else                  ball->occluded_run = 0;
          ball->occlusion_countdown = 3;
          break;
        }

      if ( ball->occluded ) balls_occluded++;
      
      // Decide whether to perform an occlusion test.
      //
      const bool do_ot = regular && ball->occlusion_countdown-- == 0;

      // Don't render this ball because it hasn't resulted in
      // anything being written to the frame buffer more than
      // 10 consecutive times.
      //
      if ( ball->occluded_run > 10 && !do_ot ) continue;

      // Maybe start an occlusion query.
      if ( do_ot )
        glBeginQuery(GL_SAMPLES_PASSED,ball->query_occlusion_id);

      if ( ball->occluded_run > 10 )
        {
          // Ball is probably not visible, so render it with
          // a simple sphere.
          //
          sphere_lite.render_simple(ball->position);
        }
      else
        {
          // Get sphere with detail level appropriate for viewer distance.
          //
          Sphere* const s = opt_pause ? &sphere : sphere_get(ball);

          // Set ball's color, position, and orientation, and
          // render it.
          //
          pMatrix_Rotation rot(ball->orientation);
          s->color = color;
          s->render(ball->position,rot);
        }
      if ( do_ot )
        {
          ball->occlusion_query_active = true;
          glEndQuery(GL_SAMPLES_PASSED);
        }
    }
}


void
World::render()
{
  // This routine called whenever window needs to be updated.

  // Get any waiting keyboard commands.
  //
  cb_keyboard();

  // Start a timer object used for tuning this code.
  //
  frame_timer.frame_start();

  const double time_now = time_wall_fp();

  if ( opt_pause || world_time == 0 )
    {
      /// Don't change simulation state.
      //
      world_time = time_now;
    }
  else
    {
      /// Advance simulation state by wall clock time.
      //
      const double elapsed_time = time_now - world_time;
      const int iter_limit = min(10, int ( 0.5 + elapsed_time / delta_t));
      for ( int iter=0; true; iter++ )
        {
          const bool last = iter == iter_limit - 1;
          if ( opt_physics_method == GP_cpu )
            time_step_cpu();
          else
            time_step_cuda(last);

          world_time += delta_t;
          if ( last ) break;
        }
      frame_timer.work_amt_set(iter_limit);
    }

  /// Emit a Graphical Representation of Simulation State
  //
  const int win_width = ogl_helper.get_width();
  const int win_height = ogl_helper.get_height();
  const float aspect = float(win_width) / win_height;

  glMatrixMode(GL_MODELVIEW);
  glLoadTransposeMatrixf(modelview);

  glMatrixMode(GL_PROJECTION);
  // Frustum: left, right, bottom, top, near, far
  pMatrix_Frustum projection(-.8,.8,-.8/aspect,.8/aspect,1,5000);
  glLoadTransposeMatrixf(projection);

  glViewport(0, 0, win_width, win_height);
  pError_Check();

  const double tri_edge_len_px = 5;

  sphere_lod_factor =
    win_width * 2.0 * M_PI * opt_ball_radius
    / ( 1.6 * tri_edge_len_px * sphere_delta_lod );
  sphere_lod_offset = sphere_lod_min / sphere_delta_lod;

  glClearColor(0,0,0,0);
  glClearDepth(1.0);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDisable(GL_BLEND);

  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,1);

  glLightfv(GL_LIGHT0, GL_POSITION, light_location);
  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.3);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 1.0);
  glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0);

  pCoor light1_location(0,40,100);
  glLightfv(GL_LIGHT1, GL_POSITION, light1_location);
  glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.3);
  glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 1.0);
  glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0);

  pColor ambient_color(0x555555);

  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient_color);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, white * opt_light_intensity);
  glLightfv(GL_LIGHT0, GL_AMBIENT, dark);
  glLightfv(GL_LIGHT0, GL_SPECULAR, white * opt_light_intensity);

  glLightfv(GL_LIGHT1, GL_DIFFUSE, white * opt_light_intensity);
  glLightfv(GL_LIGHT1, GL_AMBIENT, dark);
  glLightfv(GL_LIGHT1, GL_SPECULAR, white * opt_light_intensity);

  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  glEnable(GL_LIGHTING);

  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);

  glShadeModel(GL_SMOOTH);

  const float shininess_ball = 20;
  pColor spec_color(0.2,0.2,0.2);

  // Common to all textures.
  //
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

  glEnable(GL_RESCALE_NORMAL);
  glEnable(GL_NORMALIZE);

  ogl_helper.fbprintf("%s\n",frame_timer.frame_rate_text_get());

  ogl_helper.fbprintf
    ("Eye location: [%5.1f, %5.1f, %5.1f]  "
     "Eye direction: [%+.2f, %+.2f, %+.2f]\n",
     eye_location.x, eye_location.y, eye_location.z,
     eye_direction.x, eye_direction.y, eye_direction.z);

  ogl_helper.fbprintf
    ("Shadows: %-3s ('w')  Mirror: %-3s %d "
     "Light location: [%5.1f, %5.1f, %5.1f]\n",
     opt_shadows ? "on" : "off",
     opt_mirror ? "on" : "off", opt_mirror_method,
     light_location.x, light_location.y, light_location.z);

  Ball& ball = *balls.peek();
  ogl_helper.fbprintf
    ("Ball Count %4d (%4d/%4d)  Last Ball Pos  "
     "[%5.1f,%5.1f,%5.1f] Vel [%+5.1f,%+5.1f,%+5.1f]\n",
     balls.occ(), 
     balls.occ() - balls_occluded, balls_occluded,
     ball.position.x,ball.position.y,ball.position.z,
     ball.velocity.x,ball.velocity.y,ball.velocity.z);

  ogl_helper.fbprintf
    ("Physics: %s ('a')  Debug Option: %d ('q')  "
     "Physics Verification %d ('v')\n",
     gpu_physics_method_str[opt_physics_method], opt_debug, opt_verify);

  pVariable_Control_Elt* const cvar = variable_control.current;
  ogl_helper.fbprintf("VAR %s = %.5f  (TAB or '`' to change, +/- to adjust)\n",
                      cvar->name,cvar->var[0]);

  if ( opt_mirror )
    {
      //
      // Render ball reflection.  (Will be blended with dark tiles.)
      //

      // Write stencil at location of dark (mirrored) tiles.
      //
      glDisable(GL_LIGHTING);
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_NEVER,4,-1);
      glStencilOp(GL_REPLACE,GL_KEEP,GL_KEEP);
      platform_tile_coords.bind();
      glVertexPointer(3, GL_FLOAT, sizeof(platform_tile_coords.data[0]), 0);
      glEnableClientState(GL_VERTEX_ARRAY);
      glDrawArrays(GL_QUADS,platform_even_vtx_cnt,platform_odd_vtx_cnt);

      glEnable(GL_LIGHTING);
      glDisableClientState(GL_VERTEX_ARRAY);
      glBindBuffer(GL_ARRAY_BUFFER,0);

      // Prepare to write only stenciled locations.
      //
      glStencilFunc(GL_EQUAL,4,4);
      glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);

      // Reflected front face should still be treated as the front face.
      //
      glFrontFace(GL_CW);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D,texid_ball);

      glEnable(GL_LIGHT0);
      glEnable(GL_LIGHT1);
      glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shininess_ball);
      glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,spec_color);

      // Use a shader that reflects objects.
      //
      vs_reflect->use();

      // Send constants to shader.
      //
      glUniform1f(sun_platform_xmid, platform_xmid);
      glUniform1f(sun_platform_xrad, platform_xrad);
      glUniform1i(sun_opt_mirror_method, opt_mirror_method);
      glUniform1i(sun_opt_color_events, opt_color_events);
      glUniform4fv(sun_eye_location, 1, eye_location);
      glUniformMatrix4fv(sun_eye_to_world, 1, true, modelview_inv);
      modelview_projection = projection * modelview;
      glUniformMatrix4fv(sun_world_to_clip, 1, true, modelview_projection);

      balls_render(false);

      // Change back to fixed functionality (no user shader).
      //
      vs_fixed->use();

      glFrontFace(GL_CCW);
      glDisable(GL_STENCIL_TEST);
    }

  if ( opt_shadows )
    {
      //
      // Write framebuffer stencil with shadow.
      //

      // Use shader that maps vertices to platform surface.
      //
      vs_shadow->use();

      // Prepare constants for shader.
      //
      pCoor axis_e = modelview * pCoor(platform_xmid,0,0);
      pCoor pt1 = modelview * pCoor(0,0,0);
      pCoor pt2 = modelview * pCoor(0,0,1);
      pNorm axis_ne(pt1,pt2);
      glUniform4fv(sun_axis_e, 1, axis_e); // Point on axis in eye space.
      glUniform3fv(sun_axis_ne, 1, axis_ne); // Axis direction in eye space.
      glUniform1f(sun_platform_xrad_sq, platform_xrad * platform_xrad);

      // Turn off features that are time consuming and not needed
      // for writing the stencil buffer.
      //
      glDisable(GL_LIGHTING);
      glDisable(GL_TEXTURE_2D);

      // Set up stencil test for writing shadow.
      //
      glEnable(GL_STENCIL_TEST);
      glDepthFunc(GL_NEVER);
      glStencilOp(GL_REPLACE,GL_KEEP,GL_KEEP);  // sfail, dfail, dpass

      // Prepare for writing shadow from light 0.
      //
      glStencilFunc(GL_NEVER,2,-1); // ref, mask
      glUniform1i(sun_light_num,0);

      balls_render_simple();

      // Prepare for writing shadow from light 1
      //
      glStencilOp(GL_INCR,GL_KEEP,GL_KEEP);  // sfail, dfail, dpass
      glStencilFunc(GL_EQUAL,1,1); // ref, mask
      glUniform1i(sun_light_num,1);

      balls_render_simple();

      vs_fixed->use();

      // Restore some settings.
      //
      glEnable(GL_LIGHTING);
      glDisable(GL_STENCIL_TEST);
      glDepthFunc(GL_LESS);
    }

  glColor3f(0.5,0,0);

  // Setup texture for platform.
  //
  glBindTexture(GL_TEXTURE_2D,texid_plat);

  // Blend dark tiles with existing ball reflection.
  //
  glEnable(GL_STENCIL_TEST);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_CONSTANT_ALPHA,GL_ONE_MINUS_CONSTANT_ALPHA); // src, dst
  glBlendColor(0,0,0,0.5);

  glDepthFunc(GL_ALWAYS);

  glEnable(GL_TEXTURE_2D);
  platform_tex_coords.bind();
  glTexCoordPointer(2, GL_FLOAT,2*sizeof(float), 0);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  platform_tile_coords.bind();
  glVertexPointer
    (3, GL_FLOAT,sizeof(platform_tile_coords.data[0]), 0);
  glEnableClientState(GL_VERTEX_ARRAY);
  platform_tile_norms.bind();
  glNormalPointer
    (GL_FLOAT,sizeof(platform_tile_norms.data[0]), 0);
  glEnableClientState(GL_NORMAL_ARRAY);

  glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);  // sfail, dfail, dpass

  for ( int pass = 0;  pass < (opt_shadows?4:1);  pass++ )
    {
      switch ( pass ) {
      case 0:
        // Prepare to write unshadowed parts of frame buffer.
        //
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glStencilFunc(GL_EQUAL,0,3);
        break;
      case 1:
        // Write parts in which light 0 shadowed.
        //
        glDisable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glStencilFunc(GL_EQUAL,2,3);
        break;
      case 2:
        // Write parts in which light 1 shadowed.
        //
        glEnable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        glStencilFunc(GL_EQUAL,1,3);
        break;
      case 3:
        // Write parts in which light 0 and light 1 shadowed.
        //
        glDisable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        glStencilFunc(GL_EQUAL,3,3);
        break;
      }

      glEnable(GL_TEXTURE_2D);

      // Write lighter-colored, textured tiles.
      //
      glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,spec_color);
      glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,2.0);
      glColor3f(0.35,0.35,0.35);
      glDrawArrays(GL_QUADS,0,platform_even_vtx_cnt);

      // Write darker-colored, untextured, mirror tiles.
      //
      glEnable(GL_BLEND);
      glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
      glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,20);
      glDisable(GL_TEXTURE_2D);
      glColor3fv(lsu_spirit_purple);
      glDrawArrays(GL_QUADS,platform_even_vtx_cnt,platform_odd_vtx_cnt);
      glDisable(GL_BLEND);
    }

  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
  glBindBuffer(GL_ARRAY_BUFFER,0);
  glDisable(GL_STENCIL_TEST);
  glDepthFunc(GL_LESS);

  // Finally, Render Balls
  //
  glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shininess_ball);
  glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,spec_color);
  glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
  glEnable(GL_COLOR_SUM);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D,texid_ball);

  balls_render(true);

  glDisable(GL_TEXTURE_2D);
  glDisable(GL_COLOR_SUM);
  glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);

  // Maybe render platform normals.
  //
  if ( opt_normals_visible )
    {
      glColor3fv(lsu_spirit_gold);
      for ( int i=0; i<platform_tile_coords.elements; i++ )
        cone.render(platform_tile_coords[i],0.2,5 * platform_tile_norms[i]);
    }

  // Render Marker for Light Source
  //
  insert_tetrahedron(light_location,0.5);

  pError_Check();

  glDisable(GL_LIGHTING);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_STENCIL_TEST);
  frame_timer.frame_end();

  glColor3f(0.5,1,0.5);
  ogl_helper.user_text_reprint();
  glutSwapBuffers();
}


void
World::cb_keyboard()
{
  if ( !ogl_helper.keyboard_key ) return;
  pVect adjustment(0,0,0);
  pVect user_rot_axis(0,0,0);
  const float move_amt = 0.4;

  switch ( ogl_helper.keyboard_key ) {
  case FB_KEY_LEFT: adjustment.x = -move_amt; break;
  case FB_KEY_RIGHT: adjustment.x = move_amt; break;
  case FB_KEY_PAGE_UP: adjustment.y = move_amt; break;
  case FB_KEY_PAGE_DOWN: adjustment.y = -move_amt; break;
  case FB_KEY_DOWN: adjustment.z = move_amt; break;
  case FB_KEY_UP: adjustment.z = -move_amt; break;
  case FB_KEY_DELETE: user_rot_axis.y = 1; break;
  case FB_KEY_INSERT: user_rot_axis.y =  -1; break;
  case FB_KEY_HOME: user_rot_axis.x = 1; break;
  case FB_KEY_END: user_rot_axis.x = -1; break;
  case FB_KEY_F8:
    {
      opt_elasticity = 10;
      opt_friction_roll = 300;
      opt_friction_coeff = 0.0001;
      break;
    }
  case 'a':
    opt_physics_method++;
    if ( opt_physics_method > GP_cuda ) opt_physics_method = GP_cpu;
    break;
  case 'b': opt_move_item = MI_Ball; break;
  case 'B': opt_move_item = MI_Ball_V; break;
  case 'c': case 'C': opt_color_events = !opt_color_events; break;
  case 'e': case 'E': opt_move_item = MI_Eye; break;
  case 'd': case 'D': opt_drip = !opt_drip; if(!opt_drip)dball=NULL; break;
  case 'g': case 'G': opt_gravity = !opt_gravity; break;
  case 'l': case 'L': opt_move_item = MI_Light; break;
  case 'n': case 'N': opt_normals_visible = !opt_normals_visible; break;
  case 'm': opt_mirror = !opt_mirror; break;
  case 'M': opt_mirror_method++;
    if ( opt_mirror_method == 4 ) opt_mirror_method = 0;
    break;
  case 'p': case 'P': opt_pause = !opt_pause; break;
  case 'R': balls_remove(); break;
  case 's': balls_stop(); break;
  case 'S': balls_rot_stop(); break;
  case 'T': benchmark_setup(); break;
  case 't': benchmark_setup(5); break;
  case 'q': opt_debug = !opt_debug; break;
  case 'v': opt_verify = !opt_verify; break;
  case 'w': opt_shadows = !opt_shadows; break;
  case 'x': opt_spray_on = !opt_spray_on; break;
  case 'X':
    {
      Ball* const b1 = new Ball(this);
      b1->position = pCoor(30,22,20);
      b1->velocity = pVect(0,0,0);
      b1->color_natural = lsu_spirit_purple;
      balls += b1;
      Ball* const b2 = new Ball(this);
      b2->position = b1->position;
      b2->position.z += 4 * opt_ball_radius;
      b2->velocity = pVect(0,0,0);
      b2->color_natural = lsu_spirit_gold;
      balls += b2;
    }
    break;
  case 9: variable_control.switch_var_right(); break;
  case 96: variable_control.switch_var_left(); break; // `, until S-TAB works.
  case '-':case '_': variable_control.adjust_lower(); break;
  case '+':case '=': variable_control.adjust_higher(); break;
  default: printf("Unknown key, %d\n",ogl_helper.keyboard_key); break;
  }

  variables_update();

  // Update eye_direction based on keyboard command.
  //
  if ( user_rot_axis.x || user_rot_axis.y )
    {
      pMatrix_Rotation rotall(eye_direction,pVect(0,0,-1));
      user_rot_axis *= invert(rotall);
      eye_direction *= pMatrix_Rotation(user_rot_axis, M_PI * 0.03);
      modelview_update();
    }

  // Update eye_location based on keyboard command.
  //
  if ( adjustment.x || adjustment.y || adjustment.z )
    {
      const double angle =
        fabs(eye_direction.y) > 0.99
        ? 0 : atan2(eye_direction.x,-eye_direction.z);
      pMatrix_Rotation rotall(pVect(0,1,0),-angle);
      adjustment *= rotall;

      switch ( opt_move_item ){
      case MI_Ball: balls.peek()->translate(adjustment); break;
      case MI_Ball_V: balls.peek()->push(adjustment); break;
      case MI_Light: light_location += adjustment; break;
      case MI_Eye: eye_location += adjustment; break;
      default: break;
      }
      modelview_update();
    }
}



int
main(int argv, char **argc)
{
  pOpenGL_Helper popengl_helper(argv,argc);
  World world(popengl_helper);

  popengl_helper.rate_set(30);
  popengl_helper.display_cb_set(world.render_w,&world);
}

