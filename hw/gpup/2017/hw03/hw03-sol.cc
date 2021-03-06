/// LSU EE 4702-1 (Fall 2017), GPU Programming
//
 /// Homework 3 -- SOLUTION
 //
 //  Search for SOLUTION in this file to find changes.
 //  In git use git diff to find changes.


 /// Instructions
 //
 //  Read the assignment: http://www.ece.lsu.edu/koppel/gpup/2017/hw03.pdf

#if 0
/// CODE OUTLINE

 /// Code Outline
 //
 //  - Preparing a Texture
 //  - Division of Platform into Overlays (Use many small textures.)
 //  - Coordinate Spaces: Object (balls) to Overlays to Texture Pixels

 /// Code Organization

 /// Platform_Overlay
 //
 Platform_Overlay* platform_overlays;  // Array of overlays.
 //
 //  A data structure holding info about texture that covers part of platform.
 //
 //  Platform is covered by nx * ny (default 40*40) overlays ..
 //  .. arranged in a grid.
 //
 //  Initialized in My_Piece_Of_The_World::init():



 /// Overlay Rationale
 //
 //  We expect to update texture on CPU and send it back go GPU.
 //
 //  This may happen every frame.
 //
 //  Suppose minimum scuff size is 1/100 of a tile:
 //    19 * 19 platform tiles  * 100 * 100  * 24 bytes / texel
 //    = 86640000 B
 //
 //  Recall: PCIe x16:  15.8 GB/s
 //  Suppose we want a frame rate of 60 f/s
 //   15.8 10^9 / ( 60 * 86640000 ) = 3.04
 //   About 1/3 of the capacity will be used for texture updates.




#endif
/// Purpose
//
//   Demonstrate simulation of point masses connected by springs.


/// What Code Does

// Simulates balls connected by springs over a platform. Balls and
// springs can be initialized in different arrangements (called
// scenes). Currently scene 1 is a simple string of beads, and scenes
// 2, 3, and 4 are trusses. The platform consists of tiles, some are
// purple-tinted mirrors (showing a reflection of the ball), the
// others show the course syllabus.



///  Keyboard Commands
 //
 /// Object (Eye, Light, Ball) Location or Push
 //   Arrows, Page Up, Page Down
 //        Move object or push ball, depending on mode.
 //        With shift key pressed, motion is 5x faster.
 //   'e': Move eye.
 //   'l': Move light.
 //   'b': Move head (first) ball. (Change position but not velocity.)
 //   'B': Push head ball. (Add velocity.)
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
 //  'c'    Clean the platform.
 //  'w'    Twirl balls around axis formed by head and tail.
 //  '1'    Set up scene 1.
 //  '2'    Set up scene 2.
 //  '3'    Set up scene 3.
 //  'p'    Pause simulation. (Press again to resume.)
 //  ' '    (Space bar.) Advance simulation by 1/30 second.
 //  'S- '  (Shift-space bar.) Advance simulation by one time step.
 //  'h'    Freeze position of first (head) ball. (Press again to release.)
 //  't'    Freeze position of last (tail) ball. (Press again to release.)
 //  's'    Stop balls.
 //  'g'    Turn gravity on and off.
 //  'y'    Toggle value of opt_tryout1. Intended for experiments and debugging.
 //  'Y'    Toggle value of opt_tryout2. Intended for experiments and debugging.
 //  'F11'  Change size of green text (in upper left).
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
 //  VAR Spring Constant - Set spring constant.
 //  VAR Time Step Duration - Set physics time step.
 //  VAR Air Resistance - Set air resistance.
 //  VAR Light Intensity - The light intensity.
 //  VAR Gravity - Gravitational acceleration. (Turn on/off using 'g'.)


#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

#include <Magick++.h>

#include <gp/util.h>
#include <gp/glextfuncs.h>
#include <gp/coord.h>
#include <gp/shader.h>
#include <gp/pstring.h>
#include <gp/misc.h>
#include <gp/gl-buffer.h>
#include <gp/texture-util.h>
#include <gp/colors.h>
#include <gp/util-containers.h>
#include "shapes.h"

// Make ImageMagick symbols visible.
using namespace Magick;


///
/// Main Data Structures
///
//
// class World: All data about scene.


class World;


class Platform_Overlay {
public:
  Platform_Overlay():data(NULL){}

  // Note: When using the array below as an argument to glTexImage2D
  // use data format GL_RGBA and type GL_FLOAT.
  //
  pColor *data;
  GLuint txid;
  float overlay_xmin, overlay_zmin;

  bool texture_modified;
  bool texture_object_initialized;

  pCoor vertices[4];

};

//   Use this class to define variables and member functions.
//   Don't modify hw03-graphics.cc.
//
class My_Piece_Of_The_World {
public:
  My_Piece_Of_The_World(World& wp):w(wp){};
  World& w;
  Platform_Overlay* platform_overlays;  // Array of overlays.
  Platform_Overlay sample_overlay;
  int nx, nz;          // Number of overlays along each dimension.
  int num_overlays;
  int twid_x, twid_z;  // Dimensions of each texture in texels.
  int num_texels;
  float wid_x, wid_z;  // Width of each overlay in object space units.
  float wid_x_inv, wid_z_inv;  // Their inverses.

  float scale_x_obj_to_texel;
  float scale_z_obj_to_texel;

  int syl_wd, syl_ht;
  vector<PixelPacket> syl_pixels;

  void init();
  void sample_tex_make();

  // Return platform tile coordinate in x and z components. Tile
  // coordinates range from 0 to platform_tile_count in the x and z
  // directions. For example, (0.1,0,3.5) indicates that the
  // coordinate is near the left (low x) side of the first tile in the
  // x direction and in the middle of the fourth tile in the z
  // direction.
  //
  // If w component is 1 then tile shows texture (syllabus),
  // if w component is 0 then tile is mirrored.
  //
  pCoor platform_obj_to_tile(pCoor obj_coord);

  float opt_smear_factor;

  pCoor platform_tile_to_obj(pCoor tile_coord);

