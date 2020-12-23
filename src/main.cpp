/*
 * Program 3 base code - includes modifications to shape and initGeom in preparation to load
 * multi shape objects 
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn
 */

#include <iostream>
#include <algorithm>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "Texture.h"
#include "stb_image.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Particle.h"

#define _USE_MATH_DEFINES
#include <math.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

typedef struct BoardTile
{
   vec3 position;
   int color;
   vec3 scale;
   int originalColor;
}BoardTile;

typedef struct GamePiece
{
   int king;
   vec3 position;
   vec3 targetPosition;
   int color;
   int playersPiece;
   float direction;
   vec3 velocity;
   float radius;

   // Animation data
   float upperArmTheta; // armUpper arm theta
   float forearmTheta; // forearm theta
   float handTheta; // hand theta

   float leftUpperLegTheta; // upper leg theta
   float leftLowerLegTheta; // lower leg theta
   float leftHoofTheta; // hoof theta

   float rightUpperLegTheta; // upper leg theta
   float rightLowerLegTheta; // lower leg theta
   float rightHoofTheta; // hoof theta

   int armUp;
   int leftLegUp;
   int rightLegUp;
}GamePiece;


class Application : public EventCallbacks
{

public:

   WindowManager * windowManager = nullptr;

   // Our shader program
   std::shared_ptr<Program> prog;
   std::shared_ptr<Program> texProg;
   std::shared_ptr<Program> cubeProg;
   std::shared_ptr<Program> particleProg;

   // Textures
   shared_ptr<Texture> texture0;
   shared_ptr<Texture> texture1; 
   shared_ptr<Texture> texture2;
   shared_ptr<Texture> texture3;
   shared_ptr<Texture> particleTexture;

   // Shape to be used (from  file) - modify to support multiple
   shared_ptr<Shape> cube;
   shared_ptr<Shape> ground;
   shared_ptr<Shape> bench;
   shared_ptr<Shape> sphere;
   shared_ptr<Shape> dog;
   shared_ptr<Shape> skybox;
   vector<shared_ptr<Shape>> rocks;
   vector<shared_ptr<Shape>> tree;
   vector<shared_ptr<Shape>> dummy;

   //Particle data
   std::vector<std::shared_ptr<Particle>> particles;

   // CPU array for particles - redundant with particle structure
   // but simple
   int numP = 100;
   GLfloat points[300];
   GLfloat pointColors[400];

   GLuint pointsbuffer;
   GLuint colorbuffer;   

   int gMat = 0;

   // Display time to control fps
   float t0_disp = 0.0f;
   float t_disp = 0.0f;

   bool keyToggles[256] = { false };
   float t = 0.0f; //reset in init
   float h = 0.01f;
   glm::vec3 g = glm::vec3(0.0f, -0.01f, 0.0f);
   int spawnParticles = 0;
   vec3 particlePos;

   // Contains vertex information for OpenGL
   GLuint VertexArrayID;

   //example data that might be useful when trying to compute bounds on multi-shape
   vec3 cubeMin;
   vec3 cubeMax;
   vec3 groundMin;
   vec3 groundMax;
   vec3 dogMin;
   vec3 dogMax;
   vec3 rocksMin;
   vec3 rocksMax;
   vec3 treeMin;
   vec3 treeMax;
   vec3 benchMin;
   vec3 benchMax;
   vec3 dummyMin;
   vec3 dummyMax;
   vec3 sphereMin;
   vec3 sphereMax;

   // Material Data
   enum Material {shinyBlue, flatGrey, brass, darkGreen, brown, black, white, emerald, ruby};
   int numMaterials = 9;
   int rockMaterial = flatGrey;
   int fanMaterial1 = emerald;
   int fanMaterial2 = ruby;
   int treeTopMaterial = darkGreen;
   int treeTrunkMaterial = brown;
   int riderMaterial = brass;
   // Player material used for players
   int player1Material = emerald;
   int player2Material = ruby;
   int boardBlackSquare = black;
   int boardWhiteSquare = white;

   //animation data
   float armIncrease = 0.01;
   float legIncrease = 0.01;

   float fanUpperArmTheta = 0; // armUpper arm theta
   float fanForearmTheta = M_PI / 4; // forearm theta
   float fanHandTheta = M_PI / 8; // hand theta
   int fanArmUp = 1;

   float cubeWidthX;
   float cubeWidthZ;
   float cubeWidthY;
   float distX;
   float distZ;
   float distY;
   int boardScale;
   float cubeSize;

   vec3 eye;
   vec3 lookAtPoint;
   vec3 up = vec3(0, 1, 0);
   vec3 u, v, w, gaze;
   unsigned int cubeMapTexture;
   float cameraTheta = M_PI / 2.0;
   float cameraPhi = M_PI / 4.0;
   // Start camera looking down negative z-axis
   float theta = -M_PI / 2.0;
   float phi = -M_PI / 4.0;
   int cameraThetaAtLocation;
   int cameraPhiAtLocation;
   int thetaAtLocation;
   int phiAtLocation;

   float targetCameraTheta;
   float targetCameraPhi;
   float targetTheta;
   float targetPhi;
   float deltaCameraTheta;
   float deltaTheta;
   float deltaCameraPhi;
   float deltaPhi;

   int changePlayerCamera = 0;

   shared_ptr<GamePiece> selectedPiece = NULL;
   int movingPiece = 0;

   // Skybox faces
   vector<std::string> faces {
      "/mp_organic/organic_rt.tga",
      "/mp_organic/organic_lf.tga",
      "/mp_organic/organic_up.tga",
      "/mp_organic/organic_dn.tga",
      "/mp_organic/organic_bk.tga",
      "/mp_organic/organic_ft.tga"
   };

   float deltaX;
   float deltaY;
   double prevX = -1;
   double prevY = -1;
   

   float cameraAngleOffset = 2.0;

   vector<shared_ptr<BoardTile>> boardTiles;
   int numTiles;

   vector<shared_ptr<GamePiece>> player1Pieces;
   vector<shared_ptr<GamePiece>> player2Pieces;

   shared_ptr<BoardTile> curTile = make_shared<BoardTile>();
   shared_ptr<BoardTile> selectedTile = NULL;

   int selectingPiece = 1;
   int playerTurn = 1;
   float speed = 0.10;


   void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
   {
      if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
      {
         glfwSetWindowShouldClose(window, GL_TRUE);
      }

      if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
         glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
      }

