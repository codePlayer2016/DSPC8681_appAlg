/* ======================================================================= */
/* DSPF_sp_ifftSPxSP.c -- Complex Inverse FFT with Mixed Radix             */
/*                Driver code; tests kernel and reports result in stdout   */
/*                                                                         */
/* Rev 0.0.2                                                               */
/*                                                                         */
/* Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/  */ 
/*                                                                         */
/*                                                                         */
/*  Redistribution and use in source and binary forms, with or without     */
/*  modification, are permitted provided that the following conditions     */
/*  are met:                                                               */
/*                                                                         */
/*    Redistributions of source code must retain the above copyright       */
/*    notice, this list of conditions and the following disclaimer.        */
/*                                                                         */
/*    Redistributions in binary form must reproduce the above copyright    */
/*    notice, this list of conditions and the following disclaimer in the  */
/*    documentation and/or other materials provided with the               */
/*    distribution.                                                        */
/*                                                                         */
/*    Neither the name of Texas Instruments Incorporated nor the names of  */
/*    its contributors may be used to endorse or promote products derived  */
/*    from this software without specific prior written permission.        */
/*                                                                         */
/*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS    */
/*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      */
/*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  */
/*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT   */
/*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,  */
/*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT       */
/*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,  */
/*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY  */
/*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT    */
/*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  */
/*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.   */
/*                                                                         */
/* ======================================================================= */

#include <stdio.h>
#include <stdlib.h>

#include "DSPF_sp_ifftSPxSP_cn.h"

#include "cv.h"
#include "cxcore.h"

#define eps 1e-8

void tw_gen_i (float *w, int n);

/* ======================================================================== */
/*  MAIN -- Top level driver for the test.                                  */
/* ======================================================================== */
int myIFFT2 (CvMat *xin, CvMat *yout, int NbFeatures)
{
	int i, j, rad0, rad1, N, k;
	unsigned char  * brev = NULL;
	int row = xin->rows;
	int col = xin->cols / NbFeatures - 2;

	N = col;
	j = 0;
	for (i = 0; i <= 31; i++)
		if ((N & (1 << i)) == 0)
			j++;
		else
			break;

	if (j % 2 == 0)
		rad0 = 4;
	else
		rad0 = 2;

	float * ptr_w0 = (float *)malloc(col * 2 * sizeof(float));
	if(ptr_w0 == NULL)
		return 0;

	tw_gen_i (ptr_w0, col);

	N = row;
	j = 0;
	for (i = 0; i <= 31; i++)
		if ((N & (1 << i)) == 0)
			j++;
		else
			break;

	if (j % 2 == 0)
		rad1 = 4;
	else
		rad1 = 2;

	float * ptr_w1 = (float *)malloc(row * 2 * sizeof(float));
	if(ptr_w1 == NULL)
		return 0;

	tw_gen_i (ptr_w1, row);

	int NMax = row > col ? row : col;

	int size = NMax * 4 * sizeof(float);
	float * base = (float *)malloc(size);
	if(base == NULL)
		return 0;

	int IFFTsize = row * col * sizeof(float);
	float * IFFTreal = (float *)malloc(IFFTsize);
	float * IFFTimag = (float *)malloc(IFFTsize);
	float * colreal  = (float *)malloc(IFFTsize);
	float * colimag  = (float *)malloc(IFFTsize);
	if(IFFTreal == NULL || IFFTimag == NULL || colreal == NULL || colimag == NULL)
		return 0;

	float * ptr_x = base;
	float * ptr_y = base + NMax * 2;

	//原数组中包含的复数个数
	int Cnum = (col + 2) >> 1;

	for (int k = 0; k < NbFeatures; k++)
	{
		//提取出部分连续的二维IFFT数组
		for (int i = 0; i < row; i++)
		{
			int num = col * i;
			float * pRow = (float *)(xin->data.ptr + xin->step * i);
			for (int j = 0; j < Cnum; j++)
			{
				IFFTreal[num + j] = pRow[NbFeatures * 2 * j + k * 2];
				IFFTimag[num + j] = pRow[NbFeatures * 2 * j + 1 + k * 2];
			}
		}

		//根据对称性补全整个IFFT数组
		for (int i = 0; i < row; i++)
		{
			int num = col * i;			
			int m = (row - i) % row;
			int num2 = col * m;
			for (int j = Cnum; j < col; j++)
			{				
				int n = (col - j) % col;
				IFFTreal[num + j] =  IFFTreal[num2 + n];
				IFFTimag[num + j] = -IFFTimag[num2 + n];
			}
		}

		i = 0;
		//对每一行做一维的IFFT变换
		for (int i = 0; i < row; i++)
		{
			int rowoff = col * i;
			for (int j = 0; j < col; j++)
			{
				ptr_x[2 * j]     = IFFTreal[rowoff + j];
				ptr_x[2 * j + 1] = IFFTimag[rowoff + j]; 
			}

			DSPF_sp_ifftSPxSP_cn (col, ptr_x, ptr_w0, ptr_y, brev, rad0, 0, col);

			for (int j = 0; j < col; j++)
			{
				colreal[rowoff + j] = ptr_y[2 * j];
				colimag[rowoff + j] = ptr_y[2 * j + 1];
			}
		}

		//对每一列做一维的IFFT变换
		for (int i = 0; i < col; i++)
		{
			for (int j = 0; j < row; j++)
			{
				int rowoff = col * j;
				ptr_x[2 * j]     = colreal[rowoff + i];
				ptr_x[2 * j + 1] = colimag[rowoff + i];
			}

			DSPF_sp_ifftSPxSP_cn (row, ptr_x, ptr_w1, ptr_y, brev, rad1, 0, row);

			for (int j = 0; j < row; j++)
			{
				//fftw的IFFT变换没有归一化，反变换的结果有一个跟元素总个数相关的倍数。
				//这里给变换结果乘上（row * col），是为了和fftw的结果保持一致。
				float real = ptr_y[2 * j] * row * col;
				float imag = ptr_y[2 * j + 1] * row * col;
				real = abs(real) < eps ? 0 : real;
				imag = abs(imag) < eps ? 0 : imag;
				int rowoff = col * j;
				colreal[rowoff + i] = real;
				colimag[rowoff + i] = imag;
			}
		}

		i = 0;
		//给最终输出赋值
		for (int i = 0; i < row; i++)
		{
			int num = col * i;
			float * pRow = (float *)(yout->data.ptr + yout->step * i);
			for (int j = 0; j < col; j++)
			{
				pRow[NbFeatures * j + k] = colreal[num + j];
			}
		}
		i = 0;
	}

	if(ptr_w0 != NULL)
		free(ptr_w0);
	if(ptr_w1 != NULL)
		free(ptr_w1);
	if(base != NULL)
		free(base);
	if(IFFTreal != NULL)
		free(IFFTreal);
	if(IFFTimag != NULL)
		free(IFFTimag);
	if(colreal != NULL)
		free(colreal);
	if(colimag != NULL)
		free(colimag);

	return 1;
}

