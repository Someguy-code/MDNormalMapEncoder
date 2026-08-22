#include <cassert>
import BMPHandler;
import Color;
import ExceptionUtils;
import MathUtils;
import SerializationUtils;
import std;

static unsigned int GetBitsPerPixel(std::span<const ColorRGB> _oPalette);
static size_t GetPaletteSize(unsigned int _uBitsPerPixel);
static unsigned int GetDWordsPerLine(unsigned int _uWidth, unsigned int _uBitsPerPixel);
static unsigned int GetPixelArraySize(unsigned int _uWidth, unsigned int _uHeight, unsigned int _uBitsPerPixel);

BMPHandler::Header::Header(std::ifstream& _oFileStream)
{
	SerializationUtils::ReadValue(_oFileStream, m_sID);
	SerializationUtils::ReadValue(_oFileStream, m_uFileSize);
	SerializationUtils::ReadValue(_oFileStream, m_uReserved1);
	SerializationUtils::ReadValue(_oFileStream, m_uReserved2);
	SerializationUtils::ReadValue(_oFileStream, m_uPixelArrayOffset);
}

BMPHandler::Header::Header(size_t _uFileSize, size_t _uPixelArrayOffset)
: m_uFileSize(static_cast<uint32_t>(_uFileSize)), m_uPixelArrayOffset(static_cast<uint32_t>(_uPixelArrayOffset))
{}

void BMPHandler::Header::Validate() const
{
	if (!(m_sID[0] == 'B' && m_sID[1] == 'M'))
		throw std::runtime_error("Invalid BMP file");
}

void BMPHandler::Header::Write(std::ofstream& _oFileStream) const
{
	SerializationUtils::WriteValue(_oFileStream, m_sID);
	SerializationUtils::WriteValue(_oFileStream, m_uFileSize);
	SerializationUtils::WriteValue(_oFileStream, m_uReserved1);
	SerializationUtils::WriteValue(_oFileStream, m_uReserved2);
	SerializationUtils::WriteValue(_oFileStream, m_uPixelArrayOffset);
}

BMPHandler::BitmapInfoHeader::BitmapInfoHeader(std::ifstream& _oFileStream)
{
	SerializationUtils::ReadValue(_oFileStream, m_uHeaderSize);
	SerializationUtils::ReadValue(_oFileStream, m_iWidth);
	SerializationUtils::ReadValue(_oFileStream, m_iHeight);
	SerializationUtils::ReadValue(_oFileStream, m_uColorPlaneCount);
	SerializationUtils::ReadValue(_oFileStream, m_uBitsPerPixel);
	SerializationUtils::ReadValue(_oFileStream, m_uCompressionMethod);
	SerializationUtils::ReadValue(_oFileStream, m_uPixelArraySize);
	SerializationUtils::ReadValue(_oFileStream, m_iPixelsPerMeterWidth);
	SerializationUtils::ReadValue(_oFileStream, m_iPixelsPerMeterHeight);
	SerializationUtils::ReadValue(_oFileStream, m_uPaletteColorsCount);
	SerializationUtils::ReadValue(_oFileStream, m_uImportantColorsCount);

	if (m_uHeaderSize > m_kuDefaultSize)
		_oFileStream.seekg(m_uHeaderSize - m_kuDefaultSize, std::ios_base::cur);

	Validate();
}

BMPHandler::BitmapInfoHeader::BitmapInfoHeader(unsigned int _uWidth, unsigned int _uHeight, unsigned int _uBitsPerPixel)
	:m_iWidth(_uWidth), m_iHeight(_uHeight), m_uBitsPerPixel(_uBitsPerPixel), m_uPaletteColorsCount(_uBitsPerPixel <= 8 ? MathUtils::MulByPowerOf2(1, _uBitsPerPixel) : 0), m_uPixelArraySize(GetPixelArraySize(_uWidth, _uHeight, _uBitsPerPixel))
{}

void BMPHandler::BitmapInfoHeader::Validate() const
{
	if (m_uHeaderSize < 40)
		throw std::runtime_error("Unsupported BMP type");
	if(!(m_uBitsPerPixel == 8 || m_uBitsPerPixel == 24 || m_uBitsPerPixel == 32))
		throw std::runtime_error("Unsupported bit depth (only 8, 24 and 32 bpp suppoprted)");
}

unsigned int BMPHandler::BitmapInfoHeader::GetDWordsPerLine() const
{
	return ::GetDWordsPerLine(m_iWidth, m_uBitsPerPixel);
}

void BMPHandler::BitmapInfoHeader::Write(std::ofstream& _oFileStream) const
{
	SerializationUtils::WriteValue(_oFileStream, m_uHeaderSize);
	SerializationUtils::WriteValue(_oFileStream, m_iWidth);
	SerializationUtils::WriteValue(_oFileStream, m_iHeight);
	SerializationUtils::WriteValue(_oFileStream, m_uColorPlaneCount);
	SerializationUtils::WriteValue(_oFileStream, m_uBitsPerPixel);
	SerializationUtils::WriteValue(_oFileStream, m_uCompressionMethod);
	SerializationUtils::WriteValue(_oFileStream, m_uPixelArraySize);
	SerializationUtils::WriteValue(_oFileStream, m_iPixelsPerMeterWidth);
	SerializationUtils::WriteValue(_oFileStream, m_iPixelsPerMeterHeight);
	SerializationUtils::WriteValue(_oFileStream, m_uPaletteColorsCount);
	SerializationUtils::WriteValue(_oFileStream, m_uImportantColorsCount);
}

