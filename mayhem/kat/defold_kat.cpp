// Known-answer probe for mayhem/test.sh: runs one real Defold parser over a fixture and prints
// the computed values, which test.sh compares against exact expected strings.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <dlib/json.h>
#include <dlib/image.h>
#include <dlib/zip.h>
#include <dlib/configfile.h>
#include <dmsdk/font/font.h>
#include <modelimporter.h>

static void* ReadAll(const char* path, uint32_t* size)
{
    FILE* f = fopen(path, "rb");
    if (!f)
        return 0;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    void* mem = malloc(n ? n : 1);
    if (fread(mem, 1, n, f) != (size_t)n)
    {
        fclose(f);
        free(mem);
        return 0;
    }
    fclose(f);
    *size = (uint32_t)n;
    return mem;
}

static int Json(const char* path)
{
    uint32_t size;
    char* text = (char*)ReadAll(path, &size);
    if (!text)
        return 2;
    HJsonDocument doc = JsonCreate(0);
    uint32_t err = 0;
    if (JsonParse(doc, text, size, &err, JSON_PARSE_DECODE_VALUES) != JSON_RESULT_OK)
    {
        printf("json parse error at %u\n", err);
        return 1;
    }
    int root = 0;
    int arr = JsonField(doc, root, "list");
    int nested = JsonField(doc, root, "nested");
    int item = JsonField(doc, nested, "name");
    printf("nodes=%d list_first=%d name=%s flag=%d\n", (int)JsonGetNodeCount(doc),
           (int)JsonInteger(doc, JsonGetNode(doc, arr)->m_Container.m_First, -1), JsonString(doc, item, "?"),
           (int)JsonBoolean(doc, JsonField(doc, root, "flag"), false));
    JsonDestroy(doc);
    free(text);
    return 0;
}

static int Image(const char* path)
{
    uint32_t size;
    void* mem = ReadAll(path, &size);
    if (!mem)
        return 2;
    dmImage::Image image;
    if (dmImage::Load(mem, size, false, false, &image) != dmImage::RESULT_OK)
    {
        printf("image load failed\n");
        return 1;
    }
    const uint8_t* b = (const uint8_t*)image.m_Buffer;
    printf("%ux%u type=%d px=", image.m_Width, image.m_Height, (int)image.m_Type);
    for (int i = 0; i < 16; ++i)
        printf("%02x", b[i]);
    printf("\n");
    dmImage::Free(&image);
    free(mem);
    return 0;
}

static int Zip(const char* path)
{
    uint32_t size;
    void* mem = ReadAll(path, &size);
    if (!mem)
        return 2;
    dmZip::HZip zip = 0;
    if (dmZip::OpenStream((const char*)mem, size, &zip) != dmZip::RESULT_OK)
    {
        printf("zip open failed\n");
        return 1;
    }
    uint32_t n = dmZip::GetNumEntries(zip);
    for (uint32_t i = 0; i < n; ++i)
    {
        if (dmZip::OpenEntry(zip, i) != dmZip::RESULT_OK)
            continue;
        uint32_t esize = 0;
        dmZip::GetEntrySize(zip, &esize);
        std::string data(esize, '\0');
        if (esize)
            dmZip::GetEntryData(zip, &data[0], esize);
        printf("%s size=%u dir=%d data=", dmZip::GetEntryName(zip), esize, (int)dmZip::IsEntryDir(zip));
        for (uint32_t k = 0; k < esize && k < 8; ++k)
            printf("%02x", (uint8_t)data[k]);
        printf("\n");
        dmZip::CloseEntry(zip);
    }
    dmZip::Close(zip);
    free(mem);
    return 0;
}

