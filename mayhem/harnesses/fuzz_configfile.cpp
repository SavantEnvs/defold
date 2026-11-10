// Fuzzes dlib's INI-style config parser (game.project) through both the engine entry point
// (LoadFromBuffer) and the auxiliary Parse(), then reads values back through the getters.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dlib/configfile.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    char* buf = (char*)malloc(size ? size : 1);
    if (size)
        memcpy(buf, data, size);

    int ret = -1;
    dmConfigFile::HConfig config = 0;
    if (dmConfigFile::LoadFromBuffer(buf, (uint32_t)size, 0, 0, &config) == dmConfigFile::RESULT_OK)
    {
        ret = 0;
        (void)dmConfigFile::GetString(config, "project.title", "");
        (void)dmConfigFile::GetInt(config, "display.width", 0);
        (void)dmConfigFile::GetFloat(config, "physics.gravity_y", 0.0f);
        dmConfigFile::Delete(config);
    }
    config = 0;
    if (dmConfigFile::Parse(buf, (uint32_t)size, &config) == dmConfigFile::RESULT_OK)
    {
        ret = 0;
        (void)dmConfigFile::GetString(config, "a.b", "");
        dmConfigFile::Delete(config);
    }
    free(buf);
    return ret;
}
