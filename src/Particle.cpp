//
// sueda - geometry edits Z. Wood
// 3/16
//

#include <iostream>
#include "Particle.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Texture.h"


float randFloat(float l, float h)
{
	float r = rand() / (float) RAND_MAX;
	return (1.0f - r) * l + r * h;
}

void Particle::load(float time, vec3 position, vec3 color)
{
	// Random initialization
	rebirth(time, position, color);
}

// all particles born at the origin
void Particle::rebirth(float t, vec3 position, vec3 particleColor)
{
	charge = randFloat(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;
	m = 1.0f;
	d = randFloat(0.0f, 0.02f);
	x.x = position.x;
	x.y = position.y;
	x.z = position.z;
   v.x = randFloat(-10.0f, 10.0f);
	v.y = randFloat(5.0f, 6.0f);
	v.z = randFloat(-10.0f, 10.0f);
	lifespan = randFloat(2.0f, 4.0f);
	tEnd = t + lifespan;

	scale = randFloat(1.0f, 2.0f);
   color.r = particleColor.x;
   color.g = particleColor.y;
   color.b = particleColor.z;
	color.a = 1.0f;
}

void Particle::update(float t, float h, const vec3 &g, const bool *keyToggles)
{
   //gravity
	float accel = -0.2;
	v.y += accel;

	// very simple update
	x += h * v;
	color.a = (tEnd - t) / lifespan;
}
