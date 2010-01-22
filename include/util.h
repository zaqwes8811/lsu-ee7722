// -*- c++ -*-

#ifndef UTIL_H
#define UTIL_H

#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <Magick++.h>

#include "glextfuncs.h"
#include "pstring.h"
#include "misc.h"

double
time_wall_fp()
{
  struct timespec now;
  clock_gettime(CLOCK_REALTIME,&now);
  return now.tv_sec + ((double)now.tv_nsec) * 1e-9;
}

double
time_process_fp()
{
  struct timespec now;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&now);
  return now.tv_sec + ((double)now.tv_nsec) * 1e-9;
}

// Rename keys so a single namespace can be used for regular (ASCII)
// keys and "special" ones.

#define FB_KEY_F1         ( GLUT_KEY_F1          + 0x100 )
#define FB_KEY_F2         ( GLUT_KEY_F2          + 0x100 )
#define FB_KEY_F3         ( GLUT_KEY_F3          + 0x100 )
#define FB_KEY_F4         ( GLUT_KEY_F4          + 0x100 )
#define FB_KEY_F5         ( GLUT_KEY_F5          + 0x100 )
#define FB_KEY_F6         ( GLUT_KEY_F6          + 0x100 )
#define FB_KEY_F7         ( GLUT_KEY_F7          + 0x100 )
#define FB_KEY_F8         ( GLUT_KEY_F8          + 0x100 )
#define FB_KEY_F9         ( GLUT_KEY_F9          + 0x100 )
#define FB_KEY_F10        ( GLUT_KEY_F10         + 0x100 )
#define FB_KEY_F11        ( GLUT_KEY_F11         + 0x100 )
#define FB_KEY_F12        ( GLUT_KEY_F12         + 0x100 )
#define FB_KEY_LEFT       ( GLUT_KEY_LEFT        + 0x100 )
#define FB_KEY_UP         ( GLUT_KEY_UP          + 0x100 )
#define FB_KEY_RIGHT      ( GLUT_KEY_RIGHT       + 0x100 )
#define FB_KEY_DOWN       ( GLUT_KEY_DOWN        + 0x100 )
#define FB_KEY_PAGE_UP    ( GLUT_KEY_PAGE_UP     + 0x100 )
#define FB_KEY_PAGE_DOWN  ( GLUT_KEY_PAGE_DOWN   + 0x100 )
#define FB_KEY_HOME       ( GLUT_KEY_HOME        + 0x100 )
#define FB_KEY_END        ( GLUT_KEY_END         + 0x100 )
#define FB_KEY_INSERT     ( GLUT_KEY_INSERT      + 0x100 )
#define FB_KEY_DELETE     127

inline void
pError_Exit()
{
  exit(1);
}

inline void
pError_Msg_NP(const char *fmt, ...) __attribute__ ((format(printf,1,2)));

inline void
pError_Msg_NP(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  pError_Exit();
}

inline void
pError_Msg(const char *fmt, ...) __attribute__ ((format(printf,1,2)));

inline void
pError_Msg(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr,"User Error: ");
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  pError_Exit();
}

inline bool
pError_Check(int error = -1)
{
  const int err = glGetError();
  if ( err == GL_NO_ERROR ) return false;
  if ( err == error ) return true;
  fprintf(stderr,"GL Error: %s\n",gluErrorString(err));
  pError_Exit();
  return true; // Unreachable.
}

#define P_GL_PRINT_STRING(token) lprint_string(token,#token);

inline void
lprint_string(int token, const char *name)
{
  pError_Check();
  char* const str = (char*)glGetString(token);
  if ( pError_Check(GL_INVALID_ENUM) )
    printf("S %s: ** Unrecognized**\n",name);
  else
    printf("S %s: \"%s\"\n",name,str);
}

