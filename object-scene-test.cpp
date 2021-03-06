////////////////////////////////////////////////////////////////////////
//
//   Based on code from Harvard University.  See AUTHORS and LICENSE files
//
////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#if __GNUG__
#   include <tr1/memory>
#endif

#include <GL/glew.h>
#ifdef __MAC__
#   include <GLUT/glut.h>
#else
#   include <GL/glut.h>
#endif

#include "cvec.h"
#include "matrix4.h"
#include "glsupport.h"
#include "geometrymaker.h"

#include "visobj.h"

using namespace std;
using namespace tr1;

// ASCII key constants for the keyboard listener
#define KEY_ESC 27
#define KEY_SPACE 32
#define KEY_R_UPPER 82
#define KEY_R_LOWER 114
#define KEY_T_UPPER 84
#define KEY_T_LOWER 116
#define KEY_D_LOWER 100
#define KEY_C_LOWER 99
#define KEY_W_LOWER 119
#define KEY_A_LOWER 97
#define KEY_S_LOWER 115
#define KEY_D_LOWER 100


// G L O B A L S ///////////////////////////////////////////////////

// --------- IMPORTANT --------------------------------------------------------
// Before you start working on this assignment, set the following variable
// properly to indicate whether you want to use OpenGL 2.x with GLSL 1.0 or
// OpenGL 3.x+ with GLSL 1.3.
//
// Set g_Gl2Compatible = true to use GLSL 1.0 and g_Gl2Compatible = false to
// use GLSL 1.3. Make sure that your machine supports the version of GLSL you
// are using. In particular, on Mac OS X currently there is no way of using
// OpenGL 3.x with GLSL 1.3 when GLUT is used.
//
// If g_Gl2Compatible=true, shaders with -gl2 suffix will be loaded.
// If g_Gl2Compatible=false, shaders with -gl3 suffix will be loaded.
// To complete the assignment you only need to edit the shader files that get
// loaded
// ----------------------------------------------------------------------------
static const bool g_Gl2Compatible = true;


static const float g_frustMinFov = 60.0;  // A minimal of 60 degree field of view
static float g_frustFovY = g_frustMinFov; // FOV in y direction (updated by updateFrustFovY)

static const float g_frustNear = -0.1;    // near plane
static const float g_frustFar = -50.0;    // far plane
static const float g_groundY = -2.0;      // y coordinate of the ground
static const float g_groundSize = 10.0;   // half the ground length

static int g_windowWidth = 1324;
static int g_windowHeight = 772;
static bool g_mouseClickDown = false;    // is the mouse button pressed
static bool g_mouseLClickButton, g_mouseRClickButton, g_mouseMClickButton;
static int g_mouseClickX, g_mouseClickY; // coordinates for mouse click event
static int g_activeShader = 0;

struct ShaderState {
  GlProgram program;

  // Handles to uniform variables
  GLint h_uLight, h_uLight2;
  GLint h_uProjMatrix;
  GLint h_uModelViewMatrix;
  GLint h_uNormalMatrix;
  GLint h_uColor;

  // Handles to vertex attributes
  GLint h_aPosition;
  GLint h_aNormal;

  ShaderState(const char* vsfn, const char* fsfn) {
    readAndCompileShader(program, vsfn, fsfn);

    const GLuint h = program; // short hand

    // Retrieve handles to uniform variables
    h_uLight = safe_glGetUniformLocation(h, "uLight");
    h_uLight2 = safe_glGetUniformLocation(h, "uLight2");
    h_uProjMatrix = safe_glGetUniformLocation(h, "uProjMatrix");
    h_uModelViewMatrix = safe_glGetUniformLocation(h, "uModelViewMatrix");
    h_uNormalMatrix = safe_glGetUniformLocation(h, "uNormalMatrix");
    h_uColor = safe_glGetUniformLocation(h, "uColor");

    // Retrieve handles to vertex attributes
    h_aPosition = safe_glGetAttribLocation(h, "aPosition");
    h_aNormal = safe_glGetAttribLocation(h, "aNormal");

    if (!g_Gl2Compatible)
      glBindFragDataLocation(h, 0, "fragColor");
    checkGlErrors();
  }

};

