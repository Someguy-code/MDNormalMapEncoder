export module MegadriveUtils;

import Color;
import LABColor;
import std;
import CollectionUtils;

export struct MegadriveUtils
{
	static constexpr const std::vector<LABColor>& GetMegadriveMasterPalette()
	{
		static std::vector<LABColor> oMegadriveMasterPalette = GetMegadriveMasterPaletteInitial();
		return oMegadriveMasterPalette;
	}

	static constexpr const std::vector<LABColor>& GetWarmGradient()
	{
		static std::vector<LABColor> oWarmGradient = GetWarmGradientInitial();
		return oWarmGradient;
	}

	static constexpr const std::vector<ColorRGB>& GetWarmGradientLinear()
	{
		static std::vector<ColorRGB> oWarmGradientLinear = GetWarmGradientLinearInitial();
		return oWarmGradientLinear;
	}

	static constexpr const std::vector<LABColor>& GetCoolGradient()
	{
		static std::vector<LABColor> oCoolGradient = GetCoolGradientInitial();
		return oCoolGradient;
	}

	static constexpr const std::vector<ColorRGB>& GetCoolGradientLinear()
	{
		static std::vector<ColorRGB> oCoolGradientLinear = GetCoolGradientLinearInitial();
		return oCoolGradientLinear;
	}

	static constexpr std::uint16_t GetVDPColor(std::uint32_t _uColor)
	{
		return 
			((((((_uColor)+0x100000) < 0xFF0000 ? (_uColor)+0x100000 : 0xFF0000) >> (20)) & VDPPALETTE_REDMASK) | 
			((((((_uColor) & 0xff00) + 0x1000) < 0xFF00 ? ((_uColor) & 0xff00) + 0x1000 : 0xFF00) >> ((1 * 4) + 4)) & VDPPALETTE_GREENMASK) | 
			((((((_uColor) & 0xff) + 0x10) < 0xFF ? ((_uColor) & 0xff) + 0x10 : 0xFF) << 4) & VDPPALETTE_BLUEMASK));
	}

	static constexpr std::uint16_t GetVDPColor333(const ColorRGB& _oColor333)
	{
		return (_oColor333.m_uR << VDPPALETTE_REDSFT) | (_oColor333.m_uG << VDPPALETTE_GREENSFT) | (_oColor333.m_uB << VDPPALETTE_BLUESFT);
	}

	static constexpr ColorRGB GetRGB333Color(const ColorRGB& _oColor)
	{
		const std::vector<std::uint8_t>& oValidComponentValues = GetValidComponentValues();

		return {
			static_cast<std::uint8_t>(CollectionUtils::GetClosestValueIndexOrdered(oValidComponentValues.cbegin(), oValidComponentValues.cend(), _oColor.m_uR)),
			static_cast<std::uint8_t>(CollectionUtils::GetClosestValueIndexOrdered(oValidComponentValues.cbegin(), oValidComponentValues.cend(), _oColor.m_uG)),
			static_cast<std::uint8_t>(CollectionUtils::GetClosestValueIndexOrdered(oValidComponentValues.cbegin(), oValidComponentValues.cend(), _oColor.m_uB)),
		};
	}

private:
	static const std::uint16_t VDPPALETTE_REDMASK = 0x000E;
	static const std::uint16_t VDPPALETTE_GREENMASK = 0x00E0;
	static const std::uint16_t VDPPALETTE_BLUEMASK = 0x0E00;
	static const std::uint16_t VDPPALETTE_COLORMASK = 0x0EEE;

	static const std::uint16_t VDPPALETTE_REDSFT = 1;
	static const std::uint16_t VDPPALETTE_GREENSFT = 5;
	static const std::uint16_t VDPPALETTE_BLUESFT = 9;

	static constexpr const std::vector<std::uint8_t>& GetValidComponentValues()
	{
		const static std::vector<std::uint8_t> oValidComponentValues = { 0, 49, 87, 119, 146, 174, 206, 255 };
		return oValidComponentValues;
	}

	static constexpr std::vector<LABColor> GetWarmGradientInitial()
	{
		static std::vector<LABColor> oWarmGradient;
		const std::vector<ColorRGB>& oWarmGradientLinear = GetWarmGradientLinear();
		oWarmGradient.reserve(oWarmGradientLinear.size());
		for (const ColorRGB& oColor : oWarmGradientLinear)
			oWarmGradient.emplace_back(oColor);
		return oWarmGradient;
	}

	static constexpr std::vector<ColorRGB> GetWarmGradientLinearInitial()
	{
		std::vector<ColorRGB> oWarmGradient;
		const std::vector<std::uint8_t>& oValidComponentValues = GetValidComponentValues();
		const size_t uValidComponentsCount = oValidComponentValues.size();
		oWarmGradient.reserve(3 * uValidComponentsCount);
		ColorRGB oNextColor{};
		for (std::uint8_t uR : oValidComponentValues)
		{
			oNextColor.m_uR = uR == 0 ? 48 : uR;
			oWarmGradient.emplace_back(oNextColor);
		}

		for (std::uint8_t uG : oValidComponentValues)
		{
			oNextColor.m_uG = uG;
			oWarmGradient.emplace_back(oNextColor);
		}

		for (std::uint8_t uB : oValidComponentValues)
		{
			oNextColor.m_uB = uB;
			oWarmGradient.emplace_back(oNextColor);
		}

		return oWarmGradient;
	}

	static constexpr std::vector<LABColor> GetCoolGradientInitial()
	{
		static std::vector<LABColor> oCoolGradient;
		const std::vector<ColorRGB>& oCoolGradientLinear = GetCoolGradientLinear();
		oCoolGradient.reserve(oCoolGradientLinear.size());
		for (const ColorRGB& oColor : oCoolGradientLinear)
			oCoolGradient.emplace_back(oColor);
		return oCoolGradient;
	}

	static constexpr std::vector<ColorRGB> GetCoolGradientLinearInitial()
	{
		std::vector<ColorRGB> oCoolGradient;
		const std::vector<std::uint8_t>& oValidComponentValues = GetValidComponentValues();
		const size_t uValidComponentsCount = oValidComponentValues.size();
		oCoolGradient.reserve(3 * uValidComponentsCount);
		ColorRGB oNextColor{};
		for (std::uint8_t uB : oValidComponentValues)
		{
			oNextColor.m_uB = uB;
			oCoolGradient.emplace_back(oNextColor);
		}

		for (std::uint8_t uG : oValidComponentValues)
		{
			oNextColor.m_uG = uG;
			oCoolGradient.emplace_back(oNextColor);
		}

		for (std::uint8_t uR : oValidComponentValues)
		{
			oNextColor.m_uR = uR;
			oCoolGradient.emplace_back(oNextColor);
		}

		return oCoolGradient;
	}

	static constexpr std::vector<LABColor> GetMegadriveMasterPaletteInitial()
	{
		std::vector<LABColor> oMasterPalette;
		const std::vector<std::uint8_t>& oValidComponentValues = GetValidComponentValues();
		const size_t uValidComponentsCount = oValidComponentValues.size();
		oMasterPalette.reserve(uValidComponentsCount * uValidComponentsCount * uValidComponentsCount);
		for (std::uint8_t uR : oValidComponentValues)
		{
			for (std::uint8_t uG : oValidComponentValues)
			{
				for (std::uint8_t uB : oValidComponentValues)
					oMasterPalette.emplace_back(ColorRGB{ uR, uG, uB });
			}
		}
		return oMasterPalette;
	}
};