BMPHandler::BMPHandler(const char* _sFilename)
{
	std::ifstream oFileStream(_sFilename, std::ios_base::binary);
	if (oFileStream)
	{
		m_oHeader = Header(oFileStream);
		m_oBitmapInfoHeader = BitmapInfoHeader(oFileStream);
		ReadPalette(oFileStream);
		ReadPixelsArray(oFileStream);
	}
	else
		ExceptionUtils::ThrowOpenFileFailureException();
}

BMPHandler::BMPHandler(std::mdspan<const uint8_t, std::dextents<size_t, 2>> _oPixelsArray, std::span<const ColorRGB> _oPalette)
{
	const unsigned int uWidth = static_cast<unsigned int>(_oPixelsArray.extent(1));
	const unsigned int uHeight = static_cast<unsigned int>(_oPixelsArray.extent(0));
	const unsigned int uBitsPerPixel = GetBitsPerPixel(_oPalette);
	m_oHeader = Header(GetSize(uWidth, uHeight, uBitsPerPixel), GetPixelArrayOffset(uBitsPerPixel));
	m_oBitmapInfoHeader = BitmapInfoHeader(uWidth, uHeight, uBitsPerPixel);
	const size_t uColorsCount = _oPalette.size();
	m_oPalette.reserve(uColorsCount);
	for (const ColorRGB& oColor : _oPalette)
		m_oPalette.push_back(oColor);
	FillPixelsArray(_oPixelsArray, _oPalette);
}

BMPHandler::BMPHandler(std::mdspan<const ColorRGB, std::dextents<size_t, 2>> _oPixelsArray)
{
	const unsigned int uWidth = static_cast<unsigned int>(_oPixelsArray.extent(1));
	const unsigned int uHeight = static_cast<unsigned int>(_oPixelsArray.extent(0));
	const unsigned int uBitsPerPixel = 24;
	m_oHeader = Header(GetSize(uWidth, uHeight, uBitsPerPixel), GetPixelArrayOffset(uBitsPerPixel));
	m_oBitmapInfoHeader = BitmapInfoHeader(uWidth, uHeight, uBitsPerPixel);
	FillPixelsArray(_oPixelsArray);
}

unsigned int BMPHandler::GetWidth() const
{
	return m_oBitmapInfoHeader.m_iWidth;
}

unsigned int BMPHandler::GetHeight() const
{
	return m_oBitmapInfoHeader.m_iHeight;
}

ColorRGB BMPHandler::operator[](unsigned int _uY, unsigned int _uX) const
{
	const unsigned int uDWordsPerLine = m_oBitmapInfoHeader.GetDWordsPerLine();
	const auto oDWordsArray = std::mdspan(m_oPixelsArray.data(), m_oBitmapInfoHeader.m_iHeight, uDWordsPerLine);
	const unsigned int uBitsPerPixel = m_oBitmapInfoHeader.m_uBitsPerPixel;
	const unsigned int uBitIndex = _uX * uBitsPerPixel;
	const unsigned int uDWordIndex = uBitIndex / 32;
	const unsigned int uDWordBitIndex = MathUtils::ModByPowerOf2(uBitIndex, 5); //Modulo 32
	const uint32_t* pDWordlValue = &oDWordsArray[_uY, uDWordIndex];
	const std::uint8_t* pValue = &reinterpret_cast<const std::uint8_t*>(pDWordlValue)[uDWordBitIndex / 8];
	switch(uBitsPerPixel)
	{
		case 8:
			return m_oPalette[*pValue];
		case 24:
			return ColorBGR{ pValue, false };
		case 32:
			return ColorBGR{ pValue, false };
		default:
			assert(!"Unsupported bit depth");
			return {};
	}
}

void BMPHandler::Write(const char* _sFilename) const
{
	std::ofstream oFileStream(_sFilename, std::ios_base::binary);
	if (oFileStream)
	{
		m_oHeader.Write(oFileStream);
		m_oBitmapInfoHeader.Write(oFileStream);
		WritePalette(oFileStream);
		WritePixelsArray(oFileStream);
	}
	else
		ExceptionUtils::ThrowOpenFileFailureException();
}

