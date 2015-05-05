#include "GameController.h"

#include <glm/gtx/vector_angle.hpp>

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;
using namespace sge;

void FullScreenRenderTarget::create(int width, int height)
{
    printf("created %d x %d\n", width, height);
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &zBufferRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, zBufferRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
                          width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, zBufferRbo);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    SDL_assert(status == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FullScreenRenderTarget::destroy()
{
    if (textureId)
    {
        glDeleteTextures(1, &textureId);
        textureId = 0;
    }
    if (zBufferRbo)
    {
        glDeleteRenderbuffers(1, &zBufferRbo);
        zBufferRbo = 0;
    }
    
    if (fbo)
    {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
}

void FullScreenRenderTarget::renderScreenQuad()
{
    glBindTexture(GL_TEXTURE_2D, textureId);
    glEnable(GL_TEXTURE_2D);
    glLoadIdentity();
    
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-1, -1);
    glTexCoord2f(0, 1); glVertex2f(-1, +1);
    glTexCoord2f(1, 1); glVertex2f(+1, +1);
    glTexCoord2f(1, 0); glVertex2f(+1, -1);
    glEnd();
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

// gets 'shortest rotation' (without 'spinning')
mat4 getRotationMatrix(vec3 from, vec3 to)
{
    mat4 rotation;
    
    vec3 axis = glm::cross(from, to);
    if (glm::length(axis) < 1e-3)
        return rotation;
    
    ftype angle = glm::orientedAngle(from, to, axis);
    rotation = glm::rotate(rotation, angle, axis);
    return rotation;
}

void GameController::initializeGraphics(int width, int height)
{   
    blurBufferA.create(width, height);
    blurBufferB.create(width, height);
    
    meshCollection.loadMeshes();
        
    for (int z = 5; z >= -20; z -= 3)
    {
        for (int t = 0; t < 2; t++)
            worldContainer.addPositionedMesh(meshCollection.cubeMesh, vec3((t * 2 - 1) * 3, 0, z));
        
        //cubePositions.push_back(vec3(-3, 0, z));
        //cubePositions.push_back(vec3(3, 0, z));
    }
    
    worldContainer.addPositionedMesh(meshCollection.plane, vec3(0, 0, 0));
    
    //loadColladaMeshNew("resources/dae-inspection.dae");
    //newMesh = loadColladaMeshNew("resources/hyena-decimated.dae");
    //newMesh = loadColladaMeshNew("resources/dae-inspection.dae");
    newMesh = loadColladaMeshNew("resources/animated-cube.dae");
    newMesh = loadColladaMeshNew("resources/hyena-new-pro.dae");
    newMesh = loadColladaMeshNew("resources/hyena-decimated.dae");
    newMesh = loadColladaMeshNew("resources/astroboy.dae");
    //for (int i = 0; i < 10; i++)
    
    //loadedMesh = loadColladaMesh("resources/Hyena_Rig_Final.dae");
    
    /*int x = -1;
    glGetIntegerv(0x9048, &x);
    printf("%d\n", x);*/    
    
    //loadedMesh = loadColladaMesh("resources/nice-scene.dae");
    GLPositionedMesh positioned;
    positioned.baseMesh = &loadedMesh;
    //positioned.modelMatrix = glm::scale(positioned.modelMatrix, vec3(0.2, 0.2, 0.2));
    positioned.modelMatrix = glm::translate(positioned.modelMatrix, vec3(0, 2, -3));
    
#if 0
    // old Collada loader
    worldContainer.positionedMeshes.push_back(positioned);
#endif
    
#define testStairs(name, dx, dy, x) \
        static GLSimpleMesh name = meshCollection.createStaircase(dx, 0, 0, dy, (int)(5.0 / dy)); \
        worldContainer.addPositionedMesh(name, vec3(x, 0, -5));
    
#if 0
    testStairs(stairs0, 0.35, 0.35, -1);
    testStairs(stairs1, 0.27, 0.27, 0);
    testStairs(stairs2, 0.2, 0.2, 1);
    testStairs(stairs3, 0.1, 0.1, 2);
    testStairs(stairs4, 0.5, 0.27, -2);
#endif
    
    //static GLSimpleMesh stairs = meshCollection.createStaircase(0.27, 0, 0, 0.27, 10);
    //worldContainer.addPositionedMesh(stairs, vec3(0, 0, -1));
    
    mat4 currentModelview;
    vec3 viewDirection(0, 0, -5);
    
#if 0
    for (int i = 0; i < 2; i++)
    {
        GLSimpleMesh& simpleMesh = meshCollection.staircase;
        
        GLPositionedMesh positioned;
        positioned.baseMesh = &simpleMesh;
        positioned.modelMatrix = currentModelview;
        
        worldContainer.positionedMeshes.push_back(positioned);
        
        MeshMarker continuation = simpleMesh.getObligatoryMarker("continuation-point");
        
        currentModelview = glm::translate(currentModelview, continuation.position);
        currentModelview = currentModelview * getRotationMatrix(viewDirection, continuation.direction);
    }
#endif

    currentWidth = width;
    currentHeight = height;
    
    cameraVector = vec3(0, 0, -1);
    player.position = vec3(0, 0.5, 0);
    player.enableSmoothing = true;
    player.ySmooth = player.position.y;
    player.radius = 0.2;
    //player.radius = 0.05;
    player.currentWalkSpeed = 1;
    updatePlayerDirection();
    
    reloadShaders();
    
    //physicsEngine.physicalBodies.push_back(&player);

	glViewport(0, 0, width, height);
	glClearColor(0, 0, 0, 0);
    
	glClearDepth(1.0);
	glDepthFunc(GL_LESS);
	glShadeModel(GL_SMOOTH);
    
	glMatrixMode(GL_MODELVIEW);
}

void GameController::reloadShaders()
{
    if (vertexShader)
    {
        glDeleteShader(vertexShader);
        vertexShader = 0;
    }
    
    if (fragmentShader)
    {
        glDeleteShader(fragmentShader);
        fragmentShader = 0;
    }
    
    if (shaderProgram)
    {
        glUseProgram(0);
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    
    string defineString = "";
    for (auto& it: shaderDefines)
        defineString += "#define " + it + "\n";
    
    vertexShader = loadGlShader("resources/vertex-shader.glsl", GL_VERTEX_SHADER, defineString.c_str());
    fragmentShader = loadGlShader("resources/fragment-shader.glsl", GL_FRAGMENT_SHADER, defineString.c_str());
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    GLint linkOk;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkOk);
    if (!linkOk)
        critical_error("Failed to link shader program: %s", getShaderOrProgramLog(shaderProgram).c_str());
}

void GameController::updatePlayerDirection()
{
    player.direction = glm::normalize(vec3(cameraVector.x, 0, cameraVector.z));
}

void GameController::changeWindowSize(int newWidth, int newHeight)
{
	glViewport(0, 0, newWidth, newHeight);
    
    currentWidth = newWidth;
    currentHeight = newHeight;
    
    blurBufferA.destroy();
    blurBufferB.destroy();
    
    blurBufferA.create(newWidth, newHeight);
    blurBufferB.create(newWidth, newHeight);
}

void GameController::simulateWorld(ftype msPassed)
{
    currentTime += msPassed / 1000.0;
    
    ftype speed = 1;
    ftype dt = msPassed / 1000.0;
 
    updatePlayerDirection();
    
    if (pressedKeys.count(SDLK_a))
        player.easyMove(MovementType::STRAFE_LEFT, speed, dt);
    
    if (pressedKeys.count(SDLK_d))
        player.easyMove(MovementType::STRAFE_RIGHT, speed, dt);
    
    if (pressedKeys.count(SDLK_w))
        player.easyMove(MovementType::FORWARD, speed, dt);
    
    if (pressedKeys.count(SDLK_s))
        player.easyMove(MovementType::BACKWARD, speed, dt);
    
    if (pressedKeys.count(SDLK_SPACE))
    {
        player.yAccel = 0.1;
        //player.position += vec3(0, 1, 0);
    }
    
    if (pressedKeys.count(SDLK_j))
    {
        if (abs(player.yAccel) < 0.01)
            player.yAccel = 0.1;
    }
    
    player.currentWalkSpeed = 1;
    worldContainer.processPhysics(player, dt);
    //physicsEngine.processPhysics();
}

void GameController::keyPressed(SDL_Keycode keycode)
{
    pressedKeys.insert(keycode);
}

void GameController::keyReleased(SDL_Keycode keycode)
{
    pressedKeys.erase(keycode);
    
    if (keycode == SDLK_q)
        firstPersonMode = !firstPersonMode;
    
    if (keycode == SDLK_o)
    {
        wireframeMode = !wireframeMode;
        
        if (wireframeMode)
            enableSimpleBlur = false;
    }
    
    if (keycode == SDLK_p)
        physicsDebugMode = !physicsDebugMode;
    
    if (keycode == SDLK_f)
    {
        fogEnabled = !fogEnabled;
        
        if (fogEnabled)
            shaderDefines.insert("FOG");
        else
            shaderDefines.erase("FOG");
        
        reloadShaders();
    }
    
    if (keycode == SDLK_y)
    {
        player.enableSmoothing = !player.enableSmoothing;
    }
    
    if (keycode == SDLK_r)
    {
        player.position = vec3(0, 0.5, 0);
        cameraVector = vec3(0, 0, -1);
        player.yAccel = 0;
        player.ySmooth = player.position.y;
    }
    
    if (keycode == SDLK_b)
    {
        enableSimpleBlur = !enableSimpleBlur;
        
        if (enableSimpleBlur)
            wireframeMode = false;
    }
    
    if (keycode == SDLK_g)
    {
        newMesh.renderSkeleton = !newMesh.renderSkeleton;
    }
}

void GameController::relativeMouseMotion(int dx, int dy)
{
    ftype rdx = dx / 10000.0, rdy = dy / 10000.0;
    
    mat4 rotation;
    rotation = glm::rotate(rotation, rdx, vec3(0, 1, 0));
    cameraVector = glm::normalize(cameraVector * mat3(rotation));
    
    rotation = mat4();
    rotation = glm::rotate(rotation, rdy, glm::cross(cameraVector, vec3(0, 1, 0)));
    
    vec3 newCameraVector = glm::normalize(cameraVector * mat3(rotation));
    
    if (abs(newCameraVector.y) < 0.98)
    {
        cameraVector = newCameraVector;
        //newCameraVector.y = (newCameraVector.y > 0 ? 1 : -1) * 0.95;
        //newCameraVector = glm::normalize(newCameraVector);
    }
}

void GameController::renderFrame()
{   
    ftype angle = currentTime / 5.0;
    
    projectionMatrix = mat4();
    viewMatrix = mat4();
    modelMatrix = mat4();
    
    vec3 playerHeightVector(0, 1.7 - player.radius, 0);
    
    // make vertical FOV fixed
    ftype aspectRatio = currentWidth / (ftype)currentHeight;
    
    projectionMatrix = glm::perspective(45.0 / 180.0 * M_PI, aspectRatio, 0.1, 1e3);
    
    playerHeightVector.y += player.ySmooth - player.position.y;
    
    if (!firstPersonMode)
    {
        viewMatrix = glm::lookAt(player.position + playerHeightVector,
                                 player.position + playerHeightVector + cameraVector, vec3(0, 1, 0));
    }
    else
    {
        viewMatrix = glm::lookAt(player.position + playerHeightVector - cameraVector * 4.0,
                                 player.position + playerHeightVector, vec3(0, 1, 0));
    }
    
    modelMatrix = glm::translate(modelMatrix, vec3(0, 0, -5));
    modelMatrix = glm::rotate(modelMatrix, angle, vec3(sin(angle), sin(angle + 2 * M_PI / 3), sin(angle + M_PI / 3)));
    modelMatrix = glm::scale(modelMatrix, vec3(2, 2, 2));
    
    if (wireframeMode)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // glPolygonMode does not work in framebuffers,
    // so we should not use them unless the blur is enabled,
    // or the wireframe mode won't work
    SDL_assert(!(wireframeMode && enableSimpleBlur));
    
    if (enableSimpleBlur)
        glBindFramebuffer(GL_FRAMEBUFFER, blurBufferA.fbo);
    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
    
    glDisable(GL_COLOR_MATERIAL);
    //glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, texture.openglId);
    
    glUseProgram(shaderProgram);
    
    worldContainer.renderWorld(projectionMatrix * viewMatrix);
    
    mat4 finalMatrix = projectionMatrix * viewMatrix;
    glLoadMatrixd(glm::value_ptr(finalMatrix));
    
    //if (physicsDebugMode)
    {
        //physicsEngine.dumpRenderNoModelview(false, physicsDebugMode);
        //worldContainer.dumpRenderPhysics(player);
    }
    
    {
        glUseProgram(0);
        
        mat4 axisSwap(1, 0, 0, 0,
                      0, 0, 1, 0,
                      0, 1, 0, 0,
                      0, 0, 0, 1);
        
        double sc = 0.5;
        
        modelMatrix = glm::mat4();
        modelMatrix = glm::scale(modelMatrix, vec3(sc, sc, -sc));
        modelMatrix = glm::translate(modelMatrix, vec3(0, 2, 10));
        modelMatrix = modelMatrix * axisSwap;
        
        finalMatrix = projectionMatrix * viewMatrix * modelMatrix;
        glLoadMatrixd(glm::value_ptr(finalMatrix));
        newMesh.slowRender();
        
        glUseProgram(shaderProgram);
    }

    if (physicsDebugMode)
    {
        modelMatrix = mat4();
        modelMatrix = glm::translate(modelMatrix, player.position);
        modelMatrix = glm::scale(modelMatrix, vec3(player.radius, player.radius, player.radius));
        
        glLoadMatrixd(glm::value_ptr(projectionMatrix * viewMatrix * modelMatrix));
        meshCollection.cubeMesh.render();
    }
    
    if (enableSimpleBlur)
    {
        glDisable(GL_DEPTH_TEST);
    
        glEnable(GL_BLEND);
        glBlendColor(0, 0, 0, 0.85f);
        glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
        
        blurBufferB.renderScreenQuad();
        
        glDisable(GL_BLEND);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        blurBufferA.renderScreenQuad();
        
        swap(blurBufferA, blurBufferB);
    }
}