static const int g_numShaders = 2;
static const char * const g_shaderFiles[g_numShaders][2] = {
  {"./shaders/basic-gl3.vshader", "./shaders/diffuse-gl3.fshader"},
  {"./shaders/basic-gl3.vshader", "./shaders/solid-gl3.fshader"}
};
static const char * const g_shaderFilesGl2[g_numShaders][2] = {
  {"./shaders/basic-gl2.vshader", "./shaders/diffuse-gl2.fshader"},
  {"./shaders/basic-gl2.vshader", "./shaders/solid-gl2.fshader"}
};
static vector<shared_ptr<ShaderState> > g_shaderStates; // our global shader states

// --------- Geometry

// Macro used to obtain relative offset of a field within a struct
#define FIELD_OFFSET(StructType, field) ((GLvoid*)offsetof(StructType, field))

// A vertex with floating point position and normal
struct VertexPN {
  Cvec3f p, n;

  VertexPN() {}
  VertexPN(float x, float y, float z,
           float nx, float ny, float nz)
    : p(x,y,z), n(nx, ny, nz)
  {}

  // Define copy constructor and assignment operator from GenericVertex so we can
  // use make* functions from geometrymaker.h
  VertexPN(const GenericVertex& v) {
    *this = v;
  }

  VertexPN& operator = (const GenericVertex& v) {
    p = v.pos;
    n = v.normal;
    return *this;
  }
};

struct Geometry {
  GlBufferObject vbo, ibo;
  int vboLen, iboLen;

  Geometry(VertexPN *vtx, unsigned short *idx, int vboLen, int iboLen) {
    this->vboLen = vboLen;
    this->iboLen = iboLen;

    // Now create the VBO and IBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPN) * vboLen, vtx, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * iboLen, idx, GL_STATIC_DRAW);
  }

  void draw(const ShaderState& curSS) {
    // Enable the attributes used by our shader
    safe_glEnableVertexAttribArray(curSS.h_aPosition);
    safe_glEnableVertexAttribArray(curSS.h_aNormal);

    // bind vbo
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    safe_glVertexAttribPointer(curSS.h_aPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), FIELD_OFFSET(VertexPN, p));
    safe_glVertexAttribPointer(curSS.h_aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), FIELD_OFFSET(VertexPN, n));

    // bind ibo
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    // draw!
    glDrawElements(GL_TRIANGLES, iboLen, GL_UNSIGNED_SHORT, 0);

    // Disable the attributes used by our shader
    safe_glDisableVertexAttribArray(curSS.h_aPosition);
    safe_glDisableVertexAttribArray(curSS.h_aNormal);
  }
};


// Vertex buffer and index buffer associated with the ground and cube geometry
static shared_ptr<Geometry> g_ground, g_cube, g_sphere;

// --------- Scene

static const Matrix4 default_camera =
  Matrix4::makeTranslation(Cvec3(0.0, 0.25, 7.0));

static const Cvec3 g_light1(2.0, 3.0, 14.0), g_light2(-2, -3.0, -5.0);  // define two lights positions in world space
static Matrix4 g_eyeTransform = default_camera;

// A vector to hold all of the new pointers to VisObj instnaces
static std::vector<VisObj*> v;

static int selected_object = 0;

static const Cvec3f selected_color = Cvec3f(0, 1, 0);
static const Cvec3f default_color = Cvec3f(1, 0, 0);
static const Cvec3f black_color = Cvec3f(0, 0, 0);
static const Cvec3f white_color = Cvec3f(1, 1, 1);

// Start the slected object matrix equal to the first object in the array
static VisObj *selectedObj = NULL;

///////////////// END OF G L O B A L S //////////////////////////////////////////////////

