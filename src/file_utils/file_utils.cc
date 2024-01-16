#include "file_utils.h"

namespace net_zelcon::wikidice::file_utils {
auto get_file_size(std::istream &stream) -> std::streamoff {
    auto original_pos = stream.tellg();
    stream.seekg(0, std::ios::end);
    auto end_pos = stream.tellg();
    stream.seekg(0, std::ios::beg);
    auto file_size = end_pos - stream.tellg();
    stream.seekg(original_pos);
    return file_size;
}
} // namespace net_zelcon::wikidice::file_utils