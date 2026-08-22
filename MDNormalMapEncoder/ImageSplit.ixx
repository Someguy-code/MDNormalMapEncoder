export module ImageSplit;

import Color;
import MathUtils;
import RawImage;
import std;

export struct ImageSplit
{
	struct ImageAndPalette
	{
		RawImage<std::uint8_t> m_oImage;
		std::vector<ColorRGB> m_oPalette;
	};

	//Splits the image into a group of images in order to respect the 16 colour limit
	static std::vector<ImageAndPalette> GetSplitImageAndPalette(const RawImage<std::uint8_t>& _oImage, const std::vector<ColorRGB>& _oPalette)
	{
		const std::uint8_t uColorsCount = static_cast<std::uint8_t>(_oPalette.size());
		const std::uint8_t uMaxColorsCount = 16;
		const unsigned int uPartCount = MathUtils::DivRoundUp(uColorsCount, uMaxColorsCount);
		std::vector<ImageAndPalette> oSplitImageAndPalettes;
		oSplitImageAndPalettes.reserve(uPartCount);
		if (uColorsCount < uMaxColorsCount)
		{
			oSplitImageAndPalettes.emplace_back(_oImage, _oPalette);
		}
		else
		{
			std::uint8_t uBaseColorIndex = 0;
			for (unsigned int uPartIndex = 0; uPartIndex < uPartCount; ++uPartIndex)
			{
				const bool bIsFirstPart = uPartIndex == 0;
				const bool bIsLastPart = uPartIndex == uPartCount - 1;
				const std::uint8_t uPartIndexOffset = bIsFirstPart ? 0 : 1;
				const std::uint8_t uPartColorsCount = static_cast<std::uint8_t>(std::min(uColorsCount - uBaseColorIndex + uPartIndexOffset, static_cast<int>(uMaxColorsCount)));
				ImageAndPalette& oPart = oSplitImageAndPalettes.emplace_back(RawImage<std::uint8_t>{_oImage.m_uWidth, _oImage.m_uHeight }, std::vector<ColorRGB>{ uPartColorsCount });
				std::copy(_oPalette.cbegin() + uBaseColorIndex,
					bIsLastPart ? _oPalette.cend() :_oPalette.cbegin() + uBaseColorIndex + uPartColorsCount - uPartIndexOffset,
					oPart.m_oPalette.begin() + uPartIndexOffset);
				struct GetPartColorIndex
				{
					const std::uint8_t m_uPartIndexOffset;
					const std::uint8_t m_uMinColorIndex;
					const std::uint8_t m_iMaxColorIndex;
					std::uint8_t operator()(std::uint8_t _uOriginalIndex) const
					{
						return (_uOriginalIndex < m_uMinColorIndex || _uOriginalIndex > m_iMaxColorIndex) ? 0 : _uOriginalIndex - m_uMinColorIndex + m_uPartIndexOffset;
					}
				};
				const std::vector<std::uint8_t>& oOriginalPixelsArray = _oImage.m_oPixelsArray;
				std::transform(oOriginalPixelsArray.cbegin(), oOriginalPixelsArray.cend(), oPart.m_oImage.m_oPixelsArray.begin(),
					GetPartColorIndex{ uPartIndexOffset, uBaseColorIndex, std::uint8_t(uBaseColorIndex + uPartColorsCount - 1) });
				uBaseColorIndex += uMaxColorsCount;
			}
		}
		return oSplitImageAndPalettes;
	}
};