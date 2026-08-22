export module SerializationUtils;

import std;

export struct SerializationUtils
{
	template<typename T>
	static void ReadValue(std::ifstream& _oFileStream, T& _oObject) { (void)_oFileStream.read(reinterpret_cast<char*>(&_oObject), sizeof(T)); }

	template<typename T, std::size_t Size>
	static void ReadValue(std::ifstream& _oFileStream, T(&_pArray)[Size]) { (void)_oFileStream.read(reinterpret_cast<char*>(_pArray), sizeof(T) * Size); }

	template<typename T>
	static void WriteValue(std::ofstream& _oFileStream, const T& _oObject) {(void)_oFileStream.write(reinterpret_cast<const char*>(&_oObject), sizeof(T));}

	template<typename T, std::size_t Size>
	static void WriteValue(std::ofstream& _oFileStream, const T(&_pArray)[Size]) { (void)_oFileStream.write(reinterpret_cast<const char*>(_pArray), sizeof(T) * Size); }


	template<typename T>
	static void WriteValueBigEndian(std::ofstream& _oFileStream, const T& _oObject) { WriteBigEndian<T>{_oFileStream}(_oObject); }

	template<typename T, std::size_t Size>
	static void WriteValueEndian(std::ofstream& _oFileStream, const T(&_pArray)[Size]) { std::for_each(std::cbegin(_pArray), std::cend(_pArray), WriteBigEndian<T>{_oFileStream}); }

private:
	template<typename T>
	struct WriteBigEndian
	{
		WriteBigEndian(std::ofstream& _oFileStream) :m_oWriteByte(_oFileStream) {}

		void operator()(const T& _oValue) const
		{
			const std::uint8_t(&oBytes)[sizeof(T)] = *reinterpret_cast<const std::uint8_t(*)[sizeof(T)]>(&_oValue);
			std::for_each(std::crbegin(oBytes), std::crend(oBytes), m_oWriteByte);
		}
	private:
		struct WriteByte
		{
			std::ofstream& m_oFileStream;
			void operator()(std::uint8_t _uByte) const
			{
				m_oFileStream.write(reinterpret_cast<const char*>(&_uByte), 1);
			}
		}m_oWriteByte;
	};
};