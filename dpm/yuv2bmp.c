#include <stdio.h>
#include <stdlib.h>
//#include <malloc.h>
#include "dpmFunc.h"
#include "ti/platform/platform.h"
extern void write_uart(char* msg);

#define BMP_ALIGN (4)

typedef struct
{
	char type1;
	char type2;
} BmpFileHead;

typedef struct
{
	unsigned int imageSize;
	unsigned int blank;
	unsigned int startPosition;
	unsigned int length;
	unsigned int width;
	unsigned int height;
	unsigned short colorPlane;
	unsigned short bitColor;
	unsigned int zipFormat;
	unsigned int realSize;
	unsigned int xPels;
	unsigned int yPels;
	unsigned int colorUse;
	unsigned int colorImportant;
} BmpInfoHead;

int YUV2RGB(unsigned char* yuvBuf, unsigned int nWidth, unsigned int nHeight,
		unsigned char * pRgbBuf)
{
#define PIXELSIZE nWidth * nHeight
	const int Table_fv1[256] =
	{ -180, -179, -177, -176, -174, -173, -172, -170, -169, -167, -166, -165,
			-163, -162, -160, -159, -158, -156, -155, -153, -152, -151, -149,
			-148, -146, -145, -144, -142, -141, -139, -138, -137, -135, -134,
			-132, -131, -130, -128, -127, -125, -124, -123, -121, -120, -118,
			-117, -115, -114, -113, -111, -110, -108, -107, -106, -104, -103,
			-101, -100, -99, -97, -96, -94, -93, -92, -90, -89, -87, -86, -85,
			-83, -82, -80, -79, -78, -76, -75, -73, -72, -71, -69, -68, -66,
			-65, -64, -62, -61, -59, -58, -57, -55, -54, -52, -51, -50, -48,
			-47, -45, -44, -43, -41, -40, -38, -37, -36, -34, -33, -31, -30,
			-29, -27, -26, -24, -23, -22, -20, -19, -17, -16, -15, -13, -12,
			-10, -9, -8, -6, -5, -3, -2, 0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14, 15,
			16, 18, 19, 21, 22, 23, 25, 26, 28, 29, 30, 32, 33, 35, 36, 37, 39,
			40, 42, 43, 44, 46, 47, 49, 50, 51, 53, 54, 56, 57, 58, 60, 61, 63,
			64, 65, 67, 68, 70, 71, 72, 74, 75, 77, 78, 79, 81, 82, 84, 85, 86,
			88, 89, 91, 92, 93, 95, 96, 98, 99, 100, 102, 103, 105, 106, 107,
			109, 110, 112, 113, 114, 116, 117, 119, 120, 122, 123, 124, 126,
			127, 129, 130, 131, 133, 134, 136, 137, 138, 140, 141, 143, 144,
			145, 147, 148, 150, 151, 152, 154, 155, 157, 158, 159, 161, 162,
			164, 165, 166, 168, 169, 171, 172, 173, 175, 176, 178 };
	const int Table_fv2[256] =
	{ -92, -91, -91, -90, -89, -88, -88, -87, -86, -86, -85, -84, -83, -83, -82,
			-81, -81, -80, -79, -78, -78, -77, -76, -76, -75, -74, -73, -73,
			-72, -71, -71, -70, -69, -68, -68, -67, -66, -66, -65, -64, -63,
			-63, -62, -61, -61, -60, -59, -58, -58, -57, -56, -56, -55, -54,
			-53, -53, -52, -51, -51, -50, -49, -48, -48, -47, -46, -46, -45,
			-44, -43, -43, -42, -41, -41, -40, -39, -38, -38, -37, -36, -36,
			-35, -34, -33, -33, -32, -31, -31, -30, -29, -28, -28, -27, -26,
			-26, -25, -24, -23, -23, -22, -21, -21, -20, -19, -18, -18, -17,
			-16, -16, -15, -14, -13, -13, -12, -11, -11, -10, -9, -8, -8, -7,
			-6, -6, -5, -4, -3, -3, -2, -1, 0, 0, 1, 2, 2, 3, 4, 5, 5, 6, 7, 7,
			8, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 17, 18, 19, 20,
			20, 21, 22, 22, 23, 24, 25, 25, 26, 27, 27, 28, 29, 30, 30, 31, 32,
			32, 33, 34, 35, 35, 36, 37, 37, 38, 39, 40, 40, 41, 42, 42, 43, 44,
			45, 45, 46, 47, 47, 48, 49, 50, 50, 51, 52, 52, 53, 54, 55, 55, 56,
			57, 57, 58, 59, 60, 60, 61, 62, 62, 63, 64, 65, 65, 66, 67, 67, 68,
			69, 70, 70, 71, 72, 72, 73, 74, 75, 75, 76, 77, 77, 78, 79, 80, 80,
			81, 82, 82, 83, 84, 85, 85, 86, 87, 87, 88, 89, 90, 90 };
	const int Table_fu1[256] =
	{ -44, -44, -44, -43, -43, -43, -42, -42, -42, -41, -41, -41, -40, -40, -40,
			-39, -39, -39, -38, -38, -38, -37, -37, -37, -36, -36, -36, -35,
			-35, -35, -34, -34, -33, -33, -33, -32, -32, -32, -31, -31, -31,
			-30, -30, -30, -29, -29, -29, -28, -28, -28, -27, -27, -27, -26,
			-26, -26, -25, -25, -25, -24, -24, -24, -23, -23, -22, -22, -22,
			-21, -21, -21, -20, -20, -20, -19, -19, -19, -18, -18, -18, -17,
			-17, -17, -16, -16, -16, -15, -15, -15, -14, -14, -14, -13, -13,
			-13, -12, -12, -11, -11, -11, -10, -10, -10, -9, -9, -9, -8, -8, -8,
			-7, -7, -7, -6, -6, -6, -5, -5, -5, -4, -4, -4, -3, -3, -3, -2, -2,
			-2, -1, -1, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6,
			6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 11, 11, 11, 12, 12, 12, 13,
			13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 18,
			19, 19, 19, 20, 20, 20, 21, 21, 22, 22, 22, 23, 23, 23, 24, 24, 24,
			25, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 29, 29, 29, 30, 30,
			30, 31, 31, 31, 32, 32, 33, 33, 33, 34, 34, 34, 35, 35, 35, 36, 36,
			36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 40, 40, 40, 41, 41, 41, 42,
			42, 42, 43, 43 };
	const int Table_fu2[256] =
	{ -227, -226, -224, -222, -220, -219, -217, -215, -213, -212, -210, -208,
			-206, -204, -203, -201, -199, -197, -196, -194, -192, -190, -188,
			-187, -185, -183, -181, -180, -178, -176, -174, -173, -171, -169,
			-167, -165, -164, -162, -160, -158, -157, -155, -153, -151, -149,
			-148, -146, -144, -142, -141, -139, -137, -135, -134, -132, -130,
			-128, -126, -125, -123, -121, -119, -118, -116, -114, -112, -110,
			-109, -107, -105, -103, -102, -100, -98, -96, -94, -93, -91, -89,
			-87, -86, -84, -82, -80, -79, -77, -75, -73, -71, -70, -68, -66,
			-64, -63, -61, -59, -57, -55, -54, -52, -50, -48, -47, -45, -43,
			-41, -40, -38, -36, -34, -32, -31, -29, -27, -25, -24, -22, -20,
			-18, -16, -15, -13, -11, -9, -8, -6, -4, -2, 0, 1, 3, 5, 7, 8, 10,
			12, 14, 15, 17, 19, 21, 23, 24, 26, 28, 30, 31, 33, 35, 37, 39, 40,
			42, 44, 46, 47, 49, 51, 53, 54, 56, 58, 60, 62, 63, 65, 67, 69, 70,
			72, 74, 76, 78, 79, 81, 83, 85, 86, 88, 90, 92, 93, 95, 97, 99, 101,
			102, 104, 106, 108, 109, 111, 113, 115, 117, 118, 120, 122, 124,
			125, 127, 129, 131, 133, 134, 136, 138, 140, 141, 143, 145, 147,
			148, 150, 152, 154, 156, 157, 159, 161, 163, 164, 166, 168, 170,
			172, 173, 175, 177, 179, 180, 182, 184, 186, 187, 189, 191, 193,
			195, 196, 198, 200, 202, 203, 205, 207, 209, 211, 212, 214, 216,
			218, 219, 221, 223, 225 };
	//   *len = 3* nWidth * nHeight;
	if (!yuvBuf || !pRgbBuf)
		return 0;
	const long nYLen = PIXELSIZE;
	const int nHfWidth = (nWidth >> 1);
	if (nYLen < 1 || nHfWidth < 1)
		return 0;
	// Y data
	unsigned char* yData = yuvBuf;
	// v data
	unsigned char* vData = &yData[nYLen];
	// u data
	unsigned char* uData = &vData[nYLen >> 2];
	if (!uData || !vData)
		return 0;
	int rgb[3];
	int i, j, m, n, x, y, pu, pv, py, rdif, invgdif, bdif;
	m = -nWidth;
	n = -nHfWidth;
	int addhalf = 1;
	for (y = 0; y < nHeight; y++)
	{
		m += nWidth;
		if (addhalf)
		{
			n += nHfWidth;
			addhalf = 0;
		}
		else
		{
			addhalf = 1;
		}
		for (x = 0; x < nWidth; x++)
		{
			i = m + x;
			j = n + (x >> 1);
			py = yData[i];
			// search tables to get rdif invgdif and bidif
			rdif = Table_fv1[vData[j]]; // fv1
			invgdif = Table_fu1[uData[j]] + Table_fv2[vData[j]]; // fu1+fv2
			bdif = Table_fu2[uData[j]]; // fu2
			rgb[0] = py + rdif; // R
			rgb[1] = py - invgdif; // G
			rgb[2] = py + bdif; // B
			j = nYLen - nWidth - m + x;
			i = (j << 1) + j;
			// copy this pixel to rgb data
			for (j = 0; j < 3; j++)
			{
				if (rgb[j] >= 0 && rgb[j] <= 255)
				{
					pRgbBuf[i + j] = rgb[j];
				}
				else
				{
					pRgbBuf[i + j] = (rgb[j] < 0) ? 0 : 255;
				}
			}
		}
	}
	return 1;
}

