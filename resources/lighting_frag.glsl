#version 330 core
uniform vec3 MatDif;
uniform vec3 MatAmb;
uniform vec3 MatSpec;
uniform float shine;
in vec3 normal;
in vec3 lightDir;
in vec3 view;
out vec4 color;

void main()
{
   vec3 N = normalize(normal);
   vec3 L = normalize(lightDir);
   vec3 V = normalize(view);
   vec3 H = normalize((L + V) / (length(L + V)));

   vec3 ambient = MatAmb;
   vec3 diffuse = (MatDif * max(0, dot(N, L)));
   vec3 specular = (MatSpec * pow(max(0, dot(N, H)), shine));
   vec3 lColor = ambient + diffuse + specular;

   color = vec4(lColor, 1.0); 
}
