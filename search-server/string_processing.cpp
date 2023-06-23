#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>

#include "string_processing.h"



std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> result;
    auto pos = text.find_first_not_of(" ");
    const auto pos_end = text.npos;
    text.remove_prefix(std::min((text.size()), pos));

    while (!text.empty()) {
        auto space = text.find(' ');
        result.push_back(space == pos_end ? text.substr(0) : text.substr(0, space));
        pos = text.find_first_not_of(" ", space);
        text.remove_prefix(std::min(text.size(), pos));
    }

    return result;
}
