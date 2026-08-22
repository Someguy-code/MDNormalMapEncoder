export module DitherImage:ColorError;

import Color;
import PaletteUtils;
import std;

export struct ColorError
{
	int m_iR = 0;
	int m_iG = 0;
	int m_iB = 0;

	ColorError() = default;

	ColorError(const ColorRGB& _oOriginal)
	: m_iR(_oOriginal.m_uR), m_iG(_oOriginal.m_uG), m_iB(_oOriginal.m_uB)
	{}

	ColorError(const ColorError& _oOldColor, const ColorRGB& _oNewColor)
	: m_iR(_oOldColor.m_iR - _oNewColor.m_uR), m_iG(_oOldColor.m_iG - _oNewColor.m_uG), m_iB(_oOldColor.m_iB - _oNewColor.m_uB)
	{
	}

	ColorError& operator+=(const ColorError& _oOtherValue)
	{
		m_iR += _oOtherValue.m_iR;
		m_iG += _oOtherValue.m_iG;
		m_iB += _oOtherValue.m_iB;
		return *this;
	}

	friend ColorError operator+(ColorError _oLeftHandSide, const ColorError& _oRightHandSide)
	{
		_oLeftHandSide += _oRightHandSide;
		return _oLeftHandSide;
	}

	friend ColorRGB operator+(const ColorRGB& _oLeftHandSide, const ColorError& _oRightHandSide)
	{
		const uint8_t uR = std::clamp(_oLeftHandSide.m_uR + _oRightHandSide.m_iR, 0, 255);
		const uint8_t uG = std::clamp(_oLeftHandSide.m_uG + _oRightHandSide.m_iG, 0, 255);
		const uint8_t uB = std::clamp(_oLeftHandSide.m_uB + _oRightHandSide.m_iB, 0, 255);
		return { uR, uG, uB };
	}

	ColorError& operator*=(const float _fValue)
	{
		m_iR = static_cast<int>(std::round(static_cast<float>(m_iR) * _fValue));
		m_iG = static_cast<int>(std::round(static_cast<float>(m_iG) * _fValue));
		m_iB = static_cast<int>(std::round(static_cast<float>(m_iB) * _fValue));
		return *this;
	}

	friend ColorError operator*(ColorError _oLeftHandSide, float _fRightHandSide)
	{
		_oLeftHandSide *=_fRightHandSide;
		return _oLeftHandSide;
	}

	explicit operator ColorRGB() const
	{
		const uint8_t uR = std::clamp(m_iR, 0, 255);
		const uint8_t uG = std::clamp(m_iG, 0, 255);
		const uint8_t uB = std::clamp(m_iB, 0, 255);
		return { uR, uG, uB };
	}

	unsigned int GetClosestColorIndex(std::span<const ColorRGB> _oPalette) const
	{
		return PaletteUtils::GetClosestColorIndex(static_cast<ColorRGB>(*this), _oPalette);
	}
};