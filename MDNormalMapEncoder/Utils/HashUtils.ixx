export module HashUtils;

import std;

export template <class T>
inline void HashCombine(std::size_t& _uSeed, const T& v)
{
	std::hash<T> oHasher;
	_uSeed ^= oHasher(v) + 0x9e3779b9 + (_uSeed << 6) + (_uSeed >> 2);
}