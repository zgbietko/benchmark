#version 330

layout (std140) uniform; 
// xyz - position; w - flag
layout (location = 0) in vec4 inPosition;
// x - value; yzw - color
layout (location = 1) in vec4 inDiffuseColor;

out VertexData {
  vec4 color;
  vec4 cs_position;
  float flag;
} vert;

uniform Projection
{
  mat4 mv;
  mat4 vp; 
};

void main(void) {   
    // Calculate camera space position and store
	vert.cs_position = mv * vec4(inPosition.xyz, 1.0f);
	// Calculate clip position
	gl_Position = vp * vert.cs_position;                           
	// Store color with assigned scalar value
	vert.color = inDiffuseColor;	
	// Store edge flag
	vert.flag = inPosition.w;
}
