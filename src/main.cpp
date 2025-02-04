#include <cerrno>
#include <cstdio>
#include <ttc/core/gui.hpp>
#include <ttc/render/vulkan.hpp>
#include <ttc/render/vulkan2.hpp>
#include <tth/animation/animation.hpp>
#include <tth/convert/asset.hpp>
#include <tth/core/any.hpp>
#include <tth/core/log.hpp>
#include <tth/d3dmesh/d3dmesh.hpp>
#include <tth/skeleton/skeleton.hpp>

using namespace TTH;

int main(void)
{
    /*

    Renderer renderer;

    Stream streamMesh = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/d3dmesh/sk61_javier_bodyLower.d3dmesh", "rb");
    streamMesh.SeekMetaHeaderEnd();

    Stream streamAnimation = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/animation/sk61_javierAction_toStandA.anm", "rb");
    streamAnimation.SeekMetaHeaderEnd();

    Stream streamSkeleton = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/skl/sk61_javier.skl", "rb");
    streamSkeleton.SeekMetaHeaderEnd();

    renderer.d3dmesh.Read(streamMesh, false);
    renderer.animation.Read(streamAnimation, false);
    renderer.skeleton.Read(streamSkeleton, false);

    renderer.VulkanInit();

    SDL_Event sdlEvent;
    do
    {
        do
        {
        } while (SDL_GetWindowFlags(renderer.window) & SDL_WINDOW_MINIMIZED); // Does not work
        renderer.DrawFrame();
        SDL_PollEvent(&sdlEvent);
    } while (sdlEvent.type != SDL_EVENT_QUIT);

    vkDeviceWaitIdle(renderer.device);

    return 0;
    */

    return run();

    Stream stream = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/skl/sk61_javier.skl", "rb");
    stream.SeekMetaHeaderEnd();
    Skeleton skeleton;
    skeleton.Create();
    stream.Read(skeleton, false);

    Animation animation;
    animation.Create();
    {
        Stream streamAnimation = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/animation/sk61_javierStandA_toSit.anm", "rb");
        streamAnimation.SeekMetaHeaderEnd();
        streamAnimation.Read(animation, false);
    }

    D3DMesh mesh;
    mesh.Create();
    {
        Stream streamMesh = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/d3dmesh/sk61_javier_bodyLower.d3dmesh", "rb");
        streamMesh.SeekMetaHeaderEnd();
        streamMesh.Read(mesh, false);
    }

    skeleton.Destroy();
    animation.Destroy();
    mesh.Destroy();

    errno_t err = ExportAsset("assimpTWD.glb", skeleton, &animation, &mesh, 1, 1);
    return 0;
}