#define PRINT_ATTRIBUTE(token) lprint_attribute(token,#token);
inline void
lprint_attribute(int token, const char *name)
{
  pError_Check();
  int val;
  glGetIntegerv(token,&val);
  if ( pError_Check(GL_INVALID_ENUM) )
    printf("Attribute %s: ** Unrecognized **\n",name);
  else
    printf("Attribute %s: %d\n",name,val);
}

class pOpenGL_Helper;
pOpenGL_Helper* opengl_helper_self_ = NULL;

class pFrame_Timer {
public:
  pFrame_Timer():inited(false),work_description(NULL)
  {
    query_timer_id = 0;
    frame_group_size = 30;
    frame_rate = 0;
    cpu_frac = 0;
    cuda_in_use = false;
  }
  void work_unit_set(const char *description, double multiplier = 1)
  {
    work_multiplier = multiplier;
    work_accum = 0;
    work_description = strdup(description);
  }
  void work_amt_set(int amt){ work_accum += amt; }
  void init();
  void frame_start();
  void frame_end();
  const char* frame_rate_text_get() const { return frame_rate_text.s; }
  int frame_group_size;
  void cuda_frame_time_set(float time_ms)
  { cuda_tsum_ms += time_ms; cuda_in_use = true; }
private:
  void frame_rate_group_start();
  void var_reset()
  {
    frame_group_count = 0;
    cpu_tsum = gpu_tsum_ns = cuda_tsum_ms = 0;
    work_accum = 0;
  }
  bool inited;
  bool cuda_in_use;
  double frame_group_start_time;
  int frame_group_count;
  double gpu_tsum_ns, gpu_tlast, cpu_tsum, cpu_tlast, cuda_tsum_ms, cuda_tlast;
  double work_accum;
  double work_multiplier;
  int work_count_last;
  char *work_description;
  double work_rate;

  double frame_rate;
  double cpu_frac, gpu_frac, cuda_frac;
  double time_render_start;
  GLuint query_timer_id;
  uint xfcount;  // Frame count provided by glx.
  pString frame_rate_text;
};

void
pFrame_Timer::init()
{
  inited = true;
  if ( glutExtensionSupported("GL_EXT_timer_query") )
    glGenQueries(1,&query_timer_id);
  frame_group_start_time = time_wall_fp();
  var_reset();
  frame_rate_group_start();
#ifdef CUDA
  cudaEventCreate(&frame_start_ce);
  cudaEventCreate(&frame_stop_ce);
#endif
}

void
pFrame_Timer::frame_rate_group_start()
{
  const double last_wall_time = frame_group_start_time;
  const double last_frame_count = max(frame_group_count,1);
  const double last_frame_count_inv = 1.0 / last_frame_count;
  frame_group_start_time = time_wall_fp();
  const double group_duration = frame_group_start_time - last_wall_time;

  gpu_tlast = 1e-9 * gpu_tsum_ns * last_frame_count_inv;
  cpu_tlast = cpu_tsum * last_frame_count_inv;
  cuda_tlast = 1e-3 * cuda_tsum_ms * last_frame_count_inv;
  frame_rate = last_frame_count / group_duration;
  cpu_frac = cpu_tsum / group_duration;
  gpu_frac = 1e-9 * gpu_tsum_ns / group_duration;
  cuda_frac = 1e-3 * cuda_tsum_ms / group_duration;
  if ( work_description )
    {
      work_rate = work_multiplier * work_accum / group_duration;      
    }
  var_reset();
}

void
pFrame_Timer::frame_start()
{
  if ( !inited ) init();
  pError_Check();
  if ( query_timer_id ) glBeginQuery(GL_TIME_ELAPSED_EXT,query_timer_id);
  pError_Check();
  time_render_start = time_wall_fp();
  if ( frame_group_count++ >= frame_group_size ) frame_rate_group_start();
}

