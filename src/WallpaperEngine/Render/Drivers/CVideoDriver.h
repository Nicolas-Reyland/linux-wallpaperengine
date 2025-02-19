#pragma once

#include <glm/vec4.hpp>
#include <glm/vec2.hpp>

namespace WallpaperEngine::Render::Drivers
{
    class CVideoDriver
    {
    public:
        /**
         * @return The window handle used by this video driver
         */
        virtual void* getWindowHandle () const = 0;
        /**
         * @return The time that has passed since the driver started
         */
        virtual float getRenderTime () const = 0;
        /**
         * @return If a close was requested by the OS
         */
        virtual bool closeRequested () = 0;
        /**
         * @param size The new size for the window
         */
        virtual void resizeWindow (glm::ivec2 size) = 0;
        /**
         * @param size The new size and position of the window
         */
        virtual void resizeWindow (glm::ivec4 positionAndSize) = 0;
        /**
         * Shows the window created by the driver
         */
        virtual void showWindow () = 0;
        /**
         * Hides the window created by the driver
         */
        virtual void hideWindow () = 0;
        /**
         * @return The size of the framebuffer available for the driver
         */
        virtual glm::ivec2 getFramebufferSize () const = 0;
        /**
         * Performs buffer swapping
         */
        virtual void swapBuffers () = 0;
        /**
         * @return The number of rendered frames since the start of the driver
         */
        virtual uint32_t getFrameCounter () const = 0;
    };
}