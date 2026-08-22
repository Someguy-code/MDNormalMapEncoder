export module Vector3;

import Color;
import MathUtils;

export struct Vector3
{
	float m_fX;
	float m_fY;
	float m_fZ;

	Vector3() = default;
	Vector3(const Vector3& _oOriginal) = default;

	Vector3(float _fX, float _fY, float _fZ)
	: m_fX(_fX), m_fY(_fY), m_fZ(_fZ)
	{}

	Vector3(const ColorRGB& _oColor)
	: Vector3(_oColor.m_uR / 255.f, _oColor.m_uG / 255.f, _oColor.m_uB / 255.f)
	{
		Normalize();
	}

	bool IsNull() const
	{
		return m_fX == 0.f && m_fY == 0.f && m_fZ == 0.f;
	}

	Vector3& operator+=(const Vector3& _oOtherValue) 
	{
		m_fX += _oOtherValue.m_fX;
		m_fY += _oOtherValue.m_fY;
		m_fZ += _oOtherValue.m_fZ;
		return *this;
	}

	friend Vector3 operator+(Vector3 _oLeftHandSide, const Vector3& _oRightHandSide)
	{
		_oLeftHandSide += _oRightHandSide;
		return _oLeftHandSide;
	}

	Vector3& operator-=(const Vector3& _oOtherValue)
	{
		m_fX -= _oOtherValue.m_fX;
		m_fY -= _oOtherValue.m_fY;
		m_fZ -= _oOtherValue.m_fZ;
		return *this;
	}

	friend Vector3 operator-(Vector3 _oLeftHandSide, const Vector3& _oRightHandSide)
	{
		_oLeftHandSide -= _oRightHandSide;
		return _oLeftHandSide;
	}

	Vector3& operator*=(const float _fValue)
	{
		m_fX *= _fValue;
		m_fY *= _fValue;
		m_fZ *= _fValue;
		return *this;
	}

	friend Vector3 operator*(Vector3 _oLeftHandSide, const float _fRightHandSide)
	{
		_oLeftHandSide *= _fRightHandSide;
		return _oLeftHandSide;
	}

	float GetSquareLength() const
	{
		return MathUtils::VectorModuleSquared(m_fX, m_fY, m_fZ);
	}

	float GetLength() const
	{
		return MathUtils::VectorModule(m_fX, m_fY, m_fZ);
	}

	void Normalize()
	{
		MathUtils::NormalizeVector(m_fX, m_fY, m_fZ);
	}

	Vector3 GetNormalized() const
	{
		Vector3 oResult(*this);
		oResult.Normalize();
		return oResult;
	}

	float GetAngleDistance(const Vector3& _oOtherValue) const
	{
		return MathUtils::GetAngleDistance(m_fX, m_fY, m_fZ, _oOtherValue.m_fX, _oOtherValue.m_fY, _oOtherValue.m_fZ);
	}
};