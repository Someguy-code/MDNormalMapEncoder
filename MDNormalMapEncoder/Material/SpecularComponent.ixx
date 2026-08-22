export module Material:SpecularComponent;

import :SpecularData;

import Color;
import LABColor;
import MegadriveUtils;
import MathUtils;
import PaletteUtils;
import SerializationUtils;
import std;

export struct SpecularComponent
{
	std::uint8_t m_uFrontLogOfShadesCount;
	std::uint8_t m_uBackLogOfShadesCount;
	std::vector<ColorRGB> m_oShadedColorsFront;
	std::vector<ColorRGB> m_oShadedColorsBack;

	SpecularComponent(const SpecularData& _oSpecularData, const ColorRGB& _oFrontLightColor, const ColorRGB& _oBackLightColor)
	: m_uFrontLogOfShadesCount(_oSpecularData.m_uFrontLogOfShadesCount), m_uBackLogOfShadesCount(_oSpecularData.m_uBackLogOfShadesCount),
		m_oShadedColorsFront(ComputeShadedColors(_oSpecularData.m_fIntensity, _oSpecularData.m_fSpecularHardness, _oFrontLightColor, _oSpecularData.m_uFrontLogOfShadesCount)),
		m_oShadedColorsBack(ComputeShadedColors(_oSpecularData.m_fIntensity, _oSpecularData.m_fSpecularHardness, _oBackLightColor, _oSpecularData.m_uBackLogOfShadesCount))
	{}

	void Write(std::ofstream& _oFileStream) const
	{
		SerializationUtils::WriteValue(_oFileStream, m_uFrontLogOfShadesCount);
		SerializationUtils::WriteValue(_oFileStream, m_uBackLogOfShadesCount);

		auto fnWriteColor = [&_oFileStream](const ColorRGB& _oColor) {_oColor.Write(_oFileStream, false);};

		std::for_each(m_oShadedColorsFront.cbegin(), m_oShadedColorsFront.cend(), fnWriteColor);
		std::for_each(m_oShadedColorsBack.cbegin(), m_oShadedColorsBack.cend(), fnWriteColor);
	}

private:
	static std::vector<ColorRGB> ComputeShadedColors(float _fIntensity, float _fSpecularHardness, const ColorRGB& _oFullLightColor, std::uint8_t _uLogOfDiffuseShadesCount)
	{
		const unsigned int uSpecularShadesCount = MathUtils::MulByPowerOf2(1, _uLogOfDiffuseShadesCount);
		std::vector<ColorRGB> oShadedAlbedoColors;
		oShadedAlbedoColors.reserve(uSpecularShadesCount);
		const LABColor oFullLightColor{ _oFullLightColor };
		const LABColor oZeroColor = LABColor::Zero();
		const std::vector<LABColor> oMegadriveMasterPalette = MegadriveUtils::GetMegadriveMasterPalette();
		for (unsigned int uShadeIndex = 0; uShadeIndex < uSpecularShadesCount; ++uShadeIndex)
		{
			const float fLightIntensity = _fIntensity * std::pow(static_cast<float>(uShadeIndex) / static_cast<float>(uSpecularShadesCount - 1), _fSpecularHardness);
			const LABColor oColorShade = oZeroColor.Lerp(oFullLightColor, fLightIntensity);
			const unsigned int uClosestMegadriveColorIndex = PaletteUtils::GetClosestColorIndex<LABColor>(oColorShade, oMegadriveMasterPalette);
			oShadedAlbedoColors.push_back(MegadriveUtils::GetRGB333Color(static_cast<ColorRGB>(oMegadriveMasterPalette[uClosestMegadriveColorIndex])));
		}
		return oShadedAlbedoColors;
	}
};
