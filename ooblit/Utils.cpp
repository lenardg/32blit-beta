#include "Utils.hpp"

namespace blit {
	namespace oo {
		float next_random() {
			return (float)blit::random() / (float)UINT32_MAX;
		}

		uint32_t next_random(uint32_t lowerInclusive, uint32_t upperExclusive) {
			return (blit::random() % (upperExclusive - lowerInclusive)) + lowerInclusive;
		}

	}
}
