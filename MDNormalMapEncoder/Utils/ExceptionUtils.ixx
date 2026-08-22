module;
#include <string.h>
export module ExceptionUtils;

import std;

export struct ExceptionUtils
{
	static void ThrowOpenFileFailureException()
	{
		char sOpenError[1024];
		strerror_s(sOpenError, sizeof(sOpenError), errno);
		throw std::runtime_error(sOpenError);
	}
};