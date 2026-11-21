module;
#include <windows.h>

export module ScreenUtils;

import std;

export struct ScreenUtils
{
	static DISPLAYCONFIG_PATH_TARGET_INFO GetTargetInfoFoRect(const RECT& _oRect)
	{
		const auto [oPaths, oModes] = GetActiveDisplayInfo();

		size_t uPathIndex = 0;
        for (; uPathIndex < oPaths.size(); ++uPathIndex)
        {

        }
        return {};
	}

private:
    static std::tuple<std::vector< DISPLAYCONFIG_PATH_INFO>, std::vector< DISPLAYCONFIG_MODE_INFO>> GetActiveDisplayInfo()
    {
        std::vector<DISPLAYCONFIG_PATH_INFO> oPaths;
        std::vector<DISPLAYCONFIG_MODE_INFO> oModes;
        const UINT32 uFlags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;
        LONG uResult = ERROR_SUCCESS;

        do
        {
            // Determine how many path and mode structures to allocate
            UINT32 uPathCount, uModeCount;
            uResult = GetDisplayConfigBufferSizes(uFlags, &uPathCount, &uModeCount);

            /*if (uResult != ERROR_SUCCESS)
            {
                return HRESULT_FROM_WIN32(uResult);
            }*/

            // Allocate the path and mode arrays
            oPaths.resize(uPathCount);
            oModes.resize(uModeCount);

            // Get all active paths and their modes
            uResult = QueryDisplayConfig(uFlags, &uPathCount, oPaths.data(), &uModeCount, oModes.data(), nullptr);

            // The function may have returned fewer paths/modes than estimated
            oPaths.resize(uPathCount);
            oModes.resize(uModeCount);

            // It's possible that between the call to GetDisplayConfigBufferSizes and QueryDisplayConfig
            // that the display state changed, so loop on the case of ERROR_INSUFFICIENT_BUFFER.
        } while (uResult == ERROR_INSUFFICIENT_BUFFER);

        /*if (uResult != ERROR_SUCCESS)
        {
            return HRESULT_FROM_WIN32(uResult);
        }*/

        return { oPaths , oModes };
    }
};