int myIFFT2 (float * inreal, float * inimag, float * outreal, float * outimag, int row, int col)
{
	int i, j, rad0, rad1, N, k;
	unsigned char  * brev = NULL;

	N = col;
	j = 0;
	for (i = 0; i <= 31; i++)
		if ((N & (1 << i)) == 0)
			j++;
		else
			break;

	if (j % 2 == 0)
		rad0 = 4;
	else
		rad0 = 2;

	float * ptr_w0 = (float *)malloc(col * 2 * sizeof(float));
	if(ptr_w0 == NULL)
		return 0;

	tw_gen_i (ptr_w0, col);

	N = row;
	j = 0;
	for (i = 0; i <= 31; i++)
		if ((N & (1 << i)) == 0)
			j++;
		else
			break;

	if (j % 2 == 0)
		rad1 = 4;
	else
		rad1 = 2;

	float * ptr_w1 = (float *)malloc(row * 2 * sizeof(float));
	if(ptr_w1 == NULL)
		return 0;

	tw_gen_i (ptr_w1, row);

	int NMax = row > col ? row : col;

	int size = NMax * 4 * sizeof(float);
	float * base = (float *)malloc(size);
	if(base == NULL)
		return 0;

	int IFFTsize = row * col * sizeof(float);
	float * IFFTreal = (float *)malloc(IFFTsize);
	float * IFFTimag = (float *)malloc(IFFTsize);
	float * colreal  = (float *)malloc(IFFTsize);
	float * colimag  = (float *)malloc(IFFTsize);
	if(IFFTreal == NULL || IFFTimag == NULL || colreal == NULL || colimag == NULL)
		return 0;

	float * ptr_x = base;
	float * ptr_y = base + NMax * 2;

	//对每一行做一维的IFFT变换
	for (int i = 0; i < row; i++)
	{
		int rowoff = col * i;
		for (int j = 0; j < col; j++)
		{
			ptr_x[2 * j]     = inreal[rowoff + j];
			ptr_x[2 * j + 1] = inimag[rowoff + j]; 
		}

		DSPF_sp_ifftSPxSP_cn (col, ptr_x, ptr_w0, ptr_y, brev, rad0, 0, col);

		for (int j = 0; j < col; j++)
		{
			colreal[rowoff + j] = ptr_y[2 * j];
			colimag[rowoff + j] = ptr_y[2 * j + 1];
		}
	}

	//对每一列做一维的IFFT变换
	for (int i = 0; i < col; i++)
	{
		for (int j = 0; j < row; j++)
		{
			int rowoff = col * j;
			ptr_x[2 * j]     = colreal[rowoff + i];
			ptr_x[2 * j + 1] = colimag[rowoff + i];
		}

		DSPF_sp_ifftSPxSP_cn (row, ptr_x, ptr_w1, ptr_y, brev, rad1, 0, row);

		for (int j = 0; j < row; j++)
		{
			int rowoff = col * j;
			colreal[rowoff + i] = ptr_y[2 * j];
			colimag[rowoff + i] = ptr_y[2 * j + 1];
		}
	}

	i = 0;
	

	if(ptr_w0 != NULL)
		free(ptr_w0);
	if(ptr_w1 != NULL)
		free(ptr_w1);
	if(base != NULL)
		free(base);
	if(IFFTreal != NULL)
		free(IFFTreal);
	if(IFFTimag != NULL)
		free(IFFTimag);
	if(colreal != NULL)
		free(colreal);
	if(colimag != NULL)
		free(colimag);

	return 1;
}

