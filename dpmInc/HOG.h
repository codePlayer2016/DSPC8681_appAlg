/*
 * HOG.h
 *
 *  Created on: 2015-3-13
 *      Author: LinZW
 */

#ifndef HOG_H_
#define HOG_H_

#include "DPMDetector.h"


void HOG(const IplImage* image, CvMat* level, const int padX, const int padY, const int cellSize);

void convolve(const CvMat* x, const CvMat* y, CvMat** z,
			  const CvRect& xRect, CvPoint& actualTopLeft, int featCount, int featStep);

#endif /* HOG_H_ */