void
pFrame_Timer::frame_end()
{
  const double time_render_elapsed = time_wall_fp() - time_render_start;
  if ( query_timer_id )
    {
      glEndQuery(GL_TIME_ELAPSED_EXT);
      int timer_val = 0;
      glGetQueryObjectiv(query_timer_id,GL_QUERY_RESULT,&timer_val);
      gpu_tsum_ns += timer_val;
    }
  cpu_tsum += time_render_elapsed;
  const uint xfcount_prev = xfcount;
  if ( ptr_glXGetVideoSyncSGI ) ptr_glXGetVideoSyncSGI(&xfcount);
  frame_rate_text = "";

  frame_rate_text.sprintf("FPS: %.2f XF ", frame_rate);
  if ( ptr_glXGetVideoSyncSGI )
    frame_rate_text.sprintf("%2d", xfcount - xfcount_prev );
  else
    frame_rate_text += "--";

  frame_rate_text += "  GPU.GL ";
  if ( query_timer_id )
    frame_rate_text.sprintf
      ("%.3f ms (%.1f%%)", 1000 * gpu_tlast, 100 * gpu_frac);
  else
    frame_rate_text += "---";

  frame_rate_text += "  GPU.CU ";
  if ( cuda_in_use )
    frame_rate_text.sprintf
      ("%.3f ms (%.1f%%)", 1000 * cuda_tlast, 100 * cuda_frac);
  else
    frame_rate_text += "---";

  frame_rate_text.sprintf
    ("  CPU %.2f ms (%.1f%%)", 1000 * cpu_tlast, 100 * cpu_frac);

  if ( work_description )
    frame_rate_text.sprintf("  %s %.3f", work_description, work_rate);
}

struct pVariable_Control_Elt
{
  char *name;
  float *var;
  float inc_factor, dec_factor;
  int *ivar;
  int inc, min, max;
  bool power_of_2;
  double get_val() const { return var ? *var : double(*ivar); }
};

class pVariable_Control {
public:
  pVariable_Control()
  {
    size = 0;  storage = (pVariable_Control_Elt*)malloc(0); current = NULL;
    inc_factor_def = pow(10,1.0/45);
  }
  void insert(int &var, const char *name, int inc = 1,
              int min = 0, int max = 0x7fffffff )
  {
    pVariable_Control_Elt* const elt = insert_common(name);
    elt->ivar = &var;
    elt->inc = 1;
    elt->min = min;
    elt->max = max;
  }

  void insert(float &var, const char *name, float inc_factor = 0)
  {
    pVariable_Control_Elt* const elt = insert_common(name);
    elt->var = &var;
    elt->inc_factor = inc_factor ? inc_factor : inc_factor_def;
    elt->dec_factor = 1.0 / elt->inc_factor;
  }

  void insert_power_of_2(int &var, const char *name)
  {
    pVariable_Control_Elt* const elt = insert_common(name);
    elt->ivar = &var;
    elt->inc_factor = 1;
    elt->dec_factor = 1;
    elt->power_of_2 = true;
  }

private:
  pVariable_Control_Elt* insert_common(const char *name)
  {
    size++;
    const int cidx = current - storage;
    storage = (pVariable_Control_Elt*)realloc(storage,size*sizeof(*storage));
    pVariable_Control_Elt* const elt = &storage[size-1];
    current = &storage[ size == 1 ? 0 : cidx ];
    elt->name = strdup(name);
    elt->ivar = NULL;
    elt->var = NULL;
    elt->power_of_2 = false;
    return elt;
  }
public:

  void adjust_higher() {
    if ( !current ) return;
    if ( current->power_of_2 ) current->ivar[0] <<= 1;
    else if ( current->var )   current->var[0] *= current->inc_factor;
    else                
      {
        current->ivar[0] += current->inc;
        set_min(current->ivar[0],current->max);
      }
  }
  void adjust_lower() {
    if ( !current ) return;
    if ( current->power_of_2 ) 
      { if ( current->ivar[0] ) current->ivar[0] >>= 1; }
    else if ( current->var )   current->var[0] *= current->dec_factor;
    else
      {
        current->ivar[0] -= current->inc;
        set_max(current->ivar[0],current->min);
      }
  }
  void switch_var_right()
  {
    if ( !current ) return;
    current++;
    if ( current == &storage[size] ) current = storage;
  }
  void switch_var_left()
  {
    if ( !current ) return;
    if ( current == storage ) current = &storage[size];
    current--;
  }
  int size;
  pVariable_Control_Elt *storage, *current;
  float inc_factor_def;
};

