export module EncoderArguments;

import Color;
import RawImage;
import std;

export struct EncoderArguments
{
    //Filename for the mask texture. Black pixel will be marked as transparent.
    std::optional<RawImage<ColorRGB>> m_oMask;
    //Filename for the normal map texture
    std::optional<RawImage<ColorRGB>> m_oNormal;
    //Filename for the albedo texture
    std::optional<RawImage<ColorRGB>> m_oAlbedo;
    //Filename for the ambinet occlusion texture. Will be quantized to black and white.
    std::optional<RawImage<ColorRGB>> m_oAmbinetOcclusion;

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
};