export module Encoder;

import BMPHandler;
import Color;
import DitherImage;
import EncoderArguments;
import ImageComponentMerge;
import ImageSplit;
import LABColor;
import Material;
import MathUtils;
import MegadriveUtils;
import NormalMapUtils;
import NormalMapPalette;
import PaletteReduction;
import PaletteUtils;
import RawImage;
import std;
import Vector3;

export struct Encoder
{
    struct Result
    {
        std::vector<ImageSplit::ImageAndPalette> m_oImagesAndPalettes;
        std::optional<Material> m_oMaterial;
    };

    static Result Encode(const EncoderArguments& _oArguments)
    {
        std::optional<Dimensions> oRefDimensions;
        const Optional1BitImage oMask = GetMask(_oArguments, oRefDimensions);
        const OptionalImageAndPallete oNormalMapAndPallete = GetNormalMap(_oArguments, oMask, oRefDimensions);
        const OptionalImageAndPallete oAlbedoAndPallete = GetAlbedo(_oArguments, oMask, oRefDimensions);
        const Optional1BitImage oAmbientOcclusion = GetAmbientOcclusion(_oArguments, oMask, oRefDimensions);
        
        std::optional<Material> oMaterial;
        if(oNormalMapAndPallete)
            oMaterial = GetMaterial(_oArguments.m_sOutputMaterialsFilename, oNormalMapAndPallete, oAlbedoAndPallete, oAmbientOcclusion, _oArguments);
        return {
            GetEncodedImage(_oArguments.m_sBaseOutputFilename, oNormalMapAndPallete, oAlbedoAndPallete, oAmbientOcclusion),
            std::move(oMaterial)
        };
    }

private:

    using OptionalImageAndPallete = std::optional<std::pair<RawImage<std::uint8_t>, std::vector<ColorRGB>>>;
    using Optional1BitImage = std::optional<RawImage<bool>>;
    struct Dimensions
    {
        unsigned int m_uWidth;
        unsigned int m_uHeight;
    };

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

    static Optional1BitImage GetMask(const EncoderArguments& _oArguments, std::optional<Dimensions>& _oDimensions)
    {
        RawImage<ColorRGB> oSourceMask;
        const char* sInputMaskFilename = _oArguments.m_sMaskFilename;
        bool bHasMask = sInputMaskFilename != nullptr;
        if (bHasMask)
        {
            try
            {
                std::cout << "Reading mask " << sInputMaskFilename << "\n";
                oSourceMask = GetInputImage(sInputMaskFilename, _oDimensions);
            }
            catch (const std::runtime_error& oException)
            {
                throw std::runtime_error(std::format("Exception reading {}: {}", sInputMaskFilename, oException.what()));
            }
            catch (...)
            {
                throw std::runtime_error(std::format("Unhandled exception reading {}", sInputMaskFilename));
            }
        }
        else
            return std::nullopt;

        struct ConvertToFlags
        {
            bool operator()(const ColorRGB& _oColor) { return _oColor.IsNull(); }
        };

        RawImage<bool> oSourceMaskFlags;
        oSourceMaskFlags = oSourceMask.GetTypeConversion<bool, ConvertToFlags>();

        return oSourceMaskFlags;
    }

    static OptionalImageAndPallete GetNormalMap(const EncoderArguments& _oArguments, const std::optional<RawImage<bool>>& _oMask, std::optional<Dimensions>& _oDimensions)
    {
        RawImage<ColorRGB> oSourceNormalMap;
        const char* sInputNormalMapFilename = _oArguments.m_sNormalFilename;
        bool bHasNormalMap = sInputNormalMapFilename != nullptr;
        if (bHasNormalMap)
        {
            try
            {
                std::cout << "Reading normal map " << sInputNormalMapFilename << "\n";
                oSourceNormalMap = GetInputImage(sInputNormalMapFilename, _oDimensions);
            }
            catch (const std::runtime_error& oException)
            {
                throw std::runtime_error(std::format("Exception reading {}: {}", sInputNormalMapFilename, oException.what()));
            }
            catch (...)
            {
                throw std::runtime_error(std::format("Unhandled exception reading {}", sInputNormalMapFilename));
            }
        }
        else
            return std::nullopt;

        RawImage<std::uint8_t> oDitheredTargetNormalMap;
        std::vector<ColorRGB> oNormalMapPalette;

        const unsigned int uVerticalSidesCount = _oArguments.m_uVerticalNormalMapSides;
        const unsigned int uHorizontalSidesCount = _oArguments.m_uHorizontalNormalMapSides;

        const std::vector<Vector3> oNormalsPalette = NormalMapPalette::GetNormalsPalette(uHorizontalSidesCount, uVerticalSidesCount);
        oNormalMapPalette.reserve(oNormalsPalette.size());
        for (const Vector3& oNormal : oNormalsPalette)
            oNormalMapPalette.push_back(NormalMapUtils::GetColorFromNormal(oNormal));

        RawImage<Vector3> oSourceNormalMapNormals = NormalMapUtils::GetNormalMapNormals(oSourceNormalMap);
        oDitheredTargetNormalMap = DitheredImage<NormalMapError, Vector3>::GetDitheredImage(oSourceNormalMapNormals, oNormalsPalette, false, _oMask);

        return { { oDitheredTargetNormalMap, oNormalMapPalette } };
    }

