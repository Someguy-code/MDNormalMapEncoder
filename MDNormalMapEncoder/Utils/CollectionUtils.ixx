export module CollectionUtils;

import std;

export struct CollectionUtils
{
	template<std::input_or_output_iterator Iter, typename ValueType> requires
		std::is_same_v<typename std::iterator_traits<Iter>::value_type, ValueType>
	static constexpr std::iter_difference_t<Iter> GetClosestValueIndex(Iter _itStart, Iter _itEnd, ValueType _oValue)
	{
		struct GetClosestIndex
		{
			ValueType m_oReferenceValue;

			bool operator() (ValueType _uValue1, ValueType _uValue2) const
			{
				return std::abs(m_oReferenceValue - _uValue1) < std::abs(m_oReferenceValue - _uValue2);
			}
		};

		return std::distance(_itStart, std::min_element(_itStart, _itEnd, GetClosestIndex{ _oValue }));
	}

	template<std::input_or_output_iterator Iter, typename ValueType> requires
		std::is_same_v<typename std::iterator_traits<Iter>::value_type, ValueType>
		static constexpr std::iter_difference_t<Iter> GetClosestValueIndexOrdered(Iter _itStart, Iter _itEnd, ValueType _oValue)
	{
		return std::distance(_itStart, std::lower_bound(_itStart, _itEnd, _oValue));
	}
};