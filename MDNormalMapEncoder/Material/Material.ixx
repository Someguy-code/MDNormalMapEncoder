module;
#include <string.h>
export module Material;
import :DiffuseComponent;
import :SpecularComponent;
export import :SpecularData;

import Color;
import ExceptionUtils;
import LABColor;
import MegadriveUtils;
import MathUtils;
import PaletteUtils;
import SerializationUtils;
import std;

export struct Material
{
	unsigned int m_uBaseIndex;
	bool m_bIncludePureBlack;
	std::vector<ColorRGB> m_oAlbedoColors;
	std::vector<ColorRGB> m_oNormals;
	DiffuseComponent m_oDiffuseComponent;
	std::optional<SpecularComponent> m_oSpecularComponent;

	Material(unsigned int _uBaseIndex, bool _bIncludePureBlack, std::span<const ColorRGB> _oAlbedoColors, std::span<const ColorRGB> _oNormals, const ColorRGB& _oFrontLightColor, const ColorRGB& _oFullDarknessColor, const ColorRGB& _oBackLightColor, std::uint8_t _uLogOfShadesCount, const std::optional<SpecularData>& _oSpecularData = std::nullopt)
	: m_uBaseIndex(_uBaseIndex), m_bIncludePureBlack(_bIncludePureBlack), m_oAlbedoColors(_oAlbedoColors.cbegin(), _oAlbedoColors.cend()), m_oNormals(_oNormals.cbegin(), _oNormals.cend()),
		m_oDiffuseComponent(_oAlbedoColors, _oFrontLightColor, _oFullDarknessColor, _oBackLightColor, _uLogOfShadesCount),
		m_oSpecularComponent(std::nullopt)
	{
		if(_oSpecularData.has_value())
			m_oSpecularComponent = std::optional<SpecularComponent>{SpecularComponent{ *_oSpecularData, _oFrontLightColor, _oBackLightColor }};
	}

	void Write(const char* _sFilename) const
	{
		std::ofstream oFileStream(_sFilename, std::ios::binary);
		if (oFileStream)
		{
			SerializationUtils::WriteValue(oFileStream, static_cast<uint8_t>(m_bIncludePureBlack ? 1 : 0));
			SerializationUtils::WriteValue(oFileStream, static_cast<uint8_t>(m_oAlbedoColors.size()));
			const std::vector<LABColor> oMegadriveMasterPalette = MegadriveUtils::GetMegadriveMasterPalette();
			for (const ColorRGB& oAlbedoColor : m_oAlbedoColors)
			{
				const unsigned int uClosestMegadriveColorIndex = PaletteUtils::GetClosestColorIndex<LABColor>(oAlbedoColor, oMegadriveMasterPalette);
				MegadriveUtils::GetRGB333Color(static_cast<ColorRGB>(oMegadriveMasterPalette[uClosestMegadriveColorIndex])).Write(oFileStream, false);
			}

			WriteNormalsGroups(oFileStream, 1, m_oNormals.size() * m_oAlbedoColors.size());

			SerializationUtils::WriteValue(oFileStream, static_cast<uint8_t>(m_oNormals.size()));
			for (const ColorRGB& oNormal : m_oNormals)
			{
				SerializationUtils::WriteValue(oFileStream, static_cast<std::uint8_t>(128 - oNormal.m_uR));
				SerializationUtils::WriteValue(oFileStream, static_cast<std::uint8_t>(128 - oNormal.m_uG));
				SerializationUtils::WriteValue(oFileStream, static_cast<std::uint8_t>(128 - oNormal.m_uB));
			}

			const bool bHasSpecular = m_oSpecularComponent.has_value();
			m_oDiffuseComponent.Write(oFileStream, bHasSpecular);
			SerializationUtils::WriteValue(oFileStream, bHasSpecular);
			if (bHasSpecular)
				m_oSpecularComponent->Write(oFileStream);
		}
		else
			ExceptionUtils::ThrowOpenFileFailureException();
	}

private:
	static void WriteNormalsGroups(std::ofstream& _oFileStream, unsigned int _uBaseIndex, size_t _uNormalsCount)
	{
		struct NormalsGorup
		{
			std::uint8_t m_uFirstIndex;
			std::uint8_t m_uLastIndex;

			void Write(std::ofstream& oFileStream) const
			{
				SerializationUtils::WriteValue(oFileStream, m_uFirstIndex);
				SerializationUtils::WriteValue(oFileStream, m_uLastIndex);
			}
		};

		//Count from 1 as we cannot rely on index 0
		const std::uint8_t uInvolvedIndicesCount = static_cast<std::uint8_t>((_uBaseIndex - 1) + _uNormalsCount);
		const std::uint8_t uGroupsCount = MathUtils::DivRoundUp(uInvolvedIndicesCount, 15);
		SerializationUtils::WriteValue(_oFileStream, uGroupsCount);
		//Add the first group
		NormalsGorup{ static_cast<std::uint8_t>(_uBaseIndex), std::min((std::uint8_t)15, uInvolvedIndicesCount)}.Write(_oFileStream);
		//Add intermediate groups
		const unsigned int uIntermadiateGroupsCount = uGroupsCount - 1;
		for (unsigned int uGroupIndex = 1; uGroupIndex < uIntermadiateGroupsCount; ++uGroupIndex)
			NormalsGorup{ 1, 15 }.Write(_oFileStream);
		//Add last gorup
		if (uGroupsCount > 1)
			NormalsGorup{ 1, static_cast<std::uint8_t>(1 + uInvolvedIndicesCount % 15) }.Write(_oFileStream);
	}
};