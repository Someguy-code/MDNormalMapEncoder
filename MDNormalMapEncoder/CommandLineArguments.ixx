module;

#include <stdio.h>
#include <cassert>

export module CommandLineArguments;

import BMPHandler;
import Color;
import EncoderArguments;
import std;

export struct CommandLineArguments
{
	//Filename for the mask texture. Black pixel will be marked as transparent.
	const char* m_sMaskFilename{ nullptr };
	//Filename for the normal map texture
	const char* m_sNormalFilename{ nullptr };
	//Filename for the albedo texture
	const char* m_sAlbedoFilename{ nullptr };
	//Filename for the ambinet occlusion texture. Will be quantized to black and white.
	const char* m_sAmbientOcclussionFilename{ nullptr };

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

	CommandLineArguments(unsigned int _uArgumentsCount, char* _sArguments[])
	{
		if (_uArgumentsCount == 0)
			return;

		const char* sArgumentValue{ nullptr };
		const char* sArgumentName{ nullptr };

		const auto oVisitor = ValueVisitorType{
			[&sArgumentValue](const char** _pValue) { *_pValue = sArgumentValue; },
			[&sArgumentName , &sArgumentValue](unsigned int* _pValue) {
				if (sscanf_s(sArgumentValue, "%d", _pValue) != 1)
					throw std::runtime_error(std::format("Unexpected value type for '{}'", sArgumentName));},
			[&sArgumentName , &sArgumentValue](float* _pValue) {
				if (sscanf_s(sArgumentValue, "%f", _pValue) != 1)
					throw std::runtime_error(std::format("Unexpected value type for '{}'", sArgumentName));},
			[&sArgumentName , &sArgumentValue](ColorRGB* _pValue) {
				if (sscanf_s(sArgumentValue, "%hhd, %hhd, %hhd", &_pValue->m_uR, &_pValue->m_uG, &_pValue->m_uB) != 3)
					throw std::runtime_error(std::format("Unexpected value type for '{}'", sArgumentName));},
		};

		for (unsigned int uArgumentIndex = 0; uArgumentIndex < _uArgumentsCount; uArgumentIndex += 2)
		{
			const char sArgumentPrefix = '-';
			const char* sArgument = _sArguments[uArgumentIndex];
			const size_t uArgumentLength = std::strlen(sArgument);
			if (uArgumentLength > 1 && sArgument[0] == sArgumentPrefix)
			{
				sArgumentName = &sArgument[1];
				const auto itArgumentID = m_oArgumentNamesToIDs.find(sArgumentName);
				if (itArgumentID == m_oArgumentNamesToIDs.cend())
					throw std::runtime_error(std::format("Unknown parameter '{}'", sArgumentName));
				else if(uArgumentIndex >= _uArgumentsCount - 1)
					throw std::runtime_error(std::format("Missing value for '{}'", sArgumentName));
				else
				{
					const auto itArgument = m_oArgumentDefinitions.find(itArgumentID->second);
					assert(itArgument != m_oArgumentDefinitions.cend());

					sArgumentValue = _sArguments[uArgumentIndex + 1];
					const Argument& oArgument = itArgument->second;
					std::visit(oVisitor, oArgument.m_oValue);
				}
			}
		}

		Validate();
	}

	void PrintArgumentDescriptions() const
	{
		for (ArgumentID eArguemntID = static_cast<ArgumentID>(0); eArguemntID < ARGUMENT_ID_COUNT; eArguemntID = static_cast<ArgumentID>(eArguemntID + 1))
		{
			const auto itParameter = m_oArgumentDefinitions.find(eArguemntID);
			assert(itParameter != m_oArgumentDefinitions.cend());
			const auto itParameterName = std::find_if(m_oArgumentNamesToIDs.cbegin(), m_oArgumentNamesToIDs.cend(),
				[eArguemntID](const auto& _oEntry) {return _oEntry.second == eArguemntID;});
			assert(itParameter != m_oArgumentDefinitions.cend());
			itParameter->second.PrintDescription(itParameterName->first);
		}
	}

private:
	template<class... Ts>
	struct ValueVisitorType : Ts... { using Ts::operator()...; };

	struct Argument
	{
		std::variant<
			const char**,
			unsigned int*,
			float*,
			ColorRGB*> m_oValue;
		const char* m_sArgumentDescription;

		void PrintDescription(const char* _sArgumentName) const
		{
			std::string sDefaultValue = "";
			const auto oVisitor = ValueVisitorType{
				[&sDefaultValue](const char** _pValue) { sDefaultValue = ""; },
				[&sDefaultValue](unsigned int* _pValue) { sDefaultValue = std::format("Default: {}", *_pValue); },
				[&sDefaultValue](float* _pValue) { sDefaultValue = std::format("Default: {}", *_pValue); },
				[&sDefaultValue](ColorRGB* _pValue) { sDefaultValue = std::format("Default: ({}, {}, {})", _pValue->m_uR, _pValue->m_uG, _pValue->m_uB); },
			};
			std::visit(oVisitor, m_oValue);
			std::cout << std::format("  -{}\t{}. {}\n", _sArgumentName, m_sArgumentDescription, sDefaultValue);
		}
	};

