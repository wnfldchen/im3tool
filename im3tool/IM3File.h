#pragma once
#include <vector>
class IM3File
{
	// File header
	struct Header {

	};

	// File compression tables
	struct Tables {

	};

	// Run-length entropy-coded bitstream
	std::vector<bool> rlc;
public:
	IM3File();
	~IM3File();
};

