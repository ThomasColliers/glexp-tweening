#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <boost/filesystem.hpp>
#include <GL/glew.h>
#include <GL/glfw.h>
#include <claw/tween/single_tweener.hpp>
#include <claw/tween/easing/easing_linear.hpp>
#include <claw/tween/easing/easing_back.hpp>
#include <claw/tween/easing/easing_quad.hpp>

#include "ShaderManager.h"
#include "TriangleBatch.h"
#include "Batch.h"
#include "GeometryFactory.h"
#include "MatrixStack.h"
#include "Frustum.h"
#include "TransformPipeline.h"
#include "Math3D.h"
#include "UniformManager.h"
#include "TextureManager.h"

using namespace gliby;
using namespace Math3D;

int window_w, window_h;
// shader stuff
ShaderManager* shaderManager;
GLuint diffuseShader;
// transformation stuff
Frame cameraFrame;
Frustum viewFrustum;
TransformPipeline transformPipeline;
MatrixStack modelViewMatrix;
MatrixStack projectionMatrix;
// objects
Geometry* plane;
Geometry* cube;
// positions
double zs[3];
claw::tween::single_tweener* tweens[3];
// texture
TextureManager textureManager;
// uniform locations
UniformManager* uniformManager;

// TODO: animate them in different ways (libclaw)
// TODO: use a catmull-rom animation on the camera
// TODO: look at camera class I bookmarked
// TODO: set up a UI to change the camera control points

void setupContext(void){
    // general state
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // transform pipeline
    transformPipeline.setMatrixStacks(modelViewMatrix,projectionMatrix);
    viewFrustum.setPerspective(35.0f, float(window_w)/float(window_h), 1.0f, 500.0f);
    projectionMatrix.loadMatrix(viewFrustum.getProjectionMatrix());
    modelViewMatrix.loadIdentity();
    // move camera back
    cameraFrame.setOrigin(0.0f,5.0f,20.0f);
    cameraFrame.lookAt(0.0f,0.0f,0.0f);
    //cameraFrame.moveForward(-20.0f);

    // shader setup
    const char* searchPath[] = {"./shaders/","/home/ego/projects/personal/gliby/shaders/"};
    shaderManager = new ShaderManager(sizeof(searchPath)/sizeof(char*),searchPath);
    ShaderAttribute attrs[] = {{0,"vVertex"},{2,"vNormal"},{3,"vTexCoord"}};
    diffuseShader = shaderManager->buildShaderPair("diffuse_specular.vp","diffuse_specular.fp",sizeof(attrs)/sizeof(ShaderAttribute),attrs);
    const char* uniforms[] = {"mvpMatrix","mvMatrix","normalMatrix","lightPosition","ambientColor","diffuseColor","textureUnit","specularColor","shinyness"};
    uniformManager = new UniformManager(diffuseShader,sizeof(uniforms)/sizeof(char*),uniforms);

    // setup geometry
    plane = &GeometryFactory::plane(50.0f,50.0f,0.0f,0.0f,0.0f);
    cube = &GeometryFactory::cube(1.0f);

    zs[0] = 0.0f; zs[1] = 0.0f; zs[2] = 0.0f;
    // start up the tweens
    tweens[0] = new claw::tween::single_tweener(zs[0], 10.0f, 10, claw::tween::easing_linear::ease_in_out);
    tweens[1] = new claw::tween::single_tweener(zs[1], 10.0f, 10, claw::tween::easing_back::ease_out);
    tweens[2] = new claw::tween::single_tweener(zs[2], 10.0f, 10, claw::tween::easing_quad::ease_in_out);
    
    // setup textures
    const char* textures[] = {"textures/pavement.jpg","textures/box.jpg"};
    textureManager.loadTextures(sizeof(textures)/sizeof(char*),textures,GL_TEXTURE_2D,GL_TEXTURE0);
}

