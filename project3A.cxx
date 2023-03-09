/* project3A.cxx
 * Author: Nate Koch
 * Date: 3/21/23
 */

#include <GL/glew.h>    // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library
#include <stdio.h>
#include <stdlib.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "proj2A_data.h"

unsigned char *
GetColorMap(int &textureSize)
{
    unsigned char controlPts[8][3] =
    {
        {  71,  71, 219 },
        {   0,   0,  91 },
        {   0, 255, 255 },
        {   0, 127,   0 },
        { 255, 255,   0 },
        { 255,  96,   0 },
        { 107,   0,   0 },
        { 224,  76,  76 },
    };
    textureSize = 256;
    unsigned char *ptr = new unsigned char[textureSize*3];
    int nControlPts = 8;
    double amountPerPair = ((double)textureSize-1.0)/(nControlPts-1.0);
    for (int i = 0 ; i < textureSize ; i++)
    {
        int lowerControlPt = (int)(i/amountPerPair);
        int upperControlPt = lowerControlPt+1;
        if (upperControlPt >= nControlPts)
            upperControlPt = lowerControlPt; // happens for i == textureSize-1

        double proportion = (i/amountPerPair)-lowerControlPt;
        for (int j = 0 ; j < 3 ; j++)
            ptr[3*i+j] = controlPts[lowerControlPt][j]
                       + proportion*(controlPts[upperControlPt][j]-
                                     controlPts[lowerControlPt][j]);
    }

    return ptr;
}

unsigned char *
GetTigerStripes(int &textureSize)
{
    textureSize = 2048;
    unsigned char *ptr = new unsigned char[textureSize];
    int numStripes = 20;
    int valsPerStripe = textureSize/numStripes;
    for (int i = 0 ; i < numStripes ; i++)
    {
        for (int j = 0 ; j < valsPerStripe ; j++)
        {
           int idx = i*valsPerStripe+j;
           if (j < valsPerStripe / 3)
               ptr[idx] = 152;
           else
               ptr[idx] = 255;
        }
    }
    for (int i = numStripes*valsPerStripe ; i < textureSize ; i++)
    {
        ptr[i] = 0;
    }
    return ptr;
}

void _print_shader_info_log(GLuint shader_index) {
  int max_len = 2048;
  int actual_len = 0;
  char shader_log[2048];
  glGetShaderInfoLog(shader_index, max_len, &actual_len, shader_log);
  printf("shader info log for GL index %u:\n%s\n", shader_index, shader_log);
}

GLuint SetupDataForRendering()
{
  // Add data to VBOs and VAO for phase 3 here
  GLuint points_vbo = 0;
  glGenBuffers(1, &points_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(tri_points), tri_points, GL_STATIC_DRAW);

  GLuint data_vbo = 0;
  glGenBuffers(1, &data_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, data_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(tri_data), tri_data, GL_STATIC_DRAW);

  GLuint normals_vbo = 0;
  glGenBuffers(1, &normals_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, normals_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(tri_normals), tri_normals, GL_STATIC_DRAW);

  GLuint indices_vbo = 0;
  glGenBuffers(1, &indices_vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tri_indices)/sizeof(int) * sizeof(GLuint), tri_indices, GL_STATIC_DRAW);

  // GL Textures
  
  GLuint color_texture = 0;
  int num = 256;
  const unsigned char * data = GetColorMap(num);
  glGenTextures(1, &color_texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_1D, color_texture);
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, num, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_1D);
  
  //TODO tiger texture
  
  GLuint vao = 0;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  
  glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  
  glBindBuffer(GL_ARRAY_BUFFER, data_vbo);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, NULL);
  
  glBindBuffer(GL_ARRAY_BUFFER, normals_vbo);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);

  return vao;
}

const char *VertexShader =
  "#version 400\n"
  "layout (location = 0) in vec3 vertex_position;\n"
  "layout (location = 1) in float vertex_data;\n"
  "layout (location = 2) in vec3 vertex_normal;\n"
  "uniform mat4 MVP;\n"
  "uniform vec3 cameraloc;  // Camera position \n"
  "uniform vec3 lightdir;   // Lighting direction \n"
  "uniform vec4 lightcoeff; // Lighting coeff, Ka, Kd, Ks, alpha\n"
  "vec3 reflection, viewDir;\n"
  "float data;\n"
  "float LdotN, diffuse, RdotV, specular;\n"
  "out float tex_coord;\n"
  "out float shading_amount;\n"
  "void main() {\n"
  "  vec4 position = vec4(vertex_position, 1.0);\n"
  "  gl_Position = MVP*position;\n"
  "  data = vertex_data;\n"
  "  tex_coord = (data-1)/5.0;\n"
  // Assign shading_amount a value by calculating phong shading
  // camaraloc  : is the location of the camera
  // lightdir   : is the direction of the light
  // lightcoeff : represents a vec4(Ka, Kd, Ks, alpha) from LightingParams of 1F
  "  LdotN = dot(lightdir, vertex_normal);\n"
  "  diffuse = lightcoeff[1] * max(0.0, LdotN);\n"
  "  reflection = 2 * LdotN * vertex_normal - lightdir;\n"
  "  viewDir = cameraloc-vertex_position;\n"
  "  RdotV = dot(normalize(reflection), normalize(viewDir));\n"
  "  specular = abs(lightcoeff[2] * pow(max(0.0, RdotV), lightcoeff[3]));\n"
  "  shading_amount = lightcoeff[0] + diffuse + specular;\n"
  "}\n";