int myIFFT2 (CvMat *xin, CvMat *yout, int NbFeatures, float * inreal, float * inimag, float * outreal, float * outimag, int row, int col)
{
	int i, j, rad0, rad1, N, k;
	unsigned char  * brev = NULL;

	N = col;
	j = 0;
	for (i = 0; i <= 31; i++)
		if ((N & (1 << i)) == 0)
			j++;
		else
			break;

	if (j % 2 == 0)
		rad0 = 4;
	else
		rad0 = 2;

	float * ptr_w0 = (float *)malloc(col * 2 * sizeof(float));
	if(ptr_w0 == NULL)
		return 0;

	tw_gen_i (ptr_w0, col);

	N = row;
	j = 0;
	for (i = 0; i <= 31; i++)
		if ((N & (1 << i)) == 0)
			j++;
		else
			break;

	if (j % 2 == 0)
		rad1 = 4;
	else
		rad1 = 2;

	float * ptr_w1 = (float *)malloc(row * 2 * sizeof(float));
	if(ptr_w1 == NULL)
		return 0;

	tw_gen_i (ptr_w1, row);

	int NMax = row > col ? row : col;

	int size = NMax * 4 * sizeof(float);
	float * base = (float *)malloc(size);
	if(base == NULL)
		return 0;

	int IFFTsize = row * col * sizeof(float);
	float * IFFTreal = (float *)malloc(IFFTsize);
	float * IFFTimag = (float *)malloc(IFFTsize);
	float * colreal  = (float *)malloc(IFFTsize);
	float * colimag  = (float *)malloc(IFFTsize);
	if(IFFTreal == NULL || IFFTimag == NULL || colreal == NULL || colimag == NULL)
		return 0;

	float * ptr_x = base;
	float * ptr_y = base + NMax * 2;

	//对每一行做一维的IFFT变换
	for (int i = 0; i < row; i++)
	{
		int rowoff = col * i;
		for (int j = 0; j < col; j++)
		{
			ptr_x[2 * j]     = inreal[rowoff + j];
			ptr_x[2 * j + 1] = inimag[rowoff + j]; 
		}

		DSPF_sp_ifftSPxSP_cn (col, ptr_x, ptr_w0, ptr_y, brev, rad0, 0, col);

		for (int j = 0; j < col; j++)
		{
			colreal[rowoff + j] = ptr_y[2 * j];
			colimag[rowoff + j] = ptr_y[2 * j + 1];
		}
	}

	//对每一列做一维的IFFT变换
	for (int i = 0; i < col; i++)
	{
		for (int j = 0; j < row; j++)
		{
			int rowoff = col * j;
			ptr_x[2 * j]     = colreal[rowoff + i];
			ptr_x[2 * j + 1] = colimag[rowoff + i];
		}

		DSPF_sp_ifftSPxSP_cn (row, ptr_x, ptr_w1, ptr_y, brev, rad1, 0, row);

		for (int j = 0; j < row; j++)
		{
			int rowoff = col * j;
			colreal[rowoff + i] = ptr_y[2 * j];
			colimag[rowoff + i] = ptr_y[2 * j + 1];
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	//原数组中包含的复数个数
	int Cnum = (col + 2) >> 1;

	//提取出连续的二维IFFT数组
	for (int i = 0; i < row; i++)
	{
		int num = col * i;
		float * pRow = (float *)(xin->data.ptr + xin->step * i);
		for (int j = 0; j < Cnum; j++)
		{
			IFFTreal[num + j] = pRow[NbFeatures * 2 * j + k];
			IFFTimag[num + j] = pRow[NbFeatures * 2 * j + 1 + k];
		}


		//根据对称性补全整个IFFT数组
		int m = (row - i) % row;
		int num2 = col * m;
		for (int j = Cnum; j < col; j++)
		{				
			int n = (col - j) % col;
			IFFTreal[num + j] =  IFFTreal[num2 + n];
			IFFTimag[num + j] = -IFFTimag[num2 + n];
		}
	}

	//对每一行做一维的IFFT变换
	for (int i = 0; i < row; i++)
	{
		int rowoff = col * i;
		for (int j = 0; j < col; j++)
		{
			ptr_x[2 * j]     = IFFTreal[rowoff + j];
			ptr_x[2 * j + 1] = IFFTimag[rowoff + j]; 
		}

		DSPF_sp_ifftSPxSP_cn (col, ptr_x, ptr_w0, ptr_y, brev, rad0, 0, col);

		for (int j = 0; j < col; j++)
		{
			colreal[rowoff + j] = ptr_y[2 * j];
			colimag[rowoff + j] = ptr_y[2 * j + 1];
		}
	}

	//对每一列做一维的IFFT变换
	for (int i = 0; i < col; i++)
	{
		for (int j = 0; j < row; j++)
		{
			int rowoff = col * j;
			ptr_x[2 * j]     = colreal[rowoff + i];
			ptr_x[2 * j + 1] = colimag[rowoff + i];
		}

		DSPF_sp_ifftSPxSP_cn (row, ptr_x, ptr_w1, ptr_y, brev, rad1, 0, row);

		for (int j = 0; j < row; j++)
		{
			int rowoff = col * j;
			colreal[rowoff + i] = ptr_y[2 * j];
			colimag[rowoff + i] = ptr_y[2 * j + 1];
		}
	}

	i = 0;


	if(ptr_w0 != NULL)
		free(ptr_w0);
	if(ptr_w1 != NULL)
		free(ptr_w1);
	if(base != NULL)
		free(base);
	if(IFFTreal != NULL)
		free(IFFTreal);
	if(IFFTimag != NULL)
		free(IFFTimag);
	if(colreal != NULL)
		free(colreal);
	if(colimag != NULL)
		free(colimag);

	return 1;
}

     /* Function for generating Specialized sequence of twiddle factors */

void tw_gen_i (float *w, int n)
{
    int i, j, k;
    const double PI = 3.141592654;

    for (j = 1, k = 0; j <= n >> 2; j = j << 2)
    {
        for (i = 0; i < n >> 2; i += j)
        {
            w[k]     = (float) -sin (2 * PI * i / n);
            w[k + 1] = (float)  cos (2 * PI * i / n);
            w[k + 2] = (float) -sin (4 * PI * i / n);
            w[k + 3] = (float)  cos (4 * PI * i / n);
            w[k + 4] = (float) -sin (6 * PI * i / n);
            w[k + 5] = (float)  cos (6 * PI * i / n);

            k += 6;
        }
    }
}
/* ======================================================================= */
/*  End of file:  DSPF_sp_ifftSPxSP_d.c                                    */
/* ----------------------------------------------------------------------- */
/*            Copyright (c) 2011 Texas Instruments, Incorporated.          */
/*                           All Rights Reserved.                          */
/* ======================================================================= */
