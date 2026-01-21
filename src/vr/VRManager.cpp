//
// Created by Gabe on 1/21/2026.
//
#include "VRManager.h"
#include "glad/glad.h"

#ifdef VR

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL

#ifdef _WIN32
#include <windows.h>
#include <unknwn.h>
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>




namespace Engine{

    XrInstance xrInstance = XR_NULL_HANDLE;
    XrSystemId systemId;
    XrSession session;
    XrSpace space;
    XrSwapchain swapchain;
    XrFrameState fs{ XR_TYPE_FRAME_STATE };
    XrViewConfigurationView view;

    std::vector<XrSwapchainImageOpenGLKHR> images;
    std::vector<XrViewConfigurationView> views;


    const char* extensions[] = {
            XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
            XR_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    static const char* xrResultToString(XrInstance instance, XrResult result)
    {
        static char buffer[XR_MAX_RESULT_STRING_SIZE];
        xrResultToString(instance, result, buffer);
        return buffer;
    }

#define XR_CHECK(x)                                                     \
    do {                                                                \
        XrResult _r = (x);                                              \
        if (XR_FAILED(_r)) {                                            \
            log->error("OpenXR ERROR at {}:{}\n  {}",                  \
                __FILE__, __LINE__,                                     \
                xrResultToString(xrInstance, _r));                      \
            return;                                                     \
        }                                                               \
    } while (0)





    void VRModule::onInit()
    {
        log->info("[XR] Initializing OpenXR\n");

        // ------------------------------------------------------------
        // Instance
        // ------------------------------------------------------------
        XrInstanceCreateInfo ici{ XR_TYPE_INSTANCE_CREATE_INFO };
        strcpy_s(ici.applicationInfo.applicationName, "Game Engine VR");
        ici.applicationInfo.applicationVersion = 1;
        strcpy_s(ici.applicationInfo.engineName, "cpp-engine");
        ici.applicationInfo.engineVersion = 1;
        ici.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        ici.enabledExtensionCount = 2;
        ici.enabledExtensionNames = extensions;

        XR_CHECK(xrCreateInstance(&ici, &xrInstance));
        log->info("[XR] Instance created\n");

        // ------------------------------------------------------------
        // System (HMD)
        // ------------------------------------------------------------
        XrSystemGetInfo sgi{ XR_TYPE_SYSTEM_GET_INFO };
        sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

        XR_CHECK(xrGetSystem(xrInstance, &sgi, &systemId));
        log->info("[XR] HMD system acquired\n");

        // ------------------------------------------------------------
        // Graphics binding (OpenGL + Win32)
        // ------------------------------------------------------------
        HDC dc = wglGetCurrentDC();
        HGLRC rc = wglGetCurrentContext();

        if (!dc || !rc) {
            log->error("[XR] ERROR: No current OpenGL context\n");
            return;
        }

        XrGraphicsBindingOpenGLWin32KHR glBinding{
                XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR
        };
        glBinding.hDC   = dc;
        glBinding.hGLRC = rc;

        // ------------------------------------------------------------
        // Session
        // ------------------------------------------------------------
        XrSessionCreateInfo sci{ XR_TYPE_SESSION_CREATE_INFO };
        sci.systemId = systemId;
        sci.next = &glBinding;

        XR_CHECK(xrCreateSession(xrInstance, &sci, &session));
        log->info("[XR] Session created\n");

        // ------------------------------------------------------------
        // Reference space
        // ------------------------------------------------------------
        XrReferenceSpaceCreateInfo rsci{
                XR_TYPE_REFERENCE_SPACE_CREATE_INFO
        };
        rsci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        rsci.poseInReferenceSpace.orientation.w = 1.0f;

        XR_CHECK(xrCreateReferenceSpace(session, &rsci, &space));
        log->info("[XR] Reference space created\n");

        // ------------------------------------------------------------
        // View configuration (BUG FIXED)
        // ------------------------------------------------------------
        uint32_t viewCount = 0;

        XR_CHECK(xrEnumerateViewConfigurationViews(
                xrInstance,
                systemId,
                XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                0,
                &viewCount,
                nullptr
        ));

        if (viewCount < 1) {
            log->error("[XR] ERROR: No stereo views reported\n");
            return;
        }

        views.resize(viewCount);
        for (auto& v : views)
            v.type = XR_TYPE_VIEW_CONFIGURATION_VIEW;

        XR_CHECK(xrEnumerateViewConfigurationViews(
                xrInstance,
                systemId,
                XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                viewCount,
                &viewCount,
                views.data()
        ));

        log->info("[XR] {} views detected ({}x{})\n",
               viewCount,
               views[0].recommendedImageRectWidth,
               views[0].recommendedImageRectHeight);

        // ------------------------------------------------------------
        // Swapchain
        // ------------------------------------------------------------
        XrSwapchainCreateInfo sci2{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
        sci2.arraySize = 1;
        sci2.format = GL_RGBA8;
        sci2.width  = views[0].recommendedImageRectWidth;
        sci2.height = views[0].recommendedImageRectHeight;
        sci2.mipCount = 1;
        sci2.faceCount = 1;
        sci2.sampleCount = 1;
        sci2.usageFlags =
                XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                XR_SWAPCHAIN_USAGE_SAMPLED_BIT;

        XR_CHECK(xrCreateSwapchain(session, &sci2, &swapchain));
        log->info("[XR] Swapchain created\n");

        // ------------------------------------------------------------
        // Swapchain images (BUG FIXED)
        // ------------------------------------------------------------
        uint32_t imageCount = 0;

        XR_CHECK(xrEnumerateSwapchainImages(
                swapchain, 0, &imageCount, nullptr
        ));

        images.resize(imageCount);
        for (auto& img : images)
            img.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;

        XR_CHECK(xrEnumerateSwapchainImages(
                swapchain,
                imageCount,
                &imageCount,
                reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())
        ));

        log->info("[XR] %u swapchain images acquired\n", imageCount);

        log->info("[XR] OpenXR initialization COMPLETE\n");
    }


    void VRModule::onUpdate(float dt) {

    }

    void VRModule::onGameStart() {

    }

    void VRModule::onShutdown() {

    }

    void VRModule::waitForXRFrame() {
        XrFrameWaitInfo fwi{ XR_TYPE_FRAME_WAIT_INFO };
        xrWaitFrame(session, &fwi, &fs);

        XrFrameBeginInfo fbi{ XR_TYPE_FRAME_BEGIN_INFO };
        xrBeginFrame(session, &fbi);

        uint32_t index;
        XrSwapchainImageAcquireInfo aii{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
        xrAcquireSwapchainImage(swapchain, &aii, &index);

        XrSwapchainImageWaitInfo wii{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
        wii.timeout = XR_INFINITE_DURATION;
        xrWaitSwapchainImage(swapchain, &wii);
    }

    void VRModule::endXRFrame() {
        XrSwapchainImageReleaseInfo rii{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
        xrReleaseSwapchainImage(swapchain, &rii);

        XrCompositionLayerProjectionView pv[2]{};

        for (int i = 0; i < 2; i++)
        {
            pv[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
            pv[i].subImage.swapchain = swapchain;
            pv[i].subImage.imageRect.extent.width  = view.recommendedImageRectWidth;
            pv[i].subImage.imageRect.extent.height = view.recommendedImageRectHeight;
            pv[i].subImage.imageArrayIndex = 0;
        }

        XrCompositionLayerProjection layer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
        layer.space = space;
        layer.viewCount = 2;
        layer.views = pv;

        XrCompositionLayerBaseHeader* layers[] = {
                (XrCompositionLayerBaseHeader*)&layer
        };

        XrFrameEndInfo fei{ XR_TYPE_FRAME_END_INFO };
        fei.displayTime = fs.predictedDisplayTime;
        fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        fei.layerCount = 1;
        fei.layers = layers;

        xrEndFrame(session, &fei);
    }

}


#endif