const char *FragmentShader =
  "#version 400\n"
  "in float tex_coord;\n"
  "in float shading_amount;\n"
  "uniform sampler1D color_texture;\n"
  "out vec4 frag_color;\n"
  "void main() {\n"
  "  frag_color = texture(color_texture, tex_coord) * shading_amount;\n"
  "}\n";

int main() {
  // start GL context and O/S window using the GLFW helper library
  if (!glfwInit()) {
    fprintf(stderr, "ERROR: could not start GLFW3\n");
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(700, 700, "1D Textures", NULL, NULL);
  if (!window) {
    fprintf(stderr, "ERROR: could not open window with GLFW3\n");
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);
  // start GLEW extension handler
  glewExperimental = GL_TRUE;
  glewInit();

  // get version info
  const GLubyte *renderer = glGetString(GL_RENDERER); // get renderer string
  const GLubyte *version = glGetString(GL_VERSION);   // version as a string
  printf("Renderer: %s\n", renderer);
  printf("OpenGL version supported %s\n", version);

  // tell GL to only draw onto a pixel if the shape is closer to the viewer
  glEnable(GL_DEPTH_TEST); // enable depth-testing
  glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

  GLuint vao = 0;
  vao = SetupDataForRendering();
  const char* vertex_shader = VertexShader;
  const char* fragment_shader = FragmentShader;

  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vertex_shader, NULL);
  glCompileShader(vs);
  int params = -1;
  glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
  if (GL_TRUE != params) {
    fprintf(stderr, "ERROR: GL shader index %i did not compile\n", vs);
    _print_shader_info_log(vs);
    exit(EXIT_FAILURE);
  }

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fragment_shader, NULL);
  glCompileShader(fs);
  glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
  if (GL_TRUE != params) {
    fprintf(stderr, "ERROR: GL shader index %i did not compile\n", fs);
    _print_shader_info_log(fs);
    exit(EXIT_FAILURE);
  }

  GLuint shader_programme = glCreateProgram();
  glAttachShader(shader_programme, fs);
  glAttachShader(shader_programme, vs);
  glLinkProgram(shader_programme);

  glUseProgram(shader_programme);
  
  // Code block for camera transforms
  // Projection matrix : 30° Field of View
  // display size  : 1000x1000
  // display range : 5 unit <-> 200 units
  glm::mat4 Projection = glm::perspective(
      glm::radians(30.0f), (float)1000 / (float)1000,  40.0f, 60.0f);
  glm::vec3 camera(0, 40, 40);
  glm::vec3 origin(0, 0, 0);
  glm::vec3 up(0, 1, 0);
  // Camera matrix
  glm::mat4 View = glm::lookAt(
    camera, // Camera in world space
    origin, // looks at the origin
    up      // and the head is up
  );
  // Model matrix : an identity matrix (model will be at the origin)
  glm::mat4 Model = glm::mat4(1.0f);
  // Our ModelViewProjection : multiplication of our 3 matrices
  glm::mat4 mvp = Projection * View * Model;

  // Get a handle for our "MVP" uniform
  // Only during the initialisation
  GLuint mvploc = glGetUniformLocation(shader_programme, "MVP");
  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // This is done in the main loop since each model will have a different MVP matrix
  // (At least for the M part)
  glUniformMatrix4fv(mvploc, 1, GL_FALSE, &mvp[0][0]);

  // Code block for shading parameters
  GLuint camloc = glGetUniformLocation(shader_programme, "cameraloc");
  glUniform3fv(camloc, 1, &camera[0]);
  glm::vec3 lightdir = glm::normalize(camera - origin);   // Direction of light
  GLuint ldirloc = glGetUniformLocation(shader_programme, "lightdir");
  glUniform3fv(ldirloc, 1, &lightdir[0]);
  glm::vec4 lightcoeff(0.3, 0.7, 2.8, 50.5); // Lighting coeff, Ka, Kd, Ks, alpha
  GLuint lcoeloc = glGetUniformLocation(shader_programme, "lightcoeff");
  glUniform4fv(lcoeloc, 1, &lightcoeff[0]);

  // Code block for textures
  GLuint colorTextureLocation = glGetUniformLocation(shader_programme, "color_texture");
  glUniform1i(colorTextureLocation, 0);

  while (!glfwWindowShouldClose(window)) {
    // wipe the drawing surface clear
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(vao);
    // Draw triangles
    glDrawElements( GL_TRIANGLES, 77535, GL_UNSIGNED_INT, NULL );

    // update other events like input handling
    glfwPollEvents();
    // put the stuff we've been drawing onto the display
    glfwSwapBuffers(window);
  }

  // close GL context and any other GLFW resources
  glfwTerminate();
  return 0;
}