      if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
         glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
      }

      if (key == GLFW_KEY_A) 
      {
         if (!changePlayerCamera)
         {
            cameraTheta += radians(cameraAngleOffset);
            theta += radians(cameraAngleOffset);
            calcCameraComponents();
         }
      }
      if (key == GLFW_KEY_D) 
      {
         if (!changePlayerCamera)
         {
            cameraTheta -= radians(cameraAngleOffset);
            theta -= radians(cameraAngleOffset);
            calcCameraComponents();
         }
      }
      if (key == GLFW_KEY_W) 
      {
         if (!changePlayerCamera)
         {
            cameraPhi += radians(cameraAngleOffset);
            phi -= radians(cameraAngleOffset);
            calcCameraComponents();
         }
      }
      if (key == GLFW_KEY_S) 
      {
         if (!changePlayerCamera)
         {
            cameraPhi -= radians(cameraAngleOffset);
            phi += radians(cameraAngleOffset);
            calcCameraComponents();
         }      
      }

      // Make selected piece a king
      if (key == GLFW_KEY_K && action == GLFW_PRESS)
      {
         shared_ptr<GamePiece> piece = findGamePiece(curTile, playerTurn);
         if (piece != NULL)
            piece->king = 1 - piece->king;
      }

      if (key == GLFW_KEY_UP && action == GLFW_PRESS && !changePlayerCamera)
      {
         colorTileOriginalColor(curTile);
         if (playerTurn == 1)
         {
            curTile->position.z -= cubeSize;
            
            if (curTile->position.z <= ((-4 * cubeSize) - (cubeSize / 2.0)))
               curTile->position.z = ((-3 * cubeSize) - (cubeSize / 2.0));
         }

         else
         {
            curTile->position.z += cubeSize;
            
            if (curTile->position.z >= ((4 * cubeSize) + (cubeSize / 2.0)))
               curTile->position.z = ((3 * cubeSize) + (cubeSize / 2.0));
         }
         colorTile(curTile);
      }

      if (key == GLFW_KEY_DOWN && action == GLFW_PRESS && !changePlayerCamera)
      {
         colorTileOriginalColor(curTile);
         if (playerTurn == 1)
         {
            curTile->position.z += cubeSize;

            if (curTile->position.z >= ((4 * cubeSize) + (cubeSize / 2.0)))
               curTile->position.z = ((3 * cubeSize) + (cubeSize / 2.0));            
         }

         else
         {
            curTile->position.z -= cubeSize;

            if (curTile->position.z <= ((-4 * cubeSize) - (cubeSize / 2.0)))
               curTile->position.z = ((-3 * cubeSize) - (cubeSize / 2.0));  
         }
         colorTile(curTile);
      }

      if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS && !changePlayerCamera)
      {
         colorTileOriginalColor(curTile);

         if (playerTurn == 1)
         {
            curTile->position.x += cubeSize;

            if (curTile->position.x >= ((4 * cubeSize) + (cubeSize / 2.0)))
               curTile->position.x = ((3 * cubeSize) + (cubeSize / 2.0));
         }

         else
         {
            curTile->position.x -= cubeSize;

            if (curTile->position.x <= ((-4 * cubeSize) - (cubeSize / 2.0)))
               curTile->position.x = ((-3 * cubeSize) - (cubeSize / 2.0));
         }
         colorTile(curTile);
      }

      if (key == GLFW_KEY_LEFT && action == GLFW_PRESS && !changePlayerCamera)
      {
         colorTileOriginalColor(curTile);

         if (playerTurn == 1)
         {
            curTile->position.x -= cubeSize;

            if (curTile->position.x <= ((-4 * cubeSize) - (cubeSize / 2.0)))
               curTile->position.x = ((-3 * cubeSize) - (cubeSize / 2.0));
         }

         else
         {
            curTile->position.x += cubeSize;

            if (curTile->position.x >= ((4 * cubeSize) + (cubeSize / 2.0)))
               curTile->position.x = ((3 * cubeSize) + (cubeSize / 2.0));  
         }
         colorTile(curTile);
      }

      // Handle tile selection
      if (key == GLFW_KEY_ENTER && action == GLFW_PRESS && !changePlayerCamera)
      {
         //Select piece to move
         if (selectingPiece && tileHasPlayerPiece(playerTurn, curTile))
         {
            selectedPiece = findGamePiece(curTile, playerTurn);
            if (hasPlaceToMove(selectedPiece, 3 - playerTurn))
            {
               selectedTile = make_shared<BoardTile>();
               selectedTile->position = vec3(curTile->position.x, curTile->position.y, curTile->position.z);
               selectedTile->color = emerald;
               selectingPiece = 0;
            }
         }

         // Select tile to move piece to
         else if (!selectingPiece && curTile->color == emerald && !tileHasPiece(curTile))
         {
            colorTileOriginalColor(selectedTile);
            colorTileOriginalColor(curTile);
            setPieceTarget(selectedPiece, curTile);

            /*if (playerTurn == 1)
            {
               if (player1Pieces.size() > 0)
               {
                  curTile->position.x = player2Pieces[0]->position.x;
                  curTile->position.z = player2Pieces[0]->position.z;
               }
            }
            else
            {
               if (player2Pieces.size() > 0)
               {
                  curTile->position.x = player1Pieces[0]->position.x;
                  curTile->position.z = player1Pieces[0]->position.z;
               }
            }*/

            selectedTile = NULL;
            selectingPiece = 1;
         }
      }

      // Always color selected tile green
      if (selectedTile != NULL)
      {
         colorTileGreen(selectedTile);
      }

      // Reset the world to the origin
      if (key == GLFW_KEY_R)
      {
         cameraTheta = M_PI / 2.0;
         cameraPhi = M_PI / 4.0;
         // Start camera looking down negative z-axis
         theta = -M_PI / 2.0;
         phi = -M_PI / 4.0;

         cameraThetaAtLocation = 0;
         cameraPhiAtLocation = 0;
         thetaAtLocation = 0;
         phiAtLocation = 0;

         changePlayerCamera = 0;

         calcCameraComponents();

         // Reset game
         playerTurn = 1;
         changePlayerCamera = 0;
         selectedTile = NULL;
         selectedPiece = NULL;
         movingPiece = 0;
         spawnParticles = 0;
         player1Pieces.resize(0);
         player2Pieces.resize(0);
         colorTileOriginalColor(curTile);

         // Initialize Player pieces;
         initializeGamePieces(player1Pieces);
         initializeGamePieces(player2Pieces);
         initializeGamePiecePositions();

         curTile->position = vec3(player1Pieces[0]->position.x, player1Pieces[0]->position.y, player1Pieces[0]->position.z);
         curTile->color = emerald;
         colorTile(curTile);
      }
   }

   void changeTurn()
   {
      if (playerTurn == 1)
      {
         // Camera in bottom left
         if ((degrees(cameraTheta) >= 89.9 && degrees(cameraTheta) < 180))
         {
            targetCameraTheta = radians(270.0);
            targetTheta = radians(90.0);
         }
         // Camera in top left
         else if (degrees(cameraTheta) >= 180 && degrees(cameraTheta) < 270)
         {
            targetCameraTheta = radians(270.0);
            targetTheta = radians(-270.0);
         }
         // Camera in bottom right
         else if (degrees(cameraTheta) < 90)
         {
            targetCameraTheta = radians(-90.0);
            targetTheta = radians(-270.0);
         }
         // Camera in top right
         else if (degrees(cameraTheta) >= 270)
         {
            targetCameraTheta = radians(270.0);
            targetTheta = radians(-270.0);
         }

         calcCameraComponents();
         playerTurn = 2;
      }
      else
      {
         // Camera in bottom left, works
         if ((degrees(cameraTheta) >= 90 && degrees(cameraTheta) < 180))
         {
            targetCameraTheta = radians(90.0);
            targetTheta = radians(-90.0);
         }
         // Camera in top left, works
         else if (degrees(cameraTheta) >= 180 && degrees(cameraTheta) < 270)
         {
            targetCameraTheta = radians(90.0);
            targetTheta = radians(-90.0 - 360.0);
         }
         // Camera in bottom right
         else if (degrees(cameraTheta) < 90)
         {
            targetCameraTheta = radians(90.0);
            targetTheta = radians(-90.0);
         }
         // Camera in top right, works
         else if (degrees(cameraTheta) >= 269.9)
         {
            targetCameraTheta = radians(360.0 + 90.0);
            targetTheta = radians(-90.0);
         }

         calcCameraComponents();
         playerTurn = 1;
      }

      targetCameraPhi = M_PI / 4.0;
      targetPhi = -M_PI / 4.0;

      deltaCameraTheta = degrees(targetCameraTheta) - degrees(cameraTheta);
      deltaCameraPhi = degrees(targetCameraPhi) - degrees(cameraPhi);

      deltaTheta = degrees(targetTheta) - degrees(theta);
      deltaPhi = degrees(targetPhi) - degrees(phi);

      // "Normalize" deltas
      if (deltaCameraTheta != 0)
         deltaCameraTheta /= abs(deltaCameraTheta);
      if (deltaCameraPhi != 0)
      deltaCameraPhi /= abs(deltaCameraPhi);

      if (deltaTheta != 0)
         deltaTheta /= abs(deltaTheta);
      if (deltaPhi != 0)
         deltaPhi /= abs(deltaPhi);

      if (targetCameraTheta < 0.0)
         targetCameraTheta += radians(360.0);

      if (targetCameraTheta > radians(360.0))
         targetCameraTheta -= radians(360.0);

      if (targetTheta > 0.0)
         targetTheta -= radians(360.0);

      if (targetTheta < radians(-360.0))
         targetTheta += radians(360.0);

      cameraThetaAtLocation = 0;
      cameraPhiAtLocation = 0;
      thetaAtLocation = 0;
      phiAtLocation = 0;

      changePlayerCamera = 1;
      colorTile(curTile);
   }

   void mouseCallback(GLFWwindow *window, int button, int action, int mods)
   {
      double posX, posY;

      if (action == GLFW_PRESS)
      {
          glfwGetCursorPos(window, &posX, &posY);
          cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
      }
   }

   void resizeCallback(GLFWwindow *window, int width, int height)
   {
      glViewport(0, 0, width, height);
   }

   void scrollCallback(GLFWwindow* window, double deltaX, double deltaY)
   {
   }

   void cursorPositionCallback(GLFWwindow* window, double posX, double posY)
   {
      int width, height;
      int click = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

      glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

      // Used to set each pixel to represent a poition on the unit circle so we can scale deltaX & deltaY
      double rotateX = (2.0 * M_PI) / width;
      double rotateY = M_PI / height;

      if (click)
      {
         if (!changePlayerCamera)
         {
            deltaX = posX - prevX;
            deltaY = posY - prevY;

            theta += deltaX * rotateX;
            phi -= deltaY * rotateY;

            calcCameraComponents();
         }
      }
      prevX = posX;
      prevY = posY;
   }

   void resize_obj(std::vector<tinyobj::shape_t> &shapes) {
      float minX, minY, minZ;
      float maxX, maxY, maxZ;
      float scaleX, scaleY, scaleZ;
      float shiftX, shiftY, shiftZ;
      float epsilon = 0.001;

      minX = minY = minZ = 1.1754E+38F;
      maxX = maxY = maxZ = -1.1754E+38F;

      //Go through all vertices to determine min and max of each dimension
      for (size_t i = 0; i < shapes.size(); i++) {
         for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
            if (shapes[i].mesh.positions[3 * v + 0] < minX) minX = shapes[i].mesh.positions[3 * v + 0];
            if (shapes[i].mesh.positions[3 * v + 0] > maxX) maxX = shapes[i].mesh.positions[3 * v + 0];

            if (shapes[i].mesh.positions[3 * v + 1] < minY) minY = shapes[i].mesh.positions[3 * v + 1];
            if (shapes[i].mesh.positions[3 * v + 1] > maxY) maxY = shapes[i].mesh.positions[3 * v + 1];

            if (shapes[i].mesh.positions[3 * v + 2] < minZ) minZ = shapes[i].mesh.positions[3 * v + 2];
            if (shapes[i].mesh.positions[3 * v + 2] > maxZ) maxZ = shapes[i].mesh.positions[3 * v + 2];
         }
      }

      //From min and max compute necessary scale and shift for each dimension
      float maxExtent, xExtent, yExtent, zExtent;
      xExtent = maxX - minX;
      yExtent = maxY - minY;
      zExtent = maxZ - minZ;
      if (xExtent >= yExtent && xExtent >= zExtent) {
         maxExtent = xExtent;
      }
      if (yExtent >= xExtent && yExtent >= zExtent) {
         maxExtent = yExtent;
      }
      if (zExtent >= xExtent && zExtent >= yExtent) {
         maxExtent = zExtent;
      }
      scaleX = 2.0 / maxExtent;
      shiftX = minX + (xExtent / 2.0);
      scaleY = 2.0 / maxExtent;
      shiftY = minY + (yExtent / 2.0);
      scaleZ = 2.0 / maxExtent;
      shiftZ = minZ + (zExtent) / 2.0;

      //Go through all verticies shift and scale them
      for (size_t i = 0; i < shapes.size(); i++) {
         for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
            shapes[i].mesh.positions[3 * v + 0] = (shapes[i].mesh.positions[3 * v + 0] - shiftX) * scaleX;
            assert(shapes[i].mesh.positions[3 * v + 0] >= -1.0 - epsilon);
            assert(shapes[i].mesh.positions[3 * v + 0] <= 1.0 + epsilon);
            shapes[i].mesh.positions[3 * v + 1] = (shapes[i].mesh.positions[3 * v + 1] - shiftY) * scaleY;
            assert(shapes[i].mesh.positions[3 * v + 1] >= -1.0 - epsilon);
            assert(shapes[i].mesh.positions[3 * v + 1] <= 1.0 + epsilon);
            shapes[i].mesh.positions[3 * v + 2] = (shapes[i].mesh.positions[3 * v + 2] - shiftZ) * scaleZ;
            assert(shapes[i].mesh.positions[3 * v + 2] >= -1.0 - epsilon);
            assert(shapes[i].mesh.positions[3 * v + 2] <= 1.0 + epsilon);
         }
      }
   }

   void init(const std::string& resourceDirectory)
   {
      int width, height;
      glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
      GLSL::checkVersion();

      // Set background color.
      CHECKED_GL_CALL(glClearColor(.12f, .34f, .56f, 1.0f));

      // Enable z-buffer test.
      CHECKED_GL_CALL(glEnable(GL_DEPTH_TEST));
      CHECKED_GL_CALL(glEnable(GL_BLEND));
      CHECKED_GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
      CHECKED_GL_CALL(glPointSize(14.0f));

      // Initialize the GLSL program.
      particleProg = make_shared<Program>();
      particleProg->setVerbose(true);
      particleProg->setShaderNames(
         resourceDirectory + "/particle_vert.glsl",
         resourceDirectory + "/particle_frag.glsl");
      if (!particleProg->init())
      {
         std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
         exit(1);
      }
      particleProg->addUniform("P");
      particleProg->addUniform("V");
      particleProg->addUniform("M");
      particleProg->addUniform("alphaTexture");
      particleProg->addAttribute("vertPos");

      // Initialize the GLSL program, uses vector norms.
      prog = make_shared<Program>();
      prog->setVerbose(true);
      prog->setShaderNames(resourceDirectory + "/lighting_vert.glsl", resourceDirectory + "/lighting_frag.glsl");
      prog->init();
      prog->addUniform("P");
      prog->addUniform("V");
      prog->addUniform("M");
      prog->addUniform("lightPos");
      prog->addUniform("eye");
      prog->addUniform("MatDif");
      prog->addUniform("MatAmb");
      prog->addUniform("MatSpec");
      prog->addUniform("shine");
      prog->addAttribute("vertPos");
      prog->addAttribute("vertNor");

      texProg = make_shared<Program>();
      texProg->setVerbose(true);
      texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
      texProg->init();
      texProg->addUniform("P");
      texProg->addUniform("V");
      texProg->addUniform("M");
      texProg->addUniform("lightPos");
      texProg->addUniform("Texture0");
      texProg->addUniform("Sign");
      texProg->addAttribute("vertPos");
      texProg->addAttribute("vertNor");
      texProg->addAttribute("vertTex");

      cubeProg = make_shared<Program>();
      cubeProg->setVerbose(true);
      cubeProg->setShaderNames(resourceDirectory + "/cube_vert.glsl", resourceDirectory + "/cube_frag.glsl");
      cubeProg->init();
      cubeProg->addUniform("P");
      cubeProg->addUniform("V");
      cubeProg->addUniform("M");
      cubeProg->addAttribute("vertPos");
   }

   void initGeom(const std::string& resourceDirectory)
   {
      // generate the VAO
      CHECKED_GL_CALL(glGenVertexArrays(1, &VertexArrayID));
      CHECKED_GL_CALL(glBindVertexArray(VertexArrayID));

      // generate vertex buffer to hand off to OGL - using instancing
      CHECKED_GL_CALL(glGenBuffers(1, &pointsbuffer));
      // set the current state to focus on our vertex buffer
      CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, pointsbuffer));
      // actually memcopy the data - only do this once
      CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(points), NULL, GL_STREAM_DRAW));

      CHECKED_GL_CALL(glGenBuffers(1, &colorbuffer));
      // set the current state to focus on our vertex buffer
      CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, colorbuffer));
      // actually memcopy the data - only do this once
      CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(pointColors), NULL, GL_STREAM_DRAW));

      //EXAMPLE set up to read one shape from one obj file - convert to read several
      // Initialize cube
      // Load geometry
      // Some obj files contain material information.We'll ignore them for this assignment.
      vector<tinyobj::shape_t> TOshapes;
      vector<tinyobj::material_t> objMaterials;
      string errStr;
      //load in the cube and make the shape(s)
      bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/cube.obj").c_str());
      if (!rc) {
         cerr << errStr << endl;
      } else {
         cube = make_shared<Shape>();
         cube->createShape(TOshapes[0]);
         cube->measure();
         cube->init();
      }

      //read out information stored in the shape about its size - something like this...
      //then do something with that information.....
      cubeMin.x = cube->min.x;
      cubeMin.y = cube->min.y;
      cubeMin.z = cube->min.z;
      cubeMax.x = cube->max.x;
      cubeMax.y = cube->max.y;
      cubeMax.z = cube->max.z;

      // Load rocks mesh
      rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/Rocks/rock1.obj").c_str());
      if (!rc) {
         cerr << errStr << endl;
      }
      else {
         rocksMin.x = rocksMin.y = rocksMin.z = 1000;
         rocksMax.x = rocksMax.y = rocksMax.z = -1000;
         resize_obj(TOshapes);
         for (int i = 0; i < TOshapes.size(); i++)
         {
            rocks.push_back(make_shared<Shape>());
            rocks[i]->createShape(TOshapes[i]);
            rocks[i]->measure();
            rocks[i]->init();
            // Find min coordinates
            if (rocks[i]->min.x < rocksMin.x)
               rocksMin.x = rocks[i]->min.x;
            if (rocks[i]->min.y < rocksMin.y)
               rocksMin.y = rocks[i]->min.y;
            if (rocks[i]->min.z < rocksMin.z)
               rocksMin.z = rocks[i]->min.z;
            // Find max coordinates
            if (rocks[i]->max.x > rocksMax.x)
               rocksMax.x = rocks[i]->max.x;
            if (rocks[i]->max.y > rocksMax.y)
               rocksMax.y = rocks[i]->max.y;
            if (rocks[i]->max.z > rocksMax.z)
               rocksMax.z = rocks[i]->max.z;
         }
      }

      // Load tree mesh
      rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/Tree/Tree low.obj").c_str());
      if (!rc) {
         cerr << errStr << endl;
      }
      else {
         treeMin.x = treeMin.y = treeMin.z = 1000;
         treeMax.x = treeMax.y = treeMax.z = -1000;
         resize_obj(TOshapes);
         for (int i = 0; i < TOshapes.size(); i++)
         {
            tree.push_back(make_shared<Shape>());
            
            //Don't load the texture coordinates, they aren't needed
            TOshapes[i].mesh.texcoords.resize(0);

            tree[i]->createShape(TOshapes[i]);            
            tree[i]->measure();
            tree[i]->init();
            // Find min coordinates
            if (tree[i]->min.x < treeMin.x)
               treeMin.x = tree[i]->min.x;
            if (tree[i]->min.y < treeMin.y)
               treeMin.y = tree[i]->min.y;
            if (tree[i]->min.z < treeMin.z)
               treeMin.z = tree[i]->min.z;
            // Find max coordinates
            if (tree[i]->max.x > treeMax.x)
               treeMax.x = tree[i]->max.x;
            if (tree[i]->max.y > treeMax.y)
               treeMax.y = tree[i]->max.y;
            if (tree[i]->max.z > treeMax.z)
               treeMax.z = tree[i]->max.z;
         }
      }

      // Load dummy mesh
      rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/dummy.obj").c_str());
      if (!rc) {
         cerr << errStr << endl;
      }
      else {
         dummyMin.x = dummyMin.y = dummyMin.z = 1000;
         dummyMax.x = dummyMax.y = dummyMax.z = -1000;
         resize_obj(TOshapes);
         for (int i = 0; i < TOshapes.size(); i++)
         {
            dummy.push_back(make_shared<Shape>());
            dummy[i]->createShape(TOshapes[i]);
            dummy[i]->measure();
            dummy[i]->init();
            // Find min coordinates
            if (dummy[i]->min.x < dummyMin.x)
               dummyMin.x = dummy[i]->min.x;
            if (dummy[i]->min.y < dummyMin.y)
               dummyMin.y = dummy[i]->min.y;
            if (dummy[i]->min.z < dummyMin.z)
               dummyMin.z = dummy[i]->min.z;
            // Find max coordinates
            if (dummy[i]->max.x > dummyMax.x)
               dummyMax.x = dummy[i]->max.x;
            if (dummy[i]->max.y > dummyMax.y)
               dummyMax.y = dummy[i]->max.y;
            if (dummy[i]->max.z >dummyMax.z)
               dummyMax.z = dummy[i]->max.z;
         }
      }

      // Load bench mesh
      rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/WoodBench/Cgtuts_Wood_Bench_OBJ.obj").c_str());
      if (!rc) {
         cerr << errStr << endl;
      }
      else {        
         resize_obj(TOshapes);
         bench = make_shared<Shape>();
         bench->createShape(TOshapes[0]);

         bench->calcNormals();

         bench->measure();
         bench->init();
      }

      benchMin.x = bench->min.x;
      benchMin.y = bench->min.y;
      benchMin.z = bench->min.z;
      benchMax.x = bench->max.x;
      benchMax.y = bench->max.y;
      benchMax.z = bench->max.z;

      // Load smooth sphere mesh
      rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/SmoothSphere.obj").c_str());
      if (!rc) {
         cerr << errStr << endl;
      } else {
         resize_obj(TOshapes);
         sphere = make_shared<Shape>();
         sphere->createShape(TOshapes[0]);
         sphere->measure();
         sphere->init();
      }

      sphereMin.x = sphere->min.x;
      sphereMin.y = sphere->min.y;
      sphereMin.z = sphere->min.z;
      sphereMax.x = sphere->max.x;
      sphereMax.y = sphere->max.y;
      sphereMax.z = sphere->max.z;

      // Load smooth dog mesh
      rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/dog.obj").c_str());
      if (!rc) {
         cerr << errStr << endl;
      } else {
         resize_obj(TOshapes);
         dog = make_shared<Shape>();
         dog->createShape(TOshapes[0]);
         dog->measure();
         dog->init();
      }

      dogMin.x = dog->min.x;
      dogMin.y = dog->min.y;
      dogMin.z = dog->min.z;
      dogMax.x = dog->max.x;
      dogMax.y = dog->max.y;
      dogMax.z = dog->max.z;

      rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/skybox.obj").c_str());
      if (!rc) {
         cerr << errStr << endl;
      }
      else {
         skybox = make_shared<Shape>();
         skybox->createShape(TOshapes[0]);
         skybox->measure();
         skybox->init();
      }

      cubeWidthX = cubeMax.x - cubeMin.x;
      cubeWidthZ = cubeMax.z - cubeMin.z;
      cubeWidthY = cubeMax.y - cubeMin.y;
      distX = 0;
      distZ = 0;
      distY = 0;
      boardScale = 4;
      cubeSize = cubeWidthX * boardScale;

      calcCameraComponents();

      // Initialize board tiles
      initializeBoardTiles();

      // Initialize Player pieces;
      initializeGamePieces(player1Pieces);
      initializeGamePieces(player2Pieces);
      initializeGamePiecePositions();

      curTile->position = vec3(player1Pieces[0]->position.x, player1Pieces[0]->position.y, player1Pieces[0]->position.z);
      curTile->color = emerald;
      colorTile(curTile);
   }

   void calcCameraComponents()
   {
      if (cameraTheta > radians(360.0))
      {
         cameraTheta -= radians(360.0);
      }

      else if (cameraTheta < radians(0.0))
      {
         cameraTheta += radians(360.);
      }

      if (theta < radians(-360.0))
      {
         theta += radians(360.0);
      }
      else if (theta > 0.0)
      {
         theta -= radians(360.0);
      }

      if (cameraPhi > radians(85.0))
         cameraPhi = radians(85.0);
      else if (cameraPhi < radians(1.0))
         cameraPhi = radians(1.0);

      if (phi < radians(-85.0))
         phi = radians(-85.0);
      else if (phi > radians(1.0))
         phi = radians(1.0);

      eye.x = cos(cameraPhi)*cos(cameraTheta) * 10.0 * cubeSize;
      eye.y = sin(cameraPhi) * 10.0 * cubeSize;
      eye.z = cos(cameraPhi)*cos((M_PI / 2.0) - cameraTheta) * 10.0 * cubeSize;

      lookAtPoint.x = cos(phi)*cos(theta);
      lookAtPoint.y = sin(phi);
      lookAtPoint.z = cos(phi)*cos((M_PI / 2.0) - theta);

      lookAtPoint += eye;

      gaze = lookAtPoint - eye;
      w = -normalize(gaze);
      u = normalize(cross(up, w));
      v = cross(w, u);
   }

   void calcDirection(shared_ptr<GamePiece>& gamePiece)
   {
      float angle = atan(gamePiece->velocity.z / gamePiece->velocity.x);

      if (gamePiece->velocity.x < 0 && gamePiece->velocity.z > 0)
         gamePiece->direction = -angle;
      else if (gamePiece->velocity.x < 0 && gamePiece->velocity.z < 0)
         gamePiece->direction = -angle;
      else if (gamePiece->velocity.x > 0 && gamePiece->velocity.z < 0)
         gamePiece->direction = -(M_PI - abs(angle));
      else if (gamePiece->velocity.x > 0 && gamePiece->velocity.z > 0)
         gamePiece->direction = M_PI - angle;
      else if (gamePiece->velocity.x == 0 && gamePiece->velocity.z > 0)
         gamePiece->direction = M_PI / 2;
      else if (gamePiece->velocity.x == 0 && gamePiece->velocity.z < 0)
         gamePiece->direction = -M_PI / 2;
      else if (gamePiece->velocity.x > 0 && gamePiece->velocity.z == 0)
         gamePiece->direction = M_PI;
      else if (gamePiece->velocity.x < 0 && gamePiece->velocity.z == 0)
         gamePiece->direction = 0;
   }

   void movePiece(shared_ptr<BoardTile>& startTile, shared_ptr<BoardTile>& finishTile, vector<shared_ptr<GamePiece>>& pieces)
   {
      for (int i = 0; i < pieces.size(); i ++)
      {
         if (pieces[i]->position.x == startTile->position.x && pieces[i]->position.z == startTile->position.z)
         {
            pieces[i]->targetPosition.x = finishTile->position.x;
            pieces[i]->targetPosition.z = finishTile->position.z;

            pieces[i]->velocity.x = finishTile->position.x - startTile->position.x;
            pieces[i]->velocity.z = finishTile->position.z - startTile->position.z;
            calcDirection(pieces[i]);
            return;
         }
      }
   }

   void setPieceTarget(shared_ptr<GamePiece> piece, shared_ptr<BoardTile> tile)
   {
      piece->targetPosition.x = tile->position.x;
      piece->targetPosition.z = tile->position.z;

      piece->velocity.x = tile->position.x - piece->position.x;
      piece->velocity.z = tile->position.z - piece->position.z;
      movingPiece = 1;
      calcDirection(piece);
   }

   shared_ptr<GamePiece> findGamePiece(shared_ptr<BoardTile>& tile, int playerTurn)
   {
      if (playerTurn == 1)
      {
         for (int i = 0; i < player1Pieces.size(); i ++)
         {
            if (player1Pieces[i]->position.x == tile->position.x && player1Pieces[i]->position.z == tile->position.z)
            {
               return player1Pieces[i];
            }
         }
      }
      if (playerTurn == 2)
      {
         for (int i = 0; i < player2Pieces.size(); i ++)
         {
            if (player2Pieces[i]->position.x == tile->position.x && player2Pieces[i]->position.z == tile->position.z)
            {
               return player2Pieces[i];
            }
         }
      }
      return NULL;
   }

   int tileHasPlayerPiece(int playerTurn, shared_ptr<BoardTile>& tile)
   {
      if (playerTurn == 1)
      {
         for (int i = 0; i < player1Pieces.size(); i ++)
         {
            if (player1Pieces[i]->position.x == tile->position.x && player1Pieces[i]->position.z == tile->position.z)
            {
               return 1;
            }
         }
         return 0;
      }

      if (playerTurn == 2)
      {
         for (int i = 0; i < player2Pieces.size(); i ++)
         {
            if (player2Pieces[i]->position.x == tile->position.x && player2Pieces[i]->position.z == tile->position.z)
            {
               return 1;
            }
         }
         return 0;
      }
   }

   int tileHasPiece(shared_ptr<BoardTile>& tile)
   {
      for (int i = 0; i < player1Pieces.size(); i ++)
      {
         if (player1Pieces[i]->position.x == tile->position.x && player1Pieces[i]->position.z == tile->position.z)
         {
            return 1;
         }
      }

      for (int i = 0; i < player2Pieces.size(); i ++)
      {
         if (player2Pieces[i]->position.x == tile->position.x && player2Pieces[i]->position.z == tile->position.z)
         {
            return 1;
         }
      }
      return 0;
   }

   int tileIsWithinBoard(shared_ptr<BoardTile>& tile)
   {
      if (tile->position.x <= ((-4 * cubeSize) - (cubeSize / 2.0)))
         return 0;
      if (tile->position.z <= ((-4 * cubeSize) - (cubeSize / 2.0)))
         return 0;
      if (tile->position.x >= ((4 * cubeSize) + (cubeSize / 2.0)))
         return 0;
      if (tile->position.z >= ((4 * cubeSize) + (cubeSize / 2.0)))
         return 0;
      return 1;
   }

   int hasPlaceToMove(shared_ptr<GamePiece>& potentialPiece, int otherPlayer)
   {
      shared_ptr<BoardTile> potentialTile = make_shared<BoardTile>();

      if (playerTurn == 1)
      {
         // Check front left tile
         potentialTile->position.x = potentialPiece->position.x - cubeSize;
         potentialTile->position.z = potentialPiece->position.z - cubeSize;

         if (tileIsWithinBoard(potentialTile))
         {
            if (!tileHasPiece(potentialTile))
            {
               return 1;
            }
            else
            {
               // There is a piece on the front left tile, check for potential jump opportunity
               if (tileHasPlayerPiece(otherPlayer, potentialTile))
               {
                  potentialTile->position.x -= cubeSize;
                  potentialTile->position.z -= cubeSize;
                  if (tileIsWithinBoard(potentialTile))
                     if (!tileHasPiece(potentialTile))
                        return 1;
               }
            }
         }

         // Check front right tile
         potentialTile->position.x = potentialPiece->position.x + cubeSize;
         potentialTile->position.z = potentialPiece->position.z - cubeSize;

         if (tileIsWithinBoard(potentialTile))
         {
            if (!tileHasPiece(potentialTile))
               return 1;
            else
            {
               // There is a piece on the front right tile, check for potential jump opportunity
               if (tileHasPlayerPiece(otherPlayer, potentialTile))
               {
                  potentialTile->position.x += cubeSize;
                  potentialTile->position.z -= cubeSize;
                  if (tileIsWithinBoard(potentialTile))
                     if (!tileHasPiece(potentialTile))
                        return 1;
               }
            }
         }

         if (potentialPiece->king)
         {
            // Check back left tile
            potentialTile->position.x = potentialPiece->position.x - cubeSize;
            potentialTile->position.z = potentialPiece->position.z + cubeSize;

            if (tileIsWithinBoard(potentialTile))
            {
               if (!tileHasPiece(potentialTile))
                  return 1;
               else
               {
                  // There is a piece on the back left tile, check for potential jump opportunity
                  if (tileHasPlayerPiece(otherPlayer, potentialTile))
                  {
                     potentialTile->position.x -= cubeSize;
                     potentialTile->position.z += cubeSize;
                     if (tileIsWithinBoard(potentialTile))
                        if (!tileHasPiece(potentialTile))
                           return 1;
                  }
               }
            }

            // Check back right tile
            potentialTile->position.x = potentialPiece->position.x + cubeSize;
            potentialTile->position.z = potentialPiece->position.z + cubeSize;

            if (tileIsWithinBoard(potentialTile))
            {
               if (!tileHasPiece(potentialTile))
                  return 1;
               else
               {
                  // There is a piece on the back right tile, check for potential jump opportunity
                  if (tileHasPlayerPiece(otherPlayer, potentialTile))
                  {
                     potentialTile->position.x += cubeSize;
                     potentialTile->position.z += cubeSize;
                     if (tileIsWithinBoard(potentialTile))
                        if (!tileHasPiece(potentialTile))
                           return 1;
                  }
               }
            }
         }
         return 0;
      } 

      if (playerTurn == 2)
      {
         // Check front right tile
         potentialTile->position.x = potentialPiece->position.x - cubeSize;
         potentialTile->position.z = potentialPiece->position.z + cubeSize;

         if (tileIsWithinBoard(potentialTile))
         {
            if (!tileHasPiece(potentialTile))
               return 1;
            else
            {
               // There is a piece on the front left tile, check for potential jump opportunity
               if (tileHasPlayerPiece(otherPlayer, potentialTile))
               {
                  potentialTile->position.x -= cubeSize;
                  potentialTile->position.z += cubeSize;
                  if (tileIsWithinBoard(potentialTile))
                     if (!tileHasPiece(potentialTile))
                        return 1;
               }
            }
         }


         // Check front left tile
         potentialTile->position.x = potentialPiece->position.x + cubeSize;
         potentialTile->position.z = potentialPiece->position.z + cubeSize;

         if (tileIsWithinBoard(potentialTile))
         {
            if (!tileHasPiece(potentialTile))
               return 1;
            else
            {
               // There is a piece on the front right tile, check for potential jump opportunity
               if (tileHasPlayerPiece(otherPlayer, potentialTile))
               {
                  potentialTile->position.x += cubeSize;
                  potentialTile->position.z += cubeSize;
                  if (tileIsWithinBoard(potentialTile))
                     if (!tileHasPiece(potentialTile))
                        return 1;
               }
            }
         }

         if (potentialPiece->king)
         {
            // Check back right tile
            potentialTile->position.x = potentialPiece->position.x - cubeSize;
            potentialTile->position.z = potentialPiece->position.z - cubeSize;

            if (tileIsWithinBoard(potentialTile))
            {
               if (!tileHasPiece(potentialTile))
                  return 1;
               else
               {
                  // There is a piece on the back left tile, check for potential jump opportunity
                  if (tileHasPlayerPiece(otherPlayer, potentialTile))
                  {
                     potentialTile->position.x -= cubeSize;
                     potentialTile->position.z -= cubeSize;
                     if (tileIsWithinBoard(potentialTile))
                        if (!tileHasPiece(potentialTile))
                           return 1;
                  }
               }
            }

            // Check back left tile
            potentialTile->position.x = potentialPiece->position.x + cubeSize;
            potentialTile->position.z = potentialPiece->position.z - cubeSize;

            if (tileIsWithinBoard(potentialTile))
            {
               if (!tileHasPiece(potentialTile))
                  return 1;
               else
               {
                  // There is a piece on the back right tile, check for potential jump opportunity
                  if (tileHasPlayerPiece(otherPlayer, potentialTile))
                  {
                     potentialTile->position.x += cubeSize;
                     potentialTile->position.z -= cubeSize;
                     if (tileIsWithinBoard(potentialTile))
                        if (!tileHasPiece(potentialTile))
                           return 1;
                  }
               }
            }
         }
         return 0;
      } 
   }

   int tileWithinReach(shared_ptr<BoardTile>& tile)
   {
      int otherPlayer = 3 - playerTurn;
      if (playerTurn == 1)
      {
         // Check if tile if front left
         if (((selectedTile->position.x - cubeSize) == tile->position.x) && (selectedPiece->position.z - cubeSize) == tile->position.z)
            return 1;

         else if (((selectedTile->position.x - (2.0 * cubeSize)) == tile->position.x) && (selectedPiece->position.z - (2.0 * cubeSize)) == tile->position.z)
            return checkForJump(tile, otherPlayer);

         // Check if tile is front right
         if (((selectedTile->position.x + cubeSize) == tile->position.x) && (selectedPiece->position.z - cubeSize) == tile->position.z)
            return 1;

         else if (((selectedTile->position.x + (2.0 * cubeSize)) == tile->position.x) && (selectedPiece->position.z - (2.0 * cubeSize)) == tile->position.z)
            return checkForJump(tile, otherPlayer);

         if (selectedPiece->king)
         {
            // Check if tile if back left
            if (((selectedTile->position.x - cubeSize) == tile->position.x) && (selectedPiece->position.z + cubeSize) == tile->position.z)
               return 1;

            else if (((selectedTile->position.x - (2.0 * cubeSize)) == tile->position.x) && (selectedPiece->position.z + (2.0 * cubeSize)) == tile->position.z)
               return checkForJump(tile, otherPlayer);

            // Check if tile is back right
            if (((selectedTile->position.x + cubeSize) == tile->position.x) && (selectedPiece->position.z + cubeSize) == tile->position.z)
               return 1;

            else if (((selectedTile->position.x + (2.0 * cubeSize)) == tile->position.x) && (selectedPiece->position.z + (2.0 * cubeSize)) == tile->position.z)
               return checkForJump(tile, otherPlayer);

         }
         return 0;
      }

      if (playerTurn == 2)
      {
         // Check if tile if front right
         if (((selectedTile->position.x - cubeSize) == tile->position.x) && (selectedPiece->position.z + cubeSize) == tile->position.z)
            return 1;

         else if (((selectedTile->position.x - (2.0 * cubeSize)) == tile->position.x) && (selectedPiece->position.z + (2.0 * cubeSize)) == tile->position.z)
            return checkForJump(tile, otherPlayer);

         // Check if tile is front left
         if (((selectedTile->position.x + cubeSize) == tile->position.x) && (selectedPiece->position.z + cubeSize) == tile->position.z)
            return 1;

         else if (((selectedTile->position.x + (2.0 * cubeSize)) == tile->position.x) && (selectedPiece->position.z + (2.0 * cubeSize)) == tile->position.z)
            return checkForJump(tile, otherPlayer);

         if (selectedPiece->king)
         {
            // Check if tile if back right
            if (((selectedTile->position.x - cubeSize) == tile->position.x) && (selectedPiece->position.z - cubeSize) == tile->position.z)
               return 1;

            else if (((selectedTile->position.x - (2.0 * cubeSize)) == tile->position.x) && (selectedPiece->position.z - (2.0 * cubeSize)) == tile->position.z)
               return checkForJump(tile, otherPlayer);

            // Check if tile is back left
            if (((selectedTile->position.x + cubeSize) == tile->position.x) && (selectedPiece->position.z - cubeSize) == tile->position.z)
               return 1;

            else if (((selectedTile->position.x + (2.0 * cubeSize)) == tile->position.x) && (selectedPiece->position.z - (2.0 * cubeSize)) == tile->position.z)
               return checkForJump(tile, otherPlayer);
         }
         return 0;
      }

   }

   int checkForJump(shared_ptr<BoardTile>& tile, int otherPlayer)
   {
      shared_ptr<BoardTile> potentialTile = make_shared<BoardTile>();
      potentialTile->position.x = (tile->position.x + selectedTile->position.x) / 2.0;
      potentialTile->position.z = (tile->position.z + selectedTile->position.z) / 2.0;
      // See if there is a piece to jump
      if (tileHasPlayerPiece(otherPlayer, potentialTile))
         return 1;
      else
         return 0;
   }

   int validTile(shared_ptr<BoardTile>& boardTile, shared_ptr<BoardTile>& tile, int otherPlayer)
   {
      if (boardTile->originalColor == white)
         return 0;

      if (selectingPiece)
      {
         if (!tileHasPiece(tile))
            return 0;
         if (tileHasPlayerPiece(otherPlayer, tile))
            return 0;
         if (!tileHasPlayerPiece(playerTurn, tile))
            return 0;
         if (tileHasPlayerPiece(playerTurn, tile))
         {
            shared_ptr<GamePiece> potentialPiece = findGamePiece(tile, playerTurn);
            if (!hasPlaceToMove(potentialPiece, otherPlayer))
               return 0;
            else
               return 1;
         }
      }

      if (!selectingPiece)
      {
         if (tileHasPiece(tile))
            return 0;
         if (!tileHasPiece(tile))
         {
            if (tileWithinReach(tile))
               return 1;
            else
               return 0;
         }
      }
   }

   void colorTile(shared_ptr<BoardTile>& tile)
   {
      int otherPlayer = 3 - playerTurn;

      for (int i = 0; i < boardTiles.size(); i ++)
      {
         if (boardTiles[i]->position.x == tile->position.x && boardTiles[i]->position.z == tile->position.z)
         {
            if (!validTile(boardTiles[i], tile, otherPlayer))
            {
               boardTiles[i]->color = ruby;
               tile->color = ruby;
               tile->originalColor = white;
            }
            else
            {
               boardTiles[i]->color = emerald;
               tile->color = emerald;
               tile->originalColor = black;
            }
         }
      }
   }

   void colorTileGreen(shared_ptr<BoardTile>& tile)
   {
      for (int i = 0; i < boardTiles.size(); i ++)
      {
         if (boardTiles[i]->position.x == tile->position.x && boardTiles[i]->position.z == tile->position.z)
         {
            if (boardTiles[i]->originalColor == black)
            {
               boardTiles[i]->color = emerald;
               tile->color = emerald;
               tile->originalColor = black;
            }
         }
      }
   }

   void colorTileOriginalColor(shared_ptr<BoardTile>& tile)
   {
      for (int i = 0; i < boardTiles.size(); i ++)
      {
         if (boardTiles[i]->position.x == tile->position.x && boardTiles[i]->position.z == tile->position.z)
         {
            boardTiles[i]->color = boardTiles[i]->originalColor;
         }
      }
   }

   void initializeGamePieces(vector<shared_ptr<GamePiece>>& playerPieces)
   {
      for (int i = 0; i < 12; i++)
      {
         playerPieces.push_back(make_shared<GamePiece>());

         playerPieces[i]->king = 0;

         playerPieces[i]->upperArmTheta = 0;
         playerPieces[i]->forearmTheta = M_PI / 4.0;
         playerPieces[i]->handTheta = M_PI / 8.0;

         playerPieces[i]->leftUpperLegTheta = 0;
         playerPieces[i]->leftLowerLegTheta = M_PI / 8.0;
         playerPieces[i]->leftHoofTheta = M_PI / 7.0;

         playerPieces[i]->rightUpperLegTheta = 0;
         playerPieces[i]->rightLowerLegTheta = M_PI / 8.0;
         playerPieces[i]->rightHoofTheta = M_PI / 7.0;

         playerPieces[i]->armUp = 1;
         playerPieces[i]->leftLegUp = 1;
         playerPieces[i]->rightLegUp = 0;

         playerPieces[i]->velocity = vec3(0);
         playerPieces[i]->radius = 1.35;
      }
   }

   void initializeGamePiecePositions()
   {
      int player1Piece = 0;
      int player2Piece = 0;

      for (int i = -4; i < 4; i++)
      {
         for (int j = -4; j < 4; j++)
         {
            if ((((i & 1) == 0) && ((j & 1) == 0)) || (((i & 1) == 1) && ((j & 1) == 1)))
            {
               // Player 1 pieces
               if (j < -1)
               {
                  player1Pieces[player1Piece]->position = vec3(i * cubeSize + ((cubeSize) / 2.0), 2.8, (-j * cubeSize) - ((cubeSize) / 2.0));
                  player1Pieces[player1Piece]->targetPosition = vec3(i * cubeSize + ((cubeSize) / 2.0), 2.8, (-j * cubeSize) - ((cubeSize) / 2.0));
                  player1Pieces[player1Piece]->direction = -M_PI / 2;
                  player1Pieces[player1Piece]->color = player1Material;
                  player1Piece ++;
               }
               // Player 2 pieces
               if (j > 0)
               {
                  player2Pieces[player2Piece]->position = vec3(i * cubeSize + ((cubeSize) / 2.0), 2.8, (-j * cubeSize) - ((cubeSize) / 2.0));
                  player2Pieces[player2Piece]->targetPosition = vec3(i * cubeSize + ((cubeSize) / 2.0), 2.8, (-j * cubeSize) - ((cubeSize) / 2.0));
                  player2Pieces[player2Piece]->direction = M_PI / 2;
                  player2Pieces[player2Piece]->color = player2Material;
                  player2Piece ++;
               }
            }
         }
      }
   }

   void initializeBoardTiles()
   {
      numTiles = 0;
      for (int i = -4; i < 4; i++)
      {
         for (int j = -4; j < 4; j++)
         {

            boardTiles.push_back(make_shared<BoardTile>());;
            boardTiles[numTiles]->position = vec3(i * cubeSize + ((cubeSize) / 2.0), (cubeWidthY * (boardScale / 5.0)) / 2.0, (-j * cubeSize) - ((cubeSize) / 2.0));
            boardTiles[numTiles]->scale = vec3(boardScale, boardScale / 5.0, boardScale);

            // Black Tile
            if ((((i & 1) == 0) && ((j & 1) == 0)) || (((i & 1) == 1) && ((j & 1) == 1)))
            {
               boardTiles[numTiles]->color = black;
               boardTiles[numTiles]->originalColor = black;
            }
            // White tile
            else
            {
               boardTiles[numTiles]->color = white;
               boardTiles[numTiles]->originalColor = white;
            }

            numTiles ++;
         }
      }
   }

   void updateGamePiece()
   {
      vec3 pieceVelocity = normalize(selectedPiece->velocity) * speed;
      selectedPiece->position += pieceVelocity;

      if (playerTurn == 1)
         checkCollisions(selectedPiece, player2Pieces);
      else if (playerTurn == 2)
         checkCollisions(selectedPiece, player1Pieces);

      // Piece is at target location
      if ((selectedPiece->position.x >= selectedPiece->targetPosition.x - 0.1) && (selectedPiece->position.x <= selectedPiece->targetPosition.x + 0.1))
      {
         selectedPiece->velocity = vec3(0);
         selectedPiece->position.x = selectedPiece->targetPosition.x;
         selectedPiece->position.z = selectedPiece->targetPosition.z;
         // Check to see if the piece became a king
         if (playerTurn == 1)
         {
            if (selectedPiece->position.z == ((-3 * cubeSize) - (cubeSize / 2.0)))
            {
               if (!selectedPiece->king)
               {
                  selectedPiece->king = 1;
                  changeParticlePosition(selectedPiece->targetPosition, vec3(0.7804, 0.5686, 0.11373));
               }
            }
         }
         if (playerTurn == 2)
         {
            if (selectedPiece->position.z == ((3 * cubeSize) + (cubeSize / 2.0)))
            {
               if (!selectedPiece->king)
               {
                  selectedPiece->king = 1;
                  changeParticlePosition(selectedPiece->targetPosition, vec3(0.7804, 0.5686, 0.11373));
               }
            }
         }

         if (playerTurn == 1)
         {
            if (player2Pieces.size() > 0)
            {
               curTile->position.x = player2Pieces[0]->position.x;
               curTile->position.z = player2Pieces[0]->position.z;
            }
         }
         else
         {
            if (player1Pieces.size() > 0)
            {
               curTile->position.x = player1Pieces[0]->position.x;
               curTile->position.z = player1Pieces[0]->position.z;
            }
         }
         movingPiece = 0;
         selectedPiece = NULL;
         changeTurn();
      }
   }

   void checkCollisions(shared_ptr<GamePiece>& gamePiece, vector<shared_ptr<GamePiece>>& opponentPieces)
   {
      // Check for collisions with bunnys
      for (int i = 0; i < opponentPieces.size(); i++)
      {
         if (distance(gamePiece->position, opponentPieces[i]->position) <= (gamePiece->radius + opponentPieces[i]->radius))
         {
            spawnParticles = 1;
            if (playerTurn == 1)
               changeParticlePosition(opponentPieces[i]->position, vec3(0.61424, 0.04136, 0.04136));
            else
               changeParticlePosition(opponentPieces[i]->position, vec3(0.07568, 0.61424, 0.07568));
            particlePos.x = opponentPieces[i]->position.x;
            particlePos.y = opponentPieces[i]->position.y;
            particlePos.z = opponentPieces[i]->position.z;
            opponentPieces.erase(opponentPieces.begin() + i);
         }
      }
   }

   float distance(vec3 p1, vec3 p2)
   {
      float difX = p1.x - p2.x;
      float difY = p1.y - p2.y;
      float difZ = p1.z - p2.z;

      return sqrt(pow(difX, 2) + pow(difY, 2) + pow(difZ, 2));
   }

   void updateCamera()
   {
      if (!cameraThetaAtLocation)
         cameraTheta += radians(cameraAngleOffset) * deltaCameraTheta;
      if (!thetaAtLocation)
         theta += radians(cameraAngleOffset) * deltaTheta;
      if (!cameraPhiAtLocation)
         cameraPhi += radians(cameraAngleOffset) * deltaCameraPhi;
      if (!phiAtLocation)
         phi += radians(cameraAngleOffset) * deltaPhi;

      // Check if cameraTheta is at right position
      if ((abs(degrees(cameraTheta)) >= abs(degrees(targetCameraTheta)) - 2) && (abs(degrees(cameraTheta)) <= abs(degrees(targetCameraTheta)) + 2) )
      {
         cameraTheta = targetCameraTheta;
         cameraThetaAtLocation = 1;
      }
      // Check if cameraPhi is at right position
      if ((abs(degrees(cameraPhi)) >= abs(degrees(targetCameraPhi)) - 2) && (abs(degrees(cameraPhi)) <= abs(degrees(targetCameraPhi)) + 2) )
      {
         cameraPhi = targetCameraPhi;
         cameraPhiAtLocation = 1;
      }
      // Check if theta is at right position
      if ((abs(degrees(theta)) >= abs(degrees(targetTheta)) - 2) && (abs(degrees(theta)) <= abs(degrees(targetTheta)) + 2) )
      {
         theta = targetTheta;
         thetaAtLocation = 1;
      }
      // Check if phi is at right position
      if ((abs(degrees(phi)) >= abs(degrees(targetPhi)) - 2) && (abs(degrees(phi)) <= abs(degrees(targetPhi)) + 2) )
      {
         phi = targetPhi;
         phiAtLocation = 1;
      }

      if (cameraThetaAtLocation && cameraPhiAtLocation && thetaAtLocation && phiAtLocation)
         changePlayerCamera = 0;

      calcCameraComponents();

   }

   // Code to load in the three textures
   void initTex(const std::string& resourceDirectory){
      texture0 = make_shared<Texture>();
      texture0->setFilename(resourceDirectory + "/crate.jpg");
      texture0->init();
      texture0->setUnit(0);
      texture0->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

      texture1 = make_shared<Texture>();
      texture1->setFilename(resourceDirectory + "/world.jpg");
      texture1->init();
      texture1->setUnit(1);
      texture1->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

      texture2 = make_shared<Texture>();
      texture2->setFilename(resourceDirectory + "/grass.jpg");
      texture2->init();
      texture2->setUnit(2);
      texture2->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

      texture3 = make_shared<Texture>();
      texture3->setFilename(resourceDirectory + "/WoodBench/Maps/Bench_2K_Diffuse.jpg");
      texture3->init();
      texture3->setUnit(2);
      texture3->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

      particleTexture = make_shared<Texture>();
      particleTexture->setFilename(resourceDirectory + "/alpha.bmp");
      particleTexture->init();
      particleTexture->setUnit(0);
      particleTexture->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
   }

   void initParticles()
   {
      int n = numP;

      for (int i = 0; i < n; ++ i)
      {
         auto particle = make_shared<Particle>();
         particles.push_back(particle);
      }
   }

   void changeParticlePosition(vec3 position, vec3 color)
   {
      int n = numP;

      for (int i = 0; i < n; ++ i)
         particles[i]->load(t, position, color);
   }

   void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
      glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
   }

   void SetMaterial(int i) {
      switch (i) {
         case 0: //shiny blue plastic
            glUniform3f(prog->getUniform("MatAmb"), 0.02, 0.04, 0.2);
            glUniform3f(prog->getUniform("MatDif"), 0.0, 0.16, 0.9);
            glUniform3f(prog->getUniform("MatSpec"), 0.14, 0.2, 0.8);
            glUniform1f(prog->getUniform("shine"), 120.0);
            break;
         case 1: // flat grey
            glUniform3f(prog->getUniform("MatAmb"), 0.13, 0.13, 0.14);
            glUniform3f(prog->getUniform("MatDif"), 0.3, 0.3, 0.4);
            glUniform3f(prog->getUniform("MatSpec"), 0.3, 0.3, 0.4);
            glUniform1f(prog->getUniform("shine"), 4.0);
            break;
         case 2: //brass
            glUniform3f(prog->getUniform("MatAmb"), 0.3294, 0.2235, 0.02745);
            glUniform3f(prog->getUniform("MatDif"), 0.7804, 0.5686, 0.11373);
            glUniform3f(prog->getUniform("MatSpec"), 0.9922, 0.941176, 0.80784);
            glUniform1f(prog->getUniform("shine"), 27.9);
            break;
         case 3: // dark green
            glUniform3f(prog->getUniform("MatAmb"), 0.13, 0.13, 0.14);
            glUniform3f(prog->getUniform("MatDif"), 0.04, 0.41, 0.19);
            glUniform3f(prog->getUniform("MatSpec"), 0, 0, 0);
            glUniform1f(prog->getUniform("shine"), 4.0);
            break;
         case 4: // brown
            glUniform3f(prog->getUniform("MatAmb"), 0.13, 0.13, 0.14);
            glUniform3f(prog->getUniform("MatDif"), 0.22, 0.18, 0.03);
            glUniform3f(prog->getUniform("MatSpec"), 0, 0, 0);
            glUniform1f(prog->getUniform("shine"), 4.0);
            break;
         case 5: // black
            glUniform3f(prog->getUniform("MatAmb"), 0, 0, 0);
            glUniform3f(prog->getUniform("MatDif"), 0.01, 0.01, 0.01);
            glUniform3f(prog->getUniform("MatSpec"), 0.5, 0.5, 0.5);
            glUniform1f(prog->getUniform("shine"), 32.0);
            break;
         case 6: // white
            glUniform3f(prog->getUniform("MatAmb"), 0.2, 0.2, 0.2);
            glUniform3f(prog->getUniform("MatDif"), 1.0, 1.0, 1.0);
            glUniform3f(prog->getUniform("MatSpec"), 0.7, 0.7, 0.7);
            glUniform1f(prog->getUniform("shine"), 32.0);
            break;
         case 7: // emerald
            glUniform3f(prog->getUniform("MatAmb"), 0.1215, 0.2745, 0.1215);
            glUniform3f(prog->getUniform("MatDif"), 0.07568, 0.61424, 0.07568);
            glUniform3f(prog->getUniform("MatSpec"), 0.633, 0.727811, 0.633);
            glUniform1f(prog->getUniform("shine"), 76.8);
            break;
         case 8: // ruby
            glUniform3f(prog->getUniform("MatAmb"), 0.2745, 0.11175, 0.11175);
            glUniform3f(prog->getUniform("MatDif"), 0.61424, 0.04136, 0.04136);
            glUniform3f(prog->getUniform("MatSpec"), 0.727811, 0.626959, 0.626959);
            glUniform1f(prog->getUniform("shine"), 76.8);
            break;
      }
   }

   unsigned int createSky(string dir, vector<string> faces) 
   {
      unsigned int textureID;
      glGenTextures(1, &textureID);
      glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
      int width, height, nrChannels;
      stbi_set_flip_vertically_on_load(false);
      for(GLuint i = 0; i < faces.size(); i++)
      {
         unsigned char *data =stbi_load((dir+faces[i]).c_str(), &width, &height, &nrChannels, 0);
         if (data) 
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

         else 
            cout << "failed to load: " << (dir+faces[i]).c_str() << endl;
      }
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
      cout << " creating cube map any errors : " << glGetError() << endl;
      return textureID;
   } 

   void drawBenches(shared_ptr<MatrixStack> Model)
   {
      vector<float> benchRotationsY;
      vector<vec3> benchTranslations;
      distY = benchMax.y - benchMin.y;
      benchRotationsY.push_back(M_PI / 2.0);
      benchRotationsY.push_back(-M_PI / 2.0);

      benchTranslations.push_back(vec3((-cubeSize * 5) - 0.6, ((distY * 3) / 2.0), 0));
      benchTranslations.push_back(vec3((cubeSize * 5) + 0.6, ((distY * 3) / 2.0), 0));

      for (int i = 0; i < benchTranslations.size(); i++)
      {
         Model->loadIdentity();
         Model->translate(benchTranslations[i]);
         Model->scale(vec3(3, 3, 6));
         Model->rotate(benchRotationsY[i], vec3(0, 1, 0));
         Model->translate(vec3((benchMax.x + benchMin.x) / 2.0, (benchMax.y + benchMin.y) / 2.0, (benchMax.z + benchMin.z) / 2.0));
         setModel(texProg, Model);
         bench->draw(texProg);
      }
   }

   void drawTrees(shared_ptr<MatrixStack> Model)
   {
      //Draw trees around the scene
      vector<vec3> treeTranslations;
      vector<float> treeRotationsY;
      int treeScale = 8;
      distY = (treeMax.y - treeMin.y);
      distX = (treeMax.x - treeMin.x);
      distZ = (treeMax.z - treeMin.z);
      float moveBack = 7;

      // Trees at corners
      treeTranslations.push_back(vec3(cubeSize * moveBack, (distY * treeScale) / 2.0, cubeSize * moveBack));
      treeRotationsY.push_back(-M_PI / 4.0);
      treeTranslations.push_back(vec3(cubeSize * moveBack, (distY * treeScale) / 2.0, cubeSize * (-moveBack)));
      treeRotationsY.push_back(M_PI / 4.0);
      treeTranslations.push_back(vec3(cubeSize * (-moveBack), (distY * treeScale) / 2.0, cubeSize * moveBack));
      treeRotationsY.push_back((-3) * M_PI / 4.0);
      treeTranslations.push_back(vec3(cubeSize * (-moveBack), (distY * treeScale) / 2.0, cubeSize * (-moveBack)));
      treeRotationsY.push_back(3 * M_PI / 4.0);

      // Trees in the back
      treeTranslations.push_back(vec3(0, (distY * treeScale) / 2.0, cubeSize * (-moveBack)));
      treeRotationsY.push_back(M_PI / 2.0);
      treeTranslations.push_back(vec3(cubeSize * (moveBack / 2.0), (distY * treeScale) / 2.0, cubeSize * (-moveBack)));
      treeRotationsY.push_back(M_PI / 2.0);
      treeTranslations.push_back(vec3(cubeSize * (-moveBack / 2.0), (distY * treeScale) / 2.0, cubeSize * (-moveBack)));
      treeRotationsY.push_back(M_PI / 2.0);

      // Trees on the right
      treeTranslations.push_back(vec3(cubeSize * moveBack, (distY * treeScale) / 2.0, 0));
      treeRotationsY.push_back(0);
      treeTranslations.push_back(vec3(cubeSize * moveBack, (distY * treeScale) / 2.0, cubeSize * (moveBack / 2.0)));
      treeRotationsY.push_back(0);
      treeTranslations.push_back(vec3(cubeSize * moveBack, (distY * treeScale) / 2.0, cubeSize * (-moveBack / 2.0)));
      treeRotationsY.push_back(0);

      // Trees in the front
      treeTranslations.push_back(vec3(0, (distY * treeScale) / 2.0, cubeSize * moveBack));
      treeRotationsY.push_back(-M_PI / 2.0);
      treeTranslations.push_back(vec3(cubeSize * (moveBack / 2.0), (distY * treeScale) / 2.0, cubeSize * moveBack));
      treeRotationsY.push_back(-M_PI / 2.0);
      treeTranslations.push_back(vec3(cubeSize * (-moveBack / 2.0), (distY * treeScale) / 2.0, cubeSize * moveBack));
      treeRotationsY.push_back(-M_PI / 2.0);

      // Trees on the left
      treeTranslations.push_back(vec3(cubeSize * (-moveBack), (distY * treeScale) / 2.0, 0));
      treeRotationsY.push_back(-M_PI);
      treeTranslations.push_back(vec3(cubeSize * (-moveBack) , (distY * treeScale) / 2.0, cubeSize * (moveBack / 2.0)));
      treeRotationsY.push_back(-M_PI);
      treeTranslations.push_back(vec3(cubeSize * (-moveBack) , (distY * treeScale) / 2.0, cubeSize * (-moveBack / 2.0)));
      treeRotationsY.push_back(-M_PI);

      for (int i = 0; i < treeTranslations.size(); i++)
      {
         Model->pushMatrix();
         Model->loadIdentity();
         Model->translate(treeTranslations[i]);
         Model->scale(vec3(treeScale));
         Model->rotate(treeRotationsY[i], vec3(0, 1, 0));
         Model->translate(vec3((treeMax.x + treeMin.x) / 2.0, (treeMax.y + treeMin.y) / 2.0, (treeMax.z + treeMin.z) / 2.0));
         for (int j = 0; j < tree.size(); j++)
         {
            setModel(prog, Model);
            if (j == 0)
               SetMaterial(treeTopMaterial);
            else
               SetMaterial(treeTrunkMaterial);
            tree[j]->draw(prog);
         }
         Model->popMatrix();
      }
   }

   void drawRocks(shared_ptr<MatrixStack> Model)
   {
      // Draw rocks around the scene
      vector<vec3> rockTranslations;
      vector<float> rockRotationsY;
      float rockScale = 3;
      distY = (rocksMax.y - rocksMin.y);
      distX = (rocksMax.x - rocksMin.x);
      distZ = (rocksMax.z - rocksMin.z);

      rockTranslations.push_back(vec3(cubeSize * 5 + 0.2, (distY * rockScale) / 2.0, cubeSize * 5));
      rockTranslations.push_back(vec3(cubeSize * 5 + 0.2, (distY * rockScale) / 2.0, cubeSize * -5));
      rockTranslations.push_back(vec3(cubeSize * -5 - 0.2, (distY * rockScale) / 2.0, cubeSize * 5));
      rockTranslations.push_back(vec3(cubeSize * -5 - 0.2, (distY * rockScale) / 2.0, cubeSize * -5));

      for (int i = 0; i < rockTranslations.size(); i ++)
      {
         Model->pushMatrix();
         Model->loadIdentity();
         Model->translate(rockTranslations[i]);
         Model->scale(vec3(rockScale));
         Model->translate(vec3((rocksMax.x + rocksMin.x) / 2.0, (rocksMax.y + rocksMin.y) / 2.0, (rocksMax.z + rocksMin.z) / 2.0));
         for (int i = 0; i < rocks.size(); i++)
         {
            setModel(prog, Model);
            SetMaterial(rockMaterial);
            rocks[i]->draw(prog);
         }
         Model->popMatrix();
      }
   }

   void drawPlayers(shared_ptr<MatrixStack> Model)
   {
      vector<float> dummyRotationsY;
      vector<vec3> dummyTranslations;

      int leftArmStart = 6;
      int leftArmEnd = 11;

      distY = dummyMax.y - dummyMin.y;
      distZ = dummyMax.z - dummyMin.y;
      distX = dummyMax.x - dummyMin.x;

      dummyRotationsY.push_back(M_PI / 2.0);
      dummyRotationsY.push_back(-M_PI / 2.0);

      dummyTranslations.push_back(vec3(0, (distZ * 4) / 2.0, cubeSize * 4 + (distX * 4) + 2));
      dummyTranslations.push_back(vec3(0, (distZ * 4) / 2.0, -(cubeSize * 4 + (distX * 4) + 2)));

      for (int i = 0; i < dummyRotationsY.size(); i++)
      {
         for (int j = 0; j < dummy.size(); j++)
         {
            Model->pushMatrix();
            Model->loadIdentity();
            Model->translate(dummyTranslations[i]);
            Model->scale(vec3(4));
            Model->rotate(dummyRotationsY[i], vec3(0, 1, 0));
            Model->rotate(-M_PI / 2.0, vec3(1, 0, 0));

            // Rotate left arm down
            if (j >= leftArmStart && j < leftArmEnd)
            {
               Model->translate(vec3((dummy[leftArmEnd]->max.x + dummy[leftArmEnd]->min.x) / 2.0, (dummy[leftArmEnd]->max.y + dummy[leftArmEnd]->min.y) / 2.0, (dummy[leftArmEnd]->max.z + dummy[leftArmEnd]->min.z) / 2.0));
               Model->rotate(-M_PI / 2.25, vec3(1, 0, 0));
               Model->translate(vec3(-(dummy[leftArmEnd]->max.x + dummy[leftArmEnd]->min.x) / 2.0, -(dummy[leftArmEnd]->max.y + dummy[leftArmEnd]->min.y) / 2.0, -(dummy[leftArmEnd]->max.z + dummy[leftArmEnd]->min.z) / 2.0));
            }

            // Rotate right arm down
            if (j == 12 || j == 18 || j == 22 || j == 27 || j == 28)
            {
               Model->translate(vec3((dummy[15]->max.x + dummy[15]->min.x) / 2.0, (dummy[15]->max.y + dummy[15]->min.y) / 2.0, (dummy[15]->max.z + dummy[15]->min.z) / 2.0));
               Model->rotate(M_PI / 2.25, vec3(1, 0, 0));
               Model->translate(vec3(-(dummy[15]->max.x + dummy[15]->min.x) / 2.0, -(dummy[15]->max.y + dummy[15]->min.y) / 2.0, -(dummy[15]->max.z + dummy[15]->min.z) / 2.0));
            }

            Model->translate(vec3((dummyMax.x + dummyMin.x) / 2.0, (dummyMax.y + dummyMin.y) / 2.0, (dummyMax.z + dummyMin.z) / 2.0));
            setModel(prog, Model);
            if (i == 0)
               SetMaterial(player1Material);
            else
               SetMaterial(player2Material);
            // Set the torso area material to match the material of the rider
            if (j == 21 || j == 23 || j == 24)
               SetMaterial(riderMaterial);
            dummy[j]->draw(prog);
            Model->popMatrix();
         }
      }
   }

   void drawGroundPlane(shared_ptr<MatrixStack> Model)
   {
      // Draw the ground plane
      Model->pushMatrix();
         Model->loadIdentity();
         Model->translate(vec3(0, -distY * 0.02, 0));
         Model->scale(vec3(100, 0.02, 100));
         // Translate to origin
         Model->translate(vec3((cubeMax.x + cubeMin.x) / 2.0, (cubeMax.y + cubeMin.y) / 2.0, (cubeMax.z + cubeMin.z) / 2.0));
         setModel(texProg, Model);
         ground->draw(texProg);
      Model->popMatrix();
   }

   void drawDogs(shared_ptr<MatrixStack> Model)
   {
      vector<float> dogRotationsY;
      vector<vec3> dogTranslations;
      distY = dogMax.y - dogMin.y;
      dogRotationsY.push_back(3 * M_PI / 4.0);
      dogRotationsY.push_back(-M_PI / 4.0);
      dogRotationsY.push_back(-3 * M_PI / 4.0);
      dogRotationsY.push_back(M_PI / 4.0);

      // Place dogs on top of the rocks at the corners of the board
      dogTranslations.push_back(vec3((-cubeSize * 5) + 0.2, ((distY * 2) / 2.0) + (rocksMax.y - rocksMin.y) * 3, (cubeSize * 5) - 0.4));
      dogTranslations.push_back(vec3((cubeSize * 5) - 0.2, ((distY * 2) / 2.0) + (rocksMax.y - rocksMin.y) * 3, -(cubeSize * 5) + 0.4));
      dogTranslations.push_back(vec3((cubeSize * 5) - 0.2, ((distY * 2) / 2.0) + (rocksMax.y - rocksMin.y) * 3, (cubeSize * 5) - 0.4));
      dogTranslations.push_back(vec3((-cubeSize * 5) + 0.2, ((distY * 2) / 2.0) + (rocksMax.y - rocksMin.y) * 3, -(cubeSize * 5) + 0.4));

      for (int i = 0; i < dogTranslations.size(); i++)
      {
         Model->loadIdentity();
         Model->translate(dogTranslations[i]);
         Model->scale(vec3(2));
         Model->rotate(dogRotationsY[i], vec3(0, 1, 0));
         Model->translate(vec3((dogMax.x + dogMin.x) / 2.0, (dogMax.y + dogMin.y) / 2.0, (dogMax.z + dogMin.z) / 2.0));
         setModel(texProg, Model);
         dog->draw(texProg);
      }
   }

   void makeFan(float x, float y, float z, float rotY, shared_ptr<MatrixStack> Model, int material)
   {
      float fanScale = 1.12;
      float radius = sphereMax.x - sphereMin.x;

      SetMaterial(material);

      // draw mesh 
      Model->pushMatrix();
      Model->loadIdentity();
      Model->translate(vec3(x, y, z));
      Model->rotate(rotY, vec3(0, 1, 0));
      Model->scale(fanScale);
         // draw top head
         Model->pushMatrix();
            Model->translate(vec3(0, radius * 0.75, 0));
            Model->scale(vec3(0.5, 0.5, 0.5));
            Model->translate(vec3(((sphereMax.x + sphereMin.x) / 2.0), ((sphereMax.x + sphereMin.x) / 2.0), ((sphereMax.x + sphereMin.x) / 2.0)));
            setModel(prog, Model);
            sphere->draw(prog);
         Model->popMatrix();

         // Draw left arm
         Model->pushMatrix();
           //place at shoulder
           Model->translate(vec3(0.6, 0.7, 0));
           //rotate shoulder joint
           Model->rotate(fanUpperArmTheta, vec3(0, 0, 1));
           //move to shoulder joint
           Model->translate(vec3(radius * .35, 0, 0));
           Model->pushMatrix();
            // Forearm
            // Translate to elbow
            Model->translate(vec3(0.8, 0, 0));
            // rotate forearm
            if (fanForearmTheta < -0.05)
               Model->rotate(-0.05, vec3(0, 0, 1));
            else
               Model->rotate(fanForearmTheta, vec3(0, 0, 1));
            // Move to elbow joint
            Model->translate(vec3(0.6, 0, 0));
            Model->pushMatrix();
               // Hand
               // Translate to wrist
               Model->translate(vec3(0.45, 0, 0));
               // rotate hand
               if (fanHandTheta < -0.05)
                  Model->rotate(-0.05, vec3(0, 0, 1));
               else
                  Model->rotate(fanHandTheta, vec3(0, 0, 1));
               // Translate to wrist joint
               Model->translate(vec3(0.3, 0, 0));
               // Scale the hand
               Model->scale(vec3(.4, .25, .25));
               setModel(prog, Model);
               sphere->draw(prog);
            Model->popMatrix();
            // Scale the forearm
            Model->scale(vec3(.7, .25, .25));
            setModel(prog, Model);
            sphere->draw(prog);
           Model->popMatrix();
           //non-uniform scale of upper arm
           Model->scale(vec3(0.9, 0.25, 0.25));
           setModel(prog, Model);
           sphere->draw(prog);
         Model->popMatrix();

         // Draw right arm
         Model->pushMatrix();
           //place at shoulder
           Model->translate(vec3(-0.6, 0.7, 0));
           //rotate shoulder joint
           Model->rotate(-fanUpperArmTheta, vec3(0, 0, 1));
           //move to shoulder joint
           Model->translate(vec3(-radius * .35, 0, 0));
           Model->pushMatrix();
            // Forearm
            // Translate to elbow
            Model->translate(vec3(-0.8, 0, 0));
            // rotate forearm
            if (fanForearmTheta < -0.05)
               Model->rotate(0.05, vec3(0, 0, 1));
            else
               Model->rotate(-fanForearmTheta, vec3(0, 0, 1));
            // Move to elbow joint
            Model->translate(vec3(-0.6, 0, 0));
            Model->pushMatrix();
               // Hand
               // Translate to wrist
               Model->translate(vec3(-0.45, 0, 0));
               // rotate hand
               if (fanHandTheta < -0.05)
                  Model->rotate(0.05, vec3(0, 0, 1));
               else
                  Model->rotate(-fanHandTheta, vec3(0, 0, 1));
               // Translate to wrist joint
               Model->translate(vec3(- 0.3, 0, 0));
               // Scale the hand
               Model->scale(vec3(.4, .25, .25));
               setModel(prog, Model);
               sphere->draw(prog);
            Model->popMatrix();
            // Scale the forearm
            Model->scale(vec3(.7, .25, .25));
            setModel(prog, Model);
            sphere->draw(prog);
           Model->popMatrix();
           //non-uniform scale of upper arm
           Model->scale(vec3(0.9, 0.25, 0.25));
           setModel(prog, Model);
           sphere->draw(prog);
         Model->popMatrix();

         // Draw right leg
         Model->pushMatrix();
            //place at hip
            Model->translate(vec3(-0.45, -0.9, 0));
            //rotate hip joint
            Model->rotate(M_PI / 2.0, vec3(0, 1, 0));
            //move to hip joint
            Model->translate(vec3(-radius * .35, 0, 0));
            Model->pushMatrix();
               // place at knee
               Model->translate(vec3(-1.0, 0, 0));
               //rotate knee joint
               Model->rotate(M_PI / 2.0, vec3(0, 0, 1));
               //move to knee joint
               Model->translate(vec3(-radius * .35, 0, 0));
               Model->pushMatrix();
                  // place at ankle
                  Model->translate(vec3(-0.7, 0, 0));
                  //rotate ankle joint
                  Model->rotate(-M_PI / 2.0, vec3(0, 0, 1));
                  //move to ankle joint
                  Model->translate(vec3(-radius * .1, 0, 0));
                  Model->scale(vec3(0.35, 0.25, 0.25));
                  setModel(prog, Model);
                  sphere->draw(prog);
               Model->popMatrix();
               Model->scale(vec3(0.75, 0.25, 0.25));
               setModel(prog, Model);
               sphere->draw(prog);
            Model->popMatrix();
            Model->scale(vec3(1.10, 0.25, 0.25));
            setModel(prog, Model);
            sphere->draw(prog);
         Model->popMatrix();

         // Draw left leg
         Model->pushMatrix();
            //place at hip
            Model->translate(vec3(0.45, -0.9, 0));
            //rotate hip joint
            Model->rotate(M_PI / 2.0, vec3(0, 1, 0));
            //move to hip joint
            Model->translate(vec3(-radius * .35, 0, 0));
            Model->pushMatrix();
               // place at knee
               Model->translate(vec3(-1.0, 0, 0));
               //rotate knee joint
               Model->rotate(M_PI / 2.0, vec3(0, 0, 1));
               //move to knee joint
               Model->translate(vec3(-radius * .35, 0, 0));
               Model->pushMatrix();
                  // place at ankle
                  Model->translate(vec3(-0.7, 0, 0));
                  //rotate ankle joint
                  Model->rotate(-M_PI / 2.0, vec3(0, 0, 1));
                  //move to ankle joint
                  Model->translate(vec3(-radius * .1, 0, 0));
                  Model->scale(vec3(0.35, 0.25, 0.25));
                  setModel(prog, Model);
                  sphere->draw(prog);
               Model->popMatrix();
               Model->scale(vec3(0.75, 0.25, 0.25));
               setModel(prog, Model);
               sphere->draw(prog);
            Model->popMatrix();
            Model->scale(vec3(1.10, 0.25, 0.25));
            setModel(prog, Model);
            sphere->draw(prog);
         Model->popMatrix();

      // Torso is the base of the model
      Model->scale(vec3(1.1, 1.2, 1.1));
      Model->translate(vec3(((sphereMax.x + sphereMin.x) / 2.0), ((sphereMax.x + sphereMin.x) / 2.0), ((sphereMax.x + sphereMin.x) / 2.0)));
      setModel(prog, Model);
      // Set torso material to match the rider material
      SetMaterial(riderMaterial);
      sphere->draw(prog);
      Model->popMatrix();
      
      if (fanArmUp && fanUpperArmTheta <= M_PI / 5.0)
      {
         fanUpperArmTheta += armIncrease;
         fanForearmTheta += armIncrease;
         fanHandTheta += armIncrease / 2;
         if (fanUpperArmTheta >= M_PI / 5.0)
            fanArmUp = 0;
      }
      if(!fanArmUp && fanUpperArmTheta >= -M_PI / 6.0)
      {
         fanUpperArmTheta -= armIncrease;
         fanForearmTheta -= armIncrease;
         fanHandTheta -= armIncrease / 2;
         if (fanUpperArmTheta <= -M_PI / 6.0)
            fanArmUp = 1;
      }
   }

   void drawFans(shared_ptr<MatrixStack> Model)
   {
      vector<vec3> fanTranslations;
      vector<float> fanRotationsY;

      fanTranslations.push_back(vec3(-cubeSize * 5 - 0.2, 2.9, 0));
      fanRotationsY.push_back(M_PI / 2.0);
      fanTranslations.push_back(vec3(cubeSize * 5 + 0.2, 2.9, 0));
      fanRotationsY.push_back(-M_PI / 2.0);

      for (int i = 0; i < fanTranslations.size(); i++)
      {
         if (i == 0)
            makeFan(fanTranslations[i].x, fanTranslations[i].y, fanTranslations[i].z, fanRotationsY[i], Model, fanMaterial1);
         else
            makeFan(fanTranslations[i].x, fanTranslations[i].y, fanTranslations[i].z, fanRotationsY[i], Model, fanMaterial2);
      }
   }

   void makeRider(shared_ptr<MatrixStack> Model, int playerMaterial)
   {
      float radius = sphereMax.x - sphereMin.x;

      SetMaterial(riderMaterial);

      // Draw rider 
      Model->pushMatrix();
         Model->translate(vec3(radius * 0.7 * 0.5 - 0.4, radius * 0.7 + 0.0, 0));
         Model->rotate(-M_PI / 2.0, vec3(0, 1, 0));
         Model->rotate(M_PI / 6.0, vec3(1, 0, 0));
         Model->scale(vec3(0.6));
            // draw top head
            Model->pushMatrix();
               Model->translate(vec3(0, radius * 0.75, 0));
               Model->scale(vec3(0.5, 0.5, 0.5));
               Model->translate(vec3(((sphereMax.x + sphereMin.x) / 2.0), ((sphereMax.x + sphereMin.x) / 2.0), ((sphereMax.x + sphereMin.x) / 2.0)));
               setModel(prog, Model);
               sphere->draw(prog);
            Model->popMatrix();

            // Draw left arm
            Model->pushMatrix();
              //place at shoulder
              Model->translate(vec3(0.6, 0.7, 0));
              // Rotate arm down
              Model->rotate(M_PI / 11.0, vec3(1, 0, 0));
              // Rotate the arm to the right
              Model->rotate(-M_PI / 2.5, vec3(0, 1, 0));
              //move to shoulder joint
              Model->translate(vec3(radius * .35, 0, 0));
              Model->pushMatrix();
               // Forearm
               // Translate to elbow
               Model->translate(vec3(0.8, 0, 0));
               // rotate forearm to the right
               Model->rotate(-M_PI / 4.0, vec3(0, 1, 0));
               // Move to elbow joint
               Model->translate(vec3(0.6, 0, 0));
               Model->pushMatrix();
                  // Hand
                  // Translate to wrist
                  Model->translate(vec3(0.45, 0, 0));
                  // rotate hand to the right
                  Model->rotate(-M_PI / 4.0, vec3(0, 1, 0));
                  // Translate to wrist joint
                  Model->translate(vec3(0.3, 0, 0));
                  // Scale the hand
                  Model->scale(vec3(.4, .25, .25));
                  setModel(prog, Model);
                  sphere->draw(prog);
               Model->popMatrix();
               // Scale the forearm
               Model->scale(vec3(.7, .25, .25));
               setModel(prog, Model);
               sphere->draw(prog);
              Model->popMatrix();
              //non-uniform scale of armUpper arm
              Model->scale(vec3(0.9, 0.25, 0.25));
              setModel(prog, Model);
              sphere->draw(prog);
            Model->popMatrix();

            // Draw right arm
            Model->pushMatrix();
              //place at shoulder
              Model->translate(vec3(-0.6, 0.7, 0));
              //rotate shoulder joint
              Model->rotate(-fanUpperArmTheta, vec3(0, 0, 1));
              //move to shoulder joint
              Model->translate(vec3(-radius * .35, 0, 0));
              Model->pushMatrix();
               // Forearm
               // Translate to elbow
               Model->translate(vec3(-0.8, 0, 0));
               // rotate forearm
               if (fanForearmTheta < -0.05)
                  Model->rotate(0.05, vec3(0, 0, 1));
               else
                  Model->rotate(-fanForearmTheta, vec3(0, 0, 1));
               // Move to elbow joint
               Model->translate(vec3(-0.6, 0, 0));
               Model->pushMatrix();
                  // Hand
                  // Translate to wrist
                  Model->translate(vec3(-0.45, 0, 0));
                  // rotate hand
                  if (fanHandTheta < -0.05)
                     Model->rotate(0.05, vec3(0, 0, 1));
                  else
                     Model->rotate(-fanHandTheta, vec3(0, 0, 1));
                  // Translate to wrist joint
                  Model->translate(vec3(- 0.3, 0, 0));
                  // Scale the hand
                  Model->scale(vec3(.4, .25, .25));
                  setModel(prog, Model);
                  sphere->draw(prog);
               Model->popMatrix();
               // Scale the forearm
               Model->scale(vec3(.7, .25, .25));
               setModel(prog, Model);
               sphere->draw(prog);
              Model->popMatrix();
              //non-uniform scale of armUpper arm
              Model->scale(vec3(0.9, 0.25, 0.25));
              setModel(prog, Model);
              sphere->draw(prog);
            Model->popMatrix();

            // Draw right leg
            Model->pushMatrix();
               //place at hip
               Model->translate(vec3(-0.45, -0.9, 0));
               // Rotate leg to the right about the hip
               Model->rotate(-M_PI / 5.0, vec3(0, 1, 0));
               //rotate hip joint
               Model->rotate(M_PI / 2.0, vec3(0, 1, 0));
               //move to hip joint
               Model->translate(vec3(-radius * .35, 0, 0));
               Model->pushMatrix();
                  // place at knee
                  Model->translate(vec3(-1.0, 0, 0));
                  //rotate knee joint
                  Model->rotate(M_PI / 2.0, vec3(0, 0, 1));
                  //move to knee joint
                  Model->translate(vec3(-radius * .35, 0, 0));
                  Model->pushMatrix();
                     // place at ankle
                     Model->translate(vec3(-0.7, 0, 0));
                     //rotate ankle joint
                     Model->rotate(-M_PI / 2.0, vec3(0, 0, 1));
                     //move to ankle joint
                     Model->translate(vec3(-radius * .1, 0, 0));
                     Model->scale(vec3(0.35, 0.25, 0.25));
                     setModel(prog, Model);
                     sphere->draw(prog);
                  Model->popMatrix();
                  Model->scale(vec3(0.75, 0.25, 0.25));
                  setModel(prog, Model);
                  sphere->draw(prog);
               Model->popMatrix();
               Model->scale(vec3(1.10, 0.25, 0.25));
               setModel(prog, Model);
               sphere->draw(prog);
            Model->popMatrix();

            // Draw left leg
            Model->pushMatrix();
               //place at hip
               Model->translate(vec3(0.45, -0.9, 0));
               // Rotate leg to the left about the hip
               Model->rotate(M_PI / 5.0, vec3(0, 1, 0));
               //rotate hip joint
               Model->rotate(M_PI / 2.0, vec3(0, 1, 0));
               //move to hip joint
               Model->translate(vec3(-radius * .35, 0, 0));
               Model->pushMatrix();
                  // place at knee
                  Model->translate(vec3(-1.0, 0, 0));
                  //rotate knee joint
                  Model->rotate(M_PI / 2.0, vec3(0, 0, 1));
                  //move to knee joint
                  Model->translate(vec3(-radius * .35, 0, 0));
                  Model->pushMatrix();
                     // place at ankle
                     Model->translate(vec3(-0.7, 0, 0));
                     //rotate ankle joint
                     Model->rotate(-M_PI / 2.0, vec3(0, 0, 1));
                     //move to ankle joint
                     Model->translate(vec3(-radius * .1, 0, 0));
                     Model->scale(vec3(0.35, 0.25, 0.25));
                     setModel(prog, Model);
                     sphere->draw(prog);
                  Model->popMatrix();
                  Model->scale(vec3(0.75, 0.25, 0.25));
                  setModel(prog, Model);
                  sphere->draw(prog);
               Model->popMatrix();
               Model->scale(vec3(1.10, 0.25, 0.25));
               setModel(prog, Model);
               sphere->draw(prog);
            Model->popMatrix();
         // Torso is the base of the model
         Model->scale(vec3(1.1, 1.2, 1.1));
         setModel(prog, Model);
         sphere->draw(prog);
      Model->popMatrix();
   }

   void makePiece(shared_ptr<GamePiece> piece, shared_ptr<MatrixStack> Model)
   {
      float pieceScale = 0.8;
      float radius = sphereMax.x - sphereMin.x;

      SetMaterial(piece->color);

      // Draw horse
      Model->pushMatrix();
         // Torso will be the base
         Model->loadIdentity();
         Model->translate(piece->position);
         Model->rotate(piece->direction, vec3(0, 1, 0));
         Model->rotate(-M_PI / 4.0, vec3(0, 0, 1));
         Model->scale(pieceScale);
            // Draw Neck
            Model->pushMatrix();
               Model->translate(vec3(-radius * 1.7 * 0.4, radius * 0.7 * 0.15, 0));
               Model->rotate(fanHandTheta, vec3(0, 0, 1));
               Model->translate(vec3(0, radius * 0.35 + 0.1, 0));
                  // Draw Head
                  Model->pushMatrix();
                     Model->translate(vec3(0, radius * 0.38, 0));
                     Model->rotate(M_PI / 8.0, vec3(0, 0, 1));
                     Model->translate(vec3(-radius * 0.3, 0, 0));
                     Model->scale(vec3(0.7, 0.25, 0.25));
                     setModel(prog, Model);
                     sphere->draw(prog);
                  Model->popMatrix();
               Model->scale(vec3(0.25, 0.85, 0.25));
               setModel(prog, Model);
               sphere->draw(prog);
            Model->popMatrix();

            // Draw back left leg
            // Draw upper leg
            Model->pushMatrix();
               Model->translate(vec3(radius * 1.7 * 0.3, radius * 0.7 * 0.15, radius * 0.7 * 0.25));
               Model->rotate(-M_PI / 6.0, vec3(0, 0, 1));
               Model->translate(vec3(radius * 0.35 + 0.1, 0, 0));
                  //Draw lower leg
                  Model->pushMatrix();
                     Model->translate(vec3(radius * 0.7 * 0.3, 0, 0));
                     Model->rotate(M_PI / 8.0, vec3(0, 0, 1));
                     Model->translate(vec3(0, -radius * 0.35 + 0.2, 0));
                     // Draw hoof
                     Model->pushMatrix();
                        Model->translate(vec3(0, -radius * 0.7 * 0.3, 0));
                        Model->rotate(M_PI / 3.0, vec3(0, 0, 1));
                        Model->translate(vec3(-radius * 0.2 + 0.2, 0, 0));
                        Model->scale(vec3(0.25, 0.15, 0.15));
                        setModel(prog, Model);
                        sphere->draw(prog);
                     Model->popMatrix();
                     Model->scale(vec3(0.2, 0.6, 0.2));
                     setModel(prog, Model);
                     sphere->draw(prog);
                  Model->popMatrix();
               Model->scale(vec3(0.6, 0.25, 0.25));
               setModel(prog, Model);
               sphere->draw(prog);
            Model->popMatrix();

            // Draw back right leg
            // Draw upper leg
            Model->pushMatrix();
               Model->translate(vec3(radius * 1.7 * 0.3, radius * 0.7 * 0.15, -radius * 0.7 * 0.25));
               Model->rotate(-M_PI / 6.0, vec3(0, 0, 1));
               Model->translate(vec3(radius * 0.35 + 0.1, 0, 0));
                  //Draw lower leg
                  Model->pushMatrix();
                     Model->translate(vec3(radius * 0.7 * 0.3, 0, 0));
                     Model->rotate(M_PI / 8.0, vec3(0, 0, 1));
                     Model->translate(vec3(0, -radius * 0.35 + 0.2, 0));
                     // Draw hoof
                     Model->pushMatrix();
                        Model->translate(vec3(0, -radius * 0.7 * 0.3, 0));
                        Model->rotate(M_PI / 3.0, vec3(0, 0, 1));
                        Model->translate(vec3(-radius * 0.2 + 0.2, 0, 0));
                        Model->scale(vec3(0.25, 0.15, 0.15));
                        setModel(prog, Model);
                        sphere->draw(prog);
                     Model->popMatrix();
                     Model->scale(vec3(0.2, 0.6, 0.2));
                     setModel(prog, Model);
                     sphere->draw(prog);
                  Model->popMatrix();
               Model->scale(vec3(0.6, 0.25, 0.25));
               setModel(prog, Model);
               
               sphere->draw(prog);
            Model->popMatrix();

            // Draw front rigt leg
            // Draw upper leg
            Model->pushMatrix();
               Model->translate(vec3(-radius * 1.7 * 0.25, 0, -radius * 0.7 * 0.25));
               Model->rotate(piece->rightUpperLegTheta, vec3(0, 0, 1));
               Model->rotate(M_PI / 6.0, vec3(0, 0, 1));
               Model->translate(vec3(-radius * 0.35, 0, 0));
               // Draw lower leg
               Model->pushMatrix();
                  Model->translate(vec3(-radius * 0.7 * 0.35, 0, 0));
                  if (piece->rightLowerLegTheta > (M_PI / 8.0))
                     Model->rotate(M_PI / 8.0, vec3(0, 0, 1));
                  else
                     Model->rotate(piece->rightLowerLegTheta, vec3(0, 0, 1));
                  Model->translate(vec3(0, -radius * 0.35 + 0.2, 0));
                  // Draw hoof
                  Model->pushMatrix();
                     Model->translate(vec3(0, -radius * 0.7 * 0.3 - 0.1, 0));
                     if (piece->rightHoofTheta > (M_PI / 7.0))
                        Model->rotate(M_PI / 7.0, vec3(0, 0, 1));
                     else
                        Model->rotate(piece->rightHoofTheta, vec3(0, 0, 1));
                     Model->rotate(3.0 * M_PI / 4.0, vec3(0, 0, 1));
                     Model->translate(vec3(-radius * 0.2 + 0.2, 0, 0));
                     Model->scale(vec3(0.25, 0.15, 0.15));
                     setModel(prog, Model);
                     sphere->draw(prog);
                  Model->popMatrix();
                  Model->scale(vec3(0.2, 0.6, 0.2));
                  setModel(prog, Model);
                  sphere->draw(prog);
               Model->popMatrix();
               Model->scale(vec3(0.6, 0.25, 0.25));
               setModel(prog, Model);
               sphere->draw(prog);
            Model->popMatrix();

            // Draw front left leg
            // Draw upper leg
            Model->pushMatrix();
               Model->translate(vec3(-radius * 1.7 * 0.25, 0, radius * 0.7 * 0.25));
               Model->rotate(piece->leftUpperLegTheta, vec3(0, 0, 1));
               Model->rotate(M_PI / 6.0, vec3(0, 0, 1));
               Model->translate(vec3(-radius * 0.35, 0, 0));
               // Draw lower leg
               Model->pushMatrix();
                  Model->translate(vec3(-radius * 0.7 * 0.35, 0, 0));
                  if (piece->leftLowerLegTheta > (M_PI / 8.0))
                     Model->rotate(M_PI / 8.0, vec3(0, 0, 1));
                  else
                     Model->rotate(piece->leftLowerLegTheta, vec3(0, 0, 1));
                  Model->translate(vec3(0, -radius * 0.35 + 0.2, 0));
                  // Draw hoof
                  Model->pushMatrix();
                     Model->translate(vec3(0, -radius * 0.7 * 0.3 - 0.1, 0));
                     if (piece->leftHoofTheta > (M_PI / 7.0))
                        Model->rotate(M_PI / 7.0, vec3(0, 0, 1));
                     else
                        Model->rotate(piece->leftHoofTheta, vec3(0, 0, 1));
                     Model->rotate(3.0 * M_PI / 4.0, vec3(0, 0, 1));
                     Model->translate(vec3(-radius * 0.2 + 0.2, 0, 0));
                     Model->scale(vec3(0.25, 0.15, 0.15));
                     setModel(prog, Model);
                     sphere->draw(prog);
                  Model->popMatrix();
                  Model->scale(vec3(0.2, 0.6, 0.2));
                  setModel(prog, Model);
                  sphere->draw(prog);
               Model->popMatrix();
               Model->scale(vec3(0.6, 0.25, 0.25));
               setModel(prog, Model);
               sphere->draw(prog);
            Model->popMatrix();
            
            if (piece->king)
            {
               // Draw rider on top of horse, with respect to the horse
               makeRider(Model, piece->color);
            }
            
            SetMaterial(piece->color);

         Model->scale(vec3(1.7, 0.7, 0.7));
         Model->translate(vec3(((sphereMax.x + sphereMin.x) / 2.0), ((sphereMax.x + sphereMin.x) / 2.0), ((sphereMax.x + sphereMin.x) / 2.0)));
         setModel(prog, Model);
         sphere->draw(prog);
      Model->popMatrix();

      // Limit how far down the left leg can rotate
      if (!piece->leftLegUp && piece->leftUpperLegTheta <= M_PI / 4.0)
      {
         piece->leftUpperLegTheta += legIncrease;
         piece->leftLowerLegTheta -= legIncrease;
         piece->leftHoofTheta -= legIncrease;

         if (piece->leftUpperLegTheta >= M_PI / 4.0)
            piece->leftLegUp = 1;
      }

      // Limits how far up the left leg can rotate
      if(piece->leftLegUp && piece->leftUpperLegTheta >= -M_PI / 15.0)
      {
         piece->leftUpperLegTheta -= legIncrease;
         piece->leftLowerLegTheta += legIncrease;
         piece->leftHoofTheta += legIncrease;

         if (piece->leftUpperLegTheta <= -M_PI / 15.0)
            piece->leftLegUp = 0;
      }

      // Limit how far down the right leg can rotate
      if (!piece->rightLegUp && piece->rightUpperLegTheta <= M_PI / 4.0)
      {
         piece->rightUpperLegTheta += legIncrease;
         piece->rightLowerLegTheta -= legIncrease;
         piece->rightHoofTheta -= legIncrease;

         if (piece->rightUpperLegTheta >= M_PI / 4.0)
            piece->rightLegUp = 1;
      }

      // Limits how far up the right leg can rotate
      if(piece->rightLegUp && piece->rightUpperLegTheta >= -M_PI / 15.0)
      {
         piece->rightUpperLegTheta -= legIncrease;
         piece->rightLowerLegTheta += legIncrease;
         piece->rightHoofTheta += legIncrease;

         if (piece->rightUpperLegTheta <= -M_PI / 15.0)
            piece->rightLegUp = 0;
      }
   }

   void drawGameBoard(shared_ptr<MatrixStack> Model)
   {
      for (int i = 0; i < numTiles; i ++)
      {
         Model->pushMatrix();
            Model->loadIdentity();
            Model->translate(boardTiles[i]->position);
            Model->scale(boardTiles[i]->scale);
            setModel(prog, Model);
            SetMaterial(boardTiles[i]->color);
            cube->draw(prog);
         Model->popMatrix();
      }
   }

   void drawPieces(shared_ptr<MatrixStack> Model)
   {
      for (int i = 0; i < player1Pieces.size(); i ++)
         makePiece(player1Pieces[i], Model);

      for (int i = 0; i < player2Pieces.size(); i ++)
         makePiece(player2Pieces[i], Model);
   }

   // Note you could add scale later for each particle - not implemented
   void updateGeom()
   {
      glm::vec3 pos;
      glm::vec4 col;

      // go through all the particles and update the CPU buffer
      for (int i = 0; i < numP; i++)
      {
         pos = particles[i]->getPosition();
         col = particles[i]->getColor();
         points[i * 3 + 0] = pos.x;
         points[i * 3 + 1] = pos.y;
         points[i * 3 + 2] = pos.z;
         pointColors[i * 4 + 0] = col.r + col.a / 10.f;
         pointColors[i * 4 + 1] = col.g + col.g / 10.f;
         pointColors[i * 4 + 2] = col.b + col.b / 10.f;
         pointColors[i * 4 + 3] = col.a;
      }

      // update the GPU data
      CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, pointsbuffer));
      CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(points), NULL, GL_STREAM_DRAW));
      CHECKED_GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * numP * 3, points));

      CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, colorbuffer));
      CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(pointColors), NULL, GL_STREAM_DRAW));
      CHECKED_GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * numP * 4, pointColors));

      CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
   }

   /* note for first update all particles should be "reborn"
    * which will initialize their positions */
   void updateParticles()
   {
      // update the particles
      for (auto particle : particles)
      {
         particle->update(t, h, g, keyToggles);
      }

      // Sort the particles by Z
      auto temp = make_shared<MatrixStack>();
      temp->rotate(0, vec3(0, 1, 0));

      ParticleSorter sorter;
      sorter.C = temp->topMatrix();
      std::sort(particles.begin(), particles.end(), sorter);
   }

   void render() {
      // Get current frame buffer size.
      int width, height;
      glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
      glViewport(0, 0, width, height);

      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      // Clear framebuffer.
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      //Use the matrix stack for Lab 5
      float aspect = width/(float)height;

      // Create the matrix stacks - please leave these alone for now
      auto Projection = make_shared<MatrixStack>();
      //auto View = make_shared<MatrixStack>();
      mat4 View;
      auto Model = make_shared<MatrixStack>();

      // Apply perspective projection.
      Projection->pushMatrix();
      Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

      if (changePlayerCamera)
         updateCamera();

      View = lookAt(eye, lookAtPoint, up);

      //to draw the sky box bind the right shader
      cubeProg->bind();
         //set the projection matrix - can use the same one
         glUniformMatrix4fv(cubeProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
         //set the depth function to always draw the box!
         glDepthFunc(GL_LEQUAL);
         //set up view matrix to include your view transforms 
         //(your code likely will be different depending
         glUniformMatrix4fv(cubeProg->getUniform("V"), 1, GL_FALSE, value_ptr(View));
         Model->pushMatrix();
         Model->translate(vec3(0, 50 * (cube->max.y - cube->min.y) / 2.0, 0));
         Model->scale(vec3(100, 50, 100));
         //set and send model transforms - likely want a bigger cube
         glUniformMatrix4fv(cubeProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
         //bind the cube map texture
         glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
         //draw the actual cube
         skybox->draw(cubeProg);
         //set the depth test back to normal!
         glDepthFunc(GL_LESS);
         Model->popMatrix();
         //unbind the shader for the skybox
      cubeProg->unbind();

      texProg->bind();
      glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
      glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View));
      glUniform1i(texProg->getUniform("Sign"), 1);
      //Light Position
      glUniform3f(texProg->getUniform("lightPos"), eye.x, eye.y, eye.z);
      // Draw the benches at the sides of the board
      texture3->bind(texProg->getUniform("Texture0"));
      drawBenches(Model);

      // Draw dogs
      texture0->bind(texProg->getUniform("Texture0"));
      drawDogs(Model);

      texProg->unbind();

      // Draw the rest of the scene using vector normals to shade it
      prog->bind();
      glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
      glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(View));
      //Light Position
      glUniform3f(prog->getUniform("lightPos"), eye.x, eye.y, eye.z);
      glUniform3f(prog->getUniform("eye"), eye.x, eye.y, eye.z);

      // Draw the game board
      drawGameBoard(Model);

      // Draw the pieces
      if (movingPiece)
         updateGamePiece();
      drawPieces(Model);

      // Draw rocks around the board
      drawRocks(Model);

      //Draw trees around the baord
      drawTrees(Model);

      // Draw the players at the front and back of the board
      drawPlayers(Model);

      // Draw fans on the benches
      drawFans(Model);

      prog->unbind();

      Model->pushMatrix();
      Model->loadIdentity();

      if (spawnParticles)
      {
         // Draw particles
         particleProg->bind();
         updateParticles();
         updateGeom();

         particleTexture->bind(particleProg->getUniform("alphaTexture"));
         CHECKED_GL_CALL(glUniformMatrix4fv(particleProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix())));
         CHECKED_GL_CALL(glUniformMatrix4fv(particleProg->getUniform("V"), 1, GL_FALSE, value_ptr(View)));
         CHECKED_GL_CALL(glUniformMatrix4fv(particleProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix())));

         CHECKED_GL_CALL(glEnableVertexAttribArray(0));
         CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, pointsbuffer));
         CHECKED_GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0));

         CHECKED_GL_CALL(glEnableVertexAttribArray(1));
         CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, colorbuffer));
         CHECKED_GL_CALL(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*) 0));

         CHECKED_GL_CALL(glVertexAttribDivisor(0, 1));
         CHECKED_GL_CALL(glVertexAttribDivisor(1, 1));
         // Draw the points !
         CHECKED_GL_CALL(glDrawArraysInstanced(GL_POINTS, 0, 1, numP));

         CHECKED_GL_CALL(glVertexAttribDivisor(0, 0));
         CHECKED_GL_CALL(glVertexAttribDivisor(1, 0));
         CHECKED_GL_CALL(glDisableVertexAttribArray(0));
         CHECKED_GL_CALL(glDisableVertexAttribArray(1));
         particleProg->unbind();
      }
      t += h;

      // Pop matrix stacks.
      Model->popMatrix();
      Projection->popMatrix();

   }
};

int main(int argc, char *argv[])
{
   // Where the resources are loaded from
   std::string resourceDir = "../resources";

   if (argc >= 2)
   {
      resourceDir = argv[1];
   }

   Application *application = new Application();

   // Your main will always include a similar set up to establish your window
   // and GL context, etc.

   WindowManager *windowManager = new WindowManager();
   windowManager->init(920, 820);
   windowManager->setEventCallbacks(application);
   application->windowManager = windowManager;

   // This is the code that will likely change program to program as you
   // may need to initialize or set up different data and state

   application->init(resourceDir);
   application->initTex(resourceDir);
   application->initParticles();
   application->initGeom(resourceDir);   
   application->cubeMapTexture = application->createSky(resourceDir, application->faces);

   // Loop until the user closes the window.
   while (! glfwWindowShouldClose(windowManager->getHandle()))
   {
      // Render scene.
      application->render();

      // Swap front and back buffers.
      glfwSwapBuffers(windowManager->getHandle());
      // Poll for and process events.
      glfwPollEvents();
   }

   // Quit program.
   windowManager->shutdown();
   return 0;
}
