/// LSU EE 4702-1 (Fall 2018), GPU Programming
//
 /// Simple Demo of Dynamic Simulation, Graphics Code

 // This file includes graphics code needed by the main file. The code
 // in this file does not need to be understood early in the semester.

#undef DB_SV

#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/freeglut.h>

#include <gp/util.h>
#include <gp/shader.h>
#include <gp/pstring.h>
#include <gp/misc.h>
#include <gp/gl-buffer.h>
#include <gp/texture-util.h>
#include "shapes.h"

enum Render_Option { RO_Normally, RO_Simple, RO_Shadow_Volumes };

class Render_FCtx {
public:
  Render_FCtx(pCoor light, pCoor eye)
    :light_location(light),eye_location(eye),viewer_sv_count(0){}
  pCoor light_location;
  pCoor eye_location;
  int viewer_sv_count;
};

class Render_Ctx {
public:
  Render_Ctx(Render_FCtx& fc):fc(&fc){};
  Render_Ctx(Render_Ctx *rc, pMatrix transform_local)
    :fc(rc->fc), transform(rc->transform * transform_local){};
  Render_FCtx* const fc;
  const pMatrix transform;
};


#include <gp/colors.h>

class World {
public:
  World(pOpenGL_Helper &fb, int argcp, char **argvp)
    :argc(argcp),argv(argvp),ogl_helper(fb){init();}
  void init();
  void init_graphics();
  static void frame_callback_w(void *moi){((World*)moi)->frame_callback();}
  void frame_callback();
  void render();
  void render_objects(Render_Option render_option);
  void cb_keyboard();
  void modelview_update();

  const int argc;
  char** const argv;
  pOpenGL_Helper& ogl_helper;
  pVariable_Control variable_control;
  pFrame_Timer frame_timer;
  double world_time;
  double last_frame_wall_time;
  int time_step_count;
  float opt_gravity_accel;      // Value chosen by user.
  pVect gravity_accel;          // Set to zero when opt_gravity is false;
  bool opt_gravity;
  bool opt_head_lock, opt_tail_lock;
  bool opt_shadows;
  bool opt_tryout_1, opt_tryout_2;
  float opt_tryoutf;

  // Tiled platform for ball.
  //
  float platform_xmin, platform_xmax, platform_zmin, platform_zmax;
  float platform_pi_xwidth_inv;
  pBuffer_Object<pVect> platform_tile_coords;
  pBuffer_Object<float> platform_tex_coords;
  pVect platform_normal;
  GLuint texid_syl;
  GLuint texid_emacs;
  GLuint texid_spiral_image;
  bool opt_platform_texture;
  void platform_update();
  bool platform_collision_possible(pCoor pos);

  pCoor light_location;
  float opt_light_intensity;
  enum { MI_Eye, MI_Light, MI_Ball, MI_Ball_V, MI_COUNT } opt_move_item;
  bool opt_pause;
  bool opt_single_frame;      // Simulate for one frame.
  bool opt_single_time_step;  // Simulate for one time step.
  int viewer_shadow_volume;

  pCoor eye_location;
  pVect eye_direction;
  pMatrix modelview;
  pMatrix transform_mirror;
  int curr_setup;

  void hw03_render(bool shadows);
  int opt_n_segs;

  enum FILL_METHOD { Fill_None, Fill_Wye, Fill_Louvers, FILL_SIZE };
  enum Shader_Option { SO_Fixed, SO_Plain, SO_HW03, SO_Lines, SHADER_SIZE };

  int opt_shader;
  const char* shader_opt_str[SHADER_SIZE+1] =
    {"FIXED", "PLAIN", "HW03", "LINES", "??"};
  const char* pf_shader_opt_str
  [SHADER_SIZE+1] = {"FIXED", "HW03", "PreFin", "??"};

  void ball_setup_1();
  void ball_setup_2();
  void ball_setup_3();
  void ball_setup_4();
  void ball_setup_5();
  void time_step_cpu(double);
  void balls_stop();
  void balls_freeze();
  void balls_translate(pVect amt, int idx);
  void balls_translate(pVect amt);
  void balls_push(pVect amt, int idx);
  void balls_push(pVect amt);

  float opt_spring_constant;
  float opt_air_resistance;
  float distance_relaxed;
  int chain_length;
  Ball *balls;
  Sphere sphere;

  pShader *sp_fixed;          // Fixed functionality.
  pShader *sp_hw03;

  // Shader for Pre Final Problem 1
  bool is_solution;

  pShader *sp_lines;

  #define MAXBO 20
  GLuint bo[MAXBO];

};