  pCoor overlay_obj_to_ovr(pCoor pos_obj);

  // Return the platform overlay that includes pos, or NULL if pos is
  // not on platform.
  //
  Platform_Overlay* po_get(pCoor pos);

  // Convert object space coordinate to texel coordinate relative
  // to overlay po.
  pCoor overlay_obj_to_tex(Platform_Overlay *po, pCoor pos);

  void render();
  void clean();
};


// Object Holding Ball State
//
class Ball {
public:
  Ball():velocity(pVect(0,0,0)),locked(false),
         color(color_lsu_spirit_gold),contact(false){};
  pCoor position;
  pVect velocity;

  float mass;
  float mass_min; // Mass below which simulation is unstable.
  float radius;

  bool locked;

  pVect force;
  pColor color;
  bool contact;                 // When true, ball rendered in gray.
  float spring_constant_sum;    // Used to compute minimum mass.

  vector<pColor> glop;

  void push(pVect amt);
  void translate(pVect amt);
  void stop();
  void freeze();
};

class Link {
public:
  Link(Ball *b1, Ball *b2):ball1(b1),ball2(b2),
     distance_relaxed(pDistance(b1->position,b2->position)), snapped(false),
     natural_color(color_lsu_spirit_purple),color(color_lsu_spirit_purple){}
  Link(Ball *b1, Ball *b2, pColor colorp):ball1(b1),ball2(b2),
     distance_relaxed(pDistance(b1->position,b2->position)), snapped(false),
     natural_color(colorp),color(colorp){}
  Ball* const ball1;
  Ball* const ball2;
  float distance_relaxed;
  bool snapped;
  pColor natural_color;
  pColor color;
};

// Declare containers and iterators for Balls and Links.
// (See util_container.h.)
//
typedef pVectorI<Link> Links;
typedef pVectorI<Ball> Balls;

struct Truss_Info {

  // See make_truss for a description of what the members are for.

  // Inputs
  PStack<pCoor> base_coors;  // Coordinates of first set of balls.
  pVect unit_length;
  int num_units;

  // Output
  Balls balls;
  Links links;
};

#include "hw03-graphics.cc"

void
My_Piece_Of_The_World::init()
{
  // Number of overlays along each direction.
  //
  nx = 40; nz = 40;
  num_overlays = nx * nz;

  // Number of texels along each dimension of each overlay's texture.
  //
  twid_x = 512; twid_z = 512;
  num_texels = twid_x * twid_z;

  sample_tex_make();

  platform_overlays = new Platform_Overlay[num_overlays];

  // Dimensions of overlay in object-space coordinates.
  //
  wid_x = ( w.platform_xmax - w.platform_xmin ) / nx;
  wid_z = ( w.platform_zmax - w.platform_zmin ) / nz;
  wid_x_inv = 1.0 / wid_x;
  wid_z_inv = 1.0 / wid_z;

  //  Compute variables that will come in handy when converting
  //  from object space coordinates to texel coordinates (for our overlay).
  //
  scale_x_obj_to_texel = wid_x_inv * twid_x;
  scale_z_obj_to_texel = wid_z_inv * twid_z;

  /// SOLUTION - Minor Change - Choose a better looking smear factor.
  opt_smear_factor = 1.43;
  w.variable_control.insert(opt_smear_factor,"Smear Factor");

}

void
My_Piece_Of_The_World::sample_tex_make()
{
  /// Homework 3 -- Sample Code

  // Code in this routine creates a texture with a big red X, and
  // loads it into a texture object.  The texture object can
  // be used as a substitute for the "scuffed" texture before
  // the scuffed texture part of this assignment is finished.

  Platform_Overlay* const po = &sample_overlay;

  // Allocate storage for the array of texels.
  //
  if ( !po->data ) po->data = new pColor[ num_texels ];

  // Initialize the texels to black and transparent. (Alpha = 0)
  //
  memset(po->data,0,num_texels*sizeof(po->data[0]));

  //
  // Put syllabus in part of texture.
  //

  // ImageMagick C++ Binding Documentation:
  //     http://www.imagemagick.org/Magick++/Image++.html
  Magick::Image image(w.platform_tex_path);

  const int img_wd = image.columns();
  const int img_ht = image.rows();
  if ( img_wd )
    {
      // Image was read, include it in texture.

      syl_wd = img_wd * ( w.platform_trmax - w.platform_trmin );
      syl_ht = img_ht * ( w.platform_tsmax - w.platform_tsmin );
      const int cor_c = img_wd * w.platform_trmin;
      const int cor_r = img_ht * w.platform_tsmin;

      PixelPacket* pp = image.getPixels(cor_c,cor_r,syl_wd,syl_ht);
      syl_pixels.resize(syl_wd*syl_ht);
      for ( int c = 0;  c < syl_wd;  c++ )
        for ( int r = 0;  r < syl_ht;  r++ )
          syl_pixels[ syl_wd - c - 1 + ( syl_ht -r - 1 ) * syl_wd ]
            = pp[ c + r * syl_wd ];

      const double ixtx = syl_wd * 2.0 / twid_x;
      const double iytz = syl_ht * 2.0 / twid_z;
      const double sc = 1.0 / MaxRGB;
      for ( int tx = 0;  tx < twid_x / 2;  tx++ )
        for ( int tz = 0;  tz < twid_z / 2;  tz++ )
          {
            int ix = tx * ixtx;
            int iy = tz * iytz;
            PixelPacket p = syl_pixels[ix + iy * syl_wd];
            po->data[tx + tz * twid_x] =
              pColor( p.red * sc, p.green * sc, p.blue * sc );
          }
    }

  // Thickness of the strokes making up the letter ex.
  //
  const int thickness = max(2, twid_x/10);

  // Write the letter ex, a big one, in the texture.
  //
  for ( int tx=0; tx<twid_x-thickness; tx++ )
    {
      // Note: Compute tz without assuming twid_x == twid_z.
      int tz_raw = float(tx)/twid_x * twid_z;
      int tz = min(tz_raw,twid_z-1);

      // Array index of texel at (tx,tz).
      //
      int idx  = tx              + tz * twid_x; // Lower left to upper right.
      int idx2 = twid_x - 1 - tx + tz * twid_x; // Lower right to upper left.

      // Write colors to texels.
      //
      for ( int i=0; i<thickness; i++ )
        for ( pColor* c: { &po->data[idx+i], &po->data[idx2-i] } )
          {
            *c = 0.5 * ( *c + color_red );
            c->a = 1;
          }
    }

  // Create a new texture object.
  //
  glGenTextures(1,&po->txid);

  // Make our new texture object the current texture object.
  //
  glBindTexture(GL_TEXTURE_2D,po->txid);
  //
  // Subsequent OpenGL calls operating on GL_TEXTURE_2D ..
  // .. will now operate on po->txid, our new texture object.

  // Tell OpenGL to generate MIPMAP levels for us.
  //
  glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);

  // Load our texture into the texture object.
  //
  glTexImage2D
    (GL_TEXTURE_2D,
     0,                // Level of Detail (0 is base).
     GL_RGBA,          // Internal format to be used for texture.
     twid_x, twid_z,
     0,                // Border
     GL_RGBA,          // GL_BGRA: Format of data read by this call.
     GL_FLOAT,         // Size of component.
     (void*)po->data   // Pointer to the texture data.
     );
}

