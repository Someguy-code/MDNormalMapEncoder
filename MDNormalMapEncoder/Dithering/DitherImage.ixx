export module DitherImage;

import :Kernels;
import :ErrorTypeInterface;
export import :ColorError;
export import :LABColorError;
export import :NormalMapError;


import RawImage;
import PaletteUtils;
import std;

export template<typename ColorErrorType, typename ColorType>
requires IsErrorType<ColorErrorType, ColorType>
struct DitheredImage
{
	static RawImage<std::uint8_t> GetDitheredImage(const RawImage<ColorType>& _oOriginalImage, std::span<const ColorType> _oPalette, bool _bUseColorZero, const std::optional<RawImage<bool>>& _oMask)
	{
		RawImage<ColorErrorType> oErrorImage = GetErrorImage(_oOriginalImage);
		ApplyErrorDifussionMatrix(oErrorImage, _oOriginalImage, _oPalette);
		return GetPalettizedImage(oErrorImage, _oPalette, _bUseColorZero, _oMask);
	}

private:
	static RawImage<ColorErrorType> GetErrorImage(const RawImage<ColorType>& _oOriginalImage)
	{
		RawImage<ColorErrorType> oErrorImage{ _oOriginalImage.m_uWidth, _oOriginalImage.m_uHeight };
		std::vector<ColorErrorType>& oErrorImagePixels = oErrorImage.m_oPixelsArray;
		const std::vector<ColorType>& oOriginalImagePixels = _oOriginalImage.m_oPixelsArray;
		const size_t uPixelsCount = oOriginalImagePixels.size();
		for (size_t uPixelIndex = 0; uPixelIndex < uPixelsCount; ++uPixelIndex)
			oErrorImagePixels[uPixelIndex] = { oOriginalImagePixels[uPixelIndex] };
		return oErrorImage;
	}

	static void ApplyErrorDifussionMatrix(RawImage<ColorErrorType>& _oErrorImage, const RawImage<ColorType>& _oOriginalImage, std::span<const ColorType> _oPalette)
	{
		const unsigned int uWidth = _oErrorImage.m_uWidth;
		const unsigned int uHeight = _oErrorImage.m_uHeight;

		auto oOriginalImageRect = _oOriginalImage.GetFullRect();
		auto oErrorImageRect = _oErrorImage.GetFullRect();
		for (unsigned int uY = 0; uY < uHeight; ++uY)
		{
			const bool bIsLeftToRight = uY % 2 == 0;
			const unsigned int uXStart = bIsLeftToRight ? 0 : uWidth - 1;
			const int iXEnd = bIsLeftToRight ? uWidth : -1;
			const int iHorizontalStep = bIsLeftToRight ? 1 : -1;
			for (int iX = uXStart; iX != iXEnd; iX += iHorizontalStep)
			{
				ApplyErrorDifussionMatrix(oErrorImageRect, iX, uY, _oPalette, bIsLeftToRight);
			}
		}
	}