void
World::init_graphics()
{
  string this_executable(argv[0]);
  is_solution = this_executable.find("-sol") != string::npos;

  ///
  /// Graphical Model Initialization
  ///

  opt_tryout_1 = opt_tryout_2 = false;
  opt_platform_texture = true;
  opt_head_lock = false;
  opt_tail_lock = false;

  eye_location = pCoor(18.1,10.8,-25.7);
  eye_direction = pVect(-0.42,-0.09,0.9);


  platform_xmin = -40; platform_xmax = 40;
  platform_zmin = -40; platform_zmax = 40;
  texid_syl = pBuild_Texture_File("gpup.png",false,255);
  texid_emacs = pBuild_Texture_File("mult.png", false,-1);
  texid_spiral_image = pBuild_Texture_File("n.png");

  opt_light_intensity = 100.2;
  light_location = pCoor(15.45,40,-48);


  variable_control.insert(opt_light_intensity,"Light Intensity");

  opt_move_item = MI_Eye;
  opt_pause = false;
  opt_single_time_step = false;
  opt_single_frame = false;

  opt_shadows = true;

  const char* const shdr_path =
    is_solution ? "hw03-shdr-sol.cc" : "hw03-shdr.cc";

  // Declared like a programmable shader, but used for fixed-functionality.
  //
  sp_fixed = new pShader();

  sp_hw03 = new pShader
    (shdr_path,              // File holding shader program.
     "vs_main_hw03(); ",     // Name of vertex shader main routine.
     "fs_main_hw03();"       // Name of fragment shader main routine.
     );
  sp_lines = new pShader
    ("hw03-lines.cc",              // File holding shader program.
     "vs_main_lines();",     // Name of vertex shader main routine.
     "gs_main_lines();",
     "fs_main_lines();"       // Name of fragment shader main routine.
     );

  sphere.init(40);

  platform_update();
  modelview_update();

  opt_shader = SO_HW03;
}


void
World::platform_update()
{
  const float tile_count = 19;
  const float ep = 1.00001;
  const float delta_x = ( platform_xmax - platform_xmin ) / tile_count * ep; 
  const float zdelta = ( platform_zmax - platform_zmin ) / tile_count * ep;

  const float trmin = 0.05;
  const float trmax = 0.7;
  const float tsmin = 0;
  const float tsmax = 0.4;

  platform_normal = pVect(0,1,0);

  PStack<pVect> p_tile_coords;
  PStack<pVect> p1_tile_coords;
  PStack<float> p_tex_coords;
  bool even = true;

  for ( int i = 0; i < tile_count; i++ )
    {
      const float x0 = platform_xmin + i * delta_x;
      const float x1 = x0 + delta_x;
      const float y = 0;
      for ( float z = platform_zmin; z < platform_zmax; z += zdelta )
        {
          PStack<pVect>& t_coords = even ? p_tile_coords : p1_tile_coords;
          p_tex_coords += trmax; p_tex_coords += tsmax;
          t_coords += pVect(x0,y,z);
          p_tex_coords += trmax; p_tex_coords += tsmin;
          t_coords += pVect(x0,y,z+zdelta);
          p_tex_coords += trmin; p_tex_coords += tsmin;
          t_coords += pVect(x1,y,z+zdelta);
          p_tex_coords += trmin; p_tex_coords += tsmax;
          t_coords += pVect(x1,y,z);
          even = !even;
        }
    }

  while ( pVect* const v = p1_tile_coords.iterate() ) p_tile_coords += *v;

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
  pMatrix reflect; reflect.set_identity(); reflect.rc(1,1) = -1;
  transform_mirror = modelview * reflect * invert(modelview);
}

