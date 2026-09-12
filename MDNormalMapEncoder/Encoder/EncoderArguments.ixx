export module EncoderArguments;

import Color;
import std;

export struct EncoderArguments
{
    //Filename for the mask texture. Black pixel will be marked as transparent.
    const char* m_sMaskFilename{ nullptr };
    //Filename for the normal map texture
    const char* m_sNormalFilename{ nullptr };
    //Filename for the albedo texture
    const char* m_sAlbedoFilename{ nullptr };
    //Filename for the ambinet occlusion texture. Will be quantized to black and white.
    const char* m_sAmbinetOcclusionFilename{ nullptr };

    //Number of directions in the X-Y plane. Must be at least 2.
    unsigned int m_uHorizontalNormalMapSides{ 6 };
    //Number of directions on the X-Z axis (including looking straight up). Must be at least 1.
    unsigned int m_uVerticalNormalMapSides{ 3 };
    //Maximum number of colors for albedo
    unsigned int m_uMaxAlbedoColors{ 2 };

    //Base 2 logarithm of the pre-computed shades count
    unsigned int m_uLogLightShadesCount{ 3 };
    //Color of the front light
    ColorRGB m_oFrontLightColor{ 255, 255, 255 };
    //Color of the back light
    ColorRGB m_oBackLightColor{ 0, 0, 0 };
    //Color in absence of light (doesn't modulate)
    ColorRGB m_oPureDarknessColor{ 0, 0, 0 };

    //Intensity of the specular component (in the [0, 1] range)
    float m_fSpecularIntensity{ 0.f };
    //Exponent of the light strength power
    float m_fSpecularHardness{ 1.f };
    //Exponent of the light strength power
    unsigned int m_uSpecularLogShadesCount{ 4 };

    //Prefix filename for the output texture (in case more than one texture need to be generated)
    const char* m_sBaseOutputFilename{ nullptr };
    //Filename of the ouput materials file
    const char* m_sOutputMaterialsFilename{ nullptr };

    void Validate() const
    {
        if (m_sBaseOutputFilename == nullptr)
            throw std::runtime_error("Missing base output texture filename.");
        if (m_sOutputMaterialsFilename == nullptr && m_sNormalFilename != nullptr)
            throw std::runtime_error("Missing output materials filename.");
        if (m_sAlbedoFilename == nullptr && m_sNormalFilename == nullptr)
            throw std::runtime_error("Neither normal map nor albedo textures specified. Nothing to generate.");
        if (m_uHorizontalNormalMapSides < 2)
            throw std::runtime_error("Specified horizontal normal map sides is below 2");
        if (m_uVerticalNormalMapSides < 2)
            throw std::runtime_error("Specified vertical normal map sides is below 1");
        if (m_fSpecularIntensity < 0.f || m_fSpecularIntensity > 1.f)
            throw std::runtime_error("Specified specular intesity is not in the [0, 1] range");
    }
};