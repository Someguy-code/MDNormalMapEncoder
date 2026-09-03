export module PaletteReduction;

import CollectionUtils;
import LABColor;
import KMeansUtils;
import MathUtils;
import MegadriveUtils;
import RawImage;
import std;

export struct PaletteReduction
{
	static std::vector<LABColor> GetReducedPalette(const RawImage<LABColor>& _oImage, const std::optional<RawImage<bool>>& _oMask, unsigned int _uWantedPaletteColorsCount)
	{
		const std::vector<LABColor> oUniqueColors = GetUniqueColors(_oImage, _oMask);
		if (oUniqueColors.size() < _uWantedPaletteColorsCount)
			return PurgeRepreatedColors(std::move(GetMegadrivePalette(oUniqueColors)));
		KMeansUtils<LABColor>::KMeans oKMeans = KMeansUtils<LABColor>::GetKMeans(oUniqueColors, _uWantedPaletteColorsCount, 10, GetColorInMegadrivePalette{});
		std::vector<LABColor>& oReducedPalette = oKMeans.m_oCentroids;
		oReducedPalette.insert(oReducedPalette.begin(), LABColor{});
		return oReducedPalette;
	}

private:

	struct GetColorInMegadrivePalette
	{
		LABColor operator()(const LABColor& _oColor) const
		{
			const std::vector<LABColor>& oMasterPalette = MegadriveUtils::GetMegadriveMasterPalette();
			return CollectionUtils::GetClosestValue<LABColor, std::vector<LABColor>, CollectionUtils::GetSqrDistanceInvocable<LABColor>>(_oColor, oMasterPalette);
		}
	};

	static std::vector<LABColor> GetUniqueColors(const RawImage<LABColor>& _oImage, const std::optional<RawImage<bool>>& _oMask)
	{
		std::unordered_set<LABColor> oUniqueColors;
		const std::vector<LABColor>& oPixelsArray = _oImage.m_oPixelsArray;
		const std::vector<bool>* pMaskPixelsArray = _oMask.has_value() ? &_oMask.value().m_oPixelsArray : nullptr;
		const size_t uPixelsCount = oPixelsArray.size();
		for (size_t uPixelIndex = 0; uPixelIndex < uPixelsCount; ++uPixelIndex)
		{
			if (pMaskPixelsArray == nullptr || !(*pMaskPixelsArray)[uPixelIndex])
				oUniqueColors.insert(oPixelsArray[uPixelIndex]);
		}

		return { oUniqueColors.cbegin(), oUniqueColors.cend() };
	}

	static std::vector<LABColor> GetMegadrivePalette(const std::vector<LABColor>& _oPalette)
	{
		std::vector<LABColor> oMegadrivePalette;
		oMegadrivePalette.reserve(_oPalette.size());
		std::transform(_oPalette.cbegin(), _oPalette.cend(), std::back_inserter(oMegadrivePalette), GetColorInMegadrivePalette{});
		return oMegadrivePalette;
	}

	static std::vector<LABColor> PurgeRepreatedColors(std::vector<LABColor> _oPalette)
	{
		std::sort(_oPalette.begin(), _oPalette.end());
		auto itLast = std::unique(_oPalette.begin(), _oPalette.end());
		_oPalette.erase(itLast, _oPalette.end());
		return _oPalette;
	}
};