pCoor
My_Piece_Of_The_World::platform_obj_to_tile(pCoor pos_obj)
{
  /// Problem 1 Solution Goes Here 

  const float x = ( pos_obj.x - w.platform_xmin ) / w.platform_tile_sz_x;
  const float z = ( pos_obj.z - w.platform_zmin ) / w.platform_tile_sz_z;
  pCoor rv(x,0,z);

  // Make sure that conversion is correct by applying the inverse conversion.
  pCoor check = platform_tile_to_obj(rv);
  assert( fabs(pos_obj.x - check.x) < 0.0001 );
  assert( fabs(pos_obj.z - check.z) < 0.0001 );

  bool mirrored = ( int(x) + int(z) * w.platform_tile_count ) & 1;
  rv.w = !mirrored;

  return rv;
}

pCoor
My_Piece_Of_The_World::platform_tile_to_obj(pCoor tile_coord)
{
  pCoor rv =
    pVect(w.platform_xmin, 0, w.platform_zmin) +
    pCoor( tile_coord.x * w.platform_tile_sz_x,
           tile_coord.y,
           tile_coord.z * w.platform_tile_sz_z );

  return rv;
}


pCoor
My_Piece_Of_The_World::overlay_obj_to_tex(Platform_Overlay *po, pCoor pos_obj)
{
  pCoor lc;
  // Convert object space coordinates to texel coordinates.
  // Variables overlay_xmin and overlay_zmin were set when the overlay
  // was retrieved.
  //
  lc.x = ( pos_obj.x - po->overlay_xmin ) * scale_x_obj_to_texel;
  lc.z = ( pos_obj.z - po->overlay_zmin ) * scale_z_obj_to_texel;
  lc.y = 0;
  lc.w = 0;
  return lc;
}

pCoor
My_Piece_Of_The_World::overlay_obj_to_ovr(pCoor pos_obj)
{
  // Convert object-space coordinate, pos, into an overlay space, (x,z) ..
  // .. in which (0,0) is the lower-left overlay, etc.

  pCoor pos_ovr;
  pos_ovr.w = 1; // Will be set back to 0 if within platform.

  pos_ovr.x = ( pos_obj.x - w.platform_xmin ) * wid_x_inv;
  if ( pos_ovr.x < 0 || pos_ovr.x >= nx ) pos_ovr.w = 0;
  pos_ovr.z = ( pos_obj.z - w.platform_zmin ) * wid_z_inv;
  if ( pos_ovr.z < 0 || pos_ovr.z >= nz ) pos_ovr.w = 0;
  return pos_ovr;
}

Platform_Overlay*
My_Piece_Of_The_World::po_get(pCoor pos)
{
  // Convert object-space coordinate, pos, into an overlay space, (x,z) ..
  // .. in which (0,0) is the lower-left overlay, etc.

  pCoor pos_ovr = overlay_obj_to_ovr(pos);
  if ( pos_ovr.w == 0 ) return NULL;

  const int x = pos_ovr.x;
  const int z = pos_ovr.z;

  // Retrieve the overlay in which pos lies.
  //
  Platform_Overlay* const po = &platform_overlays[x + z * nz];

  po->overlay_xmin = w.platform_xmin + x * wid_x;
  po->overlay_zmin = w.platform_zmin + z * wid_z;

  if ( !po->data )
    {
      // This overlay has never been visited, initialize texel array.

      // Allocate the color array.
      po->data = new pColor[num_texels];

      // Initialize it. Note that this makes the colors transparent.
      memset(po->data,0,num_texels*sizeof(po->data[0]));

      po->texture_object_initialized = false;

      // Pre-compute the object-space coordinates of the corners of
      // the overlay. These will be used when constructing the primitive
      // on which the texture will be applied.
      //
      pCoor* const vertices = po->vertices;
      vertices[0] = pCoor( po->overlay_xmin, 0.01, po->overlay_zmin );
      vertices[1] = vertices[0] + pVect(wid_x,0,0);
      vertices[2] = vertices[1] + pVect(0,0,wid_z);
      vertices[3] = vertices[0] + pVect(0,0,wid_z);
      po->texture_modified = true;

      /// SOLUTION: Homework 3  Problem 3 -- Initialize texture to match image tile.

#if 0
      // Copy sample texture.
      // 
      memcpy(po->data,sample_overlay.data,num_texels*sizeof(pCoor));
      //
      /// Please remove the memcpy when this part is solved.
#endif

      // Object space coordinates of overlay lower-left (00) and
      // upper-right (11) corners.
      //
      pCoor ovr_obj_00 = vertices[0];

      /// SOLUTION Homework 3 Problem 3

      // Factors to scale overlay texel space coordinates to object space.
      //
      const float oxtx = wid_x / twid_x;
      const float oztz = wid_z / twid_z;

      // Scale ImageMagick color components to range [0,1].
      //
      const float sc = 1.0 / MaxRGB;

      // Convenience function for extracting the fractional part of a number.
      //
      auto fpart = [] ( float f ) { float i; return modf(f,&i); };

      for ( int tx = 0;  tx < twid_x;  tx++ )
        for ( int tz = 0;  tz < twid_z;  tz++ )
          {
            // Get object-space coordinates of this texel ..
            //
            pCoor coor_obj = ovr_obj_00 + pVect( tx * oxtx, 0, tz * oztz );
            //
            // .. and convert that to platform tile space.
            //
            pCoor coor_tile = platform_obj_to_tile(coor_obj);

            // Do nothing if this texel is over a mirror tile.
            //
            if ( !coor_tile.w ) continue;

            // From tile-space coordinate compute location in syl_pixels array
            // and retrieve the pixel, if it's not out of range.
            //
            const int idx_syl =
              int ( fpart(coor_tile.x) * syl_wd )
              + int ( fpart(coor_tile.z) * syl_ht ) * syl_wd;
            if ( idx_syl >= syl_wd * syl_ht ) continue;
            PixelPacket p = syl_pixels[ idx_syl ];

            // Convert ImageMagick pixel to a pColor and write it to our
            // texture.
            //
            const int idx_otex = tx + tz * twid_x;
            po->data[idx_otex] = pColor( p.red, p.green, p.blue ) * sc;
          }
    }

  return po;
}

