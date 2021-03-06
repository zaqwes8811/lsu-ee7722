/// LSU EE 4702-1 (Fall 2018), GPU Programming
//

 /// Pre-Final Problem 1 -- SOLUTION
 //
 //  Search for SOLUTION in this file to find solution code.

 /// Instructions
 //
 //  Read the assignment: https://www.ece.lsu.edu/koppel/gpup/2018/mt.pdf
 //
 //  Modify code and declarations throughout this file.
 //


// Specify version of OpenGL Shading Language.
//
#version 450 compatibility

vec4 generic_lighting
(vec4 vertex_e, vec4 color, vec3 normal_e, bool front_facing);


uniform sampler2D tex_unit_0;


// Use these variables to debug your code. Press 'y' to toggle
// tryout.x and 'Y' to toggle debug_bool.y (between true and false).
//
layout ( location = 3 ) uniform bvec2 tryout;
layout ( location = 4 ) uniform float tryoutf;


layout ( location = 5 ) uniform vec3 spiral_normal;
layout ( location = 6 ) uniform float tex_ht;


///
/// Shader Input and Output Variables
///


// Declare variables for communication between vertex shader
// and fragment shader.
//
#ifdef _VERTEX_SHADER_

out Data
{
  vec3 normal_e;
  vec4 vertex_e;
  vec2 tex_coord;
};

#endif

#ifdef _FRAGMENT_SHADER_
in Data
{
 //  Use flat interpolation so that one normal is used for the entire
 //  primitive.
 flat vec3 normal_e;

 vec4 vertex_e;
 vec2 tex_coord;
};

#endif


///
/// Shaders
///

#ifdef _VERTEX_SHADER_

void
vs_main()
{
  // Perform basic vertex shading operations.

  // Transform vertex to clip space.
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

  // Compute eye-space vertex coordinate and normal.
  // These are outputs of the vertex shader and inputs to the frag shader.
  //
  vertex_e = gl_ModelViewMatrix * gl_Vertex;
  normal_e = normalize(gl_NormalMatrix * gl_Normal);

  // Copy texture coordinate to output (no need to modify it).
  tex_coord = gl_MultiTexCoord0.xy;
}

void vs_main_plain(){ vs_main(); }
void vs_main_hw03(){ vs_main(); }


#endif


#ifdef _FRAGMENT_SHADER_

void
fs_main_hw03()
{
  /// Pre-Final Problem 1 SOLUTION

  int ncol = 6;     // Number of pieces a line is split into.
  int nlines = 60;  // Number of lines per page.

  float s_tex_coord_x = tex_coord.x * tex_ht * nlines;
  float s_line_num = floor(s_tex_coord_x);
  float s_line_offs_01 = fract(s_tex_coord_x);
  float s_line_offs_cols = s_line_offs_01 * ncol;
  float s_col_num = floor(s_line_offs_cols);
  float s_col_offs = fract(s_line_offs_cols);
  float tex_x = ( s_col_num + tex_coord.y ) / ncol;
  float tex_y = ( s_line_num + s_col_offs ) / nlines;

  vec2 tc = vec2(tex_x,tex_y);

  vec4 color =
    tex_x < 0.01 ? vec4(0.1,0.1,0.1,1) :
    s_col_offs < 0.03 || s_col_offs > 0.97 ? vec4(1,0,0,1) :
    gl_FrontFacing
    ? gl_FrontMaterial.diffuse : gl_BackMaterial.diffuse;


  // No need to modify code below.
  vec4 texel = texture(tex_unit_0,tc);
  vec3 nne = normalize(normal_e);
  float edge_dist = tex_coord.y;
  vec3 bnorm = tryout.x ? nne : edge_dist > 0.9
    ? normalize(mix(nne,spiral_normal,2*(edge_dist-0.9))) : edge_dist < 0.1
    ? normalize(mix(-spiral_normal,nne,0.8+2*edge_dist)) : nne;
  vec4 lighted_color =
    generic_lighting( vertex_e, color, bnorm, gl_FrontFacing );
  gl_FragColor = texel * lighted_color;
  gl_FragDepth = gl_FragCoord.z;
}

