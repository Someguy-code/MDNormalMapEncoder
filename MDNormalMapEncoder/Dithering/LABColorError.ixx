export module DitherImage:LABColorError;

import LABColor;
import PaletteUtils;
import MathUtils;
import NormalMapUtils;
import std;
import Vector3;

export struct LABColorError
{
	LABColor m_oColor;

	LABColorError() = default;

	LABColorError(const LABColor& _oOriginal)
		: m_oColor(_oOriginal)
	{
	}

	LABColorError(const LABColorError& _oOldNormal, const LABColor& _oNewNormal)
		: m_oColor(_oOldNormal.m_oColor - _oNewNormal)
	{
	}

	LABColorError& operator+=(const LABColorError& _oOtherValue)
	{
		m_oColor += _oOtherValue.m_oColor;
		return *this;
	}

	friend LABColorError operator+(LABColorError _oLeftHandSide, const LABColorError& _oRightHandSide)
	{
		_oLeftHandSide += _oRightHandSide;
		return _oLeftHandSide;
	}

	LABColorError& operator*=(const float _fValue)
	{
		m_oColor *= _fValue;
		return *this;
	}

	friend LABColorError operator*(LABColorError _oLeftHandSide, float _oRightHandSide)
	{
		_oLeftHandSide *= _oRightHandSide;
		return _oLeftHandSide;
	}

	explicit operator LABColor() const
	{
		return m_oColor;
	}

	unsigned int GetClosestColorIndex(std::span<const LABColor> _oPalette) const
	{
		return PaletteUtils::GetClosestColorIndex(static_cast<LABColor>(*this), _oPalette);
	}
};