void
My_Piece_Of_The_World::render()
{
  // See demo-8-texture.cc for examples.

  // Turn on texturing.
  glEnable(GL_TEXTURE_2D);

  // Specify which texture unit to use.
  glActiveTexture(GL_TEXTURE0);

  // Specify how texel and the primitive's lighted color should be
  // combined. Use modulation, so that scuffs are affected by
  // lighting.
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  // Turn on the alpha test. This is not needed if blending is in use.
  if ( w.opt_tryout1 ) glEnable(GL_ALPHA_TEST);

  // Only write fragments corresponding to scuffed parts of the texture.
  glAlphaFunc(GL_GREATER,0.4);

  // Turn on blending so that scuffs are combined with what's already there.
  glEnable(GL_BLEND);

  // Set the blend equation to blend colors, but to write the alpha value
  // carried by the fragment (from our texture).
  glBlendFuncSeparate
    (GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);

  for ( int i=0; i<num_overlays; i++ )
    {
      Platform_Overlay* const po = &platform_overlays[i];
      if ( !po->data ) continue;

      if ( !po->texture_object_initialized )
        {
          po->texture_object_initialized = true;

          //  This is the first time we've used this particular overlay,
          //  so we need to create a texture object for it and set its
          //  parameters.
          //
          glGenTextures(1,&po->txid);
          glBindTexture(GL_TEXTURE_2D,po->txid);
          glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);
          glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR_MIPMAP_LINEAR);
          glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        }

      glBindTexture(GL_TEXTURE_2D,po->txid);

      if ( po->texture_modified )
        {
          // We've modified the texture since the last render, so
          // we need to update OpenGL's copy.

          po->texture_modified = false;

          glTexImage2D
            (GL_TEXTURE_2D,
             0,                // Level of Detail (0 is base).
             GL_RGBA,          // Internal format to be used for texture.
             twid_x, twid_z,
             0,                // Border
             GL_RGBA,     // GL_BGRA: Format of data read by this call.
             GL_FLOAT,    // Size of component.
             (void*)po->data);
          pError_Check();
        }


      //  If tryout2 is active (press 'Y' to toggle) show the red x texture
      //  instead of our own.
      //
      if ( w.opt_tryout2 ) glBindTexture(GL_TEXTURE_2D, sample_overlay.txid);

      //  Apply the texture to a quad. Note that we are using
      //  our pre-computed vertices.
      //
      glBegin(GL_QUADS);
      glNormal3f(0,-1,0);
      glColor3fv(color_white);
      glTexCoord2f(0,0);
      glVertex3fv(po->vertices[0]);
      glTexCoord2f(1,0);
      glVertex3fv(po->vertices[1]);
      glTexCoord2f(1,1);
      glVertex3fv(po->vertices[2]);
      glTexCoord2f(0,1);
      glVertex3fv(po->vertices[3]);
      glEnd();
    }

  glDisable(GL_ALPHA_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}

void
My_Piece_Of_The_World::clean()
{
  /// Remove scuffs from dirty textures.

  //  Free any textures that are present.

  for ( int i=0; i<num_overlays; i++ )
    {
      Platform_Overlay* const po = &platform_overlays[i];
      if ( !po->data ) continue;

      // Tell OpenGL to free the texture object, including the
      // texture data that it has copied from us.
      //
      glDeleteTextures(1,&po->txid);

      // Free our own texel array.
      //
      free( po->data );

      // Update variables to reflect clean state.
      //
      po->data = NULL;
      po->texture_modified = false;
      po->texture_object_initialized = false;
    }

  /// SOLUTION - Remove glop from balls when clean pressed.
  //             Note: This was not specifically requested.

  for ( Ball* b: w.balls ) b->glop.clear();
}


void
World::init()
{
  chain_length = 14;

  opt_time_step_duration = 0.0003;

  distance_relaxed = 15.0 / chain_length;
  opt_spring_constant = 15000;
  variable_control.insert(opt_spring_constant,"Spring Constant");

  opt_gravity_accel = 9.8;
  opt_gravity = true;
  gravity_accel = pVect(0,-opt_gravity_accel,0);
  variable_control.insert(opt_gravity_accel,"Gravity");

  opt_air_resistance = 0.04;

  world_time = 0;
  time_step_count = 0;
  last_frame_wall_time = time_wall_fp();
  frame_timer.work_unit_set("Steps / s");

  ball_eye = NULL;
  opt_ride = false;

  // File to use for texture on platform files. Image can
  // be in any format that ImageMagick can handle, which is alot.
  //
  platform_tex_path = "code_shot.png";

  // Part of texture that covers a platform tile.
  platform_trmin = 0;
  platform_trmax = 1;
  platform_tsmin = 0;
  platform_tsmax = 1;

  init_graphics();

  mp.init();

  ball_setup_2();
  lock_update();
}

