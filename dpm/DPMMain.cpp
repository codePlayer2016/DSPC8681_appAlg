/*
 * DPMMain.cpp
 *
 *  Created on: 2015-3-13
 *      Author: LinZW
 */

#include "DPM.h"
#include "DPMMain.h"
//#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <iostream>
#include <stdio.h>
#include <time.h>
//#include <xmmintrin.h>
//#include <malloc.h>
//#include "InstructionSet.h"

using namespace std;
using namespace zftdt;

ofstream foutDebug;

//int g_copyConsNum = 0;
//int g_optEqualNum = 0;

#define RES_RECORD_EN (1)

//int main(void)
//{
//	printf("helloWorld.\n");
//	return 0;
//}
#if 0
int DPMMain()
{
#if 1
    long long int beg, end;
    double freq = 2.0e9;//double freq = cv::getTickFrequency();
    double timeDetect, timeDetectFast, timeDetectRoot;
    double accTimeDetect = 0, accTimeDetectFast = 0, accTimeDetectRoot = 0;
    int procCount = 0;
    const string filePath = "D:/DPM/LIHUOSHENGgiveChenglonghu/DMP_DSP/test/bmpImage400/100.bmp";//"E:/work/project/6678_accelerator/code/Images/singleImageBMP.txt";

        /*"filelist.txt";*/
        /*"D:/SHARED/DPMImages/test0.mp4/test0.mp4/filelist.txt";*/
        /*"D:/SHARED/DPMImages/images2/images2/filelist.txt";*/
        /*"C:/Users/zhengxuping/Desktop/dpmvehicle/testnew/filelist.txt";*/
        /*"D:/SHARED/DPMImages/YanZhiguo/�˶�ǰ��_2014-06-11.mp4-֡/filelist.txt";*/
    const string prefix = "";
    string buf;
    fstream ffstrm(filePath.c_str());


    const string modelPath = "D:/DPM/LIHUOSHENGgiveChenglonghu/DMP_DSP/DPM/src/motorcyclist.txt";


    DeformablePartModel model(modelPath);
	double threshold = -0.6, rootThreshold = 0.9;
	double overlap = 0.4;
    int padx = DPM_HOG_PADX, pady = DPM_HOG_PADY, interval = DPM_PYRAMID_LAMBDA;
    int maxLevels = DPM_PYRAMID_MAX_LEVELS, minSideLen = DPM_PYRAMID_MIN_DETECT_STRIDE;

//	time_t time_now = time(0);
//	struct tm *timeinfo;
//	timeinfo = localtime(&time_now);
//	char timeBuf[32];
//	strftime(timeBuf,sizeof(timeBuf) - 1,"%Y%m%d-%H%M%S", timeinfo);
	//std::cout << string(timeBuf) << std::endl;
//	string fDebug_path = "vector/r_" + string(timeBuf) + ".txt";
//	foutDebug.open(fDebug_path.c_str());
//	if(!foutDebug)
//	{
//		std::cout << "foutDebug open failed." << std::endl;
//		//system("pause");
//		return -1;
//	}
	/*************************************************
	define HOG pyramid to build
	*************************************************/
	if(!ffstrm.is_open())
	{
		std::cout << "open " + filePath + " failed." << std::endl;
		exit(-1);
	}
	std::getline(ffstrm, buf);
	if (buf.empty())
	{
		std::cout << "file " + filePath + " is empty." << std::endl;
		exit(-1);
	}
	//buf = "E:/08.image_store/DPM_Images/101.jpg";
	//Mat origImage = cv::imread(buf);
	//Mat normImage;
	//buf��ݲ���ȷ
	IplImage *origImage = cvLoadImageFromFile(buf.c_str());
//	char jpgFile[] = "E:/work/project/6678_accelerator/code/Images/493.bmp";
//	IplImage *origImage = cvLoadImageFromFile((const char*)jpgFile);
//	IplImage *origImage = cvLoadImageFromFile("E:/work/project/6678_accelerator/code/Images/493.jpg");
	if(origImage == NULL)
	{
		std::cout << "image " << buf << " to initialize HOG is not found." << std::endl;
		exit(-1);
	}
	//�����ͼ���Ǿ������ŵ�
	//IplImage *normImage = cvCreateImage(cvSize(origImage->width / DPM_PRE_SCALE ,origImage->height / DPM_PRE_SCALE ),origImage->depth, origImage->nChannels);
	IplImage *normImage = cvCreateImage(cvSize(origImage->width, origImage->height),origImage->depth, origImage->nChannels);
	//��ʼ��Hog����������ʱ�ĳ�ʼ��ͼ���߾�����HOG�������㷨�����ڴ�����С
	//HOGPyramid pyramid = HOGPyramid(origImage.cols/DPM_PRE_SCALE,origImage.rows/DPM_PRE_SCALE,padx,pady,interval);
	CvSize filterSize = model.getMaxSizeOfFilters();
	HOGPyramid pyramid = HOGPyramid(normImage->width,normImage->height,padx,pady,interval,std::max(filterSize.width,filterSize.height));

	//alloc memory
	g_DPM_memory = new MemAllocDPM();
	/**************************************************
	output root&DPM result
	***************************************************/
#if	RES_RECORD_EN
	ofstream foutDetectFast("dectectFast.txt");
#endif
	do
	{
		procCount++;
		std::cout << buf << std::endl;
//		foutDebug << buf << std::endl;
		//origImage = origImage(Rect(0, origImage.rows - 1200, origImage.cols, 1200));
        //if (!origImage.data)
		if(origImage == NULL)
            continue;
		//std::cout << "Image w = " << origImage.cols << ", h = " << origImage.rows << ::endl;

		//imshow("image", origImage);

        //resize(origImage, normImage, Size(), 1.0f/DPM_PRE_SCALE, 1.0f/DPM_PRE_SCALE);
		//cvResize(origImage, normImage, CV_INTER_LINEAR);
		cvCopy(origImage, normImage);

		static zftdt::DPMVector<Result> fastResults(DPM_MAX_MAXIA);
		fastResults.size = 0;

        //beg = getTickCount();
#if 1
		CvSize size = model.getMaxSizeOfFilters();

		int maxFilterSideLen = std::max(size.width, size.height);
		pyramid.build(normImage, std::max(maxFilterSideLen * DPM_HOG_CELLSIZE, minSideLen), maxLevels);//
        model.detectFast(normImage, minSideLen, maxLevels, pyramid, rootThreshold, threshold, overlap, fastResults);
#else
		model.detectFast(normImage, padx, pady, interval, minSideLen, maxLevels, rootThreshold, threshold, overlap, fastResults);
#endif
        //end = getTickCount();
        timeDetectFast = (end - beg) / freq;
        std::cout <<"fast detection: time = " << timeDetectFast << std::endl;
        accTimeDetectFast += timeDetectFast;

	#if	RES_RECORD_EN
		foutDetectFast << buf << "\t" << timeDetectFast << "\t" << fastResults.size << std::endl;
	#endif
        for (int i = 0; i < fastResults.size; i++)
        {
            printf("fast detection[%d], index = %d, level = %d, score = %f, rect = (%d, %d, %d, %d)\n",
                i, fastResults[i].index, fastResults[i].level, fastResults[i].score,
                fastResults[i].rects[0].x, fastResults[i].rects[0].y,
                fastResults[i].rects[0].width, fastResults[i].rects[0].height);
	#if	RES_RECORD_EN
				foutDetectFast << i
					<< "\t" << fastResults[i].index
					<< "\t" << fastResults[i].level
					<< "\t" << fastResults[i].score
					<< "\t" << fastResults[i].rects.size
					<< "\t" << fastResults[i].rects[0].x
					<< "\t" << fastResults[i].rects[0].y
					<< "\t" << fastResults[i].rects[0].width
					<< "\t" << fastResults[i].rects[0].height;
#endif
            //cv::rectangle(normImage, fastResults[i].rects[0], cv::Scalar(0, 255), 2);
				CvRect rect = fastResults[i].rects[0];
				CvPoint pt1 = cvPoint(rect.x, rect.y);
				CvPoint pt2 = cvPoint(rect.x + rect.width, rect.y + rect.height);
	            cvRectangle(normImage, pt1, pt2, cvScalar(0, 255), 2);
			for (int j = 1; j < fastResults[i].rects.size; j++)
            {
#if	RES_RECORD_EN
                foutDetectFast << "\t" << fastResults[i].rects[j].x
				<< "\t" << fastResults[i].rects[j].y
				<< "\t" << fastResults[i].rects[j].width
				<< "\t" << fastResults[i].rects[j].height;
#endif
                //cv::rectangle(normImage, fastResults[i].rects[j], cv::Scalar(0, 255), 2);
				CvRect rect = fastResults[i].rects[j];
				CvPoint pt1 = cvPoint(rect.x, rect.y);
				CvPoint pt2 = cvPoint(rect.x + rect.width, rect.y + rect.height);
                cvRectangle(normImage, pt1, pt2, cvScalar(0, 255), 2);
            }
#if	RES_RECORD_EN
			foutDetectFast << std::endl;
#endif
        }
#if 1
        std::cout << std::endl;
		cvReleaseImage(&origImage);
		origImage = NULL;
		getline(ffstrm, buf);
		if (buf.empty())
		{
			//origImage.release();
			continue;
		}
		//if(procCount > 2)
		//	break;//test deallocation
        //imshow("image", normImage);
		//cvWaitKey(0);
        //imwrite(string("results/") + buf.substr(buf.find_last_of("\\/") + 1), normImage);
        //waitKey(fullResults.empty() && fastResults.empty() ? 0 : 0);
		//std::cout << "copy constructor of DPMVector is called " << g_copyConsNum << " times" << std::endl;
		//std::cout << "operator = of DPMVector is called " << g_optEqualNum << " times" << std::endl;
		//buf = "E:/08.image_store/DPM_Images/101.jpg";
		//g_copyConsNum = 0;
		//g_optEqualNum = 0;
		//origImage = imread(buf);
		origImage = cvLoadImageFromFile(buf.c_str());
    }while(!ffstrm.eof());
#else
		std::cout << buf << std::endl;
	}while(1);
#endif

#if	RES_RECORD_EN
	foutDetectFast.close();
#endif
	cvReleaseImage(&normImage);
	//cvReleaseImage(&origImage);

    printf("detect time = %f, fast detect time = %f, root detect time = %f\n",
        accTimeDetect / procCount, accTimeDetectFast / procCount, accTimeDetectRoot / procCount);
    ffstrm.close();
	//if(foutDebug.is_open())
	//foutDebug.close();
    //system("pause");
	delete g_DPM_memory;
    return 0;
#endif
}
#endif


