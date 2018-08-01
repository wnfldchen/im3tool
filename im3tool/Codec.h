#pragma once
#define _USE_MATH_DEFINES
#include <algorithm>
#include <array>
#include <vector>
#include <queue>
#include <map>
#include <utility>
#include <limits>
#include <math.h>
#include "BitmapUtility.h"
#include "BitmapFile.h"

// Forward declaration of class dependencies
class IM3File;

class Codec : public BitmapUtility
{
private:
	// Quantization matrix
	static const std::array<std::array<INT8, 8>, 8> Q;

	// Zig-zag scan pattern
	static const std::array<std::pair<INT8, INT8>, 63> Z;

	// Template types

	// Before DCT: T == INT8
	// Before Quantization: T == INT16
	// Before Entropy Coding: T == INT8

	// Alias templates
	template <typename T>
	using Block = std::array<std::array<T, 8>, 8>;

	template <typename T>
	using Plane = std::vector<std::vector<Block<T>>>;

	// Keys for each plane
	enum PlaneKeys {
		Y = 0,
		U = 1,
		V = 2
	};

	// Y, U, and V image planes consisting of 8-by-8 blocks
	template <typename T>
	struct YUVPlanes {
		std::array<Plane<T>, 3> planes;
		INT32 getWidth() const;
		INT32 getHeight() const;
		YUVPlanes(const INT32 width, const INT32 height);
	};

	// Utility functions

	// Function C in DCT
	DOUBLE C(const UINT8 x);

	// DCT on a 8-by-8 block
	template <typename T, typename W>
	Block<W> dctOnBlock(const Block<T>& block);

	// Quantization on a 8-by-8 block
	template <typename T, typename W>
	Block<W> quantizeOnBlock(const Block<T>& block);

	// Compression functions

	// Transform bitmap to YUV planes
	YUVPlanes<INT8> bitmapToYUV(BitmapFile* bitmapFile);

	// Discrete Cosine Transform (DCT) on YUV planes
	YUVPlanes<INT16> dct(const YUVPlanes<INT8>& yuv);

	// Quantization on DCT'd YUV planes
	YUVPlanes<INT8> quantize(const YUVPlanes<INT16>& dct);

	// Difference code the DC components and run-length code the AC components
	typedef std::array<std::vector<INT8>, 3> CodedDC;
	typedef std::array<std::vector<std::pair<UINT8, INT8>>, 3> CodedAC;
	std::pair<CodedDC, CodedAC> runLengthDifferenceCoder(const Codec::YUVPlanes<INT8>& quantized);

	// Huffman coding utility types and functions
	struct SymbolWithCount {
		INT32 Symbol;
		UINT32 Count;
		bool operator>(const SymbolWithCount& a) const {
			return Count > a.Count;
		}
	};

	// Frequency table type
	template <typename T>
	using FrequencyTable = std::array<SymbolWithCount, std::numeric_limits<T>::max() - std::numeric_limits<T>::min() + 1>;

public:
	// Length table type
	template <typename T>
	using LengthTable = std::array<UINT8, std::numeric_limits<T>::max() - std::numeric_limits<T>::min() + 1>;

private:
	// Frequency count function
	template <typename T>
	FrequencyTable<T> freqCount(const std::vector<T>& symbols);

	// Huffman coding of symbols
	template <typename T>
	std::pair<LengthTable<T>, std::vector<bool>> huffmanEncode(const std::vector<T>& input);

	// Entropy coding on run-length difference-encoded AC and DC components
public:
	// Types output by the entropy coder
	typedef std::vector<bool> BitsDC;
	typedef std::vector<bool> BitsACFirst;
	typedef std::vector<bool> BitsACSecond;
	typedef std::pair<LengthTable<INT8>, BitsDC> EntropiedDC;
	typedef std::pair<LengthTable<UINT8>, BitsACFirst> EntropiedACFirst;
	typedef std::pair<LengthTable<INT8>, BitsACSecond> EntropiedACSecond;
	typedef std::pair<EntropiedACFirst, EntropiedACSecond> EntropiedAC;
private:
	std::array<std::pair<EntropiedDC, EntropiedAC>, 3> entropyCoder(const Codec::CodedDC& codedDC, const Codec::CodedAC& codedAC);

