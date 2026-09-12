module;

#include <stdio.h>
#include <cassert>

export module CommandLineArguments;

import EncoderArguments;
import std;

export struct CommandLineArguments
{
	EncoderArguments m_oEncoderArguments;

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
			.m_oValue = &m_oEncoderArguments.m_sMaskFilename,
			.m_sArgumentDescription = "Input mask file (.BMP)"}},
		{InputNormal,{
			.m_oValue = &m_oEncoderArguments.m_sNormalFilename,
			.m_sArgumentDescription = "Input normal map file (.BMP)"}},
		{InputAlbedo,{
			.m_oValue = &m_oEncoderArguments.m_sAlbedoFilename,
			.m_sArgumentDescription = "Input albedo file (.BMP)"}},
		{InputAmbientOcclusion,{
			.m_oValue = &m_oEncoderArguments.m_sAmbinetOcclusionFilename,
			.m_sArgumentDescription = "Input ambient occlusion file .BMP)"}},
		{NormalVerticalDivisions,{
			.m_oValue = &m_oEncoderArguments.m_uVerticalNormalMapSides,
			.m_sArgumentDescription = "Number of sides in the Z axis for the normal pallete dome. Must be at least 2"}},
		{NormalHorizontalDivisions,{
			.m_oValue = &m_oEncoderArguments.m_uHorizontalNormalMapSides,
			.m_sArgumentDescription = "Number of sides in the XY plane for the normal pallete dome. Must be at least 2"}},
		{AlbedoMaxColors,{
			.m_oValue = &m_oEncoderArguments.m_uMaxAlbedoColors,
			.m_sArgumentDescription = "Maximum number of albedo colors (quantize if needed)"}},
		{LightLogShadesCount,{
			.m_oValue = &m_oEncoderArguments.m_uLogLightShadesCount,
			.m_sArgumentDescription = "Log base 2 of the shades count"}},
		{LightFrontColor,{
			.m_oValue = &m_oEncoderArguments.m_oFrontLightColor,
			.m_sArgumentDescription = "Color of the front light"}},
		{LightBackColor,{
			.m_oValue = &m_oEncoderArguments.m_oBackLightColor,
			.m_sArgumentDescription = "Color of the back light"}},
		{LightDarknessColor,{
			.m_oValue = &m_oEncoderArguments.m_oPureDarknessColor,
			.m_sArgumentDescription = "Color of the back light"}},
		{LightSpecularIntesity,{
			.m_oValue = &m_oEncoderArguments.m_fSpecularIntensity,
			.m_sArgumentDescription = "Specular component intensity (in the [0, 1] range). Use 0 for no specular component"}},
		{LightSpecularHardness,{
			.m_oValue = &m_oEncoderArguments.m_fSpecularHardness,
			.m_sArgumentDescription = "Specular component light strength power exponent. Higher values produce more compact and defined highlights"}},
		{LightSpecularLogShadesCount,{
			.m_oValue = &m_oEncoderArguments.m_uSpecularLogShadesCount,
			.m_sArgumentDescription = "Log base 2 of the shades count of specular shades"}},
		{OutputEncodedImage,{
			.m_oValue = &m_oEncoderArguments.m_sBaseOutputFilename,
			.m_sArgumentDescription = "Output enconded image filename (index will be appended if multiple output) (.BMP)"}},
		{OutputMaterial,{
			.m_oValue = &m_oEncoderArguments.m_sOutputMaterialsFilename,
			.m_sArgumentDescription = "Output material filename (.MAT)"}},
	};
};