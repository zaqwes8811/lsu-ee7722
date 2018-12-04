/// LSU EE 4702-1 (Fall 2018), GPU Programming
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


layout ( location = 5 ) uniform int opt_n_segs;
layout ( location = 6 ) uniform float tex_ht;

layout ( binding = 1 ) buffer bctr { vec4 ctr_a[]; };
layout ( binding = 2 ) buffer bva { vec4 va_a[][3]; };
layout ( binding = 3 ) buffer bdistv { vec4 distv_a[]; };
layout ( binding = 4 ) buffer bvz { vec4 vz_a[]; };


float
total_len_compute(int j, float delta_a, vec4 distv)
{
  int j0 = (j+2)/3, j1 = (j+1)/3, j2 = (j)/3;
  float len_sum = distv.x * delta_a * (3.0 * j0 * j0 / 2 ) +
    distv.y * delta_a * (j1 + 3.0 * j1 * j1  / 2 ) +
    distv.z * delta_a * (2*j2 + 3.0 * j2 * j2 / 2);
  return len_sum;
}

// Declare variables for communication between vertex shader
// and fragment shader.
//
#ifdef _VERTEX_SHADER_

out Data
{
 int ins_id;
 int vtx_id;

 vec3 p;
 float tex_x;
};

#endif

#ifdef _GEOMETRY_SHADER_

in Data
{
 int ins_id;
 int vtx_id;

 vec3 p;
 float tex_x;
} In[2];

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
vs_main_lines()
{
  ins_id = gl_InstanceID;  // Corresponds to i on host code.
  vtx_id = gl_VertexID;    // Corresponds to j on host code.

  /// SOLUTION

  int j = gl_VertexID;
  const float delta_a = 0.6 / opt_n_segs;
  float a = j * delta_a;
  vec3 v = va_a[ins_id][j%3].xyz;
  p = ctr_a[ins_id].xyz + a * v;

  vec4 distv = distv_a[ins_id];
  const float tex_scale = 0.2;

  tex_x = total_len_compute(j,delta_a,distv) * tex_scale;

}

#endif


#ifdef _GEOMETRY_SHADER_

// Type of primitive at geometry shader input.
//
layout ( lines ) in;

// Type of primitives emitted geometry shader output.
//
layout ( triangle_strip, max_vertices = 4 ) out;

void
gs_main_lines()
{
  int ins_id = In[0].ins_id;
  vec3 vz = vz_a[ins_id].xyz;
  vec3 normal_o = cross(In[1].p-In[0].p,vz);
  normal_e = gl_NormalMatrix * normal_o;

  for ( int i=0; i<2; i++ )
    {
      for ( int j=0; j<2; j++ )
        {
          tex_coord = vec2(In[i].tex_x,j);
          vec4 vertex_o = vec4( In[i].p + ( j == 0 ? vz : -vz ), 1 );
          gl_Position = gl_ModelViewProjectionMatrix * vertex_o;
          vertex_e = gl_ModelViewMatrix * vertex_o;
          EmitVertex();
        }
      }
  EndPrimitive();
}

#endif

#ifdef _FRAGMENT_SHADER_
void
fs_main_lines()
{
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

  vec3 bnorm = nne;

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

void
fs_main_plain()
{
  /// HOMEWORK 3: DO NOT PLACE SOLUTION HERE

  // Get filtered texel.
  //
  vec4 texel = texture(tex_unit_0,tex_coord);

  // Compute lighted color of fragment.
  //
  vec4 lighted_color =
    generic_lighting( vertex_e, gl_Color, normalize(normal_e), gl_FrontFacing );

  // Combine filtered texel color with lighted color of fragment.
  //
  gl_FragColor = texel * lighted_color;

  // Copy fragment depth unmodified.
  //
  gl_FragDepth = gl_FragCoord.z;

  /// HOMEWORK 3: DO NOT PLACE SOLUTION HERE
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
