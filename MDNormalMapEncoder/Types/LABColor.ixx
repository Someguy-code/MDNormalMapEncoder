export module LABColor;

import Color;
import HashUtils;
import MathUtils;
import std;
import Vector3;

export struct LABColor
{
	//Perceived luminance
	float m_fL;
	//Green-Red balance
	float m_fA;
	//Blue-Yellow balance
	float m_fB;

	LABColor() = default;
	LABColor(float _fL, float _fA, float _fB)
		:m_fL(_fL), m_fA(_fA), m_fB(_fB)
	{}

	LABColor(const ColorRGB& _oColor)
	{
		//Normalize to [0, 1] range
		float fR = _oColor.m_uR / 255.0f;
		float fG = _oColor.m_uG / 255.0f;
		float fB = _oColor.m_uB / 255.0f;

		//Apply gamma correction from sRGB to linear RGB
		auto fnApplyGammaCorrection = [](float _fComponent) {return (_fComponent > 0.04045f) ? std::pow((_fComponent + 0.055f) / 1.055f, 2.4f) : _fComponent / 12.92f;};
		fR = fnApplyGammaCorrection(fR);
		fG = fnApplyGammaCorrection(fG);
		fB = fnApplyGammaCorrection(fB);

		//Convert from RGB to XYZ space
		float fX = fR * 0.4124f + fG * 0.3576f + fB * 0.1805f;
		float fY = fR * 0.2126f + fG * 0.7152f + fB * 0.0722f;
		float fZ = fR * 0.0193f + fG * 0.1192f + fB * 0.9505f;

		//Convert XYZ to LAB space
		const Vector3 oD65Illuminant = { 0.95047f, 1.00000f, 1.08883f };
		fX /= oD65Illuminant.m_fX;
		fY /= oD65Illuminant.m_fY;
		fZ /= oD65Illuminant.m_fZ;
		auto fnPreXYZToLAB = [](float _fComponent) {return (_fComponent > 0.008856f) ? std::pow(_fComponent, 1.f / 3.f) : 7.787f * _fComponent + 16.f / 116.f;};
		fX = fnPreXYZToLAB(fX);
		fY = fnPreXYZToLAB(fY);
		fZ = fnPreXYZToLAB(fZ);

		m_fL = (116.f * fY) - 16.f;
		m_fA = 500.f * (fX - fY);
		m_fB = 200.f * (fY - fZ);
	}

	static LABColor Zero()
	{
		return {};
	}

	bool IsNull() const { return false; }

	operator ColorRGB() const
	{
		//Convert LAB to XYZ space
		float fY = (m_fL + 16.f) / 116.f;
		float fX = m_fA / 500.f + fY;
		float fZ = fY - m_fB / 200.f;

		const Vector3 oD65Illuminant = { 0.95047f, 1.00000f, 1.08883f };
		auto fnReversePreXYZToLAB = [](float _fComponent)
		{
			const float fComponentCubed = _fComponent * _fComponent * _fComponent;
			return (fComponentCubed > 0.008856f) ? fComponentCubed : (_fComponent - 16.f / 116.f) / 7.787f;
		};
		fX = oD65Illuminant.m_fX * fnReversePreXYZToLAB(fX);
		fY = oD65Illuminant.m_fY * fnReversePreXYZToLAB(fY);
		fZ = oD65Illuminant.m_fZ * fnReversePreXYZToLAB(fZ);

		//Convert XYZ to RGB space
		float fR = fX * 3.2406f + fY * -1.5372f + fZ * -0.4986f;
		float fG = fX * -0.9689f + fY * 1.8758f + fZ * 0.0415f;
		float fB = fX * 0.0557f + fY * -0.2040f + fZ * 1.0570f;

		//Apply gamma correction from RGB to sRGB
		auto fnApplyGammaCorrection = [](float _fComponent) {return (_fComponent > 0.0031308f) ? (1.055f * std::pow(_fComponent, 1.f / 2.4f) - 0.055f) : 12.92f * _fComponent;};
		fR = fnApplyGammaCorrection(fR);
		fG = fnApplyGammaCorrection(fG);
		fB = fnApplyGammaCorrection(fB);

		return {
			static_cast<std::uint8_t>(std::round(std::clamp(fR, 0.f, 1.f) * 255.f)),
			static_cast<std::uint8_t>(std::round(std::clamp(fG, 0.f, 1.f) * 255.f)),
			static_cast<std::uint8_t>(std::round(std::clamp(fB, 0.f, 1.f) * 255.f)),
		};
	}

	auto operator<=>(const LABColor&) const = default;

	LABColor& operator+=(const LABColor& _oOtherValue)
	{
		m_fL += _oOtherValue.m_fL;
		m_fA += _oOtherValue.m_fA;
		m_fB += _oOtherValue.m_fB;
		return *this;
	}

	friend LABColor operator+(LABColor _oLeftHandSide, const LABColor& _oRightHandSide)
	{
		_oLeftHandSide += _oRightHandSide;
		return _oLeftHandSide;
	}

	LABColor& operator-=(const LABColor& _oOtherValue)
	{
		m_fL -= _oOtherValue.m_fL;
		m_fA -= _oOtherValue.m_fA;
		m_fB -= _oOtherValue.m_fB;
		return *this;
	}

	friend LABColor operator-(LABColor _oLeftHandSide, const LABColor& _oRightHandSide)
	{
		_oLeftHandSide -= _oRightHandSide;
		return _oLeftHandSide;
	}

	LABColor& operator*=(const float _fValue)
	{
		m_fL = m_fL * _fValue;
		m_fA = m_fA * _fValue;
		m_fB = m_fB * _fValue;
		return *this;
	}

	friend LABColor operator*(LABColor _oLeftHandSide, const float _fRightHandSide)
	{
		_oLeftHandSide *= _fRightHandSide;
		return _oLeftHandSide;
	}

	LABColor Lerp(const LABColor& _oOtherColor, float _fBalance) const
	{
		return {
			MathUtils::Lerp(m_fL, _oOtherColor.m_fL, _fBalance),
			MathUtils::Lerp(m_fA, _oOtherColor.m_fA, _fBalance),
			MathUtils::Lerp(m_fB, _oOtherColor.m_fB, _fBalance) };
	}

	float GetDistance(const LABColor& _oOther) const
	{
		const float fDeltaL = m_fL - _oOther.m_fL;
		const float fDeltaA = m_fA - _oOther.m_fA;
		const float fDeltaB = m_fB - _oOther.m_fB;
		return std::sqrt(fDeltaL * fDeltaL + fDeltaA * fDeltaA + fDeltaB * fDeltaB);
	}
};

template<>
struct std::hash<LABColor>
{
	inline std::size_t operator()(const LABColor& _oColor) const
	{
		std::size_t uHash = 0;
		HashCombine(uHash, _oColor.m_fL);
		HashCombine(uHash, _oColor.m_fA);
		HashCombine(uHash, _oColor.m_fB);
		return uHash;
	}
};