static void initObjects(){
  // init some objects
  VisObj *toAdd = new VisObj(
  Matrix4::makeTranslation(Cvec3(0,0.5,0)),
  default_color,
  NULL);

  v.push_back(toAdd);

  VisObj *toAdd2 = new VisObj(
  Matrix4::makeScale(Cvec3(1, 1, 1))
  * Matrix4::makeTranslation(Cvec3(1, 0, 0)),
  default_color,
  toAdd);
  v.push_back(toAdd2);

  VisObj *toAdd3 = new VisObj(
  Matrix4::makeZRotation(45)
  * Matrix4::makeTranslation(Cvec3(1, -1, 0)),
  white_color,
  toAdd);
  v.push_back(toAdd3);

  VisObj *toAdd4 = new VisObj(
  Matrix4::makeScale(Cvec3(7, 7, 1))
  * Matrix4::makeTranslation(Cvec3(0, 0, -1)),
  black_color,
  NULL);
  v.push_back(toAdd4);

  VisObj *toAdd5 = new VisObj(
  Matrix4::makeScale(Cvec3(1, 1, 1))
  * Matrix4::makeTranslation(Cvec3(0, 3, -0.7))
  * Matrix4::makeZRotation(45),
  default_color,
  NULL);
  v.push_back(toAdd5);

  selectedObj = v[selected_object];
}

static void initGround() {
  // A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
  VertexPN vtx[4] = {
    VertexPN(-g_groundSize, g_groundY, -g_groundSize, 0, 1, 0),
    VertexPN(-g_groundSize, g_groundY,  g_groundSize, 0, 1, 0),
    VertexPN( g_groundSize, g_groundY,  g_groundSize, 0, 1, 0),
    VertexPN( g_groundSize, g_groundY, -g_groundSize, 0, 1, 0),
  };
  unsigned short idx[] = {0, 1, 2, 0, 2, 3};
  g_ground.reset(new Geometry(&vtx[0], &idx[0], 4, 6));
}

static void initCubes() {
  int ibLen, vbLen;
  getCubeVbIbLen(vbLen, ibLen);


  // Temporary storage for cube geometry
  vector<VertexPN> vtx(vbLen);
  vector<unsigned short> idx(ibLen);

  makeCube(1, vtx.begin(), idx.begin());
  g_cube.reset(new Geometry(&vtx[0], &idx[0], vbLen, ibLen));
}

// takes a projection matrix and send to the the shaders
static void sendProjectionMatrix(const ShaderState& curSS, const Matrix4& projMatrix) {
  GLfloat glmatrix[16];
  projMatrix.writeToColumnMajorMatrix(glmatrix); // send projection matrix
  safe_glUniformMatrix4fv(curSS.h_uProjMatrix, glmatrix);
}


// takes MVM and its normal matrix to the shaders
static void sendModelViewNormalMatrix(const ShaderState& curSS, const Matrix4& MVM, const Matrix4& NMVM) {
  GLfloat glmatrix[16];
  MVM.writeToColumnMajorMatrix(glmatrix); // send MVM
  safe_glUniformMatrix4fv(curSS.h_uModelViewMatrix, glmatrix);

  NMVM.writeToColumnMajorMatrix(glmatrix); // send NMVM
  safe_glUniformMatrix4fv(curSS.h_uNormalMatrix, glmatrix);
}

// update g_frustFovY from g_frustMinFov, g_windowWidth, and g_windowHeight
static void updateFrustFovY() {
  if (g_windowWidth >= g_windowHeight)
    g_frustFovY = g_frustMinFov;
  else {
    const double RAD_PER_DEG = 0.5 * CS175_PI/180;
    g_frustFovY = atan2(sin(g_frustMinFov * RAD_PER_DEG) * g_windowHeight / g_windowWidth, cos(g_frustMinFov * RAD_PER_DEG)) / RAD_PER_DEG;
  }
}

