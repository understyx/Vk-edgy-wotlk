#pragma once

#include <RmlUi/Core/FileInterface.h>
#include <string>

namespace WoTLKGuiLayer {

/**
 * @class RmlFileInterface
 * @brief Provides file I/O services to RmlUi, resolving paths relative
 *        to the layer's shared library directory.
 */
class RmlFileInterface : public Rml::FileInterface {
public:
    explicit RmlFileInterface(std::string base_path);

    Rml::FileHandle Open(const Rml::String& path) override;
    void Close(Rml::FileHandle file) override;
    size_t Read(void* buffer, size_t size, Rml::FileHandle file) override;
    bool Seek(Rml::FileHandle file, long offset, int origin) override;
    size_t Tell(Rml::FileHandle file) override;

private:
    std::string m_basePath;
};

} // namespace WoTLKGuiLayer
