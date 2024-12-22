#include <cerrno>
#include <cstdio>
#include <ttc/core/gui.hpp>
#include <tth/convert/asset.hpp>
#include <tth/core/any.hpp>
#include <tth/core/log.hpp>
#include <tth/meta/animation/animation.hpp>
#include <tth/meta/container/dcarray.hpp>
#include <tth/meta/container/map.hpp>
#include <tth/meta/container/sarray.hpp>
#include <tth/meta/container/set.hpp>
#include <tth/meta/crc64/crc64.hpp>
#include <tth/meta/d3dmesh/d3dmesh.hpp>
#include <tth/meta/linalg/vector.hpp>
#include <tth/meta/meta.hpp>
#include <tth/meta/skeleton/skeleton.hpp>

using namespace TTH;

int main(void)
{
    return run();

    Stream stream = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/skl/sk61_javier.skl", "rb");
    stream.SeekMetaHeaderEnd();
    Skeleton skeleton;
    skeleton.Read(stream, false);

    Animation animation[4];
    {
        Stream streamAnimation = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/animation/sk61_javierAction_toStandA.anm", "rb");
        streamAnimation.SeekMetaHeaderEnd();
        animation[0].Read(streamAnimation, false);
    }

    {
        Stream streamAnimation = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/animation/sk61_javier_walkTenseAimPistol.anm", "rb");
        streamAnimation.SeekMetaHeaderEnd();
        animation[1].Read(streamAnimation, false);
    }

    {
        Stream streamAnimation = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/animation/sk61_javierStandA_toSit.anm", "rb");
        streamAnimation.SeekMetaHeaderEnd();
        animation[2].Read(streamAnimation, false);
    }

    {
        Stream streamAnimation = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/animation/sk61_javierSlide.anm", "rb");
        streamAnimation.SeekMetaHeaderEnd();
        animation[3].Read(streamAnimation, false);
    }

    D3DMesh mesh[4];
    {
        Stream streamMesh = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/d3dmesh/sk61_javier_bodyLower.d3dmesh", "rb");
        streamMesh.SeekMetaHeaderEnd();
        mesh[0].Read(streamMesh, false);
    }

    {
        Stream streamMesh = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/d3dmesh/sk61_javier_bodyUpper.d3dmesh", "rb");
        streamMesh.SeekMetaHeaderEnd();
        mesh[1].Read(streamMesh, false);
    }

    {
        Stream streamMesh = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/d3dmesh/sk61_javier_eyesMouth.d3dmesh", "rb");
        streamMesh.SeekMetaHeaderEnd();
        mesh[2].Read(streamMesh, false);
    }

    {
        Stream streamMesh = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/d3dmesh/sk61_javier_head.d3dmesh", "rb");
        streamMesh.SeekMetaHeaderEnd();
        mesh[3].Read(streamMesh, false);
    }

    errno_t err = ExportAsset("assimpTWD.glb", skeleton, animation, mesh, 4, 4);
    return 0;
}