	enum ArgumentID
	{
		InputMask,
		InputNormal,
		InputAlbedo,
		InputAmbientOcclusion,
		NormalVerticalDivisions,
		NormalHorizontalDivisions,
		AlbedoMaxColors,
		LightLogShadesCount,
		LightFrontColor,
		LightBackColor,
		LightDarknessColor,
		LightSpecularIntesity,
		LightSpecularHardness,
		LightSpecularLogShadesCount,
		OutputEncodedImage,
		OutputMaterial,
		ARGUMENT_ID_COUNT
	};

	struct CompareString
	{
		bool operator()(char const* _sLeftHandSide, char const* _sRightHandSide) const
		{
			return std::strcmp(_sLeftHandSide, _sRightHandSide) < 0;
		}
	};

	const std::map<const char*, ArgumentID, CompareString> m_oArgumentNamesToIDs = {
		{"im", InputMask},
		{"in", InputNormal},
		{"ia", InputAlbedo},
		{"io", InputAmbientOcclusion},
		{"nvs", NormalVerticalDivisions},
		{"nhs", NormalHorizontalDivisions},
		{"amc", AlbedoMaxColors},
		{"lsc", LightLogShadesCount},
		{"lfc", LightFrontColor},
		{"lbc", LightBackColor},
		{"ldc", LightDarknessColor},
		{"lsi", LightSpecularIntesity},
		{"lsh", LightSpecularHardness},
		{"lssc", LightSpecularLogShadesCount},
		{"ot", OutputEncodedImage},
		{"om", OutputMaterial}
	};

	const std::map<ArgumentID, Argument> m_oArgumentDefinitions =
	{
		{InputMask,{
			.m_oValue = &m_sMaskFilename,
			.m_sArgumentDescription = "Input mask file (.BMP)"}},
		{InputNormal,{
			.m_oValue = &m_sNormalFilename,
			.m_sArgumentDescription = "Input normal map file (.BMP)"}},
		{InputAlbedo,{
			.m_oValue = &m_sAlbedoFilename,
			.m_sArgumentDescription = "Input albedo file (.BMP)"}},
		{InputAmbientOcclusion,{
			.m_oValue = &m_sAmbientOcclussionFilename,
			.m_sArgumentDescription = "Input ambient occlusion file .BMP)"}},
		{NormalVerticalDivisions,{
			.m_oValue = &m_uVerticalNormalMapSides,
			.m_sArgumentDescription = "Number of sides in the Z axis for the normal pallete dome. Must be at least 2"}},
		{NormalHorizontalDivisions,{
			.m_oValue = &m_uHorizontalNormalMapSides,
			.m_sArgumentDescription = "Number of sides in the XY plane for the normal pallete dome. Must be at least 2"}},
		{AlbedoMaxColors,{
			.m_oValue = &m_uMaxAlbedoColors,
			.m_sArgumentDescription = "Maximum number of albedo colors (quantize if needed)"}},
		{LightLogShadesCount,{
			.m_oValue = &m_uLogLightShadesCount,
			.m_sArgumentDescription = "Log base 2 of the shades count"}},
		{LightFrontColor,{
			.m_oValue = &m_oFrontLightColor,
			.m_sArgumentDescription = "Color of the front light"}},
		{LightBackColor,{
			.m_oValue = &m_oBackLightColor,
			.m_sArgumentDescription = "Color of the back light"}},
		{LightDarknessColor,{
			.m_oValue = &m_oPureDarknessColor,
			.m_sArgumentDescription = "Color of the back light"}},
		{LightSpecularIntesity,{
			.m_oValue = &m_fSpecularIntensity,
			.m_sArgumentDescription = "Specular component intensity (in the [0, 1] range). Use 0 for no specular component"}},
		{LightSpecularHardness,{
			.m_oValue = &m_fSpecularHardness,
			.m_sArgumentDescription = "Specular component light strength power exponent. Higher values produce more compact and defined highlights"}},
		{LightSpecularLogShadesCount,{
			.m_oValue = &m_uSpecularLogShadesCount,
			.m_sArgumentDescription = "Log base 2 of the shades count of specular shades"}},
		{OutputEncodedImage,{
			.m_oValue = &m_sBaseOutputFilename,
			.m_sArgumentDescription = "Output enconded image filename (index will be appended if multiple output) (.BMP)"}},
		{OutputMaterial,{
			.m_oValue = &m_sOutputMaterialsFilename,
			.m_sArgumentDescription = "Output material filename (.MAT)"}},
	};

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