#pragma once
#include <vector>
class IM3File
{
	// TODO: Implement

	// File header
	struct Header {
		// TODO: Implement
	};

	// File compression tables
	struct Tables {
		// TODO: Implement
	};
	// TODO: Implement
	// Run-length entropy-coded bitstream
	std::vector<bool> rlc;
public:
	// TODO: Implement
	IM3File();
	~IM3File();
};

