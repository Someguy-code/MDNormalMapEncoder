export module Color;

import MathUtils;
import std;
import SerializationUtils;

enum class ColorComponent
{
	R = 0, G, B
};

export template <ColorComponent eFirstComponent, ColorComponent eSecondComponent, ColorComponent eThirdComponent>
struct Color
{
	std::uint8_t m_uR = 0;
	std::uint8_t m_uG = 0;
	std::uint8_t m_uB = 0;
	std::uint8_t m_uA = 0;

	Color() = default;

	Color(std::ifstream& _oFileStream, bool _bHasAlpha)
	{
		SerializationUtils::ReadValue(_oFileStream, GetComponent<eFirstComponent>());
		SerializationUtils::ReadValue(_oFileStream, GetComponent<eSecondComponent>());
		SerializationUtils::ReadValue(_oFileStream, GetComponent<eThirdComponent>());
		if (_bHasAlpha)
			SerializationUtils::ReadValue(_oFileStream, m_uA);
	}

	Color(const std::uint8_t* _pComponents, bool _bHasAlpha)
		: m_uR(_pComponents[static_cast<int>(eFirstComponent)]),
		m_uG(_pComponents[static_cast<int>(eSecondComponent)]),
		m_uB(_pComponents[static_cast<int>(eThirdComponent)]),
		m_uA(_bHasAlpha ? _pComponents[3] : 0)
	{}

	Color(std::uint8_t _uR, std::uint8_t _uG, std::uint8_t _uB)
		: m_uR(_uR), m_uG(_uG), m_uB(_uB)
	{}

	Color(std::uint8_t _uR, std::uint8_t _uG, std::uint8_t _uB, std::uint8_t _uA)
		: m_uR(_uR), m_uG(_uG), m_uB(_uB), m_uA(_uA)
	{}

	bool IsNull() const
	{
		return m_uR == 0.f && m_uG == 0.f && m_uB == 0.f;
	}

	auto operator<=>(const Color&) const = default;

	Color& operator+=(const Color& _oOtherValue)
	{
		m_uR += _oOtherValue.m_uR;
		m_uG += _oOtherValue.m_uG;
		m_uB += _oOtherValue.m_uB;
		return *this;
	}

	friend Color operator+(Color _oLeftHandSide, const Color& _oRightHandSide)
	{
		_oLeftHandSide += _oRightHandSide;
		return _oLeftHandSide;
	}

	Color& operator-=(const Color& _oOtherValue)
	{
		m_uR -= _oOtherValue.m_uR;
		m_uG -= _oOtherValue.m_uG;
		m_uB -= _oOtherValue.m_uB;
		return *this;
	}

	friend Color operator-(Color _oLeftHandSide, const Color& _oRightHandSide)
	{
		_oLeftHandSide -= _oRightHandSide;
		return _oLeftHandSide;
	}

	Color& operator*=(const float _fValue)
	{
		m_uR = static_cast<uint8_t>(round(m_uR * _fValue));
		m_uG = static_cast<uint8_t>(round(m_uG * _fValue));
		m_uB = static_cast<uint8_t>(round(m_uB * _fValue));
		return *this;
	}

	friend Color operator*(Color _oLeftHandSide, const float _fRightHandSide)
	{
		_oLeftHandSide *= _fRightHandSide;
		return _oLeftHandSide;
	}

	Color& operator*=(const Color& _oOther)
	{
		m_uR = MathUtils::DivByPowerOf2Round(m_uR * _oOther.m_uR, 8);
		m_uG = MathUtils::DivByPowerOf2Round(m_uG * _oOther.m_uG, 8);
		m_uB = MathUtils::DivByPowerOf2Round(m_uB * _oOther.m_uB, 8);
		return *this;
	}

	friend Color operator*(Color _oLeftHandSide, const Color& _oRightHandSide)
	{
		_oLeftHandSide *= _oRightHandSide;
		return _oLeftHandSide;
	}

	//Components order is only relevant for serialization, so allow free conversiosn between all Color types
	template <ColorComponent eFirstComponent2, ColorComponent eSecondComponent2, ColorComponent eThirdComponent2>
	constexpr operator Color<eFirstComponent2, eSecondComponent2, eThirdComponent2>() const
	{
		return { m_uR, m_uG, m_uB, m_uA };
	}

	constexpr explicit operator std::uint32_t() const
	{
		return m_uB | (m_uG << 8) | (m_uR << 16) | (m_uA << 24);
	}

	float GetSqrDistance(const Color& _oOtherColor) const
	{
		return MathUtils::VectorModuleSquared(static_cast<float>(_oOtherColor.m_uR - m_uR), static_cast<float>(_oOtherColor.m_uG - m_uG), static_cast<float>(_oOtherColor.m_uB - m_uB));
	}

	float GetDistance(const Color& _oOtherColor) const
	{
		return std::sqrt(GetSqrDistance(_oOtherColor));
	}

	Color Lerp(const Color& _oOtherColor, float _fBalance) const
	{
		return { 
			MathUtils::Lerp(m_uR, _oOtherColor.m_uR, _fBalance), 
			MathUtils::Lerp(m_uG, _oOtherColor.m_uG, _fBalance), 
			MathUtils::Lerp(m_uB, _oOtherColor.m_uB, _fBalance) };
	}

	void Write(std::ofstream& _oFileStream, bool _bHasAlpha) const
	{
		std::uint8_t oComponents[] = {m_uR, m_uG, m_uB};
		SerializationUtils::WriteValue(_oFileStream, GetComponent<eFirstComponent>());
		SerializationUtils::WriteValue(_oFileStream, GetComponent<eSecondComponent>());
		SerializationUtils::WriteValue(_oFileStream, GetComponent<eThirdComponent>());
		if (_bHasAlpha)
			SerializationUtils::WriteValue(_oFileStream, m_uA);
	}

private:
	template <ColorComponent eColorComponent>
	std::uint8_t& GetComponent();
	template<>
	std::uint8_t& GetComponent<ColorComponent::R>() { return m_uR; }
	template<>
	std::uint8_t& GetComponent<ColorComponent::G>() { return m_uG; }
	template<>
	std::uint8_t& GetComponent<ColorComponent::B>() { return m_uB; }

	template <ColorComponent eColorComponent>
	const std::uint8_t& GetComponent() const;
	template<>
	const std::uint8_t& GetComponent<ColorComponent::R>() const { return m_uR; }
	template<>
	const std::uint8_t& GetComponent<ColorComponent::G>() const { return m_uG; }
	template<>
	const std::uint8_t& GetComponent<ColorComponent::B>() const { return m_uB; }
};

export using ColorRGB = Color<ColorComponent::R, ColorComponent::G, ColorComponent::B>;
export using ColorBGR = Color<ColorComponent::B, ColorComponent::G, ColorComponent::R>;

template<>
struct std::hash<ColorRGB>
{
	inline std::size_t operator()(const ColorRGB& _oColor) const
	{
		return (_oColor.m_uA << 24) | (_oColor.m_uR << 16) | (_oColor.m_uG << 8) | _oColor.m_uB;
	}
};