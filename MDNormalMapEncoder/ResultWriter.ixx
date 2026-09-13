export module ResultWriter;

import BMPHandler;
import Encoder;
import ImageSplit;
import CommandLineArguments;
import std;

export struct ResultWriter
{
    static void WriteResult(const Encoder::Result& _oResult, const CommandLineArguments& _oArguments)
    {
        WriteImages(_oResult.m_oImagesAndPalettes, _oArguments.m_sBaseOutputFilename);
        if (_oResult.m_oMaterial)
            WriteMaterial(_oResult.m_oMaterial.value(), _oArguments.m_sOutputMaterialsFilename);

        PrintStatistics(_oResult);
    }

private:

    static void WriteImages(const std::vector<ImageSplit::ImageAndPalette>& _oImagesAndPalettes, const char* _sBaseOutputFilename)
    {
        const size_t uPartsCount = _oImagesAndPalettes.size();
        for (unsigned int uPartIndex = 0; uPartIndex < uPartsCount; ++uPartIndex)
        {
            const ImageSplit::ImageAndPalette& oImageAndPalette = _oImagesAndPalettes[uPartIndex];
            std::vector<ColorRGB> oPalette(256);
            std::copy(oImageAndPalette.m_oPalette.cbegin(), oImageAndPalette.m_oPalette.cend(), oPalette.begin());
            const BMPHandler oOutputNormalMap(oImageAndPalette.m_oImage.GetFullRect(), oPalette);
            std::string sOutputNormalMapFilename = uPartsCount == 1 ? _sBaseOutputFilename :
                GetPartFilename(_sBaseOutputFilename, uPartIndex);
            try
            {
                std::cout << "Writting " << sOutputNormalMapFilename << "\n";
                oOutputNormalMap.Write(sOutputNormalMapFilename.c_str());
            }
            catch (const std::runtime_error& oException)
            {
                throw std::runtime_error(std::format("Exception writting {}: {}", sOutputNormalMapFilename, oException.what()));
            }
            catch (...)
            {
                throw std::runtime_error(std::format("Unhandled exception writting {}", sOutputNormalMapFilename));
            }
        }
    }

    static void WriteMaterial(const Material& _oMaterial, const char* _sOutputMaterialsFilename)
    {
        try
        {
            std::cout << "Writting " << _sOutputMaterialsFilename << "\n";
            _oMaterial.Write(_sOutputMaterialsFilename);
        }
        catch (const std::runtime_error& oException)
        {
            throw std::runtime_error(std::format("Exception writting {}: {}", _sOutputMaterialsFilename, oException.what()));
        }
        catch (...)
        {
            throw std::runtime_error(std::format("Unhandled exception writting {}", _sOutputMaterialsFilename));
        }
    }

    static void PrintStatistics(const Encoder::Result& _oResult)
    {
        std::cout << "Statistics:\n";
        const std::vector<ImageSplit::ImageAndPalette>& oImagesAndPalettes = _oResult.m_oImagesAndPalettes;
        std::cout << "    * Number of layers: " << oImagesAndPalettes.size() << std::endl;
        const size_t uPaletteEntriesCount = std::accumulate(oImagesAndPalettes.cbegin(), oImagesAndPalettes.cend(), (size_t)0,
            [](const size_t _uPaletteEntriesCount, const auto& _oImageAndPalette) {return _uPaletteEntriesCount + _oImageAndPalette.m_oPalette.size() - 1;});
        std::cout << "    * Number of palette entries: " << uPaletteEntriesCount << std::endl;
        const size_t uNormalsCount = _oResult.m_oMaterial.transform([](const Material& _oMaterial) {return _oMaterial.m_oNormals.size();}).
            or_else([]() {return std::optional<size_t>(0);}).value();
        if(uNormalsCount > 0)
            std::cout << "    * Number of normals: " << uNormalsCount << std::endl;
        std::cout << "    * Number of albedo colors: " << (uNormalsCount == 0 ? uPaletteEntriesCount : uPaletteEntriesCount / uNormalsCount) << std::endl;
    }

    static std::string GetPartFilename(const char* _sBaseFilename, unsigned int _uPartIndex)
    {
        const char* pExtensionStart = std::strrchr(_sBaseFilename, '.');
        const unsigned int uExtensionStartIndex = static_cast<unsigned int>(pExtensionStart - _sBaseFilename);
        return std::format("{}_{}{}", std::string{ _sBaseFilename, _sBaseFilename + uExtensionStartIndex }, _uPartIndex, pExtensionStart);
    }
};