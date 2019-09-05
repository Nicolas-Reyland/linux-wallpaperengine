#include <iostream>
#include <irrlicht/irrlicht.h>
#include <sstream>
#include <WallpaperEngine/video/renderer.h>
#include <WallpaperEngine/video/material.h>
#include <WallpaperEngine/Irrlicht/CPkgReader.h>
#include <getopt.h>
#include <SDL_mixer.h>
#include <SDL.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "WallpaperEngine/Render/Shaders/Compiler.h"
#include "WallpaperEngine/project.h"
#include "WallpaperEngine/Irrlicht/CImageLoaderTEX.h"

#include "WallpaperEngine/Core/CProject.h"
#include "WallpaperEngine/Irrlicht/CContext.h"

bool IsRootWindow = false;
std::vector<std::string> Screens;
std::vector<irr::core::rect<irr::s32>> Viewports;

irr::SIrrlichtCreationParameters _irr_params;

irr::f32 g_Time = 0;

WallpaperEngine::Irrlicht::CContext* IrrlichtContext;
void initialize_viewports ()
{
    if (IsRootWindow == false || Screens.size () == 0)
        return;

    Display* display = XOpenDisplay (NULL);
    int xrandr_result, xrandr_error;

    if (!XRRQueryExtension (display, &xrandr_result, &xrandr_error))
    {
        std::cerr << "XRandr is not present, cannot detect specified screens, running in window mode" << std::endl;
        return;
    }

    XRRScreenResources* screenResources = XRRGetScreenResources (display, DefaultRootWindow (display));

    // there are some situations where xrandr returns null (like screen not using the extension)
    if (screenResources == nullptr)
        return;

    for (int i = 0; i < screenResources->noutput; i ++)
    {
        XRROutputInfo* info = XRRGetOutputInfo (display, screenResources, screenResources->outputs [i]);

        // there are some situations where xrandr returns null (like screen not using the extension)
        if (info == nullptr)
            continue;

        std::vector<std::string>::iterator cur = Screens.begin ();
        std::vector<std::string>::iterator end = Screens.end ();

        for (; cur != end; cur ++)
        {
            if (info->connection == RR_Connected && strcmp (info->name, (*cur).c_str ()) == 0)
            {
                XRRCrtcInfo* crtc = XRRGetCrtcInfo (display, screenResources, info->crtc);

                std::cout << "Found requested screen: " << info->name << " -> " << crtc->x << "x" << crtc->y << ":" << crtc->width << "x" << crtc->height << std::endl;

                irr::core::rect<irr::s32> viewport;

                viewport.UpperLeftCorner.X = crtc->x;
                viewport.UpperLeftCorner.Y = crtc->y;
                viewport.LowerRightCorner.X = crtc->x + crtc->width;
                viewport.LowerRightCorner.Y = crtc->y + crtc->height;

                Viewports.push_back (viewport);

                XRRFreeCrtcInfo (crtc);
            }
        }

        XRRFreeOutputInfo (info);
    }

    XRRFreeScreenResources (screenResources);

    _irr_params.WindowId = reinterpret_cast<void*> (DefaultRootWindow (display));
}

int init_irrlicht()
{
    IrrlichtContext = new WallpaperEngine::Irrlicht::CContext ();

    // prepare basic configuration for irrlicht
    _irr_params.AntiAlias = 8;
    _irr_params.Bits = 16;
    // _irr_params.DeviceType = Irrlicht::EIDT_X11;
    _irr_params.DriverType = irr::video::EDT_OPENGL;
    _irr_params.Doublebuffer = false;
    _irr_params.EventReceiver = nullptr;
    _irr_params.Fullscreen = false;
    _irr_params.HandleSRGB = false;
    _irr_params.IgnoreInput = true;
    _irr_params.Stencilbuffer = true;
    _irr_params.UsePerformanceTimer = false;
    _irr_params.Vsync = false;
    _irr_params.WithAlphaChannel = false;
    _irr_params.ZBufferBits = 24;
    _irr_params.LoggingLevel = irr::ELL_DEBUG;

    initialize_viewports ();

    IrrlichtContext->setDevice (irr::createDeviceEx (_irr_params));

    if (IrrlichtContext->getDevice () == nullptr)
    {
        return 1;
    }

    IrrlichtContext->getDevice ()->setWindowCaption (L"Test game");

    // check for ps and vs support
    if (
            IrrlichtContext->getDevice ()->getVideoDriver()->queryFeature (irr::video::EVDF_PIXEL_SHADER_1_1) == false &&
            IrrlichtContext->getDevice ()->getVideoDriver()->queryFeature (irr::video::EVDF_ARB_FRAGMENT_PROGRAM_1) == false)
    {
        IrrlichtContext->getDevice ()->getLogger ()->log ("WARNING: Pixel shaders disabled because of missing driver/hardware support");
    }

    if (
            IrrlichtContext->getDevice ()->getVideoDriver()->queryFeature (irr::video::EVDF_VERTEX_SHADER_1_1) == false &&
            IrrlichtContext->getDevice ()->getVideoDriver()->queryFeature (irr::video::EVDF_ARB_VERTEX_PROGRAM_1) == false)
    {
        IrrlichtContext->getDevice ()->getLogger ()->log ("WARNING: Vertex shaders disabled because of missing driver/hardware support");
    }

    return 0;
}

