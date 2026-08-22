module;

#define _USE_MATH_DEFINES
#include <cmath>

export module MathUtils;

import std;

export namespace MathUtils
{
	constexpr int Sign(int _iValue)
	{
		return (_iValue > 0) - (_iValue < 0);
	}

	constexpr int DivRoundUp(int _iDividend, int _iDivisor)
	{
		return _iDividend / _iDivisor + Sign(_iDividend % _iDivisor);
	}

	constexpr int DivByPowerOf2(int _iDividend, unsigned int _uDivisorLog)
	{
		return _iDividend >> _uDivisorLog;
	}

	constexpr int MulByPowerOf2(int _iValue, unsigned int _uMultiplierLog)
	{
		return _iValue << _uMultiplierLog;
	}

	constexpr int ModByPowerOf2(int _iDividend, unsigned int _uDivisorLog)
	{
		return _iDividend & ((1 << _uDivisorLog) - 1);
	}

	constexpr int DivByPowerOf2RoundUp(int _iDividend, unsigned int _uDivisorLog)
	{
		return DivByPowerOf2(_iDividend, _uDivisorLog) + (ModByPowerOf2(_iDividend, _uDivisorLog) > 0);
	}

	constexpr int DivByPowerOf2Round(int _iDividend, unsigned int _uDivisorLog)
	{
		const int uModPowerOf2 = ModByPowerOf2(_iDividend, _uDivisorLog);
		return DivByPowerOf2(_iDividend, _uDivisorLog) + ((uModPowerOf2 & MulByPowerOf2(1, _uDivisorLog - 1)) != 0);
	}

	constexpr unsigned int GetClosestPow2Exponent(float _fValue)
	{
		const float fPow2Exponent = std::max(0.f, std::floor(log2f(_fValue)));
		return pow(2.f, fPow2Exponent) < _fValue ? static_cast<unsigned int>(fPow2Exponent) + 1 : static_cast<unsigned int>(fPow2Exponent);
	}

	float GetNormalizedAngle(float _fAngle)
	{
		const float fPI = static_cast<float>(M_PI);
		const float f2PI = 2.f * fPI;
		float fNormalizedAngle = fmod(_fAngle, f2PI);
		return
			fNormalizedAngle < -fPI ? f2PI + fNormalizedAngle :
			fNormalizedAngle > fPI ? fNormalizedAngle - f2PI :
			fNormalizedAngle;
	}

	float GetSignedShortestAngleDifference(float _fSourceAngle, float _fTargetAngle)
	{
		return GetNormalizedAngle(_fTargetAngle - _fSourceAngle);
	}

	constexpr float DotProduct(float _fX1, float _fY1, float _fX2, float _fY2)
	{
		return _fX1 * _fX2 + _fY1 * _fY2;
	}

	constexpr float DotProduct(float _fX1, float _fY1, float _fZ1, float _fX2, float _fY2, float _fZ2)
	{
		return _fX1 * _fX2 + _fY1 * _fY2 + _fZ1 * _fZ2;
	}

	float VectorModuleSquared(float _fX, float _fY)
	{
		return _fX * _fX + _fY * _fY;
	}

	float VectorModuleSquared(float _fX, float _fY, float _fZ)
	{
		return _fX * _fX + _fY * _fY + _fZ * _fZ;
	}

	float VectorModule(float _fX, float _fY)
	{
		return std::sqrt(VectorModuleSquared(_fX, _fY));
	}

	float VectorModule(float _fX, float _fY, float _fZ)
	{
		return std::sqrt(VectorModuleSquared(_fX, _fY, _fZ));
	}

	void NormalizeVector(float& _fX, float& _fY)
	{
		if (_fX == 0.f && _fY == 0.f)
			return;
		const float fInvModule = 1.f / VectorModule(_fX, _fY);
		_fX *= fInvModule;
		_fY *= fInvModule;
	}

	void NormalizeVector(float& _fX, float& _fY, float& _fZ)
	{
		if (_fX == 0.f && _fY == 0.f && _fZ == 0.f)
			return;
		const float fInvModule = 1.f / VectorModule(_fX, _fY, _fZ);
		_fX *= fInvModule;
		_fY *= fInvModule;
		_fZ *= fInvModule;
	}

	float GetAngleDistanceNormalized(float _fX1, float _fY1, float _fX2, float _fY2)
	{
		return std::acos(std::clamp(DotProduct(_fX1, _fY1, _fX2, _fY2), -1.f, 1.f));
	}

	float GetAngleDistanceNormalized(float _fX1, float _fY1, float _fZ1, float _fX2, float _fY2, float _fZ2)
	{
		return std::acos(std::clamp(DotProduct(_fX1, _fY1, _fZ1, _fX2, _fY2, _fZ2), -1.f, 1.f));
	}

	float GetAngleDistance(float _fX1, float _fY1, float _fX2, float _fY2)
	{
		NormalizeVector(_fX1, _fY1);
		NormalizeVector(_fX2, _fY2);
		return GetAngleDistanceNormalized(_fX1, _fY1, _fX2, _fY2);
	}

	float GetAngleDistance(float _fX1, float _fY1, float _fZ1, float _fX2, float _fY2, float _fZ2)
	{
		NormalizeVector(_fX1, _fY1, _fZ1);
		NormalizeVector(_fX2, _fY2, _fZ2);
		return GetAngleDistanceNormalized(_fX1, _fY1, _fZ1, _fX2, _fY2, _fZ2);
	}

	template <typename T>
	constexpr T Lerp(T _oValue1, T _oValue2, float _fBalance)
	{
		return static_cast<T>(static_cast<float>(_oValue1) * (1.f - _fBalance) + static_cast<float>(_oValue2) * _fBalance);
	}
}