Ball*
World::make_marker(pCoor position, pColor color)
{
  Ball* const ball = new Ball;
  ball->position = position;
  ball->locked = true;
  ball->velocity = pVect(0,0,0);
  ball->radius = 0.2;
  ball->mass = 0;
  ball->contact = false;
  ball->color = color;
  return ball;
}

void
World::lock_update()
{
  // This routine called when options like opt_head_lock might have
  // changed.

  // Update locked status.
  //
  if ( head_ball ) head_ball->locked = opt_head_lock;
  if ( tail_ball ) tail_ball->locked = opt_tail_lock;

  // Re-compute minimum mass needed for stability.
  //
  for ( Ball* const ball: balls ) ball->spring_constant_sum = 0;
  const double dtis = pow( opt_time_step_duration, 2 );
  for ( Link* const link: links )
    {
      Ball* const b1 = link->ball1;
      Ball* const b2 = link->ball2;
      b1->spring_constant_sum += opt_spring_constant;
      b2->spring_constant_sum += opt_spring_constant;
    }
  for ( Ball* const ball: balls )
    ball->mass_min = ball->spring_constant_sum * dtis;
}

void
World::balls_twirl()
{
  if ( !head_ball || !tail_ball ) return;

  pCoor centroid(0,0,0);
  if ( opt_head_lock && head_ball )
    centroid = head_ball->position;
  else if ( opt_tail_lock && tail_ball )
    centroid = tail_ball->position;
  else {
    for ( Ball* const ball: balls ) centroid += pCoor(ball->position,1);
    centroid.homogenize();
  }
  for ( Ball* const ball: balls )
    ball->velocity += cross( pVect(centroid,ball->position), pVect(0,3,0) );

  return;

  pNorm axis(head_ball->position, tail_ball->position);

  for ( Ball* const ball: balls )
    {
      pVect b_to_top(ball->position,head_ball->position);
      const float dist_along_axis = dot(b_to_top,axis);
      const float lsq = b_to_top.mag_sq() - dist_along_axis * dist_along_axis;
      if ( lsq <= 1e-5 ) { ball->velocity = pVect(0,0,0); continue; }
      const float dist_to_axis = sqrt(lsq);
      pNorm rot_dir = cross(b_to_top,axis);
      ball->velocity += 10 * dist_to_axis * rot_dir;
    }
}

void
World::objects_erase()
{
  ball_eye = NULL;
  balls.erase();
  links.erase();
}

///
/// Physical Simulation Code
///

 /// Initialize Simulation
//

void
World::ball_setup_1()
{
  // Arrange and size balls to form a pendulum.

  pCoor first_pos(13.4,10,-9.2);
  pVect delta_pos = pVect(distance_relaxed,0,0);

  // Remove objects from the simulated objects lists, balls and links.
  // The delete operator is used on objects in the lists.
  //
  objects_erase();

  for ( int i=0; i<chain_length; i++ )
    {
      // Construct a new ball and add it to the simulated objects list (balls).
      //
      Ball* const ball = balls += new Ball;

      // Initialize position and other information.
      //
      ball->position = first_pos + i * delta_pos;
      ball->locked = false;
      ball->velocity = cross(pVect(ball->position,first_pos),pVect(0,1,0));
      ball->radius = 0.3;
      ball->mass = 4/3.0 * M_PI * pow(ball->radius,3);
      ball->contact = false;

      // If it's not the first ball link it to the previous ball.
      if ( i > 0 ) links += new Link( ball, balls[i-1] );
    }

  // The balls pointed to by head_ball and tail_ball can be manipulated
  // using the user interface (by pressing 'h' or 't', for example).
  // Set these variables.
  //
  head_ball = balls[0];
  tail_ball = balls[balls-1];

  opt_head_lock = true;    // Head ball will be frozen in space.
  opt_tail_lock = false;   // Tail ball can move freely.
}


void
World::make_truss(Truss_Info *truss_info)
{
  /// Construct a truss based on members of truss_info.
  //

  //            <---- num_units (=9) ----------->
  //  j
  //  0         O---O---O---O---O---O---O---O---O     ^
  //            |   |   |   |   |   |   |   |   |     |
  //  1         O---O---O---O---O---O---O---O---O   num_sides (=3)
  //            |   |   |   |   |   |   |   |   |     |
  //  2         O---O---O---O---O---O---O---O---O     v
  //
  //      i ->  0   1   2   3   4   5   6   7   8
  //
  //  Note: Not all links are shown in the diagram above.

  /// Truss_Info Inputs
  //
  //  truss_info->num_units
  //    The number of sections in the truss (see diagram above).
  //
  //  truss_info->base_coors
  //    A list containing num_sides coordinates, the coordinates of
  //    balls at i=0 (see diagram above). (num_sides is the number of
  //    elements in this list.)  These coordinates should all be
  //    in the same plane.
  //
  //  truss_info->unit_length
  //    A vector pointing from the ball at (i,j) to the ball at (i+1,j).
  //
  /// Truss_Info Outputs
  //
  //  truss_info->balls
  //    A list that should be filled with the balls making up the truss.
  //
  //  truss_info->links
  //    A list that should be filled with the links making up the truss.


  const int num_sides = truss_info->base_coors.occ();
  const int num_units = truss_info->num_units;

  // Lists to hold balls and links created for truss.
  //
  Balls& bprep = truss_info->balls;
  Links& lprep = truss_info->links;

  // Create balls for truss.
  //
  for ( int i=0; i<num_units; i++ )
    for ( pCoor bcoor; truss_info->base_coors.iterate(bcoor); )
      {
        Ball* const ball = bprep += new Ball;
        ball->position = bcoor + i * truss_info->unit_length;
        ball->locked = false;
        ball->velocity = pVect(0,0,0);
        ball->radius = 0.15;
        ball->mass = 4/3.0 * M_PI * pow(ball->radius,3);
        ball->contact = false;
      }

  // Create links.
  //
  for ( int i=0; i<num_units; i++ )
    for ( int j=0; j<num_sides; j++ )
      {
        const int idx = j + num_sides * i;

        // Retrieve the ball corresponding to (i,j).
        //
        Ball* const ball = bprep[idx];

        // Compute the index of the ball at (i, (j-1) mod num_sides ).
        //
        const int pn_idx = idx + ( j == 0 ? num_sides - 1 : -1 );

        // Insert link to neighbor ball with name i.
        //
        lprep += new Link( ball, bprep[pn_idx], color_gray );

        // Insert links to balls with same i that are not neighbors.
        //
        if ( j == i % num_sides )
          for ( int k = 2; k < num_sides-1; k++ )
            lprep += new Link
              ( ball, bprep[ idx + (k+j)%num_sides - j ], color_white );

        if ( i == 0 ) continue;

        // Insert link to ball at (i-1,j).
        //
        lprep +=
          new Link( ball, bprep[idx-num_sides], color_lsu_official_purple );

        // Insert link to ball at (i-1, j-1 mod num_sides ).
        //
        lprep += new Link( ball, bprep[pn_idx-num_sides], color_green );
      }
}

