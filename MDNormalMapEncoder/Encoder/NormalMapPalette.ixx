module;

#define _USE_MATH_DEFINES
#include <cmath>

export module NormalMapPalette;

import Color;
import MathUtils;
import RawImage;
import Vector3;
import std;

export struct NormalMapPalette
{
    static std::vector<Vector3> GetNormalsPalette(unsigned int _uHorizontalSidesCount, unsigned int _uVerticalSidesCount)
    {
        std::vector<Vector3> oNormalsPalette;
        const size_t uNormalsCount = _uHorizontalSidesCount * (_uVerticalSidesCount - 1) + 1;
        oNormalsPalette.reserve(uNormalsCount + 1);
        oNormalsPalette.emplace_back(0.f, 0.f, 0.f);

        const std::vector<std::pair<float, float>> oHorizontalSideNormals = GetHorizontalSideNormals(_uHorizontalSidesCount);
        const std::vector<float> oVerticalSideHeights = GetVerticalSidesHeights(_uVerticalSidesCount);

        unsigned int uNextColor = 0;
        for (const auto [fNormalX, fNormalY] : oHorizontalSideNormals)
        {
            for (unsigned int uVerticalSideIndex = 0; uVerticalSideIndex < _uVerticalSidesCount - 1; ++uVerticalSideIndex)
            {
                const float fVerticalCos = static_cast<float>(std::cos(GetVerticalAngle(uVerticalSideIndex, _uVerticalSidesCount)));
                oNormalsPalette.emplace_back( fNormalX * fVerticalCos, fNormalY * fVerticalCos, -oVerticalSideHeights[uVerticalSideIndex] );
            }
        }
        oNormalsPalette.emplace_back(0.f, 0.f, -1.f);

        return oNormalsPalette;
    }

private:
    static std::vector<float> GetVerticalSidesHeights(unsigned int _uVerticalSidesCount)
    {
        //The last side is looking straight up. Unlike all the other normals, this one is unique so no need to store multiple copies
        std::vector<float> oVerticalSidesHeights(_uVerticalSidesCount - 1);
        for (unsigned int uVerticalSideIndex = 0; uVerticalSideIndex < _uVerticalSidesCount - 1; ++uVerticalSideIndex)
            oVerticalSidesHeights[uVerticalSideIndex] = static_cast<float>(std::sin(GetVerticalAngle(uVerticalSideIndex, _uVerticalSidesCount)));
        return oVerticalSidesHeights;
    }

    static std::vector<std::pair<float, float>> GetHorizontalSideNormals(unsigned int _uHorizontalSidesCount)
    {
        std::vector<std::pair<float, float>> oHorizontalSidesNormals(_uHorizontalSidesCount);
        for (unsigned int uHorizontalSideIndex = 0; uHorizontalSideIndex < _uHorizontalSidesCount; ++uHorizontalSideIndex)
        {
            const float fHorizontalAlpha = (float)uHorizontalSideIndex / (float)_uHorizontalSidesCount;
            const float fHorizontalAngle = static_cast<float>(fHorizontalAlpha * 2.f * M_PI);
            float fNormalX = -static_cast<float>(std::cos(fHorizontalAngle));
            float fNormalY = -static_cast<float>(std::sin(fHorizontalAngle));
            MathUtils::NormalizeVector(fNormalX, fNormalY);
            std::pair<float, float>& oNormal = oHorizontalSidesNormals[uHorizontalSideIndex];
            oNormal.first = fNormalX;
            oNormal.second = fNormalY;
        }
        return oHorizontalSidesNormals;
    }

    static float GetVerticalAngle(unsigned int _uSideIndex, unsigned int _uSideCount)
    {
        //Divide the whole hemisphere in 2 * _uSideCount - 1 (the straight up direction will be handled separately) parts.
        //Take the direction on the center of each.
        const float fAlpha = (static_cast<float>(_uSideIndex) + 0.5f) / (float)(2 * _uSideCount - 1);
        return fAlpha * static_cast<float>(M_PI);
    }
};