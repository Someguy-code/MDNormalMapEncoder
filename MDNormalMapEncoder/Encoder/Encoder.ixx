export module Encoder;

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
        const Optional1BitImage oMask = GetMask(_oArguments);
        const OptionalImageAndPallete oNormalMapAndPallete = GetNormalMap(_oArguments, oMask);
        const OptionalImageAndPallete oAlbedoAndPallete = GetAlbedo(_oArguments, oMask);
        const Optional1BitImage oAmbientOcclusion = GetAmbientOcclusion(_oArguments, oMask);
        
        std::optional<Material> oMaterial;
        if(oNormalMapAndPallete)
            oMaterial = GetMaterial(oNormalMapAndPallete, oAlbedoAndPallete, oAmbientOcclusion, _oArguments);
        return {
            GetEncodedImage(oNormalMapAndPallete, oAlbedoAndPallete, oAmbientOcclusion),
            std::move(oMaterial)
        };
    }

private:

    using OptionalImageAndPallete = std::optional<std::pair<RawImage<std::uint8_t>, std::vector<ColorRGB>>>;
    using Optional1BitImage = std::optional<RawImage<bool>>;

    static Optional1BitImage GetMask(const EncoderArguments& _oArguments)
    {
        const std::optional<RawImage<ColorRGB>>& oInputMask = _oArguments.m_oMask;
        if (!oInputMask)
            return std::nullopt;

        struct ConvertToFlags
        {
            bool operator()(const ColorRGB& _oColor) { return _oColor.IsNull(); }
        };

        RawImage<bool> oSourceMaskFlags;
        oSourceMaskFlags = oInputMask->GetTypeConversion<bool, ConvertToFlags>();

        return oSourceMaskFlags;
    }

    static OptionalImageAndPallete GetNormalMap(const EncoderArguments& _oArguments, const std::optional<RawImage<bool>>& _oMask)
    {
        const std::optional<RawImage<ColorRGB>>& oInputNormal = _oArguments.m_oNormal;
        if(!oInputNormal)
            return std::nullopt;

        RawImage<std::uint8_t> oDitheredTargetNormalMap;
        std::vector<ColorRGB> oNormalMapPalette;

        const unsigned int uVerticalSidesCount = _oArguments.m_uVerticalNormalMapSides;
        const unsigned int uHorizontalSidesCount = _oArguments.m_uHorizontalNormalMapSides;

        const std::vector<Vector3> oNormalsPalette = NormalMapPalette::GetNormalsPalette(uHorizontalSidesCount, uVerticalSidesCount);
        oNormalMapPalette.reserve(oNormalsPalette.size());
        for (const Vector3& oNormal : oNormalsPalette)
            oNormalMapPalette.push_back(NormalMapUtils::GetColorFromNormal(oNormal));

        RawImage<Vector3> oSourceNormalMapNormals = NormalMapUtils::GetNormalMapNormals(*oInputNormal);
        oDitheredTargetNormalMap = DitheredImage<NormalMapError, Vector3>::GetDitheredImage(oSourceNormalMapNormals, oNormalsPalette, false, _oMask);

        return { { oDitheredTargetNormalMap, oNormalMapPalette } };
    }

    static OptionalImageAndPallete GetAlbedo(const EncoderArguments& _oArguments, const Optional1BitImage& _oMask)
    {
        const std::optional<RawImage<ColorRGB>>& oInputAlbedo = _oArguments.m_oAlbedo;
        if(!oInputAlbedo)
            return std::nullopt;

        const RawImage<LABColor> oSourceAlbeldoLAB = oInputAlbedo->GetTypeConversion<LABColor>();
        const std::vector<LABColor> oAlbedoPaletteLAB = PaletteReduction::GetReducedPalette(oSourceAlbeldoLAB, _oMask, _oArguments.m_uMaxAlbedoColors);
        RawImage<std::uint8_t> oDitheredAlbedo = DitheredImage<LABColorError, LABColor>::GetDitheredImage(oSourceAlbeldoLAB, oAlbedoPaletteLAB, false, _oMask);
        std::vector<ColorRGB> oAlbedoPalette;
        oAlbedoPalette.reserve(oAlbedoPaletteLAB.size());
        for (const LABColor& oLABColor : oAlbedoPaletteLAB)
            oAlbedoPalette.push_back(static_cast<ColorRGB>(oLABColor));

        return { {oDitheredAlbedo, oAlbedoPalette} };
    }

    static Optional1BitImage GetAmbientOcclusion(const EncoderArguments& _oArguments, const Optional1BitImage& _oMask)
    {
        const std::optional<RawImage<ColorRGB>>& oInputAmbientOcclusion = _oArguments.m_oAmbinetOcclusion;
        if(!oInputAmbientOcclusion)
            return std::nullopt;

        const RawImage<LABColor> oInputAmbientOcclusionLAB = oInputAmbientOcclusion->GetTypeConversion<LABColor>();
        RawImage<std::uint8_t> oDitheredAmbientOcclusion = DitheredImage<LABColorError, LABColor>::GetDitheredImage(oInputAmbientOcclusionLAB, std::vector<LABColor>{ColorRGB{ 0, 0, 0 }, ColorRGB{ 255, 255, 255 }}, true, _oMask);

        struct ConvertToFlags
        {
            bool operator()(const std::uint8_t _uColorIndex) { return _uColorIndex == 0; }
        };

        return { oDitheredAmbientOcclusion.GetTypeConversion<bool, ConvertToFlags>() };
    }

    static std::vector<ImageSplit::ImageAndPalette> GetEncodedImage(const OptionalImageAndPallete& _oNormalMapAndPallete, const OptionalImageAndPallete& _oAlbedoAndPallete, const Optional1BitImage& _oAmbientOcclusion)
    {
        const auto& [oMergedImage, oMergedPalette] = ImageComponentMerge::GetMergedResult(_oNormalMapAndPallete, _oAlbedoAndPallete, _oAmbientOcclusion);
        return ImageSplit::GetSplitImageAndPalette(oMergedImage, oMergedPalette);
    }

    static Material GetMaterial(const OptionalImageAndPallete& _oNormalMapAndPallete, const OptionalImageAndPallete& _oAlbedoAndPallete, const Optional1BitImage& oAmbientOcclusion, const EncoderArguments& _oArguments)
    {
        //If no albedo was provided, default to pure white
        const std::vector<ColorRGB> oDefaultAlbedo = { {255, 255, 255} };
        const unsigned char uLogLightShadesCount = static_cast<unsigned char>(_oArguments.m_uLogLightShadesCount);
        const float fSpecularIntensity = _oArguments.m_fSpecularIntensity;
        return { 1, oAmbientOcclusion.has_value(),
            _oAlbedoAndPallete.has_value() ? std::span{_oAlbedoAndPallete->second}.subspan(1) : oDefaultAlbedo,
            std::span{_oNormalMapAndPallete->second}.subspan(1), _oArguments.m_oFrontLightColor, _oArguments.m_oPureDarknessColor, _oArguments.m_oBackLightColor, uLogLightShadesCount,
            fSpecularIntensity > 0.f ? std::optional{SpecularData{_oArguments.m_fSpecularHardness, fSpecularIntensity, uLogLightShadesCount, uLogLightShadesCount}} : std::nullopt };
    }
};
