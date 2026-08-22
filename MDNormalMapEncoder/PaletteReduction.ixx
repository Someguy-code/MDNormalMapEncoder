export module PaletteReduction;

import LABColor;
import MathUtils;
import MegadriveUtils;
import RawImage;
import std;

export struct PaletteReduction
{
	static std::vector<LABColor> GetReducedPalette(const RawImage<LABColor>& _oImage, const std::optional<RawImage<bool>>& _oMask, unsigned int _uWantedPaletteColorsCount)
	{
		const std::unordered_map<LABColor, unsigned int> oColorsCount = GetColorsCount(_oImage, _oMask);
		const std::vector<LABColor> oMainColors = GetMainColors(oColorsCount, _uWantedPaletteColorsCount);
		const std::unordered_map<LABColor, std::vector<ColorCount>> oColorGroups = GetColorGroups(oMainColors, oColorsCount);
		return GetReducedPalette(oColorGroups);
	}

private:

	struct ColorCount
	{
		LABColor m_oColor;
		unsigned int m_uCount = 0;
	};

	static std::unordered_map<LABColor, unsigned int> GetColorsCount(const RawImage<LABColor>& _oImage, const std::optional<RawImage<bool>>& _oMask)
	{
		std::unordered_map<LABColor, unsigned int> oColorsCount;
		const std::vector<LABColor>& oPixelsArray = _oImage.m_oPixelsArray;
		const std::vector<bool>* pMaskPixelsArray = _oMask.has_value() ? &_oMask.value().m_oPixelsArray : nullptr;
		const size_t uPixelsCount = oPixelsArray.size();
		for (size_t uPixelIndex = 0; uPixelIndex < uPixelsCount; ++uPixelIndex)
		{
			if (pMaskPixelsArray == nullptr || !(*pMaskPixelsArray)[uPixelIndex])
			{
				const LABColor& oColor = oPixelsArray[uPixelIndex];
				auto itColor = oColorsCount.find(oColor);
				if (itColor == oColorsCount.end())
					oColorsCount[oColor] = 1;
				else
					++(itColor->second);
			}
		}

		return oColorsCount;
	}

	static std::vector<LABColor> GetMainColors(const std::unordered_map<LABColor, unsigned int>& _oColorsCount, unsigned int _uWantedPaletteColorsCount)
	{
		struct IsColorMoreUsedComparer
		{
			bool operator()(const ColorCount& _oLeftHandSide, const ColorCount& _oRightHandSide) const { return _oLeftHandSide.m_uCount > _oRightHandSide.m_uCount; }
		};

		std::vector<ColorCount> oMainColorCounts;
		oMainColorCounts.reserve(_uWantedPaletteColorsCount);
		std::priority_queue< ColorCount, std::vector<ColorCount>, IsColorMoreUsedComparer > oMainColorsPriorityQueue{ {}, oMainColorCounts };
		unsigned int uMinCount = _oColorsCount.cbegin()->second;
		for (const auto [oColor, uCount] : _oColorsCount)
		{
			if (oMainColorsPriorityQueue.size() < _uWantedPaletteColorsCount)
			{
				//There is still room in the palette, so add it unconditionally
				uMinCount = std::min(uMinCount, uCount);
				oMainColorsPriorityQueue.emplace(oColor, uCount);
			}
			else if (uMinCount < uCount)
			{
				//Replace the currently least used color in the palette with the new one
				(void)oMainColorsPriorityQueue.pop();
				const unsigned int uNextMinCount = oMainColorsPriorityQueue.top().m_uCount;
				uMinCount = std::min(uNextMinCount, uCount);
				oMainColorsPriorityQueue.emplace(oColor, uCount);
			}
		}

		std::vector<LABColor> oMainColors;
		oMainColors.reserve(oMainColorsPriorityQueue.size());
		while (!oMainColorsPriorityQueue.empty())
		{
			oMainColors.emplace_back(oMainColorsPriorityQueue.top().m_oColor);
			oMainColorsPriorityQueue.pop();
		}

		return oMainColors;
	}

