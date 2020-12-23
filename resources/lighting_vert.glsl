#version  330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightPos;
uniform vec3 eye;
out vec3 normal;
out vec3 lightDir;
out vec3 lightDir2;
out vec3 view;

void main()
{
   gl_Position = P * V * M * vertPos;
   normal = (M * vec4(vertNor, 0.0)).xyz;
   lightDir = (lightPos - (M * vertPos).xyz);
   view = eye - (M * vertPos).xyz;
}