	// Decompression functions

	// TODO: Implement
	// Entropy decoding
	// Dequantization
	// Inverse DCT
	// YUV to bitmap
public:
	// Compress a bitmap
	IM3File* compress(BitmapFile* bitmapFile);
	// Decompress an IM3
	BitmapFile* decompress(IM3File* im3File);
	Codec();
};

template<typename T>
inline std::pair<Codec::LengthTable<T>, std::vector<bool>> Codec::huffmanEncode(const std::vector<T>& input)
{
	// Typedef for Symbol
	typedef INT32 Symbol;
	// Constants
	static const Symbol FIRST_PARENT = std::numeric_limits<T>::max() + 1;
	static const Symbol PARENT_STEP = 1;
	static const Symbol NO_CHILD = std::numeric_limits<T>::min() - 1;
	static const Symbol NO_PARENT = std::numeric_limits<T>::min() - 1;
	// Build the frequency table
	FrequencyTable<T> freqTable = freqCount<T>(input);
	// Sort the frequency table
	std::priority_queue<
		SymbolWithCount,
		std::vector<SymbolWithCount>,
		std::greater<SymbolWithCount>>
		sorted(freqTable.begin(), freqTable.end());
	// Setup the Huffman tree
	// Node
	struct Node {
		Symbol Parent;
		Symbol Left;
		Symbol Right;
	};
	// Map to store the tree
	std::map<Symbol, Node> tree;
	Symbol parentSymbol = FIRST_PARENT;
	while (sorted.size() > 1) {
		// Get and remove the two lowest frequency symbols
		SymbolWithCount a = sorted.top();
		sorted.pop();
		SymbolWithCount b = sorted.top();
		sorted.pop();
		// Create a parent node
		SymbolWithCount parent = { parentSymbol, a.Count + b.Count };
		// Put the parent node back
		sorted.push(parent);
		// Add the parent to the tree
		tree.insert(std::pair<Symbol, Node>(parentSymbol, { NO_PARENT, a.Symbol, b.Symbol }));
		// Add the children to the tree
		if (tree.find(a.Symbol) == tree.end()) {
			tree.insert(std::pair<Symbol, Node>(a.Symbol, { parentSymbol, NO_CHILD, NO_CHILD }));
		}
		else {
			tree[a.Symbol].Parent = parentSymbol;
		}
		if (tree.find(b.Symbol) == tree.end()) {
			tree.insert(std::pair<Symbol, Node>(b.Symbol, { parentSymbol, NO_CHILD, NO_CHILD }));
		}
		else {
			tree[b.Symbol].Parent = parentSymbol;
		}
		parentSymbol += PARENT_STEP;
	}
	// The root node is the last parent
	Symbol root = parentSymbol - PARENT_STEP;
	// Set up the table for storing sorted code lengths
	std::array<
		std::pair<UINT8, T>,
		std::numeric_limits<T>::max() - std::numeric_limits<T>::min() + 1> sortedLengths;
	// Additionally generate a symbol bit length table
	LengthTable<T> lengths;
	// Store the code lengths
	for (Symbol i = std::numeric_limits<T>::min();
		i <= std::numeric_limits<T>::max();
		i++) {
		auto it = tree.find(i);
		Symbol symbol = it->first;
		Node node = tree[symbol];
		UINT8 length = 0;
		while (symbol != root) {
			Symbol nextSymbol = node.Parent;
			Node nextNode = tree[nextSymbol];
			length += 1;
			symbol = nextSymbol;
			node = nextNode;
		}
		INT32 index = i - std::numeric_limits<T>::min();
		lengths[index] = length;
		sortedLengths[index] = { length, static_cast<T>(i) };
	}
	// Sort the code lengths
	std::sort(sortedLengths.begin(), sortedLengths.end());
	// Set up the canonical code table
	std::array<
		std::pair<std::vector<bool>, T>,
		std::numeric_limits<T>::max() - std::numeric_limits<T>::min() + 1> canonicalCodes;
	// Assign canonical codes
	canonicalCodes[0] = { std::vector<bool>(sortedLengths[0].first, false), sortedLengths[0].second };
	for (INT32 i = 1; i < canonicalCodes.size(); i++) {
		// Increment from the previous code for the next code
		std::vector<bool> code = canonicalCodes[i - 1].first;
		UINT8 bitIndex;
		for (bitIndex = static_cast<UINT8>(code.size() - 1); bitIndex >= 0; bitIndex--) {
			if (code[bitIndex]) {
				code[bitIndex] = false;
			} else {
				break;
			}
		}
		code[bitIndex] = true;
		// If the code length increases, append zeroes
		UINT8 length = sortedLengths[i].first;
		while (code.size() != length) {
			code.push_back(false);
		}
		canonicalCodes[i] = { code, sortedLengths[i].second };
	}
	// Build an easier to traverse code table
	std::array<
		std::vector<bool>,
		std::numeric_limits<T>::max() - std::numeric_limits<T>::min() + 1> codes;
	for (Symbol i = std::numeric_limits<T>::min();
		i <= std::numeric_limits<T>::max();
		i++) {
		INT32 index = i - std::numeric_limits<T>::min();
		auto result = std::find_if(
			canonicalCodes.begin(),
			canonicalCodes.end(),
			[i](const std::pair<std::vector<bool>, T>& entry) {
				return entry.second == static_cast<T>(i);
			});
		codes[index] = result->first;
	}
	// Compress the data
	std::vector<bool> compressed;
	for (auto it = input.begin(); it != input.end(); it++) {
		INT32 index = (*it) - std::numeric_limits<T>::min();
		std::vector<bool> code = codes[index];
		compressed.insert(compressed.end(), code.begin(), code.end());
	}
	return std::pair<LengthTable<T>, std::vector<bool>>(lengths, compressed);
}