void BMPHandler::FillPixelsArray(std::mdspan<const uint8_t, std::dextents<size_t, 2>> _oPixelsArray, std::span<const ColorRGB> _oPalette)
{
	const unsigned int uWidth = static_cast<unsigned int>(_oPixelsArray.extent(1));
	const unsigned int uHeight = static_cast<unsigned int>(_oPixelsArray.extent(0));
	const unsigned int uBitsPerPixel = GetBitsPerPixel(_oPalette);
	assert(uBitsPerPixel == 8);
	const unsigned int uDWordsPerLine = GetDWordsPerLine(uWidth, uBitsPerPixel);
	m_oPixelsArray.resize(uDWordsPerLine * uHeight);
	const auto oDWordsArray = std::mdspan(m_oPixelsArray.data(), uHeight, uDWordsPerLine);
	for(unsigned int uY = 0; uY < uHeight; ++uY)
	{
		uint8_t* pCurrentOutputByte = reinterpret_cast<uint8_t*>(&oDWordsArray[uY, 0]);
		for (unsigned int uX = 0; uX < uHeight; ++uX)
			*pCurrentOutputByte++ = _oPixelsArray[uY, uX];
	}
}

void BMPHandler::FillPixelsArray(std::mdspan<const ColorRGB, std::dextents<size_t, 2>> _oPixelsArray)
{
	const unsigned int uWidth = static_cast<unsigned int>(_oPixelsArray.extent(1));
	const unsigned int uHeight = static_cast<unsigned int>(_oPixelsArray.extent(0));
	const unsigned int uBitsPerPixel = 24;
	const unsigned int uDWordsPerLine = GetDWordsPerLine(uWidth, uBitsPerPixel);
	m_oPixelsArray.resize(uDWordsPerLine * uHeight);
	const auto oDWordsArray = std::mdspan(m_oPixelsArray.data(), uHeight, uDWordsPerLine);
	for (unsigned int uY = 0; uY < uHeight; ++uY)
	{
		uint8_t* pCurrentOutputByte = reinterpret_cast<uint8_t*>(&oDWordsArray[uY, 0]);
		for (unsigned int uX = 0; uX < uWidth; ++uX)
		{
			const ColorRGB& oColor = _oPixelsArray[uY, uX];
			*pCurrentOutputByte++ = oColor.m_uB;
			*pCurrentOutputByte++ = oColor.m_uG;
			*pCurrentOutputByte++ = oColor.m_uR;
		}
	}
}

void BMPHandler::ReadPalette(std::ifstream& _oFileStream)
{
	const unsigned int uColorsCount = m_oBitmapInfoHeader.m_uPaletteColorsCount;
	m_oPalette.reserve(uColorsCount);
	for (unsigned int uColorIndex = 0; uColorIndex < uColorsCount; ++uColorIndex)
		m_oPalette.emplace_back(_oFileStream, true);
}

void BMPHandler::ReadPixelsArray(std::ifstream& _oFileStream)
{
	const unsigned int uPixelArraySize = GetPixelArraySize(m_oBitmapInfoHeader.m_iWidth, m_oBitmapInfoHeader.m_iHeight, m_oBitmapInfoHeader.m_uBitsPerPixel);
	m_oPixelsArray.resize(uPixelArraySize / 4);
	_oFileStream.read(reinterpret_cast<char*>(m_oPixelsArray.data()), uPixelArraySize);
	auto uReadBytes = _oFileStream.gcount();
	if (!_oFileStream)
		throw std::runtime_error(std::format("Only {} out of {} bytes read", uReadBytes, uPixelArraySize));
}

void BMPHandler::WritePalette(std::ofstream& _oFileStream) const
{
	for (const ColorBGR& oColor : m_oPalette)
		oColor.Write(_oFileStream, true);
}

void BMPHandler::WritePixelsArray(std::ofstream& _oFileStream) const
{
	const unsigned int uPixelArraySize = m_oBitmapInfoHeader.m_uPixelArraySize;
	_oFileStream.write(reinterpret_cast<const char*>(m_oPixelsArray.data()), uPixelArraySize);
}

size_t BMPHandler::GetSize(unsigned int _uWidth, unsigned int _uHeight, unsigned int _uBitsPerPixel)
{
	return GetPixelArrayOffset(_uBitsPerPixel) + GetPixelArraySize(_uWidth, _uHeight, _uBitsPerPixel);
}

size_t BMPHandler::GetPixelArrayOffset(unsigned int _uBitsPerPixel)
{
	return Header::m_kuSize + BitmapInfoHeader::m_kuDefaultSize + GetPaletteSize(_uBitsPerPixel);
}

static unsigned int GetBitsPerPixel(std::span<const ColorRGB> _oPalette)
{
	return static_cast<unsigned int>(std::floor(std::log2(_oPalette.size())));
}

static size_t GetPaletteSize(unsigned int _uBitsPerPixel)
{
	return _uBitsPerPixel <= 8 ? MathUtils::MulByPowerOf2(1, _uBitsPerPixel) * 4: 0;
}

static unsigned int GetDWordsPerLine(unsigned int _uWidth, unsigned int _uBitsPerPixel)
{
	return MathUtils::DivByPowerOf2RoundUp(_uWidth * _uBitsPerPixel, 5);
}

static unsigned int GetPixelArraySize(unsigned int _uWidth, unsigned int _uHeight, unsigned int _uBitsPerPixel)
{
	return GetDWordsPerLine(_uWidth, _uBitsPerPixel) * 4 * _uHeight;
}