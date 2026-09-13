export module EncoderArgumentsBuilder;

import BMPHandler;
import Color;
import CommandLineArguments;
import EncoderArguments;
import RawImage;
import std;

export struct EncoderArgumentsBuilder
{
	static EncoderArguments GetEncoderArguments(const CommandLineArguments& _oCommandLineArguments)
	{
		std::optional<Dimensions> oImageDimensions;
		return {
			.m_oMask = GetInputImageSafe("mask", _oCommandLineArguments.m_sMaskFilename, oImageDimensions),
			.m_oNormal = GetInputImageSafe("normal map", _oCommandLineArguments.m_sNormalFilename, oImageDimensions),
			.m_oAlbedo = GetInputImageSafe("albedo", _oCommandLineArguments.m_sAlbedoFilename, oImageDimensions),
			.m_oAmbinetOcclusion = GetInputImageSafe("ambient occlussion", _oCommandLineArguments.m_sAmbientOcclussionFilename, oImageDimensions),
			.m_uHorizontalNormalMapSides = _oCommandLineArguments.m_uHorizontalNormalMapSides,
			.m_uVerticalNormalMapSides = _oCommandLineArguments.m_uVerticalNormalMapSides,
			.m_uMaxAlbedoColors = _oCommandLineArguments.m_uMaxAlbedoColors,
			.m_uLogLightShadesCount = _oCommandLineArguments.m_uLogLightShadesCount,
			.m_oFrontLightColor = _oCommandLineArguments.m_oFrontLightColor,
			.m_oBackLightColor = _oCommandLineArguments.m_oBackLightColor,
			.m_oPureDarknessColor = _oCommandLineArguments.m_oPureDarknessColor,
			.m_fSpecularIntensity = _oCommandLineArguments.m_fSpecularIntensity,
			.m_fSpecularHardness = _oCommandLineArguments.m_fSpecularHardness,
			.m_uSpecularLogShadesCount = _oCommandLineArguments.m_uSpecularLogShadesCount,
		};
	}
private:
	struct Dimensions
	{
		unsigned int m_uWidth;
		unsigned int m_uHeight;
	};

	static std::optional<RawImage<ColorRGB> > GetInputImageSafe(const char* _sImageName, const char* _sFilename, std::optional<Dimensions>& _oDimensions)
	{
		if (_sFilename != nullptr)
		{
			try
			{
				std::cout << "Reading " << _sImageName << " " << _sFilename << std::endl;
				return GetInputImage(_sFilename, _oDimensions);
			}
			catch (const std::runtime_error& oException)
			{
				throw std::runtime_error(std::format("Exception reading {}: {}", _sFilename, oException.what()));
			}
			catch (...)
			{
				throw std::runtime_error(std::format("Unhandled exception reading {}", _sFilename));
			}
		}
		else
			return std::nullopt;
	}

	static RawImage<ColorRGB> GetInputImage(const char* _sFilename, std::optional<Dimensions>& _oDimensions)
	{
		const BMPHandler oSourceBMP(_sFilename);
		ValidateImageDimensions(oSourceBMP, _oDimensions);
		const unsigned int uWidth = oSourceBMP.GetWidth();
		const unsigned int uHeight = oSourceBMP.GetHeight();
		std::vector<ColorRGB> oPixelsArray(uWidth * uHeight);
		ColorRGB* pPixelColors = oPixelsArray.data();
		for (unsigned int uY = 0; uY < uHeight; ++uY)
		{
			for (unsigned int uX = 0; uX < uWidth; ++uX)
				*pPixelColors++ = oSourceBMP[uY, uX];
		}

		return { uWidth , uHeight, oPixelsArray };
	}

	static bool ValidateImageDimensions(const BMPHandler& _oImage, std::optional<Dimensions>& _oRefDimensions)
	{
		if (!_oRefDimensions.has_value())
		{
			_oRefDimensions = {
				.m_uWidth = _oImage.GetWidth(),
				.m_uHeight = _oImage.GetHeight()
			};
		}
		else if (_oRefDimensions.value().m_uWidth != _oImage.GetWidth() || _oRefDimensions.value().m_uHeight != _oImage.GetHeight())
			throw std::runtime_error("Dimensions missmatch (all input images must have matching dimensions)");

		return true;
	};
};