void
World::ball_setup_2()
{
  const int sides = 4;
  const double delta_angle = 2*M_PI/sides;
  const double rot = delta_angle / 2;
  const float ball_radius = 0.15;
  const float spacing = distance_relaxed;
  pCoor first_pos
    (13.4 + drand48(),0.5*spacing*cos(rot)+ball_radius,-9.2+drand48());
  pVect delta_pos = pVect(spacing,0,0);
  pNorm loc_y = delta_pos;
  pNorm loc_x = pVect(0,0,1);
  pNorm loc_z = cross(loc_y,loc_x);

  // Erase the existing balls and links.
  //
  objects_erase();

  Truss_Info truss_info;

  const int truss_units = 5;

  truss_info.num_units = truss_units;
  truss_info.unit_length = delta_pos;

  for ( int j=0; j<sides; j++ )
    {
      const double angle = rot + double(j)/sides*2*M_PI;
      pCoor chain_first_pos =
        first_pos
        + 0.5 * spacing * cos(angle) * loc_x
        + 0.5 * spacing * sin(angle) * loc_z;

      truss_info.base_coors += chain_first_pos;
    }

  make_truss(&truss_info);

  // Insert links to balls at either end.
  //
  head_ball = balls += new Ball;
  head_ball->position = first_pos - delta_pos;
  for ( int j=0; j<sides; j++ )
    links += new Link( head_ball, truss_info.balls[j], color_chocolate );

  tail_ball = balls += new Ball;
  tail_ball->position = first_pos + truss_units * delta_pos;

  const int bsize = truss_info.balls.size();

  for ( int j=0; j<sides; j++ )
    links += new Link( tail_ball, truss_info.balls[bsize-sides+j],
                       color_chocolate );

  for ( Ball* const ball: balls )
    {
      ball->locked = false;
      ball->velocity = pVect(0,0,0);
      ball->radius = ball_radius;
      ball->mass = 10 * 4/3.0 * M_PI * pow(ball->radius,3);
      ball->contact = false;
    }

  balls += truss_info.balls;
  links += truss_info.links;

  opt_tail_lock = false;
  opt_head_lock = true;

  balls_twirl();

}

void
World::ball_setup_3()
{
  pCoor first_pos(13.4,17.8,-9.2);
  const float spacing = distance_relaxed;
  pVect delta_pos = pVect(spacing*0.05,-spacing,0);
  pNorm delta_dir = delta_pos;
  pNorm tan_dir = pVect(0,0,1);
  pNorm um_dir = cross(tan_dir,delta_dir);

  // Erase the existing balls and links.
  //
  objects_erase();

  Truss_Info truss_info;

  truss_info.num_units = chain_length;
  truss_info.unit_length = delta_pos;

  const int sides = 4;

  for ( int j=0; j<sides; j++ )
    {
      const double angle = double(j)/sides*2*M_PI;
      pCoor chain_first_pos =
        first_pos
        + 0.5 * spacing * cos(angle) * tan_dir
        + 0.5 * spacing * sin(angle) * um_dir;

      truss_info.base_coors += chain_first_pos;
    }

  make_truss(&truss_info);

  const int idx_center = chain_length / 2 * sides;

  for ( int i=0; i<sides; i++ )
    {
      Truss_Info ti;
      ti.num_units = chain_length / 2;
      const int idx_1 = idx_center + ( i == 0 ? sides - 1 : i - 1 );
      const int idx_2 = idx_center + i;

      Ball* const b0 = truss_info.balls[idx_1];
      Ball* const b1 = truss_info.balls[idx_1 - sides];
      Ball* const b2 = truss_info.balls[idx_2 - sides];
      Ball* const b3 = truss_info.balls[idx_2];
      ti.base_coors += b0->position;
      ti.base_coors += b1->position;
      ti.base_coors += b2->position;
      ti.base_coors += b3->position;

      pNorm v_head = cross(b1->position,b2->position,b3->position);
      ti.unit_length = delta_dir.magnitude * v_head;
      make_truss(&ti);
      links += new Link(b0,ti.balls[0],color_red);
      links += new Link(b1,ti.balls[1],color_red);
      links += new Link(b2,ti.balls[2],color_red);
      links += new Link(b3,ti.balls[3],color_red);
      links += ti.links;
      balls += ti.balls;

      int tsz = ti.balls.size();

      if ( i == 2 )
        {
          ball_eye = ti.balls[tsz-2];
          ball_down = ti.balls[tsz-1];
        }
      else if ( i == 1 )
        {
          ball_gaze = ti.balls[tsz/2];
        }

    }

  // Insert links to balls at either end.
  //
  head_ball = balls += new Ball;
  head_ball->color = color_green;
  head_ball->position = first_pos - delta_pos;
  for ( int j=0; j<sides; j++ )
    links += new Link( head_ball, truss_info.balls[j], color_chocolate );

  tail_ball = balls += new Ball;
  tail_ball->color = color_green;
  tail_ball->position = first_pos + chain_length * delta_pos;

  const int bsize = truss_info.balls.size();

  for ( int j=0; j<sides; j++ )
    links += new Link( tail_ball, truss_info.balls[bsize-sides+j],
                       color_chocolate );

  for ( Ball* const ball: balls )
    {
      ball->locked = false;
      ball->velocity = pVect(0,0,0);
      ball->radius = 0.15;
      ball->mass = 4/3.0 * M_PI * pow(ball->radius,3);
      ball->contact = false;
    }

  balls += truss_info.balls;
  links += truss_info.links;

  opt_tail_lock = false;
  opt_head_lock = false;
}


