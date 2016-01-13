/*
 *  ======== main.c ========
 */

#include <xdc/std.h>

#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>

#include <ti/sysbios/knl/Task.h>

#include <c6x.h>

#include <math.h>

#include <stdio.h>

#include <stdlib.h>

char bins[512][512][2];
float magnitudes[512][512][2];

const static float M_PI = 3.14159265358979323846;

#if 0
//原版计算公式
void HOGTable()
{
	int dx, dy;


	for(dy = -255; dy <= 255; dy++)
	{
		for (dx = -255; dx <= 255; dx++)
		{
			float mag2 = (float)(dx * dx + dy * dy);
			float magnitude = sqrtf(mag2) / 255.0f;

			float angle = atan2f((float)dy, (float)dx);
			angle = angle * (9.0f / M_PI) + 18.0f;
			if(angle >= 18.0f)
				angle -= 18.0f;

			const int bin0 = angle;
			const int bin1 = (bin0 < 17) ? (bin0 + 1) : 0;
			const float alpha = angle - bin0;

			bins[dy + 255][dx + 255][0] = bin0;   //0,1,2,...,17
			bins[dy + 255][dx + 255][1] = bin1;   //1,2,3,...,17,0  记录每个像素点对应梯度方向所在 bin 区间
			magnitudes[dy + 255][dx + 255][0] = magnitude * (1.0 - alpha);   //梯度赋值在相邻 bin 的映射部分
			magnitudes[dy + 255][dx + 255][1] = magnitude * alpha;
		}
	}
}
#endif

//修改版计算公式
void HOGTable()
{
	int dx, dy;


	for(dy = -255; dy <= 255; dy++)
	{
		for (dx = -255; dx <= 255; dx++)
		{
			float mag2 = (float)(dx * dx + dy * dy);
			float magnitude = sqrtf(mag2) / 255.0f;

			float angle = atan2f((float)dy, (float)dx);
			angle = angle * (9.0f / M_PI) + 18.0f;
			if(angle >= 18.0f)
				angle -= 18.0f;

			const int bin0 = angle;
			const int bin1 = (bin0 < 17) ? (bin0 + 1) : 0;
			const float alpha = angle - bin0;

			bins[dy + 255][dx + 255][0] = bin0 << 2;   //0,1,2,...,17
			bins[dy + 255][dx + 255][1] = bin1 << 2;   //1,2,3,...,17,0  记录每个像素点对应梯度方向所在 bin 区间
			magnitudes[dy + 255][dx + 255][0] = magnitude * (1.0 - alpha);   //梯度赋值在相邻 bin 的映射部分
			magnitudes[dy + 255][dx + 255][1] = magnitude * alpha;
		}
	}
}

/*
 *  ======== taskFxn ========
 */
