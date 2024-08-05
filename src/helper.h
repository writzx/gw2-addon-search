#pragma once
#include<chrono>

namespace helper {
	static long long current_millis() {
		auto now = std::chrono::system_clock::now().time_since_epoch();

		// Convert duration to milliseconds
		return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
	}

	static void  str_trim(char* s) {
		char* str_end = s + strlen(s);

		while (str_end > s && str_end[-1] == ' ') {
			str_end--;
		}

		*str_end = 0;
	}
}