void
World::render_objects(Render_Option option)
{
  const float shininess_ball = 5;
  pColor spec_color(0.2,0.2,0.2);
  Render_FCtx fc(light_location,eye_location);
  Render_Ctx rc(fc);

  if ( option == RO_Shadow_Volumes )
    viewer_shadow_volume = 0;

  if ( option == RO_Normally )
    {
      glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 1.0);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D,texid_emacs);
      glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shininess_ball);
      glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,spec_color);
      glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
      glEnable(GL_COLOR_SUM);
    }

  Cone cone; cone.apex_radius = 1; cone.set_color(color_lsu_spirit_purple);

  if ( option == RO_Shadow_Volumes )
    {
      cone.light_pos = light_location;
      sphere.light_pos = light_location;

      for ( int i=0; i<chain_length; i++ )
        sphere.render_shadow_volume(balls[i].radius,balls[i].position);
      for ( int i=1; i<chain_length; i++ )
        {
          Ball *const ball1 = &balls[i-1];
          Ball *const ball2 = &balls[i];
          cone.render_shadow_volume
            (ball1->position,0.3*ball1->radius,
             ball2->position-ball1->position);
        }

      hw03_render(true);
    }
  else
    {
      for ( int i=0; i<chain_length; i++ )
        {
          if ( balls[i].contact )
            sphere.color = color_gray;
          else if ( i == 0 && opt_head_lock
                    || i == chain_length-1 && opt_tail_lock )
            sphere.color = color_pale_green;
          else
            sphere.color = color_lsu_spirit_gold;
          sphere.render(balls[i].radius,balls[i].position);
        }
      for ( int i=1; i<chain_length; i++ )
        {
          Ball *const ball1 = &balls[i-1];
          Ball *const ball2 = &balls[i];
          cone.render(ball1->position,0.3*ball1->radius,
                      ball2->position-ball1->position);
        }

      hw03_render(false);
    }

  glEnable(GL_COLOR_MATERIAL);
  glDisable(GL_COLOR_SUM);
  glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
  glLightfv(GL_LIGHT0, GL_SPECULAR, color_black);

  //
  // Render Platform
  //
  const int half_elements = platform_tile_coords.elements >> 3 << 2;

  glEnable(GL_TEXTURE_2D);

  // Set up attribute (vertex, normal, etc.) arrays.
  //
  glBindTexture(GL_TEXTURE_2D,texid_syl);
  platform_tile_coords.bind();
  glVertexPointer(3, GL_FLOAT, sizeof(platform_tile_coords.data[0]), 0);
  glEnableClientState(GL_VERTEX_ARRAY);
  glNormal3fv(platform_normal);
  platform_tex_coords.bind();
  glTexCoordPointer(2, GL_FLOAT,2*sizeof(float), 0);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  // Write lighter-colored, textured tiles.
  //
  glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,spec_color);
  glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,2.0);
  glColor3f(0.35,0.35,0.35);
  glColor3f(0.55,0.55,0.55);
  glDrawArrays(GL_QUADS,0,half_elements+4);

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glBindBuffer(GL_ARRAY_BUFFER,0);
}

void
World::render()
{
  // Get any waiting keyboard commands.
  //
  cb_keyboard();

  // Start a timer object used for tuning this code.
  //
  frame_timer.frame_start();

  /// Emit a Graphical Representation of Simulation State
  //

  // Understanding of the code below not required for introductory
  // lectures.

  // That said, much of the complexity of the code is to show
  // the ball shadow and reflection.


  const int win_width = ogl_helper.get_width();
  const int win_height = ogl_helper.get_height();
  const float aspect = float(win_width) / win_height;

  glMatrixMode(GL_MODELVIEW);
  glLoadTransposeMatrixf(modelview);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  // Frustum: left, right, bottom, top, near, far
  glFrustum(-.8,.8,-.8/aspect,.8/aspect,1,5000);

  glViewport(0, 0, win_width, win_height);
  pError_Check();

  glClearColor(0,0,0,0.5);
  glClearDepth(1.0);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDisable(GL_BLEND);

  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,1);
  glLightfv(GL_LIGHT0, GL_POSITION, light_location);

  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.5);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 1.0);
  glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0);

  pColor ambient_color(0x555555);

  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient_color);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, color_white * opt_light_intensity);
  glLightfv(GL_LIGHT0, GL_AMBIENT, color_black);
  glLightfv(GL_LIGHT0, GL_SPECULAR, color_white * opt_light_intensity);

  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHTING);

  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);

  glShadeModel(GL_SMOOTH);

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

  const double time_now = time_wall_fp();
  const bool blink_visible = int64_t(time_now*3) & 1;
