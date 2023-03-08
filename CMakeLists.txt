cmake_minimum_required(VERSION 2.8.12.1)

PROJECT(project3A)

SET(OpenGL_GL_PREFERENCE LEGACY)

find_package(OpenGL REQUIRED)
find_package(glfw3  REQUIRED)
find_package(GLEW   REQUIRED)
find_package(glm   REQUIRED)

message("GL include dir: ${OPENGL_INCLUDE_DIR}")
message("GL link libraries: ${OPENGL_gl_LIBRARY}")

add_executable(project3A project3A.cxx)
if(APPLE)
  target_link_libraries(project3A ${OPENGL_gl_LIBRARY} GLEW::glew_s glfw)
else()
  target_link_libraries(project3A ${OPENGL_gl_LIBRARY} GLEW glfw)
endif()
