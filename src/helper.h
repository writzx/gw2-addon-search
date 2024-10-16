#pragma once
#include <chrono>
#include "const.h"

namespace helper {
    static void str_trim(char *s) {
        char *str_end = s + strlen(s);

        while (str_end > s && str_end[-1] == ' ') {
            str_end--;
        }

        *str_end = 0;
    }

    static chrono::time_point datetime_parse(const std::string &datetime) {
        chrono::time_point timepoint;

        std::istringstream dt_stream(datetime);

        dt_stream >> parse(DATETIME_FORMAT, timepoint);

        return timepoint;
    }

    static std::string datetime_tostring(const chrono::time_point tp) {
        const auto string_format = std::format("{{:{0}}}", DATETIME_DISPLAY_FORMAT);
        return vformat(string_format, make_format_args(tp));
    }
}
