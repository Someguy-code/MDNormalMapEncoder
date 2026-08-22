export module MDNormalMapEncoder;

import BMPHandler;
import Color;
import DitherImage;
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

export struct MDNormalMapEncoder
{
    struct Arguments
    {
        //Filename for the mask texture. Black pixel will be marked as transparent.
        const char* m_sMaskFilename{ nullptr };
        //Filename for the normal map texture
        const char* m_sNormalFilename{ nullptr };
        //Filename for the albedo texture
        const char* m_sAlbedoFilename{ nullptr };
        //Filename for the ambinet occlusion texture. Will be quantized to black and white.
        const char* m_sAmbinetOcclusionFilename{ nullptr };

        //Number of directions in the X-Y plane. Must be at least 2.
        unsigned int m_uHorizontalNormalMapSides{ 6 };
        //Number of directions on the X-Z axis (including looking straight up). Must be at least 1.
        unsigned int m_uVerticalNormalMapSides{ 3 };
        //Maximum number of colors for albedo
        unsigned int m_uMaxAlbedoColors{ 2 };

        //Base 2 logarithm of the pre-computed shades count
        unsigned int m_uLogLightShadesCount{ 3 };
        //Color of the front light
        ColorRGB m_oFrontLightColor{ 255, 255, 255 };
        //Color of the back light
        ColorRGB m_oBackLightColor{ 0, 0, 0 };
        //Color in absence of light (doesn't modulate)
        ColorRGB m_oPureDarknessColor{ 0, 0, 0 };

        //Intensity of the specular component (in the [0, 1] range)
        float m_fSpecularIntensity{ 0.f };
        //Exponent of the light strength power
        float m_fSpecularHardness{ 1.f };
        //Exponent of the light strength power
        unsigned int m_uSpecularLogShadesCount{ 4 };

        //Prefix filename for the output texture (in case more than one texture need to be generated)
        const char* m_sBaseOutputFilename{ nullptr };
        //Filename of the ouput materials file
        const char* m_sOutputMaterialsFilename{ nullptr };

        void Validate() const
        {
            if (m_sBaseOutputFilename == nullptr)
                throw std::runtime_error("Missing base output texture filename.");
            if (m_sOutputMaterialsFilename == nullptr)
                throw std::runtime_error("Missing output materials filename.");
            if (m_sAlbedoFilename == nullptr && m_sNormalFilename == nullptr)
                throw std::runtime_error("Neither normal map nor albedo textures specified. Nothing to generate.");
            if (m_uHorizontalNormalMapSides < 2)
                throw std::runtime_error("Specified horizontal normal map sides is below 2");
            if (m_uVerticalNormalMapSides < 1)
                throw std::runtime_error("Specified vertical normal map sides is below 1");
            if (m_fSpecularIntensity < 0.f || m_fSpecularIntensity > 1.f)
                throw std::runtime_error("Specified specular intesity is not in the [0, 1] range");
        }
    };

