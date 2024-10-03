#pragma once
#ifdef CLAHE_EXPORTS
#define CLAHE_API __declspec(dllexport)
#else
#define CLAHE_API __declspec(dllimport)
#endif


extern "C" CLAHE_API void CLAHE(__int32 nRow, __int32 nCol, __int32 ROI[4], __int32 blockSize, unsigned __int16* imgIn, unsigned __int8* imgOut, __int32 clipLimit);