void render(void){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // setup camera
    Matrix44f mCamera;
    cameraFrame.getCameraMatrix(mCamera);
    modelViewMatrix.pushMatrix();
    modelViewMatrix.multMatrix(mCamera);

    // floor
    modelViewMatrix.pushMatrix();
    Matrix44f floorm;
    rotationMatrix44(floorm, -PI/2, 1.0f, 0.0f, 0.0f);
    modelViewMatrix.multMatrix(floorm);
    glUseProgram(diffuseShader);
    glBindTexture(GL_TEXTURE_2D, textureManager.get("textures/pavement.jpg"));
    glUniformMatrix4fv(uniformManager->get("mvpMatrix"),1,GL_FALSE,transformPipeline.getModelViewProjectionMatrix());
    glUniformMatrix4fv(uniformManager->get("mvMatrix"),1,GL_FALSE,transformPipeline.getModelViewMatrix());
    glUniformMatrix3fv(uniformManager->get("normalMatrix"),1,GL_FALSE,transformPipeline.getNormalMatrix());
    GLfloat lightPosition[] = {2.0f,2.0f,2.0f};
    glUniform3fv(uniformManager->get("lightPosition"),1,lightPosition);
    GLfloat ambientColor[] = {0.2f, 0.2f, 0.2f, 1.0f};
    glUniform4fv(uniformManager->get("ambientColor"),1,ambientColor);
    GLfloat diffuseColor[] = {0.7f, 0.7f, 0.7f, 1.0f};
    glUniform4fv(uniformManager->get("diffuseColor"),1,diffuseColor);
    GLfloat specularColor[] = {0.8f, 0.8f, 0.8f, 1.0f};
    glUniform4fv(uniformManager->get("specularColor"),1,specularColor);
    glUniform1f(uniformManager->get("shinyness"),128.0f);
    glUniform1i(uniformManager->get("textureUnit"),0);
    plane->draw();
    modelViewMatrix.popMatrix();

    // objects
    glBindTexture(GL_TEXTURE_2D, textureManager.get("textures/box.jpg"));
    for(int i = 0; i < 3; i++){
        tweens[i]->update(0.1);
        modelViewMatrix.pushMatrix();
        Matrix44f transl;
        translationMatrix(transl, -2.1f+i*2.1f, 1.0f, zs[i]);
        modelViewMatrix.multMatrix(transl);
        glUniformMatrix4fv(uniformManager->get("mvpMatrix"),1,GL_FALSE,transformPipeline.getModelViewProjectionMatrix());
        glUniformMatrix4fv(uniformManager->get("mvMatrix"),1,GL_FALSE,transformPipeline.getModelViewMatrix());
        glUniformMatrix3fv(uniformManager->get("normalMatrix"),1,GL_FALSE,transformPipeline.getNormalMatrix());
        cube->draw();
        modelViewMatrix.popMatrix();
    }

    modelViewMatrix.popMatrix();
}

void keyCallback(int id, int state){
    if(id == GLFW_KEY_ESC && state == GLFW_RELEASE){
        glfwCloseWindow();
    }
}

void resizeCallback(int width, int height){
    window_w = width;
    window_h = height;
    glViewport(0,0,window_w,window_h);
    viewFrustum.setPerspective(35.0f, float(window_w)/float(window_h),1.0f,500.0f);
    projectionMatrix.loadMatrix(viewFrustum.getProjectionMatrix());
}

int main(int argc, char **argv){
    // force vsync on
    putenv((char*) "__GL_SYNC_TO_VBLANK=1");

    // init glfw and window
    if(!glfwInit()){
        std::cerr << "GLFW init failed" << std::endl;
        return -1;
    }
    glfwSwapInterval(1);
    glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 8);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 4);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
    glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if(!glfwOpenWindow(800,600,8,8,8,0,24,0,GLFW_WINDOW)){
        std::cerr << "GLFW window opening failed" << std::endl;
        return -1;
    }
    window_w = 800; window_h = 600;
    glfwSetKeyCallback(keyCallback);
    glfwSetWindowSizeCallback(resizeCallback);
    glfwSetWindowTitle("gltest");

    // init glew
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if(err != GLEW_OK){
        std::cerr << "Glew error: " << glewGetErrorString(err) << std::endl;
    }

    // setup context
    setupContext();

    // main loop
    while(glfwGetWindowParam(GLFW_OPENED)){
        render();
        glfwSwapBuffers();
    }

    glfwTerminate();
}
