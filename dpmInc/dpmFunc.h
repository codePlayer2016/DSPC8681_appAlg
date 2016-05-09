/*
 * dpmFunc.h
 *
 *  Created on: 2015-8-5
 *      Author: julie
 */

#ifndef TESTALL_H_
#define TESTALL_H_

#ifdef __cplusplus

extern "C"
{

#endif
#if 0
int dpmProcess(char *rgbBuf, int width, int height, int picNum, int maxNum,
		unsigned int **pOutAddr);
#endif
#if 1
int dpmProcess(char *rgbBuf, int width, int height, int picNum, int maxNum);
#endif
void dpmInit();

#ifdef __cplusplus

}

#endif

#endif /* TESTALL_H_ */