	static LABColor GetClosestColor(const std::vector<LABColor>& _oMainColors, const LABColor& _oColor)
	{
		//Find distances to each of the main colors
		struct ColorDistance
		{
			LABColor m_oColor;
			float m_fDistance;

			bool operator()(const ColorDistance& _oOtherValue) const { return m_fDistance < _oOtherValue.m_fDistance; }
		};
		std::vector<ColorDistance> oMainColorsDistances;
		oMainColorsDistances.reserve(_oMainColors.size());
		struct GetColorDistance
		{
			LABColor m_oRefColor;
			GetColorDistance(const LABColor& _oRefColor): m_oRefColor(_oRefColor) {}
			const ColorDistance operator()(const LABColor& _oLeftHandSide) const { return { _oLeftHandSide, _oLeftHandSide.GetDistance(m_oRefColor) }; }
		};
		std::transform(_oMainColors.cbegin(), _oMainColors.cend(), std::back_inserter(oMainColorsDistances), GetColorDistance{_oColor});

		//Pick the closest color
		struct GetMinDistance
		{
			bool operator()(const ColorDistance& _oLeftHandSide, const ColorDistance& _oRightHandSide) const { return _oLeftHandSide.m_fDistance < _oRightHandSide.m_fDistance; }
		};

		return std::min_element(oMainColorsDistances.cbegin(), oMainColorsDistances.cend(), GetMinDistance{})->m_oColor;
	}

	static std::unordered_map<LABColor, std::vector<ColorCount>> GetColorGroups(const std::vector<LABColor>& _oMainColors, const std::unordered_map<LABColor, unsigned int>& oColorsCount)
	{
		std::unordered_map<LABColor, std::vector<ColorCount>> oColorGroups;

		//Create color groups
		const size_t uMainColorsCount = _oMainColors.size();
		oColorGroups.reserve(uMainColorsCount);
		for (const LABColor& oColor : _oMainColors)
			oColorGroups[oColor] = {};
		//Fill groups with closest colors
		for(const auto [oColor, uCount] : oColorsCount)
		{
			if (auto itMainColorCount = oColorGroups.find(oColor); itMainColorCount != oColorGroups.cend())
				itMainColorCount->second.emplace_back(oColor, uCount);
			else
			{
				const LABColor& oClosestMainColor = GetClosestColor(_oMainColors, oColor);
				oColorGroups[oClosestMainColor].emplace_back(oColor, uCount);
			}
		}

		return oColorGroups;
	}

	static std::vector<LABColor> GetReducedPalette(const std::unordered_map<LABColor, std::vector<ColorCount>>& _oColorGroups)
	{
		std::vector<LABColor> oReducedPalette;
		oReducedPalette.reserve(_oColorGroups.size());
		oReducedPalette.emplace_back(ColorRGB{ 0, 0, 0 });
		for (const auto [oMainColor, oColorGroup] : _oColorGroups)
		{
			struct AccumulateCount
			{
				unsigned int operator()(unsigned int _uTotalCount, const ColorCount& _oColorCount) const { return _uTotalCount + _oColorCount.m_uCount; }
			};
			const unsigned int uTotalCount = std::accumulate(oColorGroup.cbegin(), oColorGroup.cend(), 0, AccumulateCount{});

			struct AccumulateColor
			{
				const float m_fInverseTotalCount;
				AccumulateColor(unsigned int _uTotalCount) :m_fInverseTotalCount(1.f / static_cast<float>(_uTotalCount)) {}
				LABColor operator()(const LABColor& _oAccumulatedColor, const ColorCount& _oColorCount) const
				{ 
					const float fMultiplier = static_cast<float>(_oColorCount.m_uCount) * m_fInverseTotalCount;
					const auto [fL, fA, fB] = _oColorCount.m_oColor;
					return { _oAccumulatedColor.m_fL + fMultiplier * fL, _oAccumulatedColor.m_fA + fMultiplier * fA, _oAccumulatedColor.m_fB + fMultiplier * fB };
				}
			};
			const LABColor oFinalColor = std::accumulate(oColorGroup.cbegin(), oColorGroup.cend(), LABColor{ 0.f, 0.f, 0.f }, AccumulateColor{ uTotalCount });

			oReducedPalette.emplace_back(oFinalColor);
		}
		return GetClosestMegadriveColors(oReducedPalette);
	}

	static std::vector<LABColor> GetClosestMegadriveColors(const std::vector<LABColor>& _oColors)
	{
		const std::vector<LABColor>& oMasterPalette = MegadriveUtils::GetMegadriveMasterPalette();
		std::vector<LABColor> oMegadriveColors;
		oMegadriveColors.reserve(_oColors.size());
		std::transform(_oColors.cbegin(), _oColors.cend(), std::back_inserter(oMegadriveColors),
			[&oMasterPalette](const LABColor& _oColor) {
				return GetClosestColor(oMasterPalette, _oColor);
			});
		return oMegadriveColors;
	}
};