const void* const all_glut_fonts[] =
  { GLUT_STROKE_ROMAN,
    GLUT_STROKE_MONO_ROMAN,
    GLUT_BITMAP_9_BY_15,
    GLUT_BITMAP_8_BY_13,
    GLUT_BITMAP_TIMES_ROMAN_10,
    GLUT_BITMAP_TIMES_ROMAN_24,
    GLUT_BITMAP_HELVETICA_10,
    GLUT_BITMAP_HELVETICA_12,
    GLUT_BITMAP_HELVETICA_18 };

const void* const glut_fonts[] =
  { 
    GLUT_BITMAP_TIMES_ROMAN_10,
    GLUT_BITMAP_HELVETICA_10,
    GLUT_BITMAP_HELVETICA_12,
    GLUT_BITMAP_8_BY_13,
    GLUT_BITMAP_9_BY_15,
    GLUT_BITMAP_HELVETICA_18,
    GLUT_BITMAP_TIMES_ROMAN_24
 };

using namespace Magick;

class pOpenGL_Helper {
public:
  pOpenGL_Helper(int& argc, char** argv)
  {
    glut_font_idx = 2;
    opengl_helper_self_ = this;
    width = height = 0;
    frame_period = -1; // No timer callback.
    next_frame_time = 0;
    cb_keyboard();
    init_gl(argc, argv);
  }
  ~pOpenGL_Helper(){}

  void rate_set(double frames_per_second)
  {
    frame_period = 1.0 / frames_per_second;
  }

  double next_frame_time, frame_period;
  static void cb_timer_w(int data){ opengl_helper_self_->cbTimer(data); }
  void cbTimer(int data)
  {
    glutPostRedisplay();
    if ( frame_period < 0 ) return;
    if ( next_frame_time == 0 ) next_frame_time = time_wall_fp();
    const double now = time_wall_fp();
    next_frame_time += frame_period;
    const double delta = next_frame_time - now;
    const int delta_ms = delta <= 0 ? 0 : int(delta * 1000);
    glutTimerFunc(delta_ms,cb_timer_w,0);
  }

  // Use DISPLAY_FUNC to write frame buffer.
  //
  void display_cb_set
  (void (*display_func)(void *), void *data)
  {
    user_display_func = display_func;
    user_display_data = data;
    glutDisplayFunc(&cb_display_w);
    glutKeyboardFunc(&cb_keyboard_w);
    glutSpecialFunc(&cb_keyboard_special_w);
# ifdef GLX_SGI_video_sync
    glutIdleFunc(cb_idle_w);
# else
    glutTimerFunc(10,cb_timer,0);
    cbTimer(0);
# endif
    glutMainLoop();
  }

  void use_timer()
  {
    glutIdleFunc(NULL);
    glutTimerFunc(10,cb_timer_w,0);
    printf("Switching from idle callback to timer callback.\n");
  }

  static void cb_idle_w(){ opengl_helper_self_->cb_idle(); }
  void cb_idle()
  {
    if ( !ptr_glXGetVideoSyncSGI ) { use_timer(); return; }
#   ifdef GLX_SGI_video_sync
    unsigned int count;
    ptr_glXGetVideoSyncSGI(&count);
    unsigned int count_after;
    if ( ptr_glXWaitVideoSyncSGI(1,0,&count_after) )
      {
        use_timer();
        return;
      }
    glutPostRedisplay();
# endif
  }


  // Return width and height of frame buffer.
  //
  int get_width() { return width; }
  int get_height() { return height; }

