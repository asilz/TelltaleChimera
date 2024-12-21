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

void LogCallback(int error_code, const char *description) { TTH_LOG_ERROR("%d %s\n", error_code, description); }

int main(void)
{
    return run();

    Stream streamMesh = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/d3dmesh/sk61_javier_bodyUpper.d3dmesh", "rb");
    streamMesh.SeekMetaHeaderEnd();

    D3DMesh mesh;
    mesh.Read(streamMesh, false);

    Stream streamAnimation = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/animation/sk61_javierAction_toStandA.anm", "rb");
    Stream stream = Stream("/home/asil/Documents/decryption/TelltaleDevTool/cipherTexts/skl/sk61_javier.skl", "rb");
    stream.SeekMetaHeaderEnd();
    streamAnimation.SeekMetaHeaderEnd();

    Skeleton skeleton;
    skeleton.Read(stream, false);

    Animation animation;
    animation.Read(streamAnimation, false);

    errno_t err = ExportAsset("assimTWD.glb", skeleton, animation, mesh);
    return 0;
}
