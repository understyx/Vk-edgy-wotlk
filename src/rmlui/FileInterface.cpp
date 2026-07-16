#include "FileInterface.h"
#include <cstdio>

namespace WoTLKGuiLayer {

RmlFileInterface::RmlFileInterface(std::string base_path)
    : m_basePath(std::move(base_path))
{
    if (!m_basePath.empty() && m_basePath.back() != '/')
        m_basePath += '/';
}

Rml::FileHandle RmlFileInterface::Open(const Rml::String& path)
{
    // Try the path as-is first, then relative to the base path
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) {
        std::string full = m_basePath + path;
        fp = fopen(full.c_str(), "rb");
    }
    return reinterpret_cast<Rml::FileHandle>(fp);
}

void RmlFileInterface::Close(Rml::FileHandle file)
{
    if (file)
        fclose(reinterpret_cast<FILE*>(file));
}

size_t RmlFileInterface::Read(void* buffer, size_t size, Rml::FileHandle file)
{
    if (!file) return 0;
    return fread(buffer, 1, size, reinterpret_cast<FILE*>(file));
}

bool RmlFileInterface::Seek(Rml::FileHandle file, long offset, int origin)
{
    if (!file) return false;
    return fseek(reinterpret_cast<FILE*>(file), offset, origin) == 0;
}

size_t RmlFileInterface::Tell(Rml::FileHandle file)
{
    if (!file) return 0;
    return static_cast<size_t>(ftell(reinterpret_cast<FILE*>(file)));
}

} // namespace WoTLKGuiLayer