    static void Encode(const Arguments& _oArguments)
    {
        std::optional<Dimensions> oRefDimensions;
        const Optional1BitImage oMask = GetMask(_oArguments, oRefDimensions);
        const OptionalImageAndPallete oNormalMapAndPallete = GetNormalMap(_oArguments, oMask, oRefDimensions);
        const OptionalImageAndPallete oAlbedoAndPallete = GetAlbedo(_oArguments, oMask, oRefDimensions);
        const Optional1BitImage oAmbientOcclusion = GetAmbientOcclusion(_oArguments, oMask, oRefDimensions);

        WriteEncodedImage(_oArguments.m_sBaseOutputFilename, oNormalMapAndPallete, oAlbedoAndPallete, oAmbientOcclusion);

        if (oNormalMapAndPallete.has_value())
            WriteMaterial(_oArguments.m_sOutputMaterialsFilename, oNormalMapAndPallete, oAlbedoAndPallete, oAmbientOcclusion, _oArguments);
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
            throw std::runtime_error("Dimensions missmatch (all input BMPs must have matching dimensions)");

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

    static Optional1BitImage GetMask(const Arguments& _oArguments, std::optional<Dimensions>& _oDimensions)
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

    static OptionalImageAndPallete GetNormalMap(const Arguments& _oArguments, const std::optional<RawImage<bool>>& _oMask, std::optional<Dimensions>& _oDimensions)
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

    static OptionalImageAndPallete GetAlbedo(const Arguments& _oArguments, const Optional1BitImage& _oMask, std::optional<Dimensions>& _oDimensions)
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

    static Optional1BitImage GetAmbientOcclusion(const Arguments& _oArguments, const Optional1BitImage& _oMask, std::optional<Dimensions>& _oDimensions)
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

    static void WriteEncodedImage(const char* _sBaseOutputFilename, const OptionalImageAndPallete& _oNormalMapAndPallete, const OptionalImageAndPallete& _oAlbedoAndPallete, const Optional1BitImage& _oAmbientOcclusion)
    {
        const auto& [oMergedImage, oMergedPalette] = ImageComponentMerge::GetMergedResult(_oNormalMapAndPallete, _oAlbedoAndPallete, _oAmbientOcclusion);
        std::vector<ImageSplit::ImageAndPalette> oSliptImageAndPalettes = ImageSplit::GetSplitImageAndPalette(oMergedImage, oMergedPalette);
        const size_t uPartsCount = oSliptImageAndPalettes.size();
        for (unsigned int uPartIndex = 0; uPartIndex < uPartsCount; ++uPartIndex)
        {
            ImageSplit::ImageAndPalette& oImageAndPalette = oSliptImageAndPalettes[uPartIndex];
            oImageAndPalette.m_oPalette.resize(256);
            const BMPHandler oOutputNormalMap(oImageAndPalette.m_oImage.GetFullRect(), oImageAndPalette.m_oPalette);
            std::string sOutputNormalMapFilename = uPartsCount == 1 ? _sBaseOutputFilename :
                GetPartFilename(_sBaseOutputFilename, uPartIndex);
            try
            {
                std::cout << "Writting " << sOutputNormalMapFilename << "\n";
                oOutputNormalMap.Write(sOutputNormalMapFilename.c_str());
            }
            catch (const std::runtime_error& oException)
            {
                throw std::runtime_error(std::format("Exception writting {}: {}", sOutputNormalMapFilename, oException.what()));
            }
            catch (...)
            {
                throw std::runtime_error(std::format("Unhandled exception writting {}", sOutputNormalMapFilename));
            }
        }
    }

    static void WriteMaterial(const char* _sOutputMaterialsFilename, const OptionalImageAndPallete& _oNormalMapAndPallete, const OptionalImageAndPallete& _oAlbedoAndPallete, const Optional1BitImage& oAmbientOcclusion, const Arguments& _oArguments)
    {
        //If no albedo was provided, default to pure white
        const std::vector<ColorRGB> oDefaultAlbedo = { {255, 255, 255} };
        const unsigned char uLogLightShadesCount = static_cast<unsigned char>(_oArguments.m_uLogLightShadesCount);
        const float fSpecularIntensity = _oArguments.m_fSpecularIntensity;
        const Material oMaterial{ 1, oAmbientOcclusion.has_value(),
            _oAlbedoAndPallete.has_value() ? std::span{_oAlbedoAndPallete->second}.subspan(1) : oDefaultAlbedo,
            std::span{_oNormalMapAndPallete->second}.subspan(1), _oArguments.m_oFrontLightColor, _oArguments.m_oBackLightColor, _oArguments.m_oPureDarknessColor, uLogLightShadesCount,
            fSpecularIntensity > 0.f ? std::optional{SpecularData{_oArguments.m_fSpecularHardness, fSpecularIntensity, uLogLightShadesCount, uLogLightShadesCount}} : std::nullopt };
        try
        {
            std::cout << "Writting " << _sOutputMaterialsFilename << "\n";
            oMaterial.Write(_sOutputMaterialsFilename);
        }
        catch (const std::runtime_error& oException)
        {
            throw std::runtime_error(std::format("Exception writting {}: {}", _sOutputMaterialsFilename, oException.what()));
        }
        catch (...)
        {
            throw std::runtime_error(std::format("Unhandled exception writting {}", _sOutputMaterialsFilename));
        }
    }

    static std::string GetPartFilename(const char* _sBaseFilename, unsigned int _uPartIndex)
    {
        const char* pExtensionStart = std::strrchr(_sBaseFilename, '.');
        const unsigned int uExtensionStartIndex = static_cast<unsigned int>(pExtensionStart - _sBaseFilename);
        return std::format("{}_{}{}", std::string{ _sBaseFilename, _sBaseFilename + uExtensionStartIndex }, _uPartIndex, pExtensionStart);
    }
};
