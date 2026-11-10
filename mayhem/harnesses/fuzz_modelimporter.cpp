// Fuzzes Defold's model importer (cgltf-based glTF/GLB loader + draco/meshopt decompression + scene build).
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <modelimporter.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 4)
        return -1;
    void* mem = malloc(size);
    memcpy(mem, data, size);

    const char* suffix = memcmp(data, "glTF", 4) == 0 ? "glb" : "gltf";
    dmModelImporter::Options options;
    dmModelImporter::Scene* scene = dmModelImporter::LoadFromBuffer(&options, suffix, mem, (uint32_t)size);
    if (!scene)
    {
        free(mem);
        return -1;
    }
    int ret = -1;
    if (!(scene->m_LoadError && scene->m_LoadError[0]))
    {
        if (dmModelImporter::NeedsResolve(scene))
        {
            for (uint32_t i = 0; i < scene->m_Buffers.Size(); ++i)
            {
                if (scene->m_Buffers[i].m_Buffer || scene->m_Buffers[i].m_BufferCount == 0 || !scene->m_Buffers[i].m_Uri)
                    continue;
                uint32_t n = scene->m_Buffers[i].m_BufferCount;
                if (n > (1u << 20))
                    continue;
                uint8_t* b = (uint8_t*)malloc(n);
                for (uint32_t k = 0; k < n; ++k)
                    b[k] = (uint8_t)(k * 31u + i);
                dmModelImporter::ResolveBuffer(scene, scene->m_Buffers[i].m_Uri, b, n);
                free(b);
            }
        }
        if (dmModelImporter::LoadFinalize(scene))
        {
            ret = 0;
            dmModelImporter::Validate(scene);
        }
    }
    dmModelImporter::DestroyScene(scene);
    free(mem);
    return ret;
}
