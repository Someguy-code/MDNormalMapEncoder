export module ImageComponentMerge;

import std;
import RawImage;
import Color;

export struct ImageComponentMerge
{
    struct ImageAndPalette
    {
        RawImage<std::uint8_t> m_oImage;
        std::vector<ColorRGB> m_oPalette;
    };
    //Collapses all the image components ointo a single one
    static ImageAndPalette GetMergedResult(const std::optional<std::pair<RawImage<std::uint8_t>, std::vector<ColorRGB>>>& _oNormalMapAndPalette, const std::optional<std::pair<RawImage<std::uint8_t>, std::vector<ColorRGB>>>& _oAlbedoAndPalette, const std::optional<RawImage<bool>>& _oAmbientOcclusion) {
        std::vector<ColorRGB> oMergedPalette = GetMergedPalette(
            _oNormalMapAndPalette.has_value() ? &_oNormalMapAndPalette->second : nullptr,
            _oAlbedoAndPalette.has_value() ? &_oAlbedoAndPalette->second : nullptr,
            _oAmbientOcclusion.has_value());

        const size_t uColorsCount = oMergedPalette.size();
        return {
            GetMergedImage(
                _oNormalMapAndPalette.has_value() ? &_oNormalMapAndPalette->first : nullptr,
                _oAlbedoAndPalette.has_value() ? &_oAlbedoAndPalette->first : nullptr,
                _oAmbientOcclusion.has_value() ? &_oAmbientOcclusion.value() : nullptr,
                _oAlbedoAndPalette.has_value() ? _oAlbedoAndPalette->second.size() - 1 : 1,
                uColorsCount),
            std::move(oMergedPalette)
        };
    }

private:
    static RawImage<std::uint8_t> GetMergedImage(const RawImage<std::uint8_t>* _pNormalMap, const RawImage<std::uint8_t>* _pAlbedo, const RawImage<bool>* _pAmbientOcclusion, size_t _uAlbedoColorsCount, size_t _uColorsCount)
    {
        RawImage<std::uint8_t> oMergedImage;
        if (_pNormalMap != nullptr && _pAlbedo == nullptr)
            oMergedImage = *_pNormalMap;
        else if (_pNormalMap == nullptr && _pAlbedo != nullptr)
            oMergedImage = *_pAlbedo;
        else
        {
            struct CollapseAlbedoAndNormal
            {
                const uint8_t m_uAlbedoColorsCount;
                std::uint8_t operator()(std::uint8_t _uNormalIndex, std::uint8_t _uAlbedoIndex) const
                {
                    return _uAlbedoIndex == 0 ? 0 : (_uNormalIndex - 1) * m_uAlbedoColorsCount + _uAlbedoIndex;
                }
            };
            const std::vector<std::uint8_t>& oNormalMapPixels = _pNormalMap->m_oPixelsArray;
            const std::vector<std::uint8_t>& oAlbedoPixels = _pAlbedo->m_oPixelsArray;
            auto oAlbedoAndNormalImages = std::views::zip_transform(CollapseAlbedoAndNormal{ static_cast<std::uint8_t>(_uAlbedoColorsCount) }, oNormalMapPixels, oAlbedoPixels);
            oMergedImage = { _pNormalMap->m_uWidth, _pNormalMap->m_uHeight };
            std::vector<std::uint8_t>& oMergedImagePixels = oMergedImage.m_oPixelsArray;
            std::ranges::copy(oAlbedoAndNormalImages | std::views::as_rvalue, oMergedImagePixels.begin());
        }

        if (_pAmbientOcclusion != nullptr)
        {
            struct CollapseAmbientOcclusion
            {
                const std::uint8_t m_uAmbientOcclusionColor;
                std::uint8_t operator()(std::uint8_t _uColorIndex, bool _bIsAmbientOcclusion) const
                {
                    return _bIsAmbientOcclusion ? m_uAmbientOcclusionColor : _uColorIndex;
                }
            };
            std::vector<std::uint8_t>& oMergedImagePixels = oMergedImage.m_oPixelsArray;
            const std::vector<bool>& oAmbientOcclusionPixels = _pAmbientOcclusion->m_oPixelsArray;
            auto oAlbedoAndNormalImages = std::views::zip_transform(CollapseAmbientOcclusion{ static_cast<std::uint8_t>(_uColorsCount - 1) }, oMergedImagePixels, oAmbientOcclusionPixels);
            std::ranges::copy(oAlbedoAndNormalImages | std::views::as_rvalue, oMergedImagePixels.begin());
        }

        return oMergedImage;
    }

    //The merged palette is only relevant for debugging purposes
    static std::vector<ColorRGB> GetMergedPalette(const std::vector<ColorRGB>* _pNormalMapPalette, const std::vector<ColorRGB>* _pAlbedoPalette, bool _bAddPureBlack)
    {
        std::vector<ColorRGB> oMergedPalette;
        oMergedPalette.reserve(256);
        if (_pNormalMapPalette != nullptr && _pAlbedoPalette == nullptr)
            oMergedPalette = *_pNormalMapPalette;
        else if (_pNormalMapPalette == nullptr && _pAlbedoPalette != nullptr)
            oMergedPalette = *_pAlbedoPalette;
        else
        {
            oMergedPalette.emplace_back(0, 0, 0);
            const size_t uColorsCount = 1 + (_pNormalMapPalette->size() - 1) * (_pAlbedoPalette->size() - 1);
            for (const ColorRGB& oNormalColor : std::span(*_pNormalMapPalette).subspan(1))
            {
                for (const ColorRGB& oAlbedoColor : std::span(*_pAlbedoPalette).subspan(1))
                {
                    oMergedPalette.emplace_back(oNormalColor.Lerp(oAlbedoColor, .5f));
                }
            }
        }
        if (_bAddPureBlack)
            oMergedPalette.emplace_back(0, 0, 0);
        return oMergedPalette;
    }
};