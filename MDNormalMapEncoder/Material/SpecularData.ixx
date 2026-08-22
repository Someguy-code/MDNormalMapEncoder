export module Material:SpecularData;

import std;

export struct SpecularData
{
	float m_fSpecularHardness;
	float m_fIntensity;
	std::uint8_t m_uFrontLogOfShadesCount;
	std::uint8_t m_uBackLogOfShadesCount;
};