int cvSaveImageInAddr0(char* pdst, char* imgData, const int imgWidth,
		const int imgHeight, const int imgWidthStep, const int channels)
{
	char *addr = pdst;
	BmpFileHead bmpfilehead =
	{ 'B', 'M' };
	BmpInfoHead bmpinfohead;
	int step = imgWidth * channels; //windowsλͼstep������4�ı���
	int modbyte = step & (BMP_ALIGN - 1); //
	if (modbyte != 0)
		step += (BMP_ALIGN - modbyte);

	memcpy(addr, &bmpfilehead, sizeof(BmpFileHead));
	addr += sizeof(BmpFileHead); //move to next section

	bmpinfohead.blank = 0;
	bmpinfohead.startPosition = sizeof(BmpFileHead) + sizeof(BmpInfoHead);
	bmpinfohead.realSize = step * imgHeight;
	bmpinfohead.imageSize = bmpinfohead.realSize + bmpinfohead.startPosition;
	bmpinfohead.length = 0x28;
	bmpinfohead.width = imgWidth;
	bmpinfohead.height = imgHeight;
	bmpinfohead.colorPlane = 0x01;
	bmpinfohead.bitColor = 8 * channels;
	bmpinfohead.zipFormat = 0;
	bmpinfohead.xPels = 0xEC4;
	bmpinfohead.yPels = 0xEC4;
	bmpinfohead.colorUse = 0;
	bmpinfohead.colorImportant = 0;

	memcpy(addr, &bmpinfohead, sizeof(BmpInfoHead));
	addr += sizeof(BmpInfoHead); //move to next section
			/*
			 char *pdata = imgData + (imgHeight - 1) * imgWidthStep;

			 for(int idx = imgHeight - 1; idx >= 0 ; idx--)
			 {
			 memcpy(addr,pdata, step);
			 addr += step;//move to next section
			 pdata -= imgWidthStep;
			 }
			 */
	char *pdata = imgData;
	int idx;
	for (idx = imgHeight - 1; idx >= 0; idx--)
	{
		memcpy(addr, pdata, step);
		addr += step; //move to next section
		pdata += imgWidthStep;
	}
//	FILE* p = fopen("test.bmp", "wb+");
//	fwrite(pdst,1,bmpinfohead.realSize,p);
//	fclose(p);
	return bmpinfohead.imageSize;
}