void
World::ball_setup_4()
{
}

void
World::ball_setup_5()
{
}


 /// Advance Simulation State by delta_t Seconds
//
void
World::time_step_cpu(double delta_t)
{
  time_step_count++;

  // Smoothly move ball in response to user input.
  //
  if ( adj_t_stop )
    {
      const double dt = min(world_time,adj_t_stop) - adj_t_prev;
      pVect adj = dt/adj_duration * adj_vector;
      balls_translate(adj,0);
      adj_t_prev = world_time;
      if ( world_time >= adj_t_stop ) adj_t_stop = 0;
    }

  for ( Ball* const ball: balls )
    ball->force = ball->mass * gravity_accel;

  for ( Link* const link: links )
    {
      // Spring Force from Neighbor Balls
      //
      Ball* const ball1 = link->ball1;
      Ball* const ball2 = link->ball2;

      // Construct a normalized (Unit) Vector from ball to neighbor.
      //
      pNorm ball_to_neighbor(ball1->position,ball2->position);

      const float distance_between_balls = ball_to_neighbor.magnitude;

      // Compute the speed of ball towards neighbor_ball.
      //
      pVect delta_v = ball2->velocity - ball1->velocity;
      float delta_s = dot( delta_v, ball_to_neighbor );

      // Compute by how much the spring is stretched (positive value)
      // or compressed (negative value).
      //
      const float spring_stretch =
        distance_between_balls - link->distance_relaxed;

      // Determine whether spring is gaining energy (whether its length
      // is getting further from its relaxed length).
      //
      const bool gaining_e = ( delta_s > 0.0 ) == ( spring_stretch > 0 );

      // Use a smaller spring constant when spring is loosing energy,
      // a quick and dirty way of simulating energy loss due to spring
      // friction.
      //
      const float spring_constant =
        gaining_e ? opt_spring_constant : opt_spring_constant * 0.7;

      ball1->force += spring_constant * spring_stretch * ball_to_neighbor;
      ball2->force -= spring_constant * spring_stretch * ball_to_neighbor;
    }

  ///
  /// Update Position of Each Ball
  ///

  for ( Ball* const ball: balls )
    {
      if ( ball->locked )
        {
          ball->velocity = pVect(0,0,0);
          continue;
        }

      // Update Velocity
      //
      // This code assumes that force on ball is constant over time
      // step. This is clearly wrong when balls are moving with
      // respect to each other because the springs are changing
      // length. This inaccuracy will make the simulation unstable
      // when spring constant is large for the time step.
      //
      const float mass = max( ball->mass, ball->mass_min );

      pVect delta_v = ( ball->force / mass ) * delta_t;

      const float dist_above = ball->position.y - ball->radius;

      const bool collision =
        platform_collision_possible(ball->position) && dist_above < 0;

      if ( collision )
        {
          const float spring_constant_plat =
            ball->velocity.y < 0 ? 100000 : 50000;
          const float fric_coefficient = 0.1;
          const float force_up = -dist_above * spring_constant_plat;
          const float delta_v_up = force_up / mass * delta_t;
          const float fric_force_mag = fric_coefficient * force_up;
          pNorm surface_v(ball->velocity.x,0,ball->velocity.z);
          const float delta_v_surf = fric_force_mag / mass * delta_t;
          if ( delta_v_surf > surface_v.magnitude )
            {
              // Ignoring other forces?
              delta_v =
                pVect(-ball->velocity.x,delta_v.y,-ball->velocity.z);
            }
          else
            {
              delta_v -= delta_v_surf * surface_v;
            }
          delta_v.y += delta_v_up;
        }

      ball->velocity += delta_v;

      // Air Resistance
      //
      const double fs = pow(1+opt_air_resistance,-delta_t);
      ball->velocity *= fs;

      // Update Position
      //
      // Assume that velocity is constant.
      //

      //  Remember previous ball position, used for scuff mark.
      //
      pCoor pos_prev = ball->position;

      ball->position += ball->velocity * delta_t;

      if ( !collision ) continue;

      Platform_Overlay* const po = mp.po_get(ball->position);

      if ( !po ) continue;

      //  Find the location of all of the texels scuffed
      //  by this ball during this timestep. The length
      //  of the scuff mark is based on the old and new positions,
      //  the width of the scuff mark is based on the intersection
      //  of the ball with the platform.

      // Current ball location in texel coordinates.
      //
      pCoor ball_tex = mp.overlay_obj_to_tex(po,ball->position);

      // Previous location of ball in texel coordinates.
      //
      pCoor prev_tex = mp.overlay_obj_to_tex(po,pos_prev);

      float width = mp.scale_x_obj_to_texel *
        4 * sqrt( ball->radius * ball->radius - pos_prev.y * pos_prev.y );

      // Direction along scuff mark (based on sliding motion).
      //
      pNorm skid(prev_tex,ball_tex);

      // Find direction along width of scuff mark.
      //
      pNorm nskid = cross( skid, pVect(0,1,0) );

      // Iterate over texel locations scuffed by ball.
      //
      for ( float t = 0; t <= skid.magnitude; t++ )
        for ( float u = -width; u < max(width,1.0f); u++ )
        {
          pCoor tex_pos = prev_tex + t * skid + u * nskid;
          const int idx = int(tex_pos.x) + mp.twid_x * int(tex_pos.z);
          if ( idx < 0 || idx >= mp.num_texels ) continue;

          pColor& texel = po->data[ idx ];

          /// SOLUTION -- Homework 3 Problem 2

          if ( !texel.a ) continue;

          po->texture_modified = true;

          pColor cur_color = texel;

          // Determine which element of glop array corresponds to this
          // value of u.
          //
          size_t glop_idx = u < 0 ? - 2 * int(u) : 1 + 2 * int(u);
          //
          // Note: u is perpendicular to the direction of motion. Say,
          // u = -width is the leftmost part of the smear, u=0 is the
          // center, and u=width-1 is the rightmost part.

          // Initialize any new glop array elements.
          //
          while ( ball->glop.size() <= glop_idx )
            ball->glop.emplace_back( 0, 0, 0, 0 );

          pColor& glop = ball->glop[glop_idx];

          // Save the prior glop.a. Here, glop.a is the amount of ink
          // accumulated by the ball. It's not used as an alpha
          // channel.
          //
          const float glop_a = glop.a;

          // Compute amount of the glop int to deposit at this location.
          //
          const float dropoff = glop_a/(1+glop_a);

          // Compute the amount of ink that is before we arrived here.
          // Base that on the smallest (darkest) of the three color
          // components.
          //
          const float amt_ink =
            1 - min(min(cur_color.r,cur_color.g),cur_color.b);

          // Compute the amount of ink at this location that will be
          // added to the glop.
          //
          const float pickup_factor = 0.7;
          const float pickup = pickup_factor * amt_ink;

          texel = ( 1 - dropoff ) * texel + dropoff * glop;
          texel.a = 1;

          // Compute the amount of ink the glop looses.
          //
          const float glop_dropoff = dropoff/( 2 * mp.opt_smear_factor );
          //
          // Note that there is no conservation of ink law being applied.
          // That is, it's okay of glop_dropoff != dropoff.

          // Compute the amount of ink on the glop after accounting for
          // pickup and drop off.
          //
          const float glop_a_new = glop_a - glop_dropoff + pickup;

          // Compute new glop color, taking care to re-normalize it.
          //
          glop =
            glop_a_new < 0.0001 ? pColor(0.5,0.5,0.5) :
            ( 1 / glop_a_new )
            * ( ( glop_a - glop_dropoff ) * glop + pickup * cur_color );

          glop.a = glop_a_new;

          continue;

          /// SOLUTION ABOVE


          // Don't bother if already scuffed. If we wanted to be
          // fancy we could add this balls color onto what is already
          // there. But we don't want to be fancy.
          //
          if ( texel.a ) continue;

        }

    }
}

bool
World::platform_collision_possible(pCoor pos)
{
  // Assuming no motion in x or z axes.
  //
  return pos.x >= platform_xmin && pos.x <= platform_xmax
    && pos.z >= platform_zmin && pos.z <= platform_zmax;
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
void Ball::stop() {velocity = pVect(0,0,0); }

// Set the velocity and rotation (not yet supported) to zero.
//
void Ball::freeze() {velocity = pVect(0,0,0); }



void World::balls_translate(pVect amt,int b){head_ball->translate(amt);}
void World::balls_push(pVect amt,int b){head_ball->push(amt);}
void World::balls_translate(pVect amt)
{ for ( Ball* const ball: balls ) ball->translate(amt); }
void World::balls_push(pVect amt)
{ for ( Ball* const ball: balls ) ball->push(amt); }
void World::balls_stop()
{ for ( Ball* const ball: balls ) ball->stop(); }
void World::balls_freeze(){balls_stop();}
void World::render_my_piece() {mp.render();}


void
World::frame_callback()
{
  // This routine called whenever window needs to be updated.

  const double time_now = time_wall_fp();

  if ( !opt_pause || opt_single_frame || opt_single_time_step )
    {
      /// Advance simulation state.

      // Amount of time since the user saw the last frame.
      //
      const double wall_delta_t = time_now - last_frame_wall_time;

      // Compute amount by which to advance simulation state for this frame.
      //
      const double duration =
        opt_single_time_step ? opt_time_step_duration :
        opt_single_frame ? 1/30.0 :
        wall_delta_t;

      const double world_time_target = world_time + duration;

      while ( world_time < world_time_target )
        {
          time_step_cpu(opt_time_step_duration);
          world_time += opt_time_step_duration;
        }

      // Reset these, just in case they were set.
      //
      opt_single_frame = opt_single_time_step = false;
    }

  last_frame_wall_time = time_now;

  if ( opt_ride && ball_eye )
    {
      pNorm b_eye_down(ball_eye->position,ball_down->position);
      pVect b_eye_up = -b_eye_down;
      pCoor eye_pos = ball_eye->position + 2.2 * ball_eye->radius * b_eye_up;
      pNorm b_eye_direction(eye_pos,ball_gaze->position);

      pNorm b_eye_left = cross(b_eye_direction,b_eye_up);
      pMatrix_Translate center_eye(-eye_pos);
      pMatrix rotate; rotate.set_identity();
      for ( int i=0; i<3; i++ ) rotate.rc(0,i) = b_eye_left.elt(i);
      for ( int i=0; i<3; i++ ) rotate.rc(1,i) = b_eye_up.elt(i);
      for ( int i=0; i<3; i++ ) rotate.rc(2,i) = -b_eye_direction.elt(i);
      modelview = rotate * center_eye;
      pMatrix reflect; reflect.set_identity(); reflect.rc(1,1) = -1;
      transform_mirror = modelview * reflect * invert(modelview);
    }

  render();
}

int
main(int argv, char **argc)
{
  pOpenGL_Helper popengl_helper(argv,argc);
  World world(popengl_helper);

  popengl_helper.rate_set(30);
  popengl_helper.display_cb_set(world.frame_callback_w,&world);
}
