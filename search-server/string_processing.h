#pragma once

#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <string_view>

std::vector<std::string_view> SplitIntoWords(std::string_view text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;

    for (const std::string_view str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(std::string(str)); // TODO �������� ��������!
        }
    }
    return non_empty_strings;
}