/*
 void yuv2bmp(unsigned char * YUV,int width,int height)
 {
 FILE *BmpFile =fopen("E:\\test.bmp","wb");
 unsigned char *ImageFileBuf=(unsigned char *)malloc(width*height*3>>1);
 unsigned char *RgbBuf=(unsigned char *)malloc(width *height*3);
 unsigned char *BmpBuf=(unsigned char *)malloc(width *height*3+54);
 memcpy(ImageFileBuf,YUV,width*height*3>>1);
 YUV2RGB(ImageFileBuf,width,height,RgbBuf);

 cvSaveImageInAddr0((char *)BmpBuf, (char *)RgbBuf,width, height, width *3, 3);
 int ret=fwrite(BmpBuf,1,width*height*3+54,BmpFile);
 printf("ok\n ");
 fclose(BmpFile);

 }
 */

void yuv2bmp(unsigned char * YUV, int width, int height, int picNum)
{

//	FILE *BmpFile =fopen("E:\\test.bmp","wb");
//	unsigned char *ImageFileBuf=(unsigned char *)malloc(width*height*3>>1);
	unsigned char *RgbBuf=(unsigned char *)malloc(width *height*3);
	unsigned char *BmpBuf=(unsigned char *)malloc(width *height*3+54);
	if(RgbBuf==NULL)
	{
		//printf("ERROR RgbBuf\n");
		write_uart("ERROR RgbBuf\r\n");
	}
	if(BmpBuf==NULL)
	{
		//printf("ERROR BmpBuf\n");
		write_uart("ERROR BmpBuf\r\n");
	}
//	memcpy(ImageFileBuf,YUV,width*height*3>>1);

	YUV2RGB(YUV,width,height,RgbBuf);
	write_uart("YUV2RGB over\r\n");

	cvSaveImageInAddr0((char *)BmpBuf, (char *)RgbBuf,width, height, width *3, 3);
	write_uart("cvSaveImageInAddr0 over\r\n");
//	int ret=fwrite(BmpBuf,1,width*height*3+54,BmpFile);
	write_uart("dpm algorith start\r\n");

//	testlib((char *)BmpBuf,picNum);
	write_uart("dpm algorith over\r\n");
//	fclose(BmpFile);
	free(RgbBuf);
	free(BmpBuf);

	//cyx
#if 0
	unsigned char *RgbBuf = (unsigned char *) malloc(width * height * 3);
	unsigned char *BmpBuf = (unsigned char *) malloc(width * height * 3 + 54);
	if (RgbBuf == NULL)
	{

		write_uart("ERROR RgbBuf\r\n");
	}
	if (BmpBuf == NULL)
	{

		write_uart("ERROR BmpBuf\r\n");
	}

	YUV2RGB(YUV, width, height, RgbBuf);
	write_uart("YUV2RGB over\r\n");

	cvSaveImageInAddr0((char *) BmpBuf, (char *) RgbBuf, width, height,
			width * 3, 3);
	write_uart("cvSaveImageInAddr0 over\r\n");
	write_uart("dpm algorith start\r\n");

	testlib((char *) BmpBuf, picNum);
	write_uart("dpm algorith over\r\n");
	free(RgbBuf);
	free(BmpBuf);
#endif
}
