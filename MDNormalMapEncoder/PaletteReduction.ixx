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
		KMeansUtils<LABColor>::KMeans oKMeans = KMeansUtils<LABColor>::GetKMeans(oUniqueColors, _uWantedPaletteColorsCount, 1);
		std::vector<LABColor> oReducedPalette = GetCollapseRepeatedColors(std::move(GetMegadrivePalette(oKMeans.m_oCentroids)), std::move(oKMeans.m_oGroups));
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

	static std::vector<LABColor> GetCollapseRepeatedColors(std::vector<LABColor> _oCentroids, std::vector<std::vector<LABColor>> _oGroups)
	{
		auto oCentroidModified = std::vector<bool>(_oCentroids.size());
		size_t uCentroidsCount;
		do
		{
			uCentroidsCount = _oCentroids.size();
			for (size_t uCentroidIndex = 0; uCentroidIndex < uCentroidsCount; ++uCentroidIndex)
			{
				const LABColor& oCentroid = _oCentroids[uCentroidIndex];
				std::vector<LABColor>& oGroup = _oGroups[uCentroidIndex];
				for (size_t uOtherCentroidIndex = uCentroidIndex + 1; uOtherCentroidIndex < uCentroidsCount; ++uOtherCentroidIndex)
				{
					if (oCentroid == _oCentroids[uOtherCentroidIndex])
					{
						const std::vector<LABColor> oOtherGroup = _oGroups[uOtherCentroidIndex];
						oGroup.append_range(oOtherGroup);
						_oCentroids.erase(_oCentroids.begin() + uOtherCentroidIndex);
						_oGroups.erase(_oGroups.begin() + uOtherCentroidIndex);
						oCentroidModified[uCentroidIndex] = true;
						--uOtherCentroidIndex;
						--uCentroidsCount;
					}
				}
			}
		}
		while (uCentroidsCount != _oCentroids.size());

		for (size_t uCentroidIndex = 0; uCentroidIndex < uCentroidsCount; ++uCentroidIndex)
		{
			if (oCentroidModified[uCentroidIndex])
			{
				const std::vector<LABColor>& oGroup = _oGroups[uCentroidIndex];
				_oCentroids[uCentroidIndex] = std::accumulate(oGroup.cbegin(), oGroup.cend(), LABColor{}) / static_cast<float>(oGroup.size());
			}
		}

		return _oCentroids;
	}
};