// Fuzzes dlib's strict JSON parser (dmJson / JsonParse) in both decode modes, then walks the
// document through the public accessors the engine uses.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dlib/json.h>

static void Sink(void*, const char*, uint32_t) {}

static void Walk(HJsonDocument doc, int node, int depth)
{
    if (node < 0 || depth > 70)
        return;
    const JsonNode* n = JsonGetNode(doc, node);
    if (!n)
        return;
    (void)JsonGetName(doc, node);
    switch (n->m_Type)
    {
    case JSON_TYPE_OBJECT:
    case JSON_TYPE_ARRAY:
        for (int c = n->m_Container.m_First; c >= 0; c = JsonGetNode(doc, c)->m_Next)
        {
            Walk(doc, c, depth + 1);
            if (n->m_Type == JSON_TYPE_OBJECT)
                (void)JsonField(doc, node, JsonGetName(doc, c));
        }
        break;
    case JSON_TYPE_STRING:
    {
        const char* s = JsonString(doc, node, "");
        JsonWriteString(s, (uint32_t)strlen(s), Sink, 0);
        break;
    }
    case JSON_TYPE_NUMBER:
    {
        uint32_t u;
        (void)JsonInteger(doc, node, 0);
        (void)JsonUnsignedInteger(doc, node, &u);
        break;
    }
    case JSON_TYPE_BOOL:
        (void)JsonBoolean(doc, node, false);
        break;
    default:
        break;
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    char* text = (char*)malloc(size ? size : 1);
    if (size)
        memcpy(text, data, size);

    int ret = -1;
    for (int mode = 0; mode < 2; ++mode)
    {
        HJsonDocument doc = JsonCreate(0);
        uint32_t error_offset = 0;
        JsonResult r = JsonParse(doc, text, (uint32_t)size, &error_offset,
                                 mode == 0 ? JSON_PARSE_DECODE_VALUES : JSON_PARSE_PRESERVE_VALUES);
        if (r == JSON_RESULT_OK)
        {
            ret = 0;
            if (JsonGetNodeCount(doc) > 0)
                Walk(doc, 0, 0);
        }
        JsonDestroy(doc);
    }
    free(text);
    return ret;
}
