#include<string>
#include "CEVersion.hpp"
#include "Debug.hpp"

struct ChookityEngineInit {
	ChookityEngineInit() {
		cg_info("Chookity Engine V"+std::to_string(CHOOKITY_ENGINE_VERSION));
	}
};

static ChookityEngineInit init;
