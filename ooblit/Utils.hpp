#pragma once

#include "ooblit.hpp"

namespace blit {
	namespace oo {
		float next_random();
		uint32_t next_random(uint32_t lowerInclusive, uint32_t upperExclusive);
	}
}
