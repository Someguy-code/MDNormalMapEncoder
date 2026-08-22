module;
#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>

export module NormalMapUtils;

import Color;
import MathUtils;
import std;
import Vector3;
import RawImage;

export namespace NormalMapUtils
{
	//Comptes horizontal and vertical angles from a normal map color
	std::pair<float, float> GetAnglesFromColor(const ColorRGB& _oColor)
	{
		const float fHorizontalAngle = static_cast<float>(atan2(_oColor.m_uG - 128, _oColor.m_uR - 128));
		const float fVerticalAngle = static_cast<float>(std::asin((static_cast<float>(_oColor.m_uB) - 128.f) / 127.f) / M_PI_2);
		return { fHorizontalAngle, fVerticalAngle };
	}

	std::pair<float, float> GetAnglesFromNormal(const Vector3& _oNormal)
	{
		const float fHorizontalAngle = atan2(_oNormal.m_fY, _oNormal.m_fX);
		const float fVerticalAngle = asin(_oNormal.m_fZ);
		return { fHorizontalAngle, fVerticalAngle };
	}

	ColorRGB GetColorFromNormal(const Vector3& _oNormal)
	{
		if (_oNormal.IsNull())
			return { 0, 0, 0 };
		return { 
			static_cast<uint8_t>(round(_oNormal.m_fX * 127.f + 128.f)),
			static_cast<uint8_t>(round(_oNormal.m_fY * 127.f + 128.f)),
			static_cast<uint8_t>(round(-_oNormal.m_fZ * 127.f + 128.f)) };
	}

	Vector3 GetNormalFromColor(const ColorRGB& _oColor)
	{
		if (_oColor.IsNull())
			return { 0.f, 0.f, 0.f };

		float _fX = (static_cast<float>(_oColor.m_uR) - 128.f) / 127.f;
		float _fY = (static_cast<float>(_oColor.m_uG) - 128.f) / 127.f;
		float _fZ = -(static_cast<float>(_oColor.m_uB) - 128.f) / 127.f;
		MathUtils::NormalizeVector(_fX, _fY, _fZ);
		return { _fX, _fY, _fZ };
	}

	std::vector<Vector3> GetPaletteNormals(std::span<const ColorRGB> _oPalette)
	{
		std::vector<Vector3> oPaletteNormals;
		oPaletteNormals.reserve(_oPalette.size());
		for (const ColorRGB& oColor : _oPalette)
			oPaletteNormals.emplace_back(GetNormalFromColor(oColor));
		return oPaletteNormals;
	}

	RawImage<Vector3> GetNormalMapNormals(const RawImage<ColorRGB>& _oNormalMap)
	{
		RawImage<Vector3> oNormalMapNormals{ _oNormalMap.m_uWidth,_oNormalMap.m_uHeight };
		std::vector<Vector3>& oNormalMapNormalsPixels = oNormalMapNormals.m_oPixelsArray;
		const std::vector<ColorRGB>& oNormalMapPixels = _oNormalMap.m_oPixelsArray;
		const size_t uPixelsCount = oNormalMapNormalsPixels.size();
		for (size_t uPixelIndex = 0; uPixelIndex < uPixelsCount; ++uPixelIndex)
			oNormalMapNormalsPixels[uPixelIndex] = GetNormalFromColor(oNormalMapPixels[uPixelIndex]);
		return oNormalMapNormals;
	}

	unsigned int GetClosestColorIndex(const Vector3& _oNormal, std::span<const Vector3> _oPaletteNormals)
	{
		if (_oNormal.IsNull())
			return 0;

		unsigned int uClosestColorIndex = 0;
		float fClosestAngle = std::numeric_limits<float>::infinity();	
		const size_t uColorsCount = _oPaletteNormals.size();
		for (unsigned int uColorIndex = 0; uColorIndex < uColorsCount; ++uColorIndex)
		{
			const float fAngle = _oNormal.GetAngleDistance(_oPaletteNormals[uColorIndex]);
			if (fAngle < fClosestAngle)
			{
				fClosestAngle = fAngle;
				uClosestColorIndex = uColorIndex;
			}
		}

		return uClosestColorIndex;
	}
}