#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> result;
    size_t pos = text.find_first_not_of(" ");
    text.remove_prefix(std::min(text.size(), pos));
    const size_t pos_end = text.npos;

    while (!text.empty()) {
        size_t space = text.find(' ');
        result.push_back(space == pos_end ? text.substr(0, pos_end) : text.substr(0, space));
        pos = text.find_first_not_of(" ", space);
        text.remove_prefix(std::min(text.size(), pos));
    }

    return result;
}