void preconfigure_wallpaper_engine ()
{
    // load the assets from wallpaper engine
    IrrlichtContext->getDevice ()->getFileSystem ()->addFileArchive ("assets.zip", true, false);

    // register custom loaders
    IrrlichtContext->getDevice ()->getVideoDriver()->addExternalImageLoader (
        new WallpaperEngine::Irrlicht::CImageLoaderTex (IrrlichtContext)
    );
    IrrlichtContext->getDevice ()->getFileSystem ()->addArchiveLoader (
        new WallpaperEngine::Irrlicht::CArchiveLoaderPkg (IrrlichtContext)
    );
}

void print_help (const char* route)
{
    std::cout
        << "Usage:" << route << " [options] " << std::endl
        << "options:" << std::endl
        << "  --silent\t\tMutes all the sound the wallpaper might produce" << std::endl
        << "  --dir <folder>\tLoads an uncompressed background from the given <folder>" << std::endl
        << "  --pkg <folder>\tLoads a scene.pkg file from the given <folder>" << std::endl
        << "  --screen-root <screen name>\tDisplay as screen's background" << std::endl;
}

std::string stringPathFixes(const std::string& s){
    std::string str(s);
    if(str.empty())
        return s;
    if(str[0] == '\'' && str[str.size() - 1] == '\''){
        str.erase(str.size() - 1, 1);
        str.erase(0,1);
    }
    if(str[str.size() - 1] != '/')
        str += '/';
    return std::move(str);
}

int main (int argc, char* argv[])
{
    int mode = 0;
    bool audio_support = true;
    std::string path;

    int option_index = 0;

    static struct option long_options [] = {
            {"screen-root", required_argument, 0, 'r'},
            {"pkg",         required_argument, 0, 'p'},
            {"dir",         required_argument, 0, 'd'},
            {"silent",      optional_argument, 0, 's'},
            {"help",        optional_argument, 0, 'h'},
            {nullptr,                   0, 0,   0}
    };

    while (true)
    {
        int c = getopt_long (argc, argv, "r:p:d:sh", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case 'r':
                IsRootWindow = true;
                Screens.push_back (optarg);
                break;

            case 'p':
                mode = 1;
                path = optarg;
                break;

            case 'd':
                mode = 2;
                path = optarg;
                break;

            case 's':
                audio_support = false;
                break;

            case 'h':
                print_help (argv [0]);
                return 0;

            default:
                break;
        }
    }

    if (init_irrlicht ())
    {
        return 1;
    }

    preconfigure_wallpaper_engine ();

    irr::io::path wallpaper_path;
    irr::io::path project_path;
    irr::io::path scene_path;

    switch (mode)
    {
        case 0:
            print_help (argv [0]);
            return 0;

        // pkg mode
        case 1:
            path = stringPathFixes(path);
            wallpaper_path = IrrlichtContext->getDevice ()->getFileSystem ()->getAbsolutePath (path.c_str ());
            project_path = wallpaper_path + "project.json";
            scene_path = wallpaper_path + "scene.pkg";

            IrrlichtContext->getDevice ()->getFileSystem ()->addFileArchive (scene_path, true, false); // add the pkg file to the lookup list
            break;

        // folder mode
        case 2:
            path = stringPathFixes(path);
            wallpaper_path = IrrlichtContext->getDevice ()->getFileSystem ()->getAbsolutePath (path.c_str ());
            project_path = wallpaper_path + "project.json";

            // set our working directory
            IrrlichtContext->getDevice ()->getFileSystem ()->changeWorkingDirectoryTo (wallpaper_path);
            break;

        default:
            break;
    }

    if (audio_support == true)
    {
        int mixer_flags = MIX_INIT_MP3 | MIX_INIT_FLAC | MIX_INIT_OGG;

        if (SDL_Init (SDL_INIT_AUDIO) < 0 || mixer_flags != Mix_Init (mixer_flags))
        {
            IrrlichtContext->getDevice ()->getLogger ()->log ("Cannot initialize SDL audio system", irr::ELL_ERROR);
            return -1;
        }

        // initialize audio engine
        Mix_OpenAudio (22050, AUDIO_S16SYS, 2, 640);
    }

    WallpaperEngine::project* wp_project = new WallpaperEngine::project (project_path);

    if (wp_project->getScene ()->isOrthogonal() == true)
    {
        WallpaperEngine::video::renderer::setupOrthographicCamera (wp_project->getScene ());
    }
    else
    {
        IrrlichtContext->getDevice ()->getLogger ()->log (
            "Non-orthogonal cameras not supported yet!!", irr::ELL_ERROR
        );

        return -2;
    }

    // register nodes
    WallpaperEngine::video::renderer::queueNode (wp_project->getScene ());

    int32_t lastTime = 0;
    int32_t minimumTime = 1000 / 90;
    int32_t currentTime = 0;

    while (IrrlichtContext->getDevice ()->run () && IrrlichtContext && IrrlichtContext->getDevice ())
    {
        // if (device->isWindowActive ())
        {
            currentTime = IrrlichtContext->getDevice ()->getTimer ()->getTime ();
            g_Time = currentTime / 1000.0f;

            if (currentTime - lastTime > minimumTime)
            {
                std::vector<irr::core::rect<irr::s32>>::iterator cur = Viewports.begin ();
                std::vector<irr::core::rect<irr::s32>>::iterator end = Viewports.end ();

                for (; cur != end; cur ++)
                {
                    // change viewport to render to the correct portion of the display
                    IrrlichtContext->getDevice ()->getVideoDriver ()->setViewPort (*cur);
                    WallpaperEngine::video::renderer::render ();
                }

                lastTime = currentTime;
            }
            else
            {
                IrrlichtContext->getDevice ()->sleep (1, false);
            }
        }
    }

    SDL_Quit ();
    delete wp_project;

    return 0;
}