	static void ApplyErrorDifussionMatrix(std::mdspan<ColorErrorType, std::dextents<size_t, 2>> _oErrorImageRect, unsigned int _uX, unsigned int _uY, std::span<const ColorType> _oPalette, bool _bIsLeftToRight)
	{
		static const RawImage<float>& oFloydSteinbergErrorDifussionMatrix = GetDitherKernel("FloydSteinberg");

		static const unsigned int uErrorDifussionMatrixHalfWidth = oFloydSteinbergErrorDifussionMatrix.m_uWidth / 2;
		static const unsigned int uErrorDifussionMatrixHalfHeight = oFloydSteinbergErrorDifussionMatrix.m_uHeight / 2;
		static const auto oErrorDifussionMatrix = oFloydSteinbergErrorDifussionMatrix.GetFullRect();

		const unsigned int uWidth = static_cast<unsigned int>(_oErrorImageRect.extent(1));
		const unsigned int uHeight = static_cast<unsigned int>(_oErrorImageRect.extent(0));

		const unsigned int uWindowStartX = _bIsLeftToRight ? std::max<int>(_uX - uErrorDifussionMatrixHalfWidth, 0) : std::min<unsigned int>(_uX + uErrorDifussionMatrixHalfWidth, uWidth - 1);
		const unsigned int uWindowStartY = std::max<int>(_uY - uErrorDifussionMatrixHalfHeight, 0);
		const unsigned int uWindowEndX = _bIsLeftToRight ? std::min<unsigned int>(_uX + uErrorDifussionMatrixHalfWidth, uWidth - 1) + 1 : std::max<int>(_uX - uErrorDifussionMatrixHalfWidth, 0) - 1;
		const unsigned int uWindowEndY = std::min<unsigned int>(_uY + uErrorDifussionMatrixHalfHeight, uHeight - 1);

		unsigned int uErrorDifussionOffsetX = _bIsLeftToRight ? -std::min<int>(_uX - uErrorDifussionMatrixHalfWidth, 0) : 2 * uErrorDifussionMatrixHalfWidth - std::max<int>(_uX + uErrorDifussionMatrixHalfWidth - (uWidth - 1), 0);
		unsigned int uErrorDifussionOffsetY = -std::min<int>(_uY - uErrorDifussionMatrixHalfHeight, 0);

		ColorErrorType& oOldColor = _oErrorImageRect[_uY, _uX];
		const uint8_t uClosestColorIndex = oOldColor.GetClosestColorIndex(_oPalette);
		ColorType oNewColor = _oPalette[uClosestColorIndex];
		const ColorErrorType oError{ oOldColor, oNewColor };
		oOldColor = { oNewColor };
		const int iHorizontalStep = _bIsLeftToRight ? 1 : -1;
		for (unsigned int uWindowY = uWindowStartY, uErrorDiffusionY = uErrorDifussionOffsetY; uWindowY <= uWindowEndY; ++uWindowY, ++uErrorDiffusionY)
		{
			for (int iWindowX = uWindowStartX, uErrorDiffusionX = uErrorDifussionOffsetX; iWindowX != uWindowEndX; iWindowX += iHorizontalStep, uErrorDiffusionX += iHorizontalStep)
			{
				if (uWindowY != _uY || iWindowX != _uX)
				{
					const float fErrorDiffusionMultiplier = oErrorDifussionMatrix[uErrorDiffusionY, uErrorDiffusionX];
					if (fErrorDiffusionMultiplier != 0)
						_oErrorImageRect[uWindowY, iWindowX] += oError * fErrorDiffusionMultiplier;
				}
			}
		}
	}

	static RawImage<std::uint8_t> GetPalettizedImage(RawImage<ColorErrorType>& _oErrorImage, std::span<const ColorType> _oPalette, bool _bUseColorZero, const std::optional<RawImage<bool>>& _oMask)
	{
		struct CollapseErrorImage
		{
			const bool m_bUseColorZero;
			std::span<const ColorType> m_oPalette;
			std::uint8_t operator()(const ColorErrorType& _oColorError) const
			{
				return GetBaseColorIndex() + _oColorError.GetClosestColorIndex(m_oPalette);
			}
			std::uint8_t operator()(const ColorErrorType& _oColorError, bool _bMasked) const
			{
				return _bMasked ? 0 : GetBaseColorIndex() + _oColorError.GetClosestColorIndex(m_oPalette);
			}
			unsigned int GetBaseColorIndex() const { return m_bUseColorZero ? 0 : 1; }
		};

		//If color 0 is not usable, remove it from the palette. Closest color indices will need adjustment to account for it.
		_oPalette = _bUseColorZero ? _oPalette : _oPalette.subspan(1);
		RawImage<std::uint8_t> oPalettizedImage{ _oErrorImage.m_uWidth, _oErrorImage.m_uHeight };
		std::vector<std::uint8_t>& oPalettizedImagePixels = oPalettizedImage.m_oPixelsArray;
		const std::vector<ColorErrorType>& oErrorImagePixels = _oErrorImage.m_oPixelsArray;
		if (_oMask.has_value())
		{
			const std::vector<bool>& oMaskFlags = _oMask->m_oPixelsArray;
			auto oCollapseErrorImageTransform = std::views::zip_transform(CollapseErrorImage{_bUseColorZero, _oPalette}, oErrorImagePixels, oMaskFlags);
			std::ranges::copy(oCollapseErrorImageTransform | std::views::as_rvalue, oPalettizedImagePixels.begin());
		}
		else
			std::transform(oErrorImagePixels.cbegin(), oErrorImagePixels.cend(), oPalettizedImagePixels.begin(), CollapseErrorImage{_bUseColorZero, _oPalette});

		return oPalettizedImage;
	}
};