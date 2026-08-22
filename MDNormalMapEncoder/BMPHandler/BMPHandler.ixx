export module BMPHandler;

import std;
import Color;

export class BMPHandler
{
	struct Header
	{
		char m_sID[2] = { 'B', 'M' };
		std::uint32_t m_uFileSize;
		std::uint16_t m_uReserved1 = 0;
		std::uint16_t m_uReserved2 = 0;
		std::uint32_t m_uPixelArrayOffset;

		static const size_t m_kuSize = 14;

		Header() = default;
		Header(std::ifstream& _oFileStream);
		Header(size_t _uFileSize, size_t _uPixelArrayOffset);

		void Write(std::ofstream& _oFileStream) const;

	private:
		void Validate() const;
	};

	// BITMAPINFOHEADER
	struct BitmapInfoHeader
	{
		static const size_t m_kuDefaultSize = 40;

		std::uint32_t m_uHeaderSize = m_kuDefaultSize;
		std::int32_t m_iWidth;
		std::int32_t m_iHeight;
		std::uint16_t m_uColorPlaneCount = 1;
		std::uint16_t m_uBitsPerPixel;
		std::uint32_t m_uCompressionMethod = 0;
		//In theory, size of the pixels array in bits. NOT REALIABLE
		std::uint32_t m_uPixelArraySize;
		std::int32_t m_iPixelsPerMeterWidth = 0;
		std::int32_t m_iPixelsPerMeterHeight = 0;
		std::uint32_t m_uPaletteColorsCount = 0;
		std::uint32_t m_uImportantColorsCount = 0;

		BitmapInfoHeader() = default;
		BitmapInfoHeader(std::ifstream& _oFileStream);
		BitmapInfoHeader(unsigned int _uWidth, unsigned int _uHeight, unsigned int _uBitsPerPixel);

		unsigned int GetDWordsPerLine() const;

		void Write(std::ofstream& _oFileStream) const;

	private:
		void Validate() const;
	};

	Header m_oHeader;
	BitmapInfoHeader m_oBitmapInfoHeader;
	std::vector<ColorBGR> m_oPalette;
	std::vector<std::uint32_t> m_oPixelsArray;

public:
	BMPHandler(const char* _sFilename);
	//Palletized (8bit) BMP
	BMPHandler(std::mdspan<const std::uint8_t, std::dextents<size_t, 2>> _oPixelsArray, std::span<const ColorRGB> _oPalette);
	//Direct-color (24bit) BMP
	BMPHandler(std::mdspan<const ColorRGB, std::dextents<size_t, 2>> _oPixelsArray);

	unsigned int GetWidth() const;
	unsigned int GetHeight() const;
	ColorRGB operator[](unsigned int _uX, unsigned int _uY) const;

	void Write(const char* _sFilename) const;

private:
	void FillPixelsArray(std::mdspan<const std::uint8_t, std::dextents<size_t, 2>> _oPixelsArray, std::span<const ColorRGB> _oPalette);
	void FillPixelsArray(std::mdspan<const ColorRGB, std::dextents<size_t, 2>> _oPixelsArray);

	void ReadPalette(std::ifstream& _oFileStream);
	void ReadPixelsArray(std::ifstream& _oFileStream);

	void WritePalette(std::ofstream& _oFileStream) const;
	void WritePixelsArray(std::ofstream& _oFileStream) const;

	static size_t GetSize(unsigned int _uWidth, unsigned int _uHeight, unsigned int _uBitsPerPixel);
	static size_t GetPixelArrayOffset(unsigned int _uBitsPerPixel);
};