# define BLINK(txt,pad) ( blink_visible ? txt : pad )

  ogl_helper.fbprintf("%s\n",frame_timer.frame_rate_text_get());

  ogl_helper.fbprintf
    ("Shader: %s ('f')  Shadow Effect: %s ('w')  "
     "Time Step: %8d  World Time: %11.6f  %s\n",
     shader_opt_str[opt_shader],
     opt_shadows ? "ON" : "OFF",
     time_step_count, world_time,
     opt_pause ? BLINK("PAUSED, 'p' to unpause, SPC or S-SPC to step.","") :
     "Press 'p' to pause."
     );

  ogl_helper.fbprintf
    ("Tryout %c%c ('y','Y')\n",
     opt_tryout_1 ? '1' : '_',
     opt_tryout_2 ? '2' : '_' );


  ogl_helper.fbprintf
    ("Eye location: [%5.1f, %5.1f, %5.1f]  "
     "Eye direction: [%+.2f, %+.2f, %+.2f]  "
     "Light Location: [%+.2f, %+.2f, %+.2f]\n",
     eye_location.x, eye_location.y, eye_location.z,
     eye_direction.x, eye_direction.y, eye_direction.z,
     light_location.x, light_location.y, light_location.z);

  Ball& ball = balls[0];

  ogl_helper.fbprintf
    ("Head Ball Pos  [%5.1f,%5.1f,%5.1f] Vel [%+5.1f,%+5.1f,%+5.1f]\n",
     ball.position.x,ball.position.y,ball.position.z,
     ball.velocity.x,ball.velocity.y,ball.velocity.z );

  pVariable_Control_Elt* const cvar = variable_control.current;
  ogl_helper.fbprintf("VAR %s = %.5f  (TAB or '`' to change, +/- to adjust)\n",
                      cvar->name,cvar->get_val());

  const int half_elements = platform_tile_coords.elements >> 3 << 2;

  //
  // Render ball reflection.  (Will be blended with dark tiles.)
  //

  // Write stencil at location of dark (mirrored) tiles.
  //
  glDisable(GL_LIGHTING);
  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NEVER,2,2);
  glStencilOp(GL_REPLACE,GL_KEEP,GL_KEEP);
  platform_tile_coords.bind();
  glVertexPointer(3, GL_FLOAT, sizeof(platform_tile_coords.data[0]), 0);
  glEnableClientState(GL_VERTEX_ARRAY);
  glDrawArrays(GL_QUADS,half_elements+4,half_elements-4);
  glEnable(GL_LIGHTING);
  glDisableClientState(GL_VERTEX_ARRAY);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  // Prepare to write only stenciled locations.
  //
  glStencilFunc(GL_EQUAL,2,2);
  glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);

  // Use a transform that reflects objects to other side of platform.
  //
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glMultTransposeMatrixf(transform_mirror);

  // Reflected front face should still be treated as the front face.
  //
  glFrontFace(GL_CW);

  render_objects(RO_Normally);

  glFrontFace(GL_CCW);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glDisable(GL_STENCIL_TEST);


  // Setup texture for platform.
  //
  glBindTexture(GL_TEXTURE_2D,texid_syl);

  // Blend dark tiles with existing ball reflection.
  //
  glEnable(GL_STENCIL_TEST);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_CONSTANT_ALPHA,GL_ONE_MINUS_CONSTANT_ALPHA); // src, dst
  glBlendColor(0,0,0,0.5);

  glDepthFunc(GL_ALWAYS);

  if ( opt_platform_texture )
    {
      glEnable(GL_TEXTURE_2D);
      platform_tex_coords.bind();
      glTexCoordPointer(2, GL_FLOAT,2*sizeof(float), 0);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }

  platform_tile_coords.bind();
  glVertexPointer
    (3, GL_FLOAT,sizeof(platform_tile_coords.data[0]), 0);
  glEnableClientState(GL_VERTEX_ARRAY);
  glNormal3fv(platform_normal);

  if ( opt_platform_texture ) glEnable(GL_TEXTURE_2D);

  // Write lighter-colored, textured tiles.
  //
  glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,color_gray);
  glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,2.0);
  glColor3f(0.35,0.35,0.35);
  glDrawArrays(GL_QUADS,0,half_elements+4);

  // Write darker-colored, untextured, mirror tiles.
  //
  glEnable(GL_BLEND);
  glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,color_white);
  glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,20);
  glDisable(GL_TEXTURE_2D);
  glColor3fv(color_lsu_spirit_purple);
  glDrawArrays(GL_QUADS,half_elements+4,half_elements-4);
  glDisable(GL_BLEND);

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
  glBindBuffer(GL_ARRAY_BUFFER,0);
  glDisable(GL_STENCIL_TEST);
  glDepthFunc(GL_LESS);

  if ( opt_shadows )
    {
      glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient_color);

      //
      // First pass, render using only ambient light.
      //
      glDisable(GL_LIGHT0);

      // Send balls, tiles, and platform to opengl.
      // Do occlusion test too.
      //
      render_objects(RO_Normally);

      //
      // Second pass, add on light0.
      //

      // Turn off ambient light, turn on light 0.
      //
      glLightModelfv(GL_LIGHT_MODEL_AMBIENT, color_black);
      glEnable(GL_LIGHT0);


      glClear(GL_STENCIL_BUFFER_BIT);

      // Make sure that only stencil buffer written.
      //
      glColorMask(false,false,false,false);
      glDepthMask(false);

      // Don't waste time computing lighting.
      //
      glDisable(GL_LIGHTING);

      glDisable(GL_TEXTURE_2D);

      // Set up stencil test to count shadow volume surfaces: plus 1 for
      // entering the shadow volume, minus 1 for leaving the shadow
      // volume.
      //
      glEnable(GL_STENCIL_TEST);
      // sfail, dfail, dpass
      glStencilOpSeparate(GL_FRONT,GL_KEEP,GL_KEEP,GL_INCR_WRAP);
      glStencilOpSeparate(GL_BACK,GL_KEEP,GL_KEEP,GL_DECR_WRAP);
      glStencilFuncSeparate(GL_FRONT_AND_BACK,GL_ALWAYS,1,-1); // ref, mask
 
      // Write stencil with shadow locations based on shadow volumes
      // cast by light0 (light_location).  Shadowed locations will
      // have a positive stencil value.  Routine will set viewer_shadow_volume
      // to the number of view volumes containing the eye.
      //
      render_objects(RO_Shadow_Volumes);

      glEnable(GL_LIGHTING);
      glColorMask(true,true,true,true);
      glDepthMask(true);

      // Use stencil test to prevent writes to shadowed areas.
      //
      glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
      glStencilFunc(GL_EQUAL,viewer_shadow_volume,-1); // ref, mask

      // Allow pixels to be re-written.
      //
      glDepthFunc(GL_LEQUAL);
      glEnable(GL_BLEND);
      glBlendEquation(GL_FUNC_ADD);
      glBlendFunc(GL_ONE,GL_ONE);

      // Render.
      //
      render_objects(RO_Normally);

      glDisable(GL_BLEND);
      glDisable(GL_STENCIL_TEST);

    }
  else
    {
      // Render.
      //
      render_objects(RO_Normally);
    }


  // Render Marker for Light Source
  //
  insert_tetrahedron(light_location,0.5);

  pError_Check();

  glColor3f(0.5,1,0.5);

  frame_timer.frame_end();

  ogl_helper.user_text_reprint();

  glutSwapBuffers();
}


