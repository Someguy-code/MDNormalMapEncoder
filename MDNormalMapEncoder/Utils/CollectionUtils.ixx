export module CollectionUtils;

import std;

template<typename GetDistanceInvocableType, typename ElementType>
concept IsGetDistanceValueInvocable = 
	std::constructible_from<GetDistanceInvocableType, const ElementType&>&&
	std::regular_invocable<GetDistanceInvocableType, const ElementType&>;

template<typename GetDistanceInvocableType, typename ElementType>
concept IsGetDistanceReferenceInvocable =
	std::constructible_from<GetDistanceInvocableType, const ElementType*>&&
	std::regular_invocable<GetDistanceInvocableType, const ElementType&>;

template<typename CollectionType, typename ElementType>
concept IsCollectionOf = std::same_as<typename CollectionType::value_type, ElementType>;

export template <typename T>
concept HasGetSqrDistance = requires (const T Value1, const T & Value2)
{
	{ Value1.GetSqrDistance(Value2) } -> std::same_as<float>;
};

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

	template <HasGetSqrDistance ElementType>
	struct GetSqrDistanceInvocable
	{
		const ElementType* m_pElement;
		GetSqrDistanceInvocable(const ElementType& _oElement) :m_pElement(&_oElement) {}
		float operator()(const ElementType& _pOtherElement) const
		{
			return m_pElement->GetSqrDistance(_pOtherElement);
		}
	};

	template <typename ElementType, IsCollectionOf<ElementType> ConatinerType, IsGetDistanceValueInvocable<ElementType> GetDistanceInvocableType>
	static size_t GetClosestIndex(const ElementType& _oElement, const ConatinerType& _oCollection)
	{
		return std::distance(_oCollection.cbegin(), std::ranges::min_element(_oCollection, {}, GetDistanceInvocableType{ _oElement }));
	}

	template <typename ElementType, IsCollectionOf<ElementType> ConatinerType, IsGetDistanceReferenceInvocable<ElementType> GetDistanceInvocableType>
	static size_t GetClosestIndex(const ElementType& _oElement, const ConatinerType& _oCollection)
	{
		return std::distance(_oCollection.cbegin(), std::ranges::min_element(_oCollection, {}, GetDistanceInvocableType{&_oElement}));
	}

	template <typename ElementType, IsCollectionOf<ElementType> ConatinerType, IsGetDistanceValueInvocable<ElementType> GetDistanceInvocableType>
	static ElementType GetClosestValue(const ElementType& _oElement, const ConatinerType& _oCollection)
	{
		return _oCollection[GetClosestIndex<ElementType, ConatinerType, GetDistanceInvocableType>(_oElement, _oCollection)];
	}

	template <typename ElementType, IsCollectionOf<ElementType> ConatinerType, IsGetDistanceReferenceInvocable<ElementType> GetDistanceInvocableType>
	static size_t GetClosesValue(const ElementType& _oElement, const ConatinerType& _oCollection)
	{
		return _oCollection[GetClosestIndex<ElementType, ConatinerType, GetDistanceInvocableType>(_oElement, _oCollection)];
	}
};