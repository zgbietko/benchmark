#version 330

layout(std140) uniform;

layout (location = 0) in vec4 inPos;

out vec4 color;

uniform Projection
{
  mat4 mv;
  mat4 vp; 
};

const int numberOfBreaks = 32;
uniform IsoValues
{
	vec4 wireframe_col;
	vec4 border_col;
	vec4 iso_col;
	vec4 iso_values[numberOfBreaks];
	int num_breaks;
};

void main(void) {                              
	vec4 temp = mv * vec4(inPos.xyz, 1.0);
	gl_Position = vp * temp;
	color = wireframe_col;
}
 

