#include "Shape.h"
#include <iostream>
#include <cassert>

#include "GLSL.h"
#include "Program.h"

using namespace std;
using namespace glm;

typedef struct Vertex
{
   double x;
   double y;
   double z;
}Vertex;

typedef struct Triangle
{
   Vertex v1;
   Vertex v2;
   Vertex v3;
}Triangle;


// copy the data from the shape to this object
void Shape::createShape(tinyobj::shape_t & shape)
{
   posBuf = shape.mesh.positions;
   norBuf = shape.mesh.normals;
   texBuf = shape.mesh.texcoords;
   eleBuf = shape.mesh.indices;
}

void Shape::measure()
{
   float minX, minY, minZ;
   float maxX, maxY, maxZ;

   /*minX = minY = minZ = std::numeric_limits<float>::max();
   maxX = maxY = maxZ = -std::numeric_limits<float>::max();*/
   minX = minY = minZ = 1.1754E+38F;
   maxX = maxY = maxZ = -1.1754E+38F;

   //Go through all vertices to determine min and max of each dimension
   for (size_t v = 0; v < posBuf.size() / 3; v++)
   {
      if (posBuf[3*v+0] < minX) minX = posBuf[3 * v + 0];
      if (posBuf[3*v+0] > maxX) maxX = posBuf[3 * v + 0];

      if (posBuf[3*v+1] < minY) minY = posBuf[3 * v + 1];
      if (posBuf[3*v+1] > maxY) maxY = posBuf[3 * v + 1];

      if (posBuf[3*v+2] < minZ) minZ = posBuf[3 * v + 2];
      if (posBuf[3*v+2] > maxZ) maxZ = posBuf[3 * v + 2];
   }

   min.x = minX;
   min.y = minY;
   min.z = minZ;
   max.x = maxX;
   max.y = maxY;
   max.z = maxZ;
}

// Fill in the fields of a Triangle object
void makeTriangle(int i, Triangle* tri, vector<unsigned int>& triBuf, vector<float>& posBuf)
{
   Vertex v1, v2, v3;
   
   // Vertex 1
   v1.x = posBuf[(triBuf[i]) * 3];
   v1.y = posBuf[(triBuf[i]) * 3 + 1];
   v1.z = posBuf[(triBuf[i]) * 3 + 2];

   // Vertex 2
   v2.x = posBuf[(triBuf[i + 1]) * 3];
   v2.y = posBuf[(triBuf[i + 1]) * 3 + 1];
   v2.z = posBuf[(triBuf[i + 1]) * 3 + 2];

   // Vertex 3
   v3.x = posBuf[(triBuf[i + 2]) * 3];
   v3.y = posBuf[(triBuf[i + 2]) * 3 + 1];
   v3.z = posBuf[(triBuf[i + 2]) * 3 + 2];

   tri->v1 = v1;
   tri->v2 = v2;
   tri->v3 = v3;
}

void Shape::calcNormals()
{
   // Initialize normals
   for(int i = 0; i < posBuf.size(); i ++)
      norBuf.push_back(0);
   
   Triangle tri;
   vec3 v1, v2, out;
   for (int i = 0; i < eleBuf.size(); i += 3)
   {
      makeTriangle(i, &tri, eleBuf, posBuf);

      v1.x = tri.v2.x - tri.v1.x;
      v1.y = tri.v2.y - tri.v1.y;
      v1.z = tri.v2.z - tri.v1.z;

      v2.x = tri.v3.x - tri.v1.x;
      v2.y = tri.v3.y - tri.v1.y;
      v2.z = tri.v3.z - tri.v1.z;

      out = cross(v1, v2);

      // Vertex 1's normals
      norBuf[(eleBuf[i]) * 3] += out.x;
      norBuf[(eleBuf[i]) * 3 + 1] += out.y;
      norBuf[(eleBuf[i]) * 3 + 2] += out.z;

      // Vertex 2's normals
      norBuf[(eleBuf[i + 1]) * 3] += out.x;
      norBuf[(eleBuf[i + 1]) * 3 + 1] += out.y;
      norBuf[(eleBuf[i + 1]) * 3 + 2] += out.z;

      // Vertex 3's normals
      norBuf[(eleBuf[i + 2]) * 3] += out.x;
      norBuf[(eleBuf[i + 2]) * 3 + 1] += out.y;
      norBuf[(eleBuf[i + 2]) * 3 + 2] += out.z;
   }
   for (int i = 0; i < norBuf.size(); i += 3)
   {
      vec3 normal = vec3(norBuf[i], norBuf[i + 1], norBuf[i + 2]);
      vec3 N = normalize(normal);
      norBuf[i] = N.x;
      norBuf[i + 1] = N.y;
      norBuf[i + 2] = N.z;
   }
}

void Shape::init()
{
   // Initialize the vertex array object
   CHECKED_GL_CALL(glGenVertexArrays(1, &vaoID));
   CHECKED_GL_CALL(glBindVertexArray(vaoID));

   // Send the position array to the GPU
   CHECKED_GL_CALL(glGenBuffers(1, &posBufID));
   CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, posBufID));
   CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW));

   // Send the normal array to the GPU
   if (norBuf.empty())
   {
      norBufID = 0;
   }
   else {
      CHECKED_GL_CALL(glGenBuffers(1, &norBufID));
      CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, norBufID));
      CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW));
   }

   // Send the texture array to the GPU
   if (texBuf.empty())
   {
      texBufID = 0;
   }
   else
   {
      CHECKED_GL_CALL(glGenBuffers(1, &texBufID));
      CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, texBufID));
      CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW));
   }

   // Send the element array to the GPU
   CHECKED_GL_CALL(glGenBuffers(1, &eleBufID));
   CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID));
   CHECKED_GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, eleBuf.size()*sizeof(unsigned int), &eleBuf[0], GL_STATIC_DRAW));

   // Unbind the arrays
   CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
   CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void Shape::draw(const shared_ptr<Program> prog) const
{
   int h_pos, h_nor, h_tex;
   h_pos = h_nor = h_tex = -1;

   CHECKED_GL_CALL(glBindVertexArray(vaoID));

   // Bind position buffer
   h_pos = prog->getAttribute("vertPos");
   GLSL::enableVertexAttribArray(h_pos);
   CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, posBufID));
   CHECKED_GL_CALL(glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0));

   if (norBufID != 0)
   {
      // Bind normal buffer
      h_nor = prog->getAttribute("vertNor");
      if(h_nor != -1 && norBufID != 0) {
         GLSL::enableVertexAttribArray(h_nor);
         glBindBuffer(GL_ARRAY_BUFFER, norBufID);
         glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
      }
   }

   if (texBufID != 0)
   {
      // Bind texcoords buffer
      h_tex = prog->getAttribute("vertTex");

      if (h_tex != -1 && texBufID != 0)
      {
         GLSL::enableVertexAttribArray(h_tex);
         CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, texBufID));
         CHECKED_GL_CALL(glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0));
      }
   }

   // Bind element buffer
   CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID));

   // Draw
   CHECKED_GL_CALL(glDrawElements(GL_TRIANGLES, (int)eleBuf.size(), GL_UNSIGNED_INT, (const void *)0));

   // Disable and unbind
   if (h_tex != -1)
   {
      GLSL::disableVertexAttribArray(h_tex);
   }
   if (h_nor != -1)
   {
      GLSL::disableVertexAttribArray(h_nor);
   }
   GLSL::disableVertexAttribArray(h_pos);
   CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
   CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}
