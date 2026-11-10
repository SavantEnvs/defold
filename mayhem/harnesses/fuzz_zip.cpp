// Fuzzes dlib's zip reader (miniz-based dmZip) over an in-memory archive: directory parsing,
// entry lookup, size query and inflate of every entry.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dlib/zip.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    char* buf = (char*)malloc(size ? size : 1);
    if (size)
        memcpy(buf, data, size);

    dmZip::HZip zip = 0;
    int ret = -1;
    if (dmZip::OpenStream(buf, (uint32_t)size, &zip) == dmZip::RESULT_OK)
    {
        ret = 0;
        uint32_t n = dmZip::GetNumEntries(zip);
        for (uint32_t i = 0; i < n; ++i)
        {
            if (dmZip::OpenEntry(zip, i) != dmZip::RESULT_OK)
                continue;
            (void)dmZip::GetEntryName(zip);
            (void)dmZip::IsEntryDir(zip);
            uint32_t esize = 0, index = 0;
            (void)dmZip::GetEntryIndex(zip, &index);
            // An embedder allocates the entry size it was told about; cap it so a lying header
            // is an out-of-memory input for the harness rather than for the library.
            if (dmZip::GetEntrySize(zip, &esize) == dmZip::RESULT_OK && esize <= (64u << 20))
            {
                void* out = malloc(esize ? esize : 1);
                (void)dmZip::GetEntryData(zip, out, esize);
                free(out);
            }
            dmZip::CloseEntry(zip);
        }
        dmZip::Close(zip);
    }
    free(buf);
    return ret;
}
