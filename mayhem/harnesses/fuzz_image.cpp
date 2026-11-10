// Fuzzes dlib's image loader (stb_image: PNG/JPEG/GIF/BMP/PSD/TGA/HDR/PIC/PNM + ASTC header check),
// as used when the engine loads texture resources.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dlib/image.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    void* buf = malloc(size ? size : 1);
    if (size)
        memcpy(buf, data, size);

    int ret = -1;
    dmImage::ImageInfo info;
    if (dmImage::GetInfo(buf, (uint32_t)size, &info) == dmImage::RESULT_OK)
        ret = 0;
    (void)dmImage::IsHDR(buf, (uint32_t)size);

    for (int variant = 0; variant < 2; ++variant)
    {
        dmImage::Image image;
        if (dmImage::Load(buf, (uint32_t)size, variant == 1, variant == 1, &image) == dmImage::RESULT_OK)
        {
            ret = 0;
            dmImage::Free(&image);
        }
    }
    free(buf);
    return ret;
}