template<typename T, typename W>
inline Codec::Block<W> Codec::dctOnBlock(const Block<T>& block) {
	Block<W> output;
	for (UINT8 v = 0; v < 8; v++) {
		for (UINT8 u = 0; u < 8; u++) {
			DOUBLE sum = 0.0;
			for (UINT8 j = 0; j < 8; j++) {
				for (UINT8 i = 0; i < 8; i++) {
					sum += cos((2 * i + 1) * u * M_PI / 16)
						* cos((2 * j + 1) * v * M_PI / 16)
						* block[j][i];
				}
			}
			W F = static_cast<W>(C(u) * C(v) * sum / 4);
			output[v][u] = F;
		}
	}
	return output;
}

template<typename T, typename W>
inline Codec::Block<W> Codec::quantizeOnBlock(const Block<T>& block)
{
	Block<W> output;
	for (UINT8 i = 0; i < 8; i++) {
		for (UINT8 j = 0; j < 8; j++) {
			output[i][j] = block[i][j] / Q[i][j];
		}
	}
	return output;
}

template<typename T>
inline Codec::FrequencyTable<T>
Codec::freqCount(const std::vector<T>& symbols)
{
	FrequencyTable<T> table;
	T symbol = std::numeric_limits<T>::min();
	for (auto it = table.begin(); it != table.end(); it++) {
		it->Symbol = symbol;
		it->Count = 0;
		symbol += 1;
	}
	for (auto it = symbols.begin(); it != symbols.end(); it++) {
		table[(*it) - std::numeric_limits<T>::min()].Count += 1;
	}
	return table;
}

template<typename T>
inline INT32 Codec::YUVPlanes<T>::getWidth() const
{
	return static_cast<INT32>(planes[0][0].size() * 8);
}

template<typename T>
inline INT32 Codec::YUVPlanes<T>::getHeight() const
{
	return static_cast<INT32>(planes[0].size() * 8);
}

template<typename T>
inline Codec::YUVPlanes<T>::YUVPlanes(const INT32 width, const INT32 height)
{
	for (UINT8 i = 0; i < 3; i++) {
		planes[i].resize(height / 8);
		for (INT32 j = 0; j < height / 8; j++) {
			planes[i][j].resize(width / 8);
		}
	}
}
