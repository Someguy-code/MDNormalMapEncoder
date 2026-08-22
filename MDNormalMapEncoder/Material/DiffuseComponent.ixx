export module Material:DiffuseComponent;

import Color;
import LABColor;
import MegadriveUtils;
import MathUtils;
import PaletteUtils;
import SerializationUtils;
import std;

export struct DiffuseComponent
{
	std::uint8_t m_uFrontLogOfShadesCount;
	std::uint8_t m_uBackLogOfShadesCount;
	std::vector<std::vector<ColorRGB>> m_oShadedAlbedoColorsFront;
	std::vector<std::vector<ColorRGB>> m_oShadedAlbedoColorsBack;

	DiffuseComponent(std::span<const ColorRGB> _oAlbedoColors, const ColorRGB& _oFrontLightColor, const ColorRGB& _oFullDarknessColor, const ColorRGB& _oBackLightColor, std::uint8_t _uLogOfShadesCount)
		: m_uFrontLogOfShadesCount(_uLogOfShadesCount), m_uBackLogOfShadesCount(_uLogOfShadesCount),
		m_oShadedAlbedoColorsFront(ComputeShadedAlbedoColors(_oAlbedoColors, _oFrontLightColor, _oFullDarknessColor, _uLogOfShadesCount)),
		m_oShadedAlbedoColorsBack(ComputeShadedAlbedoColors(_oAlbedoColors, _oBackLightColor, _oFullDarknessColor, _uLogOfShadesCount))
	{}

	void Write(std::ofstream& _oFileStream, bool _bWriteColorsAsRGB333) const
	{
		SerializationUtils::WriteValue(_oFileStream, _bWriteColorsAsRGB333);
		SerializationUtils::WriteValue(_oFileStream, m_uFrontLogOfShadesCount);
		SerializationUtils::WriteValue(_oFileStream, m_uBackLogOfShadesCount);
		//The following values must be aligned to WORD so they can be accessed as such (uneven addresses are only valid for byte access)
		//If the colors are writen as RGB333 values, they will be read byte by byte, so no need to pad
		const bool bNeedsDWORDPadding = !_bWriteColorsAsRGB333 && (_oFileStream.tellp() & 1) > 0;
		if (bNeedsDWORDPadding)
			SerializationUtils::WriteValue(_oFileStream, static_cast<std::uint8_t>(0));

		struct WriteShades
		{
			std::ofstream& m_oFileStream;
			const bool m_bWriteColorsAsRGB333;

			void operator()(const std::vector<ColorRGB>& _oShadedColors) const
			{
				for (const ColorRGB& oShadedColor : _oShadedColors)
				{
					ColorRGB oShadeRGBColor333 = MegadriveUtils::GetRGB333Color(oShadedColor);
					if (m_bWriteColorsAsRGB333)
						oShadeRGBColor333.Write(m_oFileStream, false);
					else
						SerializationUtils::WriteValueBigEndian(m_oFileStream, MegadriveUtils::GetVDPColor333(oShadeRGBColor333));
				}
			}
		};

		std::for_each(m_oShadedAlbedoColorsFront.cbegin(), m_oShadedAlbedoColorsFront.cend(), WriteShades{ _oFileStream, _bWriteColorsAsRGB333 });
		std::for_each(m_oShadedAlbedoColorsBack.cbegin(), m_oShadedAlbedoColorsBack.cend(), WriteShades{ _oFileStream, _bWriteColorsAsRGB333 });
	}

private:
	static std::vector<std::vector<ColorRGB>> ComputeShadedAlbedoColors(std::span<const ColorRGB> _oAlbedoColors, const ColorRGB& _oFullLightColor, const ColorRGB& _oFullDarknessColor, unsigned int _uLogOfShadesCount)
	{
		const size_t uAlbedoColorsCount = _oAlbedoColors.size();
		std::vector<std::vector<ColorRGB>> oShadedAlbedoColors{ uAlbedoColorsCount };
		for (unsigned int uColorIndex = 0; uColorIndex < uAlbedoColorsCount; ++uColorIndex)
			oShadedAlbedoColors[uColorIndex] = ComputeShadedAlbedoColor(_oAlbedoColors[uColorIndex], _oFullLightColor, _oFullDarknessColor, _uLogOfShadesCount);

		return oShadedAlbedoColors;
	}

	static std::vector<ColorRGB> ComputeShadedAlbedoColor(const ColorRGB& _oAlbedoColor, const ColorRGB& _oFullLightColor, const ColorRGB& _oFullDarknessColor, std::uint8_t _uLogOfDiffuseShadesCount)
	{
		const unsigned int uDiffuseShadesCount = MathUtils::MulByPowerOf2(1, _uLogOfDiffuseShadesCount);
		std::vector<ColorRGB> oShadedAlbedoColors;
		oShadedAlbedoColors.reserve(uDiffuseShadesCount);
		const LABColor oModulatedFrontLightColor{ _oAlbedoColor * _oFullLightColor };
		const LABColor oModulatedZeroLightColor{ _oFullDarknessColor };
		const std::vector<LABColor> oMegadriveMasterPalette = MegadriveUtils::GetMegadriveMasterPalette();
		for (unsigned int uShadeIndex = 0; uShadeIndex < uDiffuseShadesCount; ++uShadeIndex)
		{
			const LABColor oColorShade = oModulatedZeroLightColor.Lerp(oModulatedFrontLightColor, static_cast<float>(uShadeIndex) / static_cast<float>(uDiffuseShadesCount - 1));
			const ColorRGB oTestColor = static_cast<ColorRGB>(oColorShade);
			unsigned int uClosestMegadriveColorIndex = PaletteUtils::GetClosestColorIndex<LABColor>(oColorShade, oMegadriveMasterPalette);
			oShadedAlbedoColors.push_back(static_cast<ColorRGB>(oMegadriveMasterPalette[uClosestMegadriveColorIndex]));
		}
		return oShadedAlbedoColors;
	}
};