export module DitherImage:ErrorTypeInterface;

import std;

export template <class ErrorType, class ColorType>
concept IsErrorType = std::constructible_from<ErrorType, ColorType> &&
	requires (ErrorType E1, const ErrorType& E2, float F, std::span<const ColorType> S)
{
	{ E1 += E2 } -> std::same_as<ErrorType&>;
	{ E1 + E2 } -> std::same_as<ErrorType>;
	{ E1 *= F } -> std::same_as<ErrorType&>;
	{ E1 * F } -> std::same_as<ErrorType>;
	{ E1.GetClosestColorIndex(S) } -> std::same_as<unsigned int>;
};