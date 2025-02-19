#include "CX11OpenGLDriver.h"
#include "common.h"
#include <FreeImage.h>

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

using namespace WallpaperEngine::Render::Drivers;

void CustomGLFWErrorHandler (int errorCode, const char* reason)
{
    sLog.error ("GLFW error ", errorCode, ": ", reason);
}

CX11OpenGLDriver::CX11OpenGLDriver (const char* windowTitle, CApplicationContext& context) :
    m_frameCounter (0)
{
    glfwSetErrorCallback (CustomGLFWErrorHandler);

    // initialize glfw
    if (glfwInit () == GLFW_FALSE)
        sLog.exception ("Failed to initialize glfw");

    // set some window hints (opengl version to be used)
    glfwWindowHint (GLFW_SAMPLES, 4);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint (GLFW_VISIBLE, GLFW_FALSE);
    // set X11-specific hints
    glfwWindowHintString (GLFW_X11_CLASS_NAME, "linux-wallpaperengine");
    glfwWindowHintString (GLFW_X11_INSTANCE_NAME, "linux-wallpaperengine");

    // for forced window mode, we can set some hints that'll help position the window
    if (context.settings.render.mode == Application::CApplicationContext::EXPLICIT_WINDOW)
    {
        glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint (GLFW_DECORATED, GLFW_FALSE);
        glfwWindowHint (GLFW_FLOATING, GLFW_TRUE);
    }

#if !NDEBUG
    glfwWindowHint (GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif /* DEBUG */

    // create window, size doesn't matter as long as we don't show it
    this->m_window = glfwCreateWindow (640, 480, windowTitle, nullptr, nullptr);

    if (this->m_window == nullptr)
        sLog.exception ("Cannot create window");

    // make context current, required for glew initialization
    glfwMakeContextCurrent (this->m_window);

    // initialize glew for rendering
    GLenum result = glewInit ();

    if (result != GLEW_OK)
        sLog.error("Failed to initialize GLEW: ", glewGetErrorString (result));

    // initialize free image
    FreeImage_Initialise (TRUE);
}

CX11OpenGLDriver::~CX11OpenGLDriver ()
{
    glfwTerminate ();
    FreeImage_DeInitialise();
}

void* CX11OpenGLDriver::getWindowHandle () const
{
    return reinterpret_cast <void*> (glfwGetX11Window (this->m_window));
}

float CX11OpenGLDriver::getRenderTime () const
{
    return (float) glfwGetTime ();
}

bool CX11OpenGLDriver::closeRequested ()
{
    return glfwWindowShouldClose (this->m_window);
}

void CX11OpenGLDriver::resizeWindow (glm::ivec2 size)
{
    glfwSetWindowSize (this->m_window, size.x, size.y);
}

void CX11OpenGLDriver::resizeWindow (glm::ivec4 sizeandpos)
{
    glfwSetWindowPos (this->m_window, sizeandpos.x, sizeandpos.y);
    glfwSetWindowSize (this->m_window, sizeandpos.z, sizeandpos.w);
}

void CX11OpenGLDriver::showWindow ()
{
    glfwShowWindow (this->m_window);
}

void CX11OpenGLDriver::hideWindow ()
{
    glfwHideWindow (this->m_window);
}

glm::ivec2 CX11OpenGLDriver::getFramebufferSize () const
{
    glm::ivec2 size;

    glfwGetFramebufferSize (this->m_window, &size.x, &size.y);

    return size;
}

void CX11OpenGLDriver::swapBuffers ()
{
    // do buffer swapping first
    glfwSwapBuffers (this->m_window);
    // poll for events
    glfwPollEvents ();
    // increase frame counter
    this->m_frameCounter ++;
}

uint32_t CX11OpenGLDriver::getFrameCounter () const
{
    return this->m_frameCounter;
}

GLFWwindow* CX11OpenGLDriver::getWindow ()
{
    return this->m_window;
}
