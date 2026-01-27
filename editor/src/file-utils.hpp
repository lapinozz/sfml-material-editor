#pragma once

#include "nfd.h"

#include <fstream>
#include <optional>
#include <string>
#include <filesystem>

namespace FileUtils
{

inline const nfdu8filteritem_t defaultImageFilter{"Image File", "bmp,png,tga,jpg,gif,psd,hdr,pic,pnm"};
inline const nfdu8filteritem_t MlspFilter{"MLS Project", "mlsp"};

inline std::optional<std::string> browseFile(bool save, nfdu8filteritem_t filter)
{
    nfdu8char_t* outPath{};
    nfdresult_t result;

    if (save)
    {
        nfdsavedialogu8args_t args{};
        args.filterList = &filter;
        args.filterCount = 1;
        result = NFD_SaveDialogU8_With(&outPath, &args);
    }
    else
    {
        nfdopendialogu8args_t args{};
        args.filterList = &filter;
        args.filterCount = 1;
        result = NFD_OpenDialogU8_With(&outPath, &args);
    }

    if (result != NFD_OKAY)
    {
        return std::nullopt;
    }

    std::string path = outPath;
    NFD_FreePathU8(outPath);

    return std::move(path);
}

inline std::optional<std::string> readFile(const std::string& path, bool binaryMode = true)
{
    std::ifstream inputFile(path, binaryMode ? std::ios_base::binary : 0);
    if (!inputFile)
    {
        return std::nullopt;
    }

    return std::string{(std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>()};
}

inline bool writeFile(const std::string& path, std::string_view data, bool binaryMode = true)
{
    const auto directoryPath = std::filesystem::path{path}.remove_filename();
    std::filesystem::create_directories(directoryPath);

    if (std::ofstream of{path, binaryMode ? std::ios_base::binary : 0})
    {
        of << data;
        return true;
    }

    return false;
}

} // namespace FileUtils