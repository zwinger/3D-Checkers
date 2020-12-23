#version 330 core
uniform sampler2D Texture0;

in vec2 vTexCoord;
in float diffuse;
out vec4 Outcolor;

void main() {

	vec4 texColor0 =  texture(Texture0, vTexCoord);

	Outcolor = vec4(0.1, 0.1, 0.1, 1.0) + diffuse * texColor0;
}

