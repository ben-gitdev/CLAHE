
#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include<omp.h>
#include "pch.h"
#include "CLAHE.h"
#include <cstdlib>

void CLAHE(__int32 nRow, __int32 nCol, __int32 ROI[4], __int32 blockSize, unsigned __int16* imgIn, unsigned __int8* imgOut, __int32 clipLimit)
{
	__int32 i;
	// create pointers to first row elements, then pixels can be accessed like ImgInPtr[i][j]
	unsigned __int16** imgInPtr = (unsigned __int16**)malloc(sizeof(unsigned __int16*) * nRow);
	unsigned __int8** imgOutPtr = (unsigned __int8**)malloc(sizeof(unsigned __int8*) * nRow);
	__int32 blockPixNum = (blockSize * 2 + 1) * (blockSize * 2 + 1);



	for (i = 0; i < nRow; i++)
	{
		imgInPtr[i] = imgIn + nCol * i;
		imgOutPtr[i] = imgOut + nCol * i;
	}
	if (clipLimit < 1)
	{
		clipLimit = 1;
	}
	if (clipLimit > 1024)
	{
		clipLimit = 1024;
	}
	clipLimit = (clipLimit * blockPixNum) >> 10;
	__int32 totalExcess = 0;
	__int32* totalExcessCol = (__int32*)calloc(ROI[2], sizeof(__int32));
	__int32 firstExcess = 0;
	__int32* firstExcessCol = (__int32*)calloc(ROI[2], sizeof(__int32));
	__int32 firstExcessLastPixInCurBlk = 0;


	__int32* histRow;// histogram of the block for addding/subtracting one row of pixel, i.e. column advancement
	unsigned __int8* LUT;//look up table to determin equalized pixel intensity, table size = pixel num of block. new intensity = LUT[rank of pixel]
	histRow = (__int32*)calloc((ROI[2] << 12), sizeof(__int32));
	unsigned __int32 histCol[(1 << 12)] = { 0 };// histogram of the block for adding/subtracting one col of pixel, i.e. row advancement

	LUT = (unsigned __int8*)calloc(blockPixNum, sizeof(unsigned __int8));
	// create the look up table	
	if (LUT != NULL)
	{
		for (i = 0; i < blockPixNum; i++)
		{
			LUT[i] = ((i << 16) / blockPixNum) >> 8;
			//printf("LUT: %d \n", *(LUT + i));
		}
	};

	/*
		for (m = 0; m < 4096; m++)
		{
			if(histRow[m]>0)
			{
				printf("m: %d ", m);
				printf("histRow: %d ", histRow[m]);
			}

		}
	*/


	__int32 rank = 0;
	__int32* rankCol = (__int32*)calloc(ROI[2], sizeof(__int32));
	//__int32 rankLastPixInCurBlkRow = 0;
	__int32 rankLastPixInCurBlk = 0;
	//	unsigned __int16 k;

		// start doing adaptive histogram equalization for the entire image
	__int32 j; // index numbers of the entire image
	__int32 indColAdd;
	__int32 indColSubtract;
	__int32 indRowAdd;
	__int32 indRowSubtract;


	// do the adaptive histogram equalization
// calculate histogram for the first pixel
	__int32 startI;
	__int32 endI;
	__int32 startJ;
	__int32 endJ;
	if (ROI[0] < blockSize)
	{
		startI = 0;
	}
	else
	{
		startI = ROI[0] - blockSize;
	}
	if (ROI[0] + blockSize > nRow - 1)
	{
		endI = nRow - 1;
	}
	else
	{
		endI = ROI[0] + blockSize;
	}
	if (ROI[1] < blockSize)
	{
		startJ = 0;
	}
	else
	{
		startJ = ROI[1] - blockSize;
	}
	if (ROI[1] + blockSize > nCol - 1)
	{
		endJ = nCol - 1;
	}
	else
	{
		endJ = ROI[1] + blockSize;
	}

	// calculate histogram and rank for the pixels of the first column

	__int32 m = 0;
	__int32 n = 0; // indexing numbers of pixels in a block;
	unsigned __int16 pixI; // center pixel value
	unsigned __int16 pixILast;
	unsigned __int16 pixI0;
	// histogram, rank, totalExcess and firstExcess of the first pixel
	pixI0 = (imgInPtr[startI][startJ]) >> 4;
	for (m = startI; m <= endI; m++)
	{
		for (n = startJ; n <= endJ; n++)
		{
			pixI = (imgInPtr[m][n]) >> 4;
			histRow[pixI]++;
			if (histRow[pixI] > clipLimit)
			{
				totalExcessCol[0]++;
				if (pixI < pixI0)
				{
					firstExcessCol[0]++;
				}
			}
			if (pixI < pixI0)
			{
				rankCol[0]++;
			}
		}
	}
	//rank for the first pixel
/*	pixI = pixI0;
	for (i = 0; i < pixI; i++)
	{
		rankCol[0] += histRow[i];
	}
*/
	pixILast = pixI0;
	rankLastPixInCurBlk = rankCol[0];
	firstExcessLastPixInCurBlk = firstExcessCol[0];
	for (i = 1; i < ROI[2]; i++)
	{
		//printf("i: %d \n", i);
		memcpy(histRow + (i << 12), histRow + ((i - 1) << 12), (sizeof(unsigned __int32)) << 12);
		totalExcess = totalExcessCol[i - 1];
		indRowSubtract = i - blockSize - 1;
		indRowAdd = i + blockSize;
		if (indRowSubtract >= 0)
		{
			for (n = startJ; n <= endJ; n++)
			{
				pixI = (imgInPtr[indRowSubtract][n]) >> 4;
				histRow[pixI + (i << 12)]--;
				if (histRow[pixI + (i << 12)] >= clipLimit)
				{
					totalExcess--;
					if (pixI < pixILast)
					{
						firstExcessLastPixInCurBlk--;
					}
				}
				if (pixI < pixILast)
				{
					rankLastPixInCurBlk--;
				}
			}
		}
		if (indRowAdd < nRow && i>ROI[0])
		{
			for (n = startJ; n <= endJ; n++)
			{
				pixI = (imgInPtr[indRowAdd][n]) >> 4;
				histRow[pixI + (i << 12)]++;
				if (histRow[pixI + (i << 12)] > clipLimit)
				{
					totalExcess++;
					if (pixI < pixILast)
					{
						firstExcessLastPixInCurBlk++;
					}
				}
				if (pixI < pixILast)
				{
					rankLastPixInCurBlk++;
				}
			}
		}
		pixI = (imgInPtr[i][startJ]) >> 4;
		totalExcessCol[i] = totalExcess;
		firstExcessCol[i] = 0;
		//calculate rank,  and firstExcess from rank,  and firstExcess of last pixel in current block
		if (pixILast < pixI)
		{
			for (j = pixILast; j < pixI; j++)
			{
				rankCol[i] += histRow[(i << 12) + j];
				if (histRow[(i << 12) + j] > clipLimit)
				{
					firstExcessCol[i] = firstExcessCol[i] + histRow[(i << 12) + j] - clipLimit;
				}
			}
			rankCol[i] += rankLastPixInCurBlk;
			firstExcessCol[i] = firstExcessCol[i] + firstExcessLastPixInCurBlk;
		}
		else if (pixILast > pixI)
		{
			for (j = pixI; j < pixILast; j++)
			{
				rankCol[i] += histRow[(i << 12) + j];
				if (histRow[(i << 12) + j] > clipLimit)
				{
					firstExcessCol[i] = firstExcessCol[i] + histRow[(i << 12) + j] - clipLimit;
				}
			}
			rankCol[i] = rankLastPixInCurBlk - rankCol[i];
			firstExcessCol[i] = firstExcessLastPixInCurBlk - firstExcessCol[i];
		}
		else
		{
			rankCol[i] = rankLastPixInCurBlk;
			firstExcessCol[i] = firstExcessLastPixInCurBlk;
		}
		pixILast = pixI;
		rankLastPixInCurBlk = rankCol[i];
		firstExcessLastPixInCurBlk = firstExcessCol[i];
	}

	// ///////////////////main loop/////////////////

#pragma omp parallel shared(ROI,imgInPtr,imgOutPtr,histRow,imgIn,imgOut,rankCol,totalExcessCol,firstExcessCol) private(i,j,m,n,indColSubtract,indColAdd,startI,endI,pixI,histCol,rank,pixILast,rankLastPixInCurBlk,firstExcessLastPixInCurBlk,totalExcess,firstExcess)
	{
#pragma omp for
		for (i = ROI[0]; i < ROI[0] + ROI[2]; i++)
		{
			//printf("i: %d \n", i);
			// row index range of the column to be added/subtracted
			if (i < blockSize)
			{
				startI = 0;
			}
			else
			{
				startI = i - blockSize;
			}
			if (i + blockSize > nRow - 1)
			{
				endI = nRow - 1;
			}
			else
			{
				endI = i + blockSize;
			}
			// core inner loop for doing AHE
			for (j = ROI[1]; j < ROI[1] + ROI[3]; j++)
			{
				if (j == ROI[1])
				{
					memcpy(histCol, histRow + ((i - ROI[0]) << 12), (sizeof(unsigned __int32) << 12));
					rankLastPixInCurBlk = rankCol[i];
					totalExcess = totalExcessCol[i];
					firstExcess = firstExcessCol[i];
					firstExcessLastPixInCurBlk = firstExcessCol[i];
					pixILast = (imgInPtr[i][j]) >> 4;
				}
				indColAdd = j + blockSize;
				indColSubtract = j - blockSize - 1;
				if (indColAdd < nCol && j>ROI[1])
				{
					for (m = startI; m <= endI; m++)
					{
						pixI = (imgInPtr[m][indColAdd]) >> 4;
						histCol[pixI]++;
						if (histCol[pixI] > clipLimit)
						{
							totalExcess++;
							if (pixI < pixILast)
							{
								firstExcessLastPixInCurBlk++;
							}
						}
						if (pixI < pixILast)
						{
							rankLastPixInCurBlk++;
						}
					}
				}
				if (indColSubtract >= 0)
				{
					for (m = startI; m <= endI; m++)
					{
						pixI = (imgInPtr[m][indColSubtract]) >> 4;
						histCol[pixI]--;
						if (histCol[pixI] >= clipLimit)
						{
							totalExcess--;
							if (pixI < pixILast)
							{
								firstExcessLastPixInCurBlk--;
							}
						}
						if (pixI < pixILast)
						{
							rankLastPixInCurBlk--;
						}
					}
				}
				//calculate rank from rank of last pixel in current block
				pixI = (imgInPtr[i][j]) >> 4;
				rank = 0;
				firstExcess = 0;
				if (pixILast < pixI)
				{
					for (n = pixILast; n < pixI; n++)
					{
						rank += histCol[n];
						if (histCol[n] > clipLimit)
						{
							firstExcess = firstExcess + histCol[n] - clipLimit;
						}
					}
					rank += rankLastPixInCurBlk;
					firstExcess = firstExcess + firstExcessLastPixInCurBlk;
				}
				else if (pixILast > pixI)
				{
					for (n = pixI; n < pixILast; n++)
					{
						rank += histCol[n];
						if (histCol[n] > clipLimit)
						{
							firstExcess = firstExcess + histCol[n] - clipLimit;
						}
					}
					rank = rankLastPixInCurBlk - rank;
					firstExcess = firstExcessLastPixInCurBlk - firstExcess;

				}
				else
				{
					rank = rankLastPixInCurBlk;
					firstExcess = firstExcessLastPixInCurBlk;
				}
				pixILast = pixI;
				rankLastPixInCurBlk = rank;
				firstExcessLastPixInCurBlk = firstExcess;
				////////////////////////////////////////////			
				/*			printf("\n");
							for (m = 0; m < 4096; m++)
							{
								if (histCol[m] > 0)
								{
									printf("m: %d ", m);
									printf("histCol: %d \n", histCol[m]);
								}

							}
				*/

				// done calculating histogram of the block, now calculate rank
				// redistribute totalexcess and calculate new rank
				rank = rank + ((totalExcess * pixI) >> 12) - firstExcess;
				imgOutPtr[i][j] = LUT[rank];
			}
		}
	}


	free(rankCol);
	free(LUT);
	//	free(histCol);
	free(histRow);
	free(firstExcessCol);
	free(totalExcessCol);
	free(imgOutPtr);
	free(imgInPtr);
}