static int Config(const char* path)
{
    uint32_t size;
    void* mem = ReadAll(path, &size);
    if (!mem)
        return 2;
    dmConfigFile::HConfig config = 0;
    if (dmConfigFile::Parse((const char*)mem, size, &config) != dmConfigFile::RESULT_OK)
    {
        printf("config parse failed\n");
        return 1;
    }
    printf("foo=%s foo_int=%d pound=%s semi=%s bar=%d ext_string=%s ext_float=%.1f\n",
           dmConfigFile::GetString(config, "main.foo", "?"), (int)dmConfigFile::GetInt(config, "main.foo", -1),
           dmConfigFile::GetString(config, "comments.pound", "?"), dmConfigFile::GetString(config, "comments.semi", "?"),
           (int)dmConfigFile::GetInt(config, "sub.bar", -1), dmConfigFile::GetString(config, "ext.string", "?"),
           (double)dmConfigFile::GetFloat(config, "ext.float", -1.0f));
    dmConfigFile::Delete(config);
    free(mem);
    return 0;
}

static int Model(const char* path, const char* bin_path)
{
    uint32_t size;
    void* mem = ReadAll(path, &size);
    if (!mem)
        return 2;
    dmModelImporter::Options options;
    const char* suffix = strrchr(path, '.') + 1;
    dmModelImporter::Scene* scene = dmModelImporter::LoadFromBuffer(&options, suffix, mem, size);
    if (!scene || (scene->m_LoadError && scene->m_LoadError[0]))
    {
        printf("model load failed\n");
        return 1;
    }
    if (dmModelImporter::NeedsResolve(scene) && bin_path)
    {
        uint32_t bsize;
        void* bmem = ReadAll(bin_path, &bsize);
        if (!bmem)
            return 2;
        dmModelImporter::ResolveBuffer(scene, scene->m_Buffers[0].m_Uri, bmem, bsize);
        free(bmem);
    }
    bool fin = dmModelImporter::LoadFinalize(scene);
    bool val = fin && dmModelImporter::Validate(scene);
    uint32_t vertices = 0;
    for (uint32_t i = 0; i < scene->m_Models.Size(); ++i)
        for (uint32_t m = 0; m < scene->m_Models[i].m_Meshes.Size(); ++m)
            vertices += scene->m_Models[i].m_Meshes[m].m_VertexCount;
    printf("finalize=%d validate=%d buffers=%u nodes=%u models=%u vertices=%u\n", (int)fin, (int)val,
           scene->m_Buffers.Size(), scene->m_Nodes.Size(), scene->m_Models.Size(), vertices);
    dmModelImporter::DestroyScene(scene);
    free(mem);
    return 0;
}

static int Font(const char* path)
{
    uint32_t size;
    void* mem = ReadAll(path, &size);
    if (!mem)
        return 2;
    HFont font = FontLoadFromMemory("kat.ttf", mem, size, true);
    free(mem);
    if (!font)
    {
        printf("font load failed\n");
        return 1;
    }
    float scale = FontGetScaleFromSize(font, 40.0f);
    uint32_t index = FontGetGlyphIndex(font, 'A');
    FontGlyphOptions opts;
    opts.m_Scale = scale;
    FontGlyph glyph;
    memset(&glyph, 0, sizeof(glyph));
    if (FontGetGlyphByIndex(font, index, &opts, &glyph) != FONT_RESULT_OK)
    {
        printf("glyph failed\n");
        return 1;
    }
    printf("index_nonzero=%d advance=%.1f ascent_positive=%d\n", (int)(index != 0), (double)glyph.m_Advance,
           (int)(FontGetAscent(font, scale) > 0.0f));
    FontFreeGlyph(font, &glyph);
    FontDestroy(font);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 3)
        return 64;
    const char* cmd = argv[1];
    if (!strcmp(cmd, "json"))
        return Json(argv[2]);
    if (!strcmp(cmd, "image"))
        return Image(argv[2]);
    if (!strcmp(cmd, "zip"))
        return Zip(argv[2]);
    if (!strcmp(cmd, "config"))
        return Config(argv[2]);
    if (!strcmp(cmd, "model"))
        return Model(argv[2], argc > 3 ? argv[3] : 0);
    if (!strcmp(cmd, "font"))
        return Font(argv[2]);
    return 64;
}