static Matrix4 makeProjectionMatrix() {
  return Matrix4::makeProjection(
           g_frustFovY, g_windowWidth / static_cast <double> (g_windowHeight),
           g_frustNear, g_frustFar);
}
static void drawStuff() {
  // short hand for current shader state
  const ShaderState& curSS = *g_shaderStates[g_activeShader];

  // build & send proj. matrix to vshader
  const Matrix4 projmat = makeProjectionMatrix();
  sendProjectionMatrix(curSS, projmat);

  const Matrix4 eyeTransform = g_eyeTransform;
  const Matrix4 invEyeTransform = inv(eyeTransform);

  const Cvec3 eyeLight1 = Cvec3(invEyeTransform * Cvec4(g_light1, 1));
  const Cvec3 eyeLight2 = Cvec3(invEyeTransform * Cvec4(g_light2, 1));
  safe_glUniform3f(curSS.h_uLight, eyeLight1[0], eyeLight1[1], eyeLight1[2]);
  safe_glUniform3f(curSS.h_uLight2, eyeLight2[0], eyeLight2[1], eyeLight2[2]);

  // draw ground
  // ===========
  const Matrix4 groundTransform = Matrix4();  // identity
  Matrix4 MVM = invEyeTransform * groundTransform;
  Matrix4 NMVM = normalMatrix(MVM);
  sendModelViewNormalMatrix(curSS, MVM, NMVM);
  safe_glUniform3f(curSS.h_uColor, 0.1, 0.95, 0.1); // set color
  g_ground->draw(curSS);

  // draw the chair
  for (int i = 0; i < v.size(); ++i) {
    MVM = invEyeTransform * v[i] -> getTransform();
    NMVM = normalMatrix(MVM);
    sendModelViewNormalMatrix(curSS, MVM, NMVM);
    if (v[i] != selectedObj) {
        safe_glUniform3f(curSS.h_uColor, v[i] -> getColor()[0], v[i] -> getColor()[1], v[i] -> getColor()[2]);
    } else {
      safe_glUniform3f(curSS.h_uColor, selected_color[0], selected_color[1], selected_color[2]);
    }
    g_cube->draw(curSS);
  }

}

static void display() {
  glUseProgram(g_shaderStates[g_activeShader]->program);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);                   // clear framebuffer color&depth

  drawStuff();

  glutSwapBuffers();                                    // show the back buffer (where we rendered stuff)

  checkGlErrors();
}

static void reshape(const int w, const int h) {
  g_windowWidth = w;
  g_windowHeight = h;
  glViewport(0, 0, w, h);
  cerr << "Size of window is now " << w << "x" << h << endl;
  updateFrustFovY();
  glutPostRedisplay();
}

static void motion(const int x, const int y) {
    // ...
    //
    //
    glutPostRedisplay();   // we always redraw if we changed the scene
}


static void mouse(const int button, const int state, const int x, const int y) {
  g_mouseClickX = x;
  g_mouseClickY = g_windowHeight - y - 1; // conversion from GLUT
                                          // window-coordinate-system to
                                          // OpenGL window-coordinate-system

  g_mouseLClickButton |= (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN);
  g_mouseRClickButton |= (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN);
  g_mouseMClickButton |= (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN);

  g_mouseLClickButton &= !(button == GLUT_LEFT_BUTTON && state == GLUT_UP);
  g_mouseRClickButton &= !(button == GLUT_RIGHT_BUTTON && state == GLUT_UP);
  g_mouseMClickButton &= !(button == GLUT_MIDDLE_BUTTON && state == GLUT_UP);

  g_mouseClickDown = g_mouseLClickButton || g_mouseRClickButton || g_mouseMClickButton;
  glutPostRedisplay();
}

