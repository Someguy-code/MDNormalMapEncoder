module;
#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>

export module PaletteUtils;

import CollectionUtils;
import RawImage;
import std;

export namespace PaletteUtils
{
	template<typename ColorType>
	RawImage<ColorType> GetCollapsedImage(const RawImage<std::uint8_t> _oPlaettizedImage, std::span<const ColorType> _oPalette)
	{
		RawImage<ColorType> oCollapsedImage{ _oPlaettizedImage.m_uWidth, _oPlaettizedImage.m_uHeight };
		std::vector<ColorType>& oCollapsedPixels = oCollapsedImage.m_oPixelsArray;
		const std::vector<std::uint8_t>& oPalettizedPixels = _oPlaettizedImage.m_oPixelsArray;
		const size_t uPixeldCount = oPalettizedPixels.size();
		for (size_t uPixelIndex = 0; uPixelIndex < uPixeldCount; ++uPixelIndex)
			oCollapsedPixels[uPixelIndex] = _oPalette[oPalettizedPixels[uPixelIndex]];
		return oCollapsedImage;
	}

	template<typename ColorType, typename ColorDistance = CollectionUtils::GetSqrDistanceInvocable<ColorType>>
	unsigned int GetClosestColorIndex(const ColorType& _oColor, std::span<const ColorType> _oPalette)
	{
		unsigned int uClosestIndex = 0;
		float fClosestDistance = std::numeric_limits<float>::infinity();
		const size_t uColorCount = _oPalette.size();
		const auto oColorDistance = ColorDistance{ _oColor };
		for(unsigned int uColorIndex = 0; uColorIndex < uColorCount; ++uColorIndex)
		{
			const float fDistance = oColorDistance(_oPalette[uColorIndex]);
			if (fDistance < fClosestDistance)
			{
				fClosestDistance = fDistance;
				uClosestIndex = uColorIndex;
			}
		}
		return uClosestIndex;
	}
}