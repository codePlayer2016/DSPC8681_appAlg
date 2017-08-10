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


int dpmProcess(char *rgbBuf, int width, int height, int picNum, int maxNum,int totalNum,registerTable *pRegisterTable);

void dpmInit();

#ifdef __cplusplus

}

#endif

#endif /* TESTALL_H_ */
