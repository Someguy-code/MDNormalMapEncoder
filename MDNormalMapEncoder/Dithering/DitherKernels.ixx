export module DitherImage:Kernels;

import RawImage;
import std;

static const std::map<const char*, RawImage<float> > oDitherKernels =
{
	{"None", {1, 1, {1.f}}},
	{"FloydSteinberg", {3, 3, {
		0.f,      0.f,      0.f,
		0.f,      0.f,      7.f / 16.f,
		3.f / 16.f, 5.f / 16.f, 1.f / 16.f
	} } }
};

export const RawImage<float>& GetDitherKernel(const char* _sDitherKernelName)
{
	return oDitherKernels.at(_sDitherKernelName);
}