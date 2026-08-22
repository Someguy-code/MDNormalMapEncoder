export module RawImage;

import std;

export template<typename PixelFormat>
struct RawImage
{
    bool m_bIsValid{ false };
    unsigned int m_uWidth;
    unsigned int m_uHeight;
    std::vector<PixelFormat> m_oPixelsArray;

    RawImage() = default;

    RawImage(unsigned int _uWidth, unsigned int _uHeight)
    : m_bIsValid(true), m_uWidth(_uWidth), m_uHeight(_uHeight), m_oPixelsArray(_uWidth* _uHeight)
    {}

    RawImage(unsigned int _uWidth, unsigned int _uHeight, const std::vector<PixelFormat>& _oInputPixelsArray)
    : m_bIsValid(true), m_uWidth(_uWidth), m_uHeight(_uHeight), m_oPixelsArray(_oInputPixelsArray)
    {}

    template<typename TargetPixelFormat, typename TypeConversion = DefaultTypeConversion<TargetPixelFormat>>
    RawImage<TargetPixelFormat> GetTypeConversion() const
    {
        //Converts the current PixelFomrat to the target using a conversion cache
        RawImage<TargetPixelFormat> oTypeConversion{ m_uWidth, m_uHeight };
        struct ConvertValue
        {
            std::unordered_map<PixelFormat, TargetPixelFormat> m_oConversionCache;
            TargetPixelFormat operator()(const PixelFormat& _oPixel)
            {
                auto itResult = m_oConversionCache.find(_oPixel);
                if (itResult == m_oConversionCache.cend())
                    itResult = m_oConversionCache.emplace(_oPixel, TypeConversion{}(_oPixel)).first;
                return itResult->second;
            }
        };
        std::transform(m_oPixelsArray.cbegin(), m_oPixelsArray.cend(), oTypeConversion.m_oPixelsArray.begin(), ConvertValue{});
        return oTypeConversion;
    }

    std::mdspan<const PixelFormat, std::dextents<size_t, 2>> GetFullRect() const
    {
        return std::mdspan(m_oPixelsArray.data(), m_uHeight, m_uWidth);
    }

    std::mdspan<PixelFormat, std::dextents<size_t, 2>> GetFullRect()
    {
        return std::mdspan(m_oPixelsArray.data(), m_uHeight, m_uWidth);
    }

    auto GetlRect(unsigned int _uX, unsigned int _uY, unsigned int _uWidth, unsigned int _uHeight) const
    {
        auto oRectStride = std::layout_stride::mapping<std::dextents<size_t, 2>>(std::dextents<size_t, 2>{}, std::array<size_t, 2>{ _uHeight, _uWidth });
        return std::mdspan(&GetFullRect()[_uY, _uX], oRectStride);
    }

private:
    template<typename TargetPixelFormat>
    struct DefaultTypeConversion
    {
        TargetPixelFormat operator()(const PixelFormat& _oOriginal) { return static_cast<TargetPixelFormat>(_oOriginal); }
    };
};
