export module DitherImage:NormalMapError;

import Color;
import MathUtils;
import NormalMapUtils;
import std;
import Vector3;

export struct NormalMapError
{
	Vector3 m_oNormal;

	NormalMapError() = default;

	NormalMapError(const Vector3& _oOriginal)
	: m_oNormal(_oOriginal)
	{}

	NormalMapError(const NormalMapError& _oOldNormal, const Vector3& _oNewNormal)
	: m_oNormal(_oOldNormal.m_oNormal - _oNewNormal)
	{}

	NormalMapError& operator+=(const NormalMapError& _oOtherValue)
	{
		m_oNormal += _oOtherValue.m_oNormal;
		return *this;
	}

	friend NormalMapError operator+(NormalMapError _oLeftHandSide, const NormalMapError& _oRightHandSide)
	{
		_oLeftHandSide += _oRightHandSide;
		return _oLeftHandSide;
	}

	NormalMapError& operator*=(const float _fValue)
	{
		m_oNormal *= _fValue;
		return *this;
	}

	friend NormalMapError operator*(NormalMapError _oLeftHandSide, float _oRightHandSide)
	{
		_oLeftHandSide *= _oRightHandSide;
		return _oLeftHandSide;
	}

	explicit operator Vector3() const
	{
		Vector3 oNormal = m_oNormal.GetNormalized();
		float fX = oNormal.m_fX;
		float fY = oNormal.m_fY;
		float fZ = std::clamp(oNormal.m_fZ, -1.f, 0.f);
		MathUtils::NormalizeVector(fX, fY, fZ);
		return { fX, fY, fZ };
	}

	unsigned int GetClosestColorIndex(std::span<const Vector3> _oPalette) const
	{
		return NormalMapUtils::GetClosestColorIndex(static_cast<Vector3>(*this), _oPalette);
	}
};