void
World::cb_keyboard()
{
  if ( !ogl_helper.keyboard_key ) return;
  pVect adjustment(0,0,0);
  pVect user_rot_axis(0,0,0);
  const bool shift = ogl_helper.keyboard_shift;
  const float move_amt = shift ? 2.0 : 0.4;

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
  case '1': ball_setup_1(); break;
  case '2': ball_setup_2(); break;
  case '3': ball_setup_3(); break;
  case '4': ball_setup_4(); break;
  case '5': ball_setup_5(); break;
  case 'b': opt_move_item = MI_Ball; break;
  case 'B': opt_move_item = MI_Ball_V; break;
  case 'e': case 'E': opt_move_item = MI_Eye; break;
  case 'f': case 'F':
    opt_shader++; if ( opt_shader == SHADER_SIZE ) opt_shader = SO_HW03; break;
  case 'g': case 'G': opt_gravity = !opt_gravity; break;
  case 'h': case 'H': opt_head_lock = !opt_head_lock; break;
  case 't': case 'T': opt_tail_lock = !opt_tail_lock; break;
  case 'l': case 'L': opt_move_item = MI_Light; break;
  case 'n': case 'N': opt_platform_texture = !opt_platform_texture; break;
  case 'p': case 'P': opt_pause = !opt_pause; break;
  case 's': case 'S': balls_stop(); break;
  case 'w': case 'W': opt_shadows = !opt_shadows; break;
  case 'y': opt_tryout_1 = !opt_tryout_1; break;
  case 'Y': opt_tryout_2 = !opt_tryout_2; break;

  case ' ':
    if ( shift ) opt_single_time_step = true; else opt_single_frame = true;
    opt_pause = true; 
    break;
  case 9: variable_control.switch_var_right(); break;
  case 96: variable_control.switch_var_left(); break; // `, until S-TAB works.
  case '-':case '_': variable_control.adjust_lower(); break;
  case '+':case '=': variable_control.adjust_higher(); break;
  default: printf("Unknown key, %d\n",ogl_helper.keyboard_key); break;
  }

  uint32_t key_dig = ogl_helper.keyboard_key - '0';
  if ( key_dig < 6 ) curr_setup = key_dig;

  gravity_accel.y = opt_gravity ? -opt_gravity_accel : 0;

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
      case MI_Ball: balls_translate(adjustment,0); break;
      case MI_Ball_V: balls_push(adjustment,0); break;
      case MI_Light: light_location += adjustment; break;
      case MI_Eye: eye_location += adjustment; break;
      default: break;
      }
      modelview_update();
    }

}