  // Key pressed by user since last call of DISPLAY_FUNC.
  // ASCII value, one of the FB_KEY_XXXX macros below, or 0 if
  // no key pressed.
  //
  int keyboard_key;
  int keyboard_x, keyboard_y;  // Mouse location when key pressed.

  PStack<pString> user_frame_text;

  // Print text in frame buffer, starting at upper left.
  // Arguments same as printf.
  //
  void fbprintf(const char* fmt, ...)
  {
    va_list ap;
    pString& str = *user_frame_text.pushi();
    va_start(ap,fmt);
    str.vsprintf(fmt,ap);
    va_end(ap);
    if ( !frame_print_calls ) glWindowPos2i(10,height-20);
    frame_print_calls++;

    glutBitmapString((void*)glut_fonts[glut_font_idx],(unsigned char*)str.s);
  }

  void user_text_reprint()
  {
    glDisable(GL_DEPTH_TEST);
    glWindowPos2i(10,height-20);
    while ( pString* const str = user_frame_text.iterate() )
      glutBitmapString
        ((void*)glut_fonts[glut_font_idx],(unsigned char*)str->s);  
    user_frame_text.reset();
    glEnable(GL_DEPTH_TEST);
  }

  void cycle_font()
  {
    const int font_cnt = sizeof(glut_fonts) / sizeof(glut_fonts[0]);
    glut_font_idx++;
    if ( glut_font_idx >= font_cnt ) glut_font_idx = 0;
    printf("Changed to %d\n",glut_font_idx);
  }

private:
  void init_gl(int& argc, char** argv)
  {
    exe_file_name = argv && argv[0] ? argv[0] : "unknown name";
    glutInit(&argc, argv);
    lglext_ptr_init();

    glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL );
    glutInitWindowSize(640,480);

    pStringF title("OpenGL Demo - %s",exe_file_name);

    glut_window_id = glutCreateWindow(title);

    // Note: These functions don't work before a window is created.
    //
    P_GL_PRINT_STRING(GL_VENDOR);
    P_GL_PRINT_STRING(GL_RENDERER);
    P_GL_PRINT_STRING(GL_VERSION);

  }

  static void cb_display_w(void){ opengl_helper_self_->cb_display(); }
  void cb_display(void)
  {
    shape_update();
    frame_print_calls = 0;
    user_display_func(user_display_data);
    user_text_reprint();
    cb_keyboard();
  }

  void shape_update()
  {
    const int width_new = glutGet(GLUT_WINDOW_WIDTH);
    const int height_new = glutGet(GLUT_WINDOW_HEIGHT);
    width = width_new;
    height = height_new;
  }

  static void cb_keyboard_w(unsigned char key, int x, int y)
  {opengl_helper_self_->cb_keyboard(key,x,y);}
  static void cb_keyboard_special_w(int key, int x, int y)
  {opengl_helper_self_->cb_keyboard(key+0x100,x,y);}
  void cb_keyboard(int key=0, int x=0, int y=0)
  {
    keyboard_key = key;
    keyboard_x = x;
    keyboard_y = y;
    if ( !key ) return;
    if ( keyboard_key == FB_KEY_F12 ) { write_img(); return; }
    if ( keyboard_key == FB_KEY_F11 ) { cycle_font(); return; }
    glutPostRedisplay();
  }

  void write_img()
  {
    pStringF file_name("%s.png",exe_file_name);
    glReadBuffer(GL_FRONT_LEFT);
    const int size = width * height;
    const int bsize = size * 4;
    char* const pbuffer = (char*) malloc(bsize);
    glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,pbuffer);
    Image image( width, height, "RGBA", CharPixel, pbuffer);
    image.flip();
    image.write(file_name.s);
    free(pbuffer);
  }

private:
  const char* exe_file_name;
  double render_start;
  int width;
  int height;
  int frame_print_calls;
  int glut_font_idx;
  int glut_window_id;
  void (*user_display_func)(void *data);
  void *user_display_data;

};

#endif