// Camera pan with the arrow keys
static void special_keyboard(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP: // pan camera up
            cout << "Up key pressed\n";
            g_eyeTransform =
              g_eyeTransform * Matrix4::makeXRotation(10);
            break;
        case GLUT_KEY_DOWN: // pan camera down
            cout << "Down key pressed\n";
            g_eyeTransform =
              g_eyeTransform * Matrix4::makeXRotation(-10);
            break;
        case GLUT_KEY_LEFT: // pan camera left
            cout << "Left key pressed\n";
            g_eyeTransform =
              g_eyeTransform * Matrix4::makeYRotation(10);
            break;
        case GLUT_KEY_RIGHT: // pan camera right
            cout << "Right key pressed\n";
            g_eyeTransform =
              g_eyeTransform * Matrix4::makeYRotation(-10);
            break;
    }
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
  switch (key) {
    case KEY_ESC:
        cout << "ESC key pressed, exiting...\n";
        exit(0);
    case KEY_SPACE: // cycle through selected object
        if (selected_object == v.size()-1) {
          selected_object = 0;
        } else {
          selected_object ++;
        }
        selectedObj = v[selected_object];
        //selectedObj -> setColor(selected_color);
        cout << "The object selected is: " << selected_object << "\n";
        break;
    case KEY_R_UPPER: // Rotate positively
        cout << "R key pressed\n";
        selectedObj -> setTransform(Matrix4::makeZRotation(45));
        break;
    case KEY_R_LOWER: // Rotate negatively
        cout << "r key pressed\n";
        selectedObj -> setTransform(Matrix4::makeZRotation(-45));
        break;
    case KEY_C_LOWER:
        cout << "Resetting camera...\n";
        g_eyeTransform = default_camera;
        break;
    case KEY_T_UPPER:

        break;
    case KEY_W_LOWER:
        cout << "w key pressed\n";
        g_eyeTransform =
        g_eyeTransform * Matrix4::makeTranslation(Cvec3(0, 0, -1));
        break;
    case KEY_A_LOWER:
        cout << "a key pressed\n";
        g_eyeTransform =
        g_eyeTransform * Matrix4::makeTranslation(Cvec3(-1, 0, 0));
        break;
    case KEY_S_LOWER:
        cout << "w key pressed\n";
          g_eyeTransform =
          g_eyeTransform * Matrix4::makeTranslation(Cvec3(0, 0, 1));
        break;
    case KEY_D_LOWER:
        cout << "d key pressed\n";
        g_eyeTransform =
        g_eyeTransform * Matrix4::makeTranslation(Cvec3(1, 0, 0));
        break;
  }
  glutPostRedisplay();
}

static void initGlutState(int argc, char * argv[]) {
  glutInit(&argc, argv);                                  // initialize Glut based on cmd-line args
  glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);  //  RGBA pixel channels and double buffering
  glutInitWindowSize(g_windowWidth, g_windowHeight);      // create a window
  glutCreateWindow("Making a Scene");                     // title the window

  glutDisplayFunc(display);                               // display rendering callback
  glutReshapeFunc(reshape);                               // window reshape callback
  glutMotionFunc(motion);                                 // mouse movement callback
  glutMouseFunc(mouse);                                   // mouse click callback
  glutSpecialFunc(special_keyboard);
  glutKeyboardFunc(keyboard);
}

static void initGLState() {
  glClearColor(128./255., 200./255., 255./255., 0.);
  glClearDepth(0.);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_GREATER);
  glReadBuffer(GL_BACK);
  if (!g_Gl2Compatible)
    glEnable(GL_FRAMEBUFFER_SRGB);
}

static void initShaders() {
  g_shaderStates.resize(g_numShaders);
  for (int i = 0; i < g_numShaders; ++i) {
    if (g_Gl2Compatible)
      g_shaderStates[i].reset(new ShaderState(g_shaderFilesGl2[i][0], g_shaderFilesGl2[i][1]));
    else
      g_shaderStates[i].reset(new ShaderState(g_shaderFiles[i][0], g_shaderFiles[i][1]));
  }
}

static void initGeometry() {
  initGround();
  initCubes();
}

int main(int argc, char * argv[]) {
  try {
    initGlutState(argc,argv);

    glewInit(); // load the OpenGL extensions

    initGLState();
    initShaders();
    initGeometry();
    initObjects();
    glutMainLoop();
    return 0;
  }
  catch (const runtime_error& e) {
    cout << "Exception caught: " << e.what() << endl;
    return -1;
  }
}