    static OptionalImageAndPallete GetAlbedo(const EncoderArguments& _oArguments, const Optional1BitImage& _oMask, std::optional<Dimensions>& _oDimensions)
    {
        RawImage<ColorRGB> oSourceAlbedo;
        const char* sInputAlbedoFilename = _oArguments.m_sAlbedoFilename;
        bool bHasAlbedo = sInputAlbedoFilename != nullptr;
        if (bHasAlbedo)
        {
            try
            {
                std::cout << "Reading albedo " << sInputAlbedoFilename << "\n";
                oSourceAlbedo = GetInputImage(sInputAlbedoFilename, _oDimensions);
            }
            catch (const std::runtime_error& oException)
            {
                throw std::runtime_error(std::format("Exception reading {}: {}", sInputAlbedoFilename, oException.what()));
            }
            catch (...)
            {
                throw std::runtime_error(std::format("Unhandled exception reading {}", sInputAlbedoFilename));
            }
        }
        else
            return std::nullopt;

        std::vector<ColorRGB> oAlbedoPalette;
        RawImage<std::uint8_t> oDitheredAlbedo;
        if (bHasAlbedo)
        {
            const RawImage<LABColor> oSourceAlbeldoLAB = oSourceAlbedo.GetTypeConversion<LABColor>();
            const std::vector<LABColor> oAlbedoPaletteLAB = PaletteReduction::GetReducedPalette(oSourceAlbeldoLAB, _oMask, _oArguments.m_uMaxAlbedoColors);
            oDitheredAlbedo = DitheredImage<LABColorError, LABColor>::GetDitheredImage(oSourceAlbeldoLAB, oAlbedoPaletteLAB, false, _oMask);
            oAlbedoPalette.reserve(oAlbedoPaletteLAB.size());
            for (const LABColor& oLABColor : oAlbedoPaletteLAB)
                oAlbedoPalette.push_back(static_cast<ColorRGB>(oLABColor));
        }

        return { {oDitheredAlbedo, oAlbedoPalette} };
    }

    static Optional1BitImage GetAmbientOcclusion(const EncoderArguments& _oArguments, const Optional1BitImage& _oMask, std::optional<Dimensions>& _oDimensions)
    {
        RawImage<ColorRGB> oSourceAmbientOcclusion;
        const char* sInputAmbientOcclusionFilename = _oArguments.m_sAmbinetOcclusionFilename;
        bool bHasAmbientOcclusion = sInputAmbientOcclusionFilename != nullptr;
        if (bHasAmbientOcclusion)
        {
            try
            {
                std::cout << "Reading ambient occlusion " << sInputAmbientOcclusionFilename << "\n";
                oSourceAmbientOcclusion = GetInputImage(sInputAmbientOcclusionFilename, _oDimensions);
            }
            catch (const std::runtime_error& oException)
            {
                throw std::runtime_error(std::format("Exception reading {}: {}", sInputAmbientOcclusionFilename, oException.what()));
            }
            catch (...)
            {
                throw std::runtime_error(std::format("Unhandled exception reading {}", sInputAmbientOcclusionFilename));
            }
        }
        else
            return std::nullopt;

        RawImage<std::uint8_t> oDitheredAmbientOcclusion;
        if (bHasAmbientOcclusion)
        {
            const RawImage<LABColor> oSourceAlbeldoLAB = oSourceAmbientOcclusion.GetTypeConversion<LABColor>();
            oDitheredAmbientOcclusion = DitheredImage<LABColorError, LABColor>::GetDitheredImage(oSourceAlbeldoLAB, std::vector<LABColor>{ColorRGB{ 0, 0, 0 }, ColorRGB{ 255, 255, 255 }}, true, _oMask);
        }

        struct ConvertToFlags
        {
            bool operator()(const std::uint8_t _uColorIndex) { return _uColorIndex == 0; }
        };

        return { oDitheredAmbientOcclusion.GetTypeConversion<bool, ConvertToFlags>() };
    }

    static std::vector<ImageSplit::ImageAndPalette> GetEncodedImage(const char* _sBaseOutputFilename, const OptionalImageAndPallete& _oNormalMapAndPallete, const OptionalImageAndPallete& _oAlbedoAndPallete, const Optional1BitImage& _oAmbientOcclusion)
    {
        const auto& [oMergedImage, oMergedPalette] = ImageComponentMerge::GetMergedResult(_oNormalMapAndPallete, _oAlbedoAndPallete, _oAmbientOcclusion);
        return ImageSplit::GetSplitImageAndPalette(oMergedImage, oMergedPalette);
    }

    static Material GetMaterial(const char* _sOutputMaterialsFilename, const OptionalImageAndPallete& _oNormalMapAndPallete, const OptionalImageAndPallete& _oAlbedoAndPallete, const Optional1BitImage& oAmbientOcclusion, const EncoderArguments& _oArguments)
    {
        //If no albedo was provided, default to pure white
        const std::vector<ColorRGB> oDefaultAlbedo = { {255, 255, 255} };
        const unsigned char uLogLightShadesCount = static_cast<unsigned char>(_oArguments.m_uLogLightShadesCount);
        const float fSpecularIntensity = _oArguments.m_fSpecularIntensity;
        return { 1, oAmbientOcclusion.has_value(),
            _oAlbedoAndPallete.has_value() ? std::span{_oAlbedoAndPallete->second}.subspan(1) : oDefaultAlbedo,
            std::span{_oNormalMapAndPallete->second}.subspan(1), _oArguments.m_oFrontLightColor, _oArguments.m_oBackLightColor, _oArguments.m_oPureDarknessColor, uLogLightShadesCount,
            fSpecularIntensity > 0.f ? std::optional{SpecularData{_oArguments.m_fSpecularHardness, fSpecularIntensity, uLogLightShadesCount, uLogLightShadesCount}} : std::nullopt };
    }
};
