#include <string>
#include <vector>

namespace StringUtils
{

std::string replaceAll(std::string str, const std::string& from, const std::string& to)
{
    auto&& pos = str.find(from, size_t{});
    while (pos != std::string::npos)
    {
        str.replace(pos, from.length(), to);
        pos = str.find(from, pos + to.length());
    }
    return str;
}

std::vector<std::string> splitLines(const std::string& str, bool keepLineEnding)
{
    std::vector<std::string> lines;
    std::size_t start = 0;
    std::size_t end = 0;

    while ((end = str.find('\n', start)) != std::string::npos)
    {
        lines.push_back(str.substr(start, end - start + keepLineEnding));
        start = end + 1;
    }

    if (start < str.length())
    {
        lines.push_back(str.substr(start));
    }

    return lines;
}

} // namespace StringUtils