#pragma once

#define DETAILED_LOCAL_WARNING_FORMATTED(_sMessage, ...) DebugUtils::ErrorMessage("WARNING!!! " __FUNCTION__ ":" _sMessage "\n", __VA_ARGS__)
#define DETAILED_LOCAL_ERROR_FORMATTED(_sMessage, ...) DebugUtils::ErrorMessage("ERROR!!! " __FUNCTION__ ":" _sMessage "\n", __VA_ARGS__)
#define DETAILED_LOCAL_ERROR(_sMessage) DebugUtils::ErrorMessage("ERROR!!! " __FUNCTION__ ":" _sMessage "\n")