Void hog0(unsigned char *pB, unsigned char *pG, unsigned char *pR, const short width, const short height, float * restrict level)
{
    HOGTable();


//    unsigned char * restrict mx = (unsigned char *)malloc(width * 2);
//    unsigned char * restrict my = (unsigned char *)malloc(width * 2);

    const unsigned char cellSize = 8;
    const unsigned char padX = 1;
    const unsigned char padY = 1;
    const unsigned char NbFeatures = 32;

    const int levelHeight = (height + cellSize / 2) / cellSize + 2 * padY;
    const int levelWidth = (width + cellSize / 2) / cellSize + 2 * padX;

    const int halFeatures = 18;


    float * restrict level0 = (float *)malloc(levelHeight * levelWidth * halFeatures * sizeof(float) * 4);
    memset(level0, 0, levelHeight * levelWidth * halFeatures * sizeof(float) * 4);

    float * restrict level1 = level0 + 1;
    float * restrict level2 = level0 + halFeatures * 4 + 2;
    float * restrict level3 = level0 + halFeatures * 4 + 3;
    float * restrict level4 = level0 + levelWidth * halFeatures * 4;
    float * restrict level5 = level4 + 1;
    float * restrict level6 = level4 + halFeatures * 4 + 2;
    float * restrict level7 = level4 + halFeatures * 4 + 3;

    float * restrict magnit0 = (float *)magnitudes;
    float * restrict magnit1 = magnit0;
	float * restrict magnit2 = magnit0;
	float * restrict magnit3 = magnit0;

	char * restrict bin0 = (char *)bins;
	char * restrict bin1 = bin0;
	char * restrict bin2 = bin0;
	char * restrict bin3 = bin0;

    const unsigned int leWid = levelWidth * halFeatures;

    const float invCellSize = 1.0f / (cellSize * 2);
    const long long ioffset = 0x00FF00FF00FF00FF;
    const unsigned int iones   = 0x00010001;
    //可尝试移到循环内部
    //__x128_t sinvcell     = _fto128(invCellSize, invCellSize, invCellSize, invCellSize);
    const __float2_t sinvcell = _ftof2(invCellSize, invCellSize);
    const __float2_t shalf = _ftof2(0.5f, 0.5f);
    const __float2_t sones = _ftof2(1.0f, 1.0f);
    const unsigned int row = leWid << 2;

    unsigned char * restrict packBin = (unsigned char *)malloc(width * height * 2);
    float * restrict packMag = (float *)malloc(width * height * sizeof(float) * 2);
    unsigned char * restrict pBin;
    float * restrict pMag;
    unsigned int cellNum = levelHeight * levelWidth;

    //__x128_t w0, w1, w2, w3;

	int x, y;
    for (y = 0; y < height - 2; ++y)
    {
		unsigned int offset = width * y;
		const unsigned char* restrict lineB0 = pB + offset;
		const unsigned char* restrict lineB1 = lineB0 + width;
		const unsigned char* restrict lineB2 = lineB1 + width;;
		const unsigned char* restrict lineG0 = pG + offset;
		const unsigned char* restrict lineG1 = lineG0 + width;
		const unsigned char* restrict lineG2 = lineG1 + width;
		const unsigned char* restrict lineR0 = pR + offset;
		const unsigned char* restrict lineR1 = lineR0 + width;
		const unsigned char* restrict lineR2 = lineR1 + width;

		pBin = packBin;
		pMag = packMag;


		for (x = 0; x < width - 2; x += 4)
		{
			unsigned int load0, load1, load2, load3;
			unsigned int loadB0, loadB1, loadB2, loadB3;
			unsigned int loadG0, loadG1, loadG2, loadG3;
			unsigned int loadR0, loadR1, loadR2, loadR3;
			long long ld0, ld1, ld2, ld3;
			long long ldB0, ldB1, ldB2, ldB3;
			long long ldG0, ldG1, ldG2, ldG3;
			long long ldR0, ldR1, ldR2, ldR3;
			long long gradBx, gradBy, gradGx, gradGy, gradRx, gradRy, gradx, grady;
			long long gradx0, grady0, gradx1, grady1;
			long long gradx_2_0, gradx_3_1, grady_2_0, grady_3_1;
			__x128_t x2, y2;
			__x128_t Bx2, By2;
			__x128_t Gx2, Gy2;
			__x128_t Rx2, Ry2;
			long long grads;
			__x128_t index;
			unsigned int index0, index1, index2, index3;
			long long x2_1_0, x2_3_2, y2_1_0, y2_3_2;
			long long Bx2_1_0, Bx2_3_2, By2_1_0, By2_3_2;
			long long Gx2_1_0, Gx2_3_2, Gy2_1_0, Gy2_3_2;
			long long Rx2_1_0, Rx2_3_2, Ry2_1_0, Ry2_3_2;
			long long magB_1_0, magB_3_2, magG_1_0, magG_3_2, magR_1_0, magR_3_2;
			unsigned int magB_0, magB_1, magB_2, magB_3, magG_0, magG_1, magG_2, magG_3, magR_0, magR_1, magR_2, magR_3;
			unsigned int magM_0, magM_1, magM_2, magM_3;
			unsigned int bit_0, bit_1, bit_2, bit_3, bit;
			unsigned int bit0_0, bit0_1, bit0_2, bit0_3, bit0;
			unsigned int bit1_0, bit1_1, bit1_2, bit1_3, bit1;
			long long mask1, mask2;
			long long mask0_0, mask0_1;
			long long mask1_0, mask1_1;
			unsigned int pox_0, pox_1, pox_2, pox_3, poy;
			long long pox_2_0, pox_3_1, pox;
			__float2_t xp_1_0, xp_3_2;
			__x128_t xp;
			float yp, syp0, syp1;
			unsigned int iyp;
			unsigned int ixp_1_0, ixp_3_2;
			__float2_t sxp_1_0, sxp_3_2;
			__float2_t xp0_1_0, xp0_3_2, xp1_1_0, xp1_3_2;;
			__x128_t xp0, xp1, yp0, yp1;
			__x128_t w0, w1, w2, w3;
			unsigned int bin0_0, bin0_1, bin0_2, bin0_3, bin1_0, bin1_1, bin1_2, bin1_3;
			float magnitude0_0, magnitude0_1, magnitude0_2, magnitude0_3;
			float magnitude1_0, magnitude1_1, magnitude1_2, magnitude1_3;
			unsigned int argDx, argDy;
			unsigned int argDx0, argDy0, argDx1, argDy1, argDx2, argDy2, argDx3, argDy3;
			char * bin;
			float * magnit;
			__x128_t mag0, mag1, mag00, mag01, mag10, mag11, mag20, mag21, mag30, mag31;
			unsigned int levelstart;
			long long levels;
			long long ixp;
			unsigned int ixpL, ixpH;
			long long lxp_1_0, lxp_3_2;
			long long irow;
			long long levelptr0_1_0, levelptr0_3_2, levelptr1_1_0, levelptr1_3_2;
			float * ptrCell;
			unsigned int levelptr0L, levelptr0H, levelptr1L, levelptr1H;
			unsigned int b0, b1;
			long long bin_02_00, bin_12_10, bin_03_01, bin_13_11;
			long long bin_12_02_10_00, bin_13_03_11_01;
			long long binc;
			__float2_t mag_10_00, mag_11_01, mag_12_02, mag_13_03;

			//B通道

			loadB3 = _mem4((void *)(lineB1 + x));
			loadB1 = _mem4((void *)(lineB1 + x + 2));

			ldB1 = _unpkbu4(loadB1);
			ldB3 = _unpkbu4(loadB3);
			gradBx = _dsub2(ldB1, ldB3);

			loadB0 = _mem4((void *)(lineB0 + x + 1));
			loadB2 = _mem4((void *)(lineB2 + x + 1));

			ldB0 = _unpkbu4(loadB0);
			ldB2 = _unpkbu4(loadB2);
			gradBy = _dsub2(ldB2, ldB0);

			By2 = _dmpy2(gradBy, gradBy);
			Bx2 = _dmpy2(gradBx, gradBx);

			Bx2_1_0 = _lo128(Bx2);
			Bx2_3_2 = _hi128(Bx2);
			By2_1_0 = _lo128(By2);
			By2_3_2 = _hi128(By2);

			magB_1_0 = _dadd(Bx2_1_0, By2_1_0);
			magB_3_2 = _dadd(Bx2_3_2, By2_3_2);

			//G通道

			loadG3 = _mem4((void *)(lineG1 + x));
			loadG1 = _mem4((void *)(lineG1 + x + 2));

			ldG1 = _unpkbu4(loadG1);
			ldG3 = _unpkbu4(loadG3);
			gradGx = _dsub2(ldG1, ldG3);

			loadG0 = _mem4((void *)(lineG0 + x + 1));
			loadG2 = _mem4((void *)(lineG2 + x + 1));

			ldG0 = _unpkbu4(loadG0);
			ldG2 = _unpkbu4(loadG2);
			gradGy = _dsub2(ldG2, ldG0);

			Gy2 = _dmpy2(gradGy, gradGy);
			Gx2 = _dmpy2(gradGx, gradGx);

			Gx2_1_0 = _lo128(Gx2);
			Gx2_3_2 = _hi128(Gx2);
			Gy2_1_0 = _lo128(Gy2);
			Gy2_3_2 = _hi128(Gy2);

			magG_1_0 = _dadd(Gx2_1_0, Gy2_1_0);
			magG_3_2 = _dadd(Gx2_3_2, Gy2_3_2);

			//通道选择
			magB_0 = _loll(magB_1_0);
			magB_1 = _hill(magB_1_0);
			magB_2 = _loll(magB_3_2);
			magB_3 = _hill(magB_3_2);

			magG_0 = _loll(magG_1_0);
			magG_1 = _hill(magG_1_0);
			magG_2 = _loll(magG_3_2);
			magG_3 = _hill(magG_3_2);

			bit0_0 = magB_0 > magG_0;
			bit0_1 = (magB_1 > magG_1) << 1;
			bit0_2 = (magB_2 > magG_2) << 2;
			bit0_3 = (magB_3 > magG_3) << 3;

			//乘法选择是否更好？
			magM_0 = bit0_0 ? magB_0 : magG_0;
			magM_1 = bit0_1 ? magB_1 : magG_1;
			magM_2 = bit0_2 ? magB_2 : magG_2;
			magM_3 = bit0_3 ? magB_3 : magG_3;

			bit0 = bit0_0 | bit0_1 | bit0_2 | bit0_3;
			mask0_0 = _dxpnd2(bit0);
			mask0_1 = _xorll_c(-1, mask0_0);
			gradx0 = (gradBx & mask0_0) | (gradGx & mask0_1);
			grady0 = (gradBy & mask0_0) | (gradGy & mask0_1);

			//R通道

			loadR3 = _mem4((void *)(lineR1 + x));
			loadR1 = _mem4((void *)(lineR1 + x + 2));

			ldR1 = _unpkbu4(loadR1);
			ldR3 = _unpkbu4(loadR3);
			gradRx = _dsub2(ldR1, ldR3);

			loadR0 = _mem4((void *)(lineR0 + x + 1));
			loadR2 = _mem4((void *)(lineR2 + x + 1));

			ldR0 = _unpkbu4(loadR0);
			ldR2 = _unpkbu4(loadR2);
			gradRy = _dsub2(ldR2, ldR0);

			Ry2 = _dmpy2(gradRy, gradRy);
			Rx2 = _dmpy2(gradRx, gradRx);


			Rx2_1_0 = _lo128(Rx2);
			Rx2_3_2 = _hi128(Rx2);
			Ry2_1_0 = _lo128(Ry2);
			Ry2_3_2 = _hi128(Ry2);

			magR_1_0 = _dadd(Rx2_1_0, Ry2_1_0);
			magR_3_2 = _dadd(Rx2_3_2, Ry2_3_2);

			magR_0 = _loll(magR_1_0);
			magR_1 = _hill(magR_1_0);
			magR_2 = _loll(magR_3_2);
			magR_3 = _hill(magR_3_2);

			bit_0 = magR_0 > magM_0;
			bit_1 = (magR_1 > magM_1) << 1;
			bit_2 = (magR_2 > magM_2) << 2;
			bit_3 = (magR_3 > magM_3) << 3;

			bit = bit_0 | bit_1 | bit_2 | bit_3;
			mask1_0 = _dxpnd2(bit);
			mask1_1 = _xorll_c(-1, mask1_0);
			gradx1 = (gradRx & mask1_0) | (gradx0 & mask1_1);
			grady1 = (gradRy & mask1_0) | (grady0 & mask1_1);

			gradx = _dadd2(gradx1, ioffset);
			grady = _dadd2(grady1, ioffset);

			grads = _dshl2(gradx, 1);
			index = _dmpyu2(grads, grady);

			index0 = _get32_128(index, 0);
			index1 = _get32_128(index, 1);
			index2 = _get32_128(index, 2);
			index3 = _get32_128(index, 3);


			bin0_0 = bin0[index0];
			bin1_0 = bin0[index0 + 1];

			bin0_1 = bin1[index1];
			bin1_1 = bin1[index1 + 1];

			bin0_2 = bin2[index2];
			bin1_2 = bin2[index2 + 1];

			bin0_3 = bin3[index3];
			bin1_3 = bin3[index3 + 1];


			magnitude0_0 = magnit0[index0];
			magnitude1_0 = magnit0[index0 + 1];

			magnitude0_1 = magnit1[index1];
			magnitude1_1 = magnit1[index1 + 1];

			magnitude0_2 = magnit2[index2];
			magnitude1_2 = magnit2[index2 + 1];

			magnitude0_3 = magnit3[index3];
			magnitude1_3 = magnit3[index3 + 1];



//			isum += bin0_0 + bin1_0 + bin0_1 + bin1_1 + bin0_2 + bin1_2 + bin0_3 + bin1_3;
//			fsum += magnitude0_0 + magnitude1_0 + magnitude0_1 + magnitude1_1 +
//					magnitude0_2 + magnitude1_2 + magnitude0_3 + magnitude1_3;

			bin_02_00 = _itoll(bin0_2, bin0_0);
			bin_12_10 = _itoll(bin1_2, bin1_0);
			bin_12_02_10_00 = _dpackl2(bin_12_10, bin_02_00);
			bin_03_01 = _itoll(bin0_3, bin0_1);
			bin_13_11 = _itoll(bin1_3, bin1_1);
			bin_13_03_11_01 = _dpackl2(bin_13_11, bin_03_01);
			binc = _dpackl4(bin_13_03_11_01, bin_12_02_10_00);
			_amem8(pBin) = binc;
			pBin += 8;

			mag_10_00 = _ftof2(magnitude1_0, magnitude0_0);
			_amem8_f2(pMag) = mag_10_00;
			pMag += 2;
			mag_11_01 = _ftof2(magnitude1_1, magnitude0_1);
			_amem8_f2(pMag) = mag_11_01;
			pMag += 2;
			mag_12_02 = _ftof2(magnitude1_2, magnitude0_2);
			_amem8_f2(pMag) = mag_12_02;
			pMag += 2;
			mag_13_03 = _ftof2(magnitude1_3, magnitude0_3);
			_amem8_f2(pMag) = mag_13_03;
			pMag += 2;
		}

		pBin = packBin;
		pMag = packMag;



		//#pragma MUST_ITERATE(100, , 2)
		for (x = 0; x < width - 2; x += 2)
		{
//			l0 = TIME_READ;
			unsigned int bin0_0, bin0_1, bin1_0, bin1_1;
			float magnitude0_0, magnitude0_1, magnitude0_2, magnitude0_3;
			float magnitude1_0, magnitude1_1, magnitude1_2, magnitude1_3;
			//long long ibin;
			unsigned int ibin;
			int ibin_7_4, ibin_3_0;
			long long lbin_7_4, lbin_3_0;
			int ibin_7_6, ibin_5_4, ibin_3_2, ibin_1_0;
			long long lbin_7_6, lbin_5_4, lbin_3_2, lbin_1_0;
			__float2_t mag0_3_2, mag0_1_0, mag1_3_2, mag1_1_0;
			__x128_t mag0, mag1, mag00, mag01, mag10, mag11, mag20, mag21, mag30, mag31;
			__float2_t mag00H, mag00L, mag01H, mag01L, mag10H, mag10L, mag11H, mag11L,
					   mag20H, mag20L, mag21H, mag21L, mag30H, mag30L, mag31H, mag31L;
			float mag00_0, mag00_1, mag00_2, mag00_3, mag01_0, mag01_1, mag01_2, mag01_3,
				  mag10_0, mag10_1, mag10_2, mag10_3, mag11_0, mag11_1, mag11_2, mag11_3,
				  mag20_0, mag20_1, mag20_2, mag20_3, mag21_0, mag21_1, mag21_2, mag21_3,
				  mag30_0, mag30_1, mag30_2, mag30_3, mag31_0, mag31_1, mag31_2, mag31_3;
			unsigned int levelstart;
			long long levels;
			//long long ixp;
			unsigned int ixp, ixpt, ixpL, ixpH;
			long long lxp, lxpt, lxp_1_0, lxp_3_2;
			long long irow;
			long long levelptr0_1_0, levelptr0_3_2, levelptr1_1_0, levelptr1_3_2;
			long long levelptr0, levelptr1;
			float * restrict ptrCell;
			unsigned int levelptr0L, levelptr0H, levelptr1L, levelptr1H;
			unsigned int b0, b1;
			unsigned int pox_0, pox_1, pox_2, pox_3, pox, poxt, poy;
			__float2_t poxsp;
			long long pox_2_0, pox_3_1;
			__float2_t xp_1_0, xp_3_2;
			//__x128_t xp;
			__float2_t xpt, xp;
			float yp, syp0, syp1;
			unsigned int iyp;
			unsigned int ixp_1_0, ixp_3_2;
			__float2_t sxp, sxp_1_0, sxp_3_2;
			__float2_t xp0_1_0, xp0_3_2, xp1_1_0, xp1_3_2;;
			//__x128_t xp0, xp1, yp0, yp1;
			__float2_t xp0, xp1, yp0, yp1;
			__x128_t w0, w1, w2, w3;
			float cellSum0, cellSum1;
			unsigned int index;
			float hs0e, hs1e, hs0o, hs1o, ha0e, ha1e, ha0o, ha1o,
				  hs2e, hs3e, hs2o, hs3o, ha2e, ha3e, ha2o, ha3o;
			int bn0e, bn1e, bn0o, bn1o, bn2e, bn3e, bn2o, bn3o;
			float w00, w01, w02, w03,
				  w10, w11, w12, w13,
				  w20, w21, w22, w23,
				  w30, w31, w32, w33;
			__float2_t mag_1_0;
			__float2_t wMag_01_00, wMag_10_11, wMag_20_21, wMag_30_31;
			__float2_t xpt_1_0, xpt_3_2;

			pox_0 = x + 1;
			pox_1 = x + 2;
			poxt = _pack2(pox_1, pox_0) << 1;
			pox = _add2(poxt, iones);
			poxsp = _dinthspu(pox);
			xpt = _dmpysp(poxsp, sinvcell);
			xp = _daddsp(xpt, shalf);
			ixp = _dspinth(xp);
			sxp = _dinthspu(ixp);
			xp0 = _dsubsp(xp, sxp);
			xp1 = _dsubsp(sones, xp0);


			poy = ((y + 1) << 1) + 1;
			yp = poy * invCellSize + 0.5f;
			iyp = yp;
			syp0 = yp - iyp;
			syp1 = 1.0f - syp0;

//			ixpt = _shl2(ixp, 7);
//			lxp = _dpack2(0, ixp);
			//直方图有18个bin，每个bin占有4字节，共72字节，16进制为0x48
			//未使用_mpyu2，反而使用两个_mpyu，是为了避免寄存器压力过大导致ii值大于Loop Carried Dependency Bound
			//lxp = _mpyu2(ixp, 0x00480048);
			lxpt = _dpack2(0, ixp);
			ixpL = _mpyu(_loll(lxpt), 0x48);
			ixpH = _mpyu(_hill(lxpt), 0x48);
			lxp = _itoll(ixpH, ixpL);
			levelstart = (unsigned int)(leWid * iyp);
			levels = _itoll(levelstart, levelstart);
			levelptr0 = _dadd(levels, lxp);
			irow = _itoll(row, row);
			levelptr1 = _dadd(levelptr0, irow);

			levelptr0L = _loll(levelptr0);
			levelptr0H = _hill(levelptr0);
			levelptr1L = _loll(levelptr1);
			levelptr1H = _hill(levelptr1);

			ibin = _amem4(pBin);
			pBin += 4;
			lbin_3_0 = _unpkbu4(ibin);
			ibin_1_0 = _loll(lbin_3_0);
			ibin_3_2 = _hill(lbin_3_0);
			lbin_1_0 = _unpkhu2(ibin_1_0);

			bin0_0 = _loll(lbin_1_0);
			bin1_0 = _hill(lbin_1_0);

//			l1 = TIME_READ;

			ptrCell = (float *)levelptr0L;
			b0 = bin0_0;
			b1 = bin1_0;

			w00 = _lof2(xp1) * syp1;
			mag_1_0 = _amem8_f2(pMag);
			wMag_01_00 = _dmpysp(_ftof2(w00, w00), mag_1_0);

			level0[levelptr0L + b0] += _lof2(wMag_01_00);
			level1[levelptr0L + b1] += _hif2(wMag_01_00);
//			index = levelptr0L + NbFeatures;

			w01 = _lof2(xp0) * syp1;
			wMag_10_11 = _dmpysp(_ftof2(w01, w01), mag_1_0);

			level2[levelptr0L + b0] += _lof2(wMag_10_11);
			level3[levelptr0L + b1] += _hif2(wMag_10_11);

			w02 = _lof2(xp1) * syp0;
			wMag_20_21 = _dmpysp(_ftof2(w02, w02), mag_1_0);

			level4[levelptr0L + b0] += _lof2(wMag_20_21);
			level5[levelptr0L + b1] += _hif2(wMag_20_21);
//			index = levelptr1L + NbFeatures;

			w03 = _lof2(xp0) * syp0;
			wMag_30_31 = _dmpysp(_ftof2(w03, w03), mag_1_0);

			level6[levelptr0L + b0] += _lof2(wMag_30_31);
			level7[levelptr0L + b1] += _hif2(wMag_30_31);

			lbin_3_2 = _unpkhu2(ibin_3_2);
			bin0_1 = _loll(lbin_3_2);
			bin1_1 = _hill(lbin_3_2);

			b0 = bin0_1;
			b1 = bin1_1;

			w10 = _hif2(xp1) * syp1;
			mag_1_0 = _amem8_f2(pMag + 2);
			wMag_01_00 = _dmpysp(_ftof2(w10, w10), mag_1_0);

			level0[levelptr0H + b0] += _lof2(wMag_01_00);
			level1[levelptr0H + b1] += _hif2(wMag_01_00);
//			index = levelptr0H + NbFeatures;

			w11 = _hif2(xp0) * syp1;
			wMag_10_11 = _dmpysp(_ftof2(w11, w11), mag_1_0);

			level2[levelptr0H + b0] += _lof2(wMag_10_11);
			level3[levelptr0H + b1] += _hif2(wMag_10_11);

			w12 = _hif2(xp1) * syp0;
			wMag_20_21 = _dmpysp(_ftof2(w12, w12), mag_1_0);

			level4[levelptr0H + b0] += _lof2(wMag_20_21);
			level5[levelptr0H + b1] += _hif2(wMag_20_21);
//			index = levelptr1H + NbFeatures;

			w13 = _hif2(xp0) * syp0;
			wMag_30_31 = _dmpysp(_ftof2(w13, w13), mag_1_0);

			level6[levelptr0H + b0] += _lof2(wMag_30_31);
			level7[levelptr0H + b1] += _hif2(wMag_30_31);

			pMag += 4;

		}


    }


    float * restrict lvl  = level;
    float * restrict lvl0 = level0;
    float * restrict lvl1 = level1;
    float * restrict lvl2 = level2;
    float * restrict lvl3 = level3;

	for (x = 0; x < cellNum; x++)
	{
		__float2_t lvlsp0, lvlsp1, sum;

//////////// 1_0 ////////////////////////////////
		lvlsp0 = _amem8_f2(lvl0);
		lvl0 += 2;
		lvlsp1 = _amem8_f2(lvl1);
		lvl1 += 2;
		sum = _daddsp(lvlsp0, lvlsp1);

		lvlsp0 = _amem8_f2(lvl2);
		lvl2 += 2;
		sum = _daddsp(sum, lvlsp0);

		lvlsp0 = _amem8_f2(lvl3);
		lvl3 += 2;
		sum = _daddsp(sum, lvlsp0);

		_amem8_f2(lvl) = sum;
		lvl += 2;

//////////// 3_2 ////////////////////////////////
		lvlsp0 = _amem8_f2(lvl0);
		lvl0 += 2;
		lvlsp1 = _amem8_f2(lvl1);
		lvl1 += 2;
		sum = _daddsp(lvlsp0, lvlsp1);

		lvlsp0 = _amem8_f2(lvl2);
		lvl2 += 2;
		sum = _daddsp(sum, lvlsp0);

		lvlsp0 = _amem8_f2(lvl3);
		lvl3 += 2;
		sum = _daddsp(sum, lvlsp0);

		_amem8_f2(lvl) = sum;
		lvl += 2;

//////////// 5_4 ////////////////////////////////
		lvlsp0 = _amem8_f2(lvl0);
		lvl0 += 2;
		lvlsp1 = _amem8_f2(lvl1);
		lvl1 += 2;
		sum = _daddsp(lvlsp0, lvlsp1);

		lvlsp0 = _amem8_f2(lvl2);
		lvl2 += 2;
		sum = _daddsp(sum, lvlsp0);

		lvlsp0 = _amem8_f2(lvl3);
		lvl3 += 2;
		sum = _daddsp(sum, lvlsp0);

		_amem8_f2(lvl) = sum;
		lvl += 2;

//////////// 7_6 ////////////////////////////////
		lvlsp0 = _amem8_f2(lvl0);
		lvl0 += 2;
		lvlsp1 = _amem8_f2(lvl1);
		lvl1 += 2;
		sum = _daddsp(lvlsp0, lvlsp1);

		lvlsp0 = _amem8_f2(lvl2);
		lvl2 += 2;
		sum = _daddsp(sum, lvlsp0);

		lvlsp0 = _amem8_f2(lvl3);
		lvl3 += 2;
		sum = _daddsp(sum, lvlsp0);

		_amem8_f2(lvl) = sum;
		lvl += 2;

//////////// 9_8 ////////////////////////////////
		lvlsp0 = _amem8_f2(lvl0);
		lvl0 += 2;
		lvlsp1 = _amem8_f2(lvl1);
		lvl1 += 2;
		sum = _daddsp(lvlsp0, lvlsp1);

		lvlsp0 = _amem8_f2(lvl2);
		lvl2 += 2;
		sum = _daddsp(sum, lvlsp0);

		lvlsp0 = _amem8_f2(lvl3);
		lvl3 += 2;
		sum = _daddsp(sum, lvlsp0);

		_amem8_f2(lvl) = sum;
		lvl += 2;

//////////// 11_10 ////////////////////////////////
		lvlsp0 = _amem8_f2(lvl0);
		lvl0 += 2;
		lvlsp1 = _amem8_f2(lvl1);
		lvl1 += 2;
		sum = _daddsp(lvlsp0, lvlsp1);

		lvlsp0 = _amem8_f2(lvl2);
		lvl2 += 2;
		sum = _daddsp(sum, lvlsp0);

		lvlsp0 = _amem8_f2(lvl3);
		lvl3 += 2;
		sum = _daddsp(sum, lvlsp0);

		_amem8_f2(lvl) = sum;
		lvl += 2;

//////////// 13_12 ////////////////////////////////
		lvlsp0 = _amem8_f2(lvl0);
		lvl0 += 2;
		lvlsp1 = _amem8_f2(lvl1);
		lvl1 += 2;
		sum = _daddsp(lvlsp0, lvlsp1);

		lvlsp0 = _amem8_f2(lvl2);
		lvl2 += 2;
		sum = _daddsp(sum, lvlsp0);

		lvlsp0 = _amem8_f2(lvl3);
		lvl3 += 2;
		sum = _daddsp(sum, lvlsp0);

		_amem8_f2(lvl) = sum;
		lvl += 2;

//////////// 15_14 ////////////////////////////////
		lvlsp0 = _amem8_f2(lvl0);
		lvl0 += 2;
		lvlsp1 = _amem8_f2(lvl1);
		lvl1 += 2;
		sum = _daddsp(lvlsp0, lvlsp1);

		lvlsp0 = _amem8_f2(lvl2);
		lvl2 += 2;
		sum = _daddsp(sum, lvlsp0);

		lvlsp0 = _amem8_f2(lvl3);
		lvl3 += 2;
		sum = _daddsp(sum, lvlsp0);

		_amem8_f2(lvl) = sum;
		lvl += 2;

//////////// 17_16 ////////////////////////////////
		lvlsp0 = _amem8_f2(lvl0);
		lvl0 += 2;
		lvlsp1 = _amem8_f2(lvl1);
		lvl1 += 2;
		sum = _daddsp(lvlsp0, lvlsp1);

		lvlsp0 = _amem8_f2(lvl2);
		lvl2 += 2;
		sum = _daddsp(sum, lvlsp0);

		lvlsp0 = _amem8_f2(lvl3);
		lvl3 += 2;
		sum = _daddsp(sum, lvlsp0);

		_amem8_f2(lvl) = sum;
		lvl += 16;

	}

	free(level0);
	free(packBin);
	free(packMag);

}


