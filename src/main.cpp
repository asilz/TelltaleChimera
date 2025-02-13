#include <cerrno>
#include <cstdio>
#include <ttc/core/gui.hpp>
#include <ttc/render/vulkan3.hpp>
#include <tth/animation/animation.hpp>
#include <tth/convert/asset.hpp>
#include <tth/core/any.hpp>
#include <tth/core/log.hpp>
#include <tth/d3dmesh/d3dmesh.hpp>
#include <tth/skeleton/skeleton.hpp>

using namespace TTH;

int main(void)
{

    Renderer renderer;
    renderer.d3dmesh.Create();
    renderer.animation.Create();
    renderer.skeleton.Create();

    Stream streamMesh = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/d3dmesh/sk61_javier_bodyUpper.d3dmesh", "rb");
    streamMesh.SeekMetaHeaderEnd();

    Stream streamAnimation = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/animation/sk61_javierAction_toStandA.anm", "rb");
    streamAnimation.SeekMetaHeaderEnd();

    Stream streamSkeleton = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/skl/sk61_javier.skl", "rb");
    streamSkeleton.SeekMetaHeaderEnd();

    streamMesh.Read(renderer.d3dmesh, false);
    streamAnimation.Read(renderer.animation, false);
    streamSkeleton.Read(renderer.skeleton, false);

    renderer.VulkanInit();

    SDL_Event sdlEvent;
    do
    {
        do
        {
        } while (SDL_GetWindowFlags(renderer.window) & SDL_WINDOW_MINIMIZED); // Does not work
        VkResult err = renderer.DrawFrame();
        SDL_PollEvent(&sdlEvent);
    } while (sdlEvent.type != SDL_EVENT_QUIT);

    vkDeviceWaitIdle(renderer.device);

    return 0;

    // return run();

    Stream stream = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/skl/sk61_javier.skl", "rb");
    stream.SeekMetaHeaderEnd();
    Skeleton skeleton;
    skeleton.Create();
    stream.Read(skeleton, false);

    Animation animation[4];
    animation[0].Create();
    {
        Stream streamAnimation = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/animation/sk61_javierStandA_toSit.anm", "rb");
        streamAnimation.SeekMetaHeaderEnd();
        streamAnimation.Read(animation[0], false);
    }
    animation[1].Create();
    {
        Stream streamAnimation = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/animation/sk61_javierStandHoldBeer_toLeanPorchHoldBeer.anm", "rb");
        streamAnimation.SeekMetaHeaderEnd();
        streamAnimation.Read(animation[1], false);
    }
    animation[2].Create();
    {
        Stream streamAnimation = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/animation/sk61_javierSupine_rubJaw.anm", "rb");
        streamAnimation.SeekMetaHeaderEnd();
        streamAnimation.Read(animation[2], false);
    }
    animation[3].Create();
    {
        Stream streamAnimation = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/animation/sk61_javierSlide.anm", "rb");
        streamAnimation.SeekMetaHeaderEnd();
        streamAnimation.Read(animation[3], false);
    }

    D3DMesh mesh[4];
    mesh[0].Create();
    {
        Stream streamMesh = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/d3dmesh/sk61_javier_bodyUpper.d3dmesh", "rb");
        streamMesh.SeekMetaHeaderEnd();
        streamMesh.Read(mesh[0], false);
    }

    mesh[1].Create();
    {
        Stream streamMesh = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/d3dmesh/sk61_javier_bodyLower.d3dmesh", "rb");
        streamMesh.SeekMetaHeaderEnd();
        streamMesh.Read(mesh[1], false);
    }

    mesh[2].Create();
    {
        Stream streamMesh = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/d3dmesh/sk61_javier_head.d3dmesh", "rb");
        streamMesh.SeekMetaHeaderEnd();
        streamMesh.Read(mesh[2], false);
    }

    mesh[3].Create();
    {
        Stream streamMesh = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/d3dmesh/sk61_javier_eyesMouth.d3dmesh", "rb");
        streamMesh.SeekMetaHeaderEnd();
        streamMesh.Read(mesh[3], false);
    }

    errno_t err = ExportAsset("assimpTWD.glb", skeleton, animation, mesh, 4, 4);

    skeleton.Destroy();
    animation[0].Destroy();
    mesh[0].Destroy();
    animation[1].Destroy();
    mesh[1].Destroy();
    animation[2].Destroy();
    mesh[2].Destroy();
    animation[3].Destroy();
    mesh[3].Destroy();
    return 0;
}