void
fs_main_plain()
{
  // This file contains code from the Homework 3 Problem 2 solution that
  // was originally in a shader named fs_main_hw03.

  /// Homework 3:  Can put solution here, and other places.

  /// SOLUTION -- Problem 2b
  //
  //  The value of tex_coord.x indicates distance along the triangular
  //  spiral. The value at the beginning of the spiral is zero and it
  //  increases based on the length of the spiral segments. This value
  //  is passed unmodified to the texture library call below (see the
  //  line starting "vec4 texel"). On the host (CPU) code the texture
  //  object for the image has been set to wrap along the x (called s
  //  in OpenGL) dimension, and so a tex_coord.x value of 1.1 is
  //  equivalent to a value of 0.1.
  //
  //  The value of tex_coord.y varies from 0 to 1, a 0 indicates that
  //  the fragment is on the back edge of the segment, 0.5 indicates
  //  the middle, and so on. If tex_coord.y were used in the call to
  //  texture, below, then a segment, rather than showing about two
  //  lines of text, would show all lines (the entire vertical length)
  //  of a page. The actual value used in the call to texture, tc.y,
  //  is computed below. The computation uses line_num, which is the
  //  integer portion of tex_coord.x. The idea is that if tex_coord.x
  //  is, say, 0.1, then we should be near the top of the image, lets
  //  call that the first line. If tex_coord.x is 1.1 then we have
  //  gone off the right edge (since tex_coord.x > 1 ) and so we
  //  advance down in the y direction to the second line. Variable
  //  line_num is the number of lines down. Basically, we are adding
  //  the integer part of tex_coord.x to tex_coord.y, then scaling it
  //  by tex_ht. (Recall that tex_ht is the fraction of a page covered
  //  by one "line". If tex_ht = 0.1, that means that the text applied
  //  to a segment is 1/10 the height of the texture image.)

  float line_num = floor(tex_coord.x);
  vec2 tc = vec2( tex_coord.x, ( line_num + tex_coord.y ) * tex_ht );

  // Get filtered texel.
  //
  /// SOLUTION -- Problem 2
  //
  //  Use texture coordinate computed above in which y is advanced by
  //  the number of lines.
  //
  vec4 texel = texture(tex_unit_0,tc);

  vec3 nne = normalize(normal_e);

  // Homework 3: This was a placeholder value. It has been changed
  // to something based on fragment's position within primitive.
  //
  /// SOLUTION - Compute edge distance from texture y coordinate.
  //
  //  Use tex_coord.y unmodified.
  //
  float edge_dist = tex_coord.y;
  //
  // Range [0,1].  0.5, center of segment; 0, back edge; 1, front edge.

  // Homework 3: This was a placeholder value. It has been commented
  // out since spiral_normal has been declared as a uniform.
  //
  /// SOLUTION - Problem 3
  //  Use spiral normal value sent as a uniform.
  //  vec3 spiral_normal = vec3(1,0,0);

  // Homework 3: Blend incoming normal with triangular spiral's normal.
  vec3 bnorm =
    tryout.x
    ? nne
    : edge_dist > 0.9
    ? normalize(mix(nne,spiral_normal,2*(edge_dist-0.9)))
    : edge_dist < 0.1
    ? normalize(mix(-spiral_normal,nne,0.8+2*edge_dist))
    : nne;

  /// SOLUTION -- Problem 1b
  //  Use material property uniforms instead of color value.
  //
  vec4 color = gl_FrontFacing
    ? gl_FrontMaterial.diffuse : gl_BackMaterial.diffuse;

  // Compute lighted color of fragment.
  //
  /// SOLUTION - Problem 1 and 3
  //
  //  Use color and bnorm computed above.
  //
  vec4 lighted_color =
    generic_lighting( vertex_e, color, bnorm, gl_FrontFacing );

  // Combine filtered texel color with lighted color of fragment.
  //
  gl_FragColor = texel * lighted_color;

  // Copy fragment depth unmodified.
  //
  gl_FragDepth = gl_FragCoord.z;
}

#endif


vec4
generic_lighting(vec4 vertex_e, vec4 color, vec3 normal_e, bool front_facing)
{
  // Return lighted color of vertex_e.
  //
  vec4 light_pos = gl_LightSource[0].position;
  vec3 v_vtx_light = light_pos.xyz - vertex_e.xyz;
  float dist = length(v_vtx_light);
  float d_n_vl = dot(normalize(normal_e), v_vtx_light) / dist;
  float phase_light = max(0, front_facing ? d_n_vl : -d_n_vl );

  vec3 ambient_light = gl_LightSource[0].ambient.rgb;
  vec3 diffuse_light = gl_LightSource[0].diffuse.rgb;
  float distsq = dist * dist;
  float atten_inv =
    0 * gl_LightSource[0].constantAttenuation +
    gl_LightSource[0].linearAttenuation * dist +
    gl_LightSource[0].quadraticAttenuation * distsq;
  vec4 lighted_color;
  lighted_color.rgb =
    tryoutf * color.rgb * gl_LightModel.ambient.rgb
    + color.rgb
    * ( tryoutf * ambient_light + phase_light * diffuse_light ) / atten_inv;
  lighted_color.a = color.a;
  return lighted_color;
}
