/*
 * DPMDetector0.h
 *
 *  Created on: 2015-3-13
 *      Author: LinZW
 */

#ifndef DPMDETECTOR_H_
#define DPMDETECTOR_H_

#define Z_LIB_EXPORT
#include <string>
#include "cv.h"
#include "cxcore.h"
//#include <stdlib.h>
#include <assert.h>

#define FULL_DETECT (0)
#define FAST_DETECT (1)
//����ͼ�������ͼ������ű���
#define DPM_PRE_SCALE (4)
#define DPM_PYRAMID_MAX_LEVELS (20)
#define DPM_PYRAMID_LAMBDA (5)
#define DPM_PYRAMID_MIN_DETECT_STRIDE (20)
#define DPM_HOG_PADX	(1)
#define DPM_HOG_PADY	(1)
//����HOG�����سߴ磬rootΪ1/2
#define DPM_HOG_CELLSIZE (8)
//�������ͼ����
#define DPM_MAX_IN_WIDTH	(1600U)
#define DPM_MAX_IN_HEIGHT	(1264U)
//DPM���ģ�͸���
#define DPM_MAX_MODELS	(2)
//DPMģ������ģ��������
#define DPM_MAX_MODEL_PARTS	(9)
//���ɨ������������
#define DPM_MAX_MAXIA	((DPM_MAX_IN_WIDTH/DPM_HOG_CELLSIZE/4) * (DPM_MAX_IN_HEIGHT/DPM_HOG_CELLSIZE/4) / 40)

#define NbFeatures (32)
//�����޸�vector
#define OPT_LEVEL_VECTOR (1)
//�޸�miture��model��filter size
#define OPT_LEVEL_MAXFLTSIZE (1)
#define OPT_LEVEL_CVMAT	(1)
#define DPM_ALLOC_ALIGN	(2 << 4)
//extern int g_copyConsNum ;
//extern int g_optEqualNum ;

namespace zftdt
{

template <typename _Tp>
struct DPMVector
{
	_Tp *ptr;
	size_t maxNum;
	size_t size;
	typedef _Tp value_type;

	DPMVector(int max)
		:ptr(NULL),maxNum(max),size(0)
	{
		//assert(max > 0);
		if(max > 0)
		{
			ptr = new _Tp[max];
			assert( ptr != NULL);
		}
		else//inialized  empty
		{
			maxNum = 0;
			ptr = NULL;
		}
	}
	DPMVector(const zftdt::DPMVector<_Tp>& Input)//called 0 time when loop
	{
		this->maxNum = Input.maxNum;
		assert(this->maxNum > 0);
		this->ptr = new _Tp[Input.maxNum];
		assert(this->ptr != NULL);
		//std::cout << "entering zftdt::DPMVector copy constructor" << std::endl;
		this->size = Input.size;
		memcpy(this->ptr,Input.ptr,Input.size * sizeof(_Tp));
		//g_copyConsNum++;
	}
	//DPMVector(int max, value_type value)
	//	:ptr(NULL),maxNum(0),size(0)
	//{
	//	assert(max > 0);
	//	maxNum = max;
	//	size = 0;
	//	ptr = new _Tp[max]();
	//	assert( ptr != NULL);

	//	for(int i= 0; i < max; i++)
	//		ptr[i] = value;
	//}
	//DPMVector():ptr(NULL),maxNum(0),size(0){};
	~DPMVector()
	{
		if(ptr != NULL)
		{
			delete[] ptr;
			ptr = NULL;
		}
		size = 0 ;
		maxNum = 0;
	}
	//
	inline void push_back(const _Tp &Input)
	{
		size++;
		assert(size < maxNum);
		ptr[size - 1] = Input;
	}

	inline _Tp& operator[](unsigned index)const
	{
		//assert(size < maxNum && index <= size);
		assert(index < size);
		return *(ptr + index);
	}
	inline DPMVector<_Tp>& operator=(const DPMVector<_Tp> &Input)//called 1-100 times when loop
	{
		assert(this->maxNum >= Input.size);
		//std::cout << "entering zftdt::DPMVector operator=" << std::endl;
		this->size = Input.size;
		memcpy(this->ptr,Input.ptr,Input.size * sizeof(_Tp));
		//g_optEqualNum++;
		return *this;
	}
	zftdt::DPMVector<_Tp>& reAllocate(int max)
	{
		//assert(max > 0);
		this->~DPMVector();
		if(max > 0)
		{
			ptr = new _Tp[max];
			size = 0;
			maxNum = max;
			assert( ptr != NULL);
		}
		else//inialized  empty
		{
			maxNum = 0;
			size = 0;
			ptr = NULL;
		}
		return *this;
	}
};

template <typename _Tp>
struct DPM2DVector
{
	_Tp *ptr;
	size_t maxXNum;
	size_t maxYNum;
	size_t xSize;
	size_t ySize;
	//typedef typename _Tp value_type;

	DPM2DVector(int yMax, int xMax)
		:ptr(NULL),maxXNum(xMax),maxYNum(yMax),xSize(0),ySize(0)
	{
		assert(yMax > 0 && xMax > 0);
		ptr = new _Tp[xMax * yMax]();
		assert( ptr != NULL);
	}

	~DPM2DVector()
	{
		if(ptr != NULL)
		{
			delete[] ptr;
			ptr = NULL;
		}
		maxXNum = 0;
		maxYNum = 0;
		xSize = 0;
		ySize = 0;
	}
	inline _Tp& at(unsigned yIndex, unsigned xIndex)const
	{
		assert(xIndex < xSize && yIndex < ySize);
		return *(ptr + xIndex + maxXNum * yIndex);
	}
};
struct HOGPyramid
{
    //! Build HOG pyramid
	//cyx modify

	HOGPyramid(const int imageW, const int imageH,const int padx,const int pady, const int interval, const int maxFilterSideLen);
	~HOGPyramid();
    /*!
        \param[in] Image image for building HOG pyramid.
        \param[in] padx_ Number of cells padded to the left and right of each HOG level.
        \param[in] pady_ Number of cells padded to the top and bottom of each HOG level.
     */


	Z_LIB_EXPORT void build(const IplImage* image, const int minSideLen, const int maxNumLevels);

    //Z_LIB_EXPORT void build(const cv::Mat& image, const int padx_, const int pady_, const int interval_,
    //    const int minSideLen, const std::vector<int>& selLevels, bool checkValid);
    //! Convolve the filter with each level in the HOG pyramid.
    /*!
        \param[in] filter Filter to be convolved with HOG levels.
        \param[out] result Convolution results, result.size() = levels.size().
     */
    //Z_LIB_EXPORT void convolve(const cv::Mat& filter, std::vector<cv::Mat>& result) const;
    Z_LIB_EXPORT bool isEmpty(void) const;
    int padx, pady, interval;

	DPMVector<CvMat*> levels;
private:
	DPMVector<IplImage*> scaledImage;
	//void *ptr_image;//to hold input image and its resizes
	//void *ptr_levels[DPM_PYRAMID_MAX_LEVELS + 1];//to hold hog features
	//int widthStep[DPM_PYRAMID_MAX_LEVELS + 1];//ptr_levels' step
};
//! ģ�ͼ����
struct Result
{
    int index;       ///< ģ�ͱ��
    int level;       ///< ��ģ�����ڵĲ���
    double score;    ///< ���÷�
    //! ��ģ�ͺͲ���ģ����ͼ�еľ��ο�, rects[0] Ϊ��ģ�͵ľ��ο�, �����Ϊ����ģ�͵ľ��ο�

	zftdt::DPMVector<CvRect> rects;
	Result()
		:index(0),level(0),score(0.0f),rects(DPM_MAX_MODEL_PARTS + 1)
	{
	}
	~Result(){};
};
struct Mixture;
//! �ɱ���ģ��
class DeformablePartModel;

struct Detection;
//Singleton pattern
struct MemAllocDPM
{
	MemAllocDPM();
	~MemAllocDPM();
	//scores��argmaxes��ά���Ѿ���ݼ����HOGPyramid�������Mixture�̶�
	zftdt::DPMVector<IplImage *>& mem_GetScores(const zftdt::HOGPyramid& pyramid, const zftdt::Mixture& mixture);
	zftdt::DPMVector<IplImage *>& mem_GetArgmaxes(const zftdt::HOGPyramid& pyramid, const zftdt::Mixture& mixture);
	zftdt::DPMVector<zftdt::Detection> & mem_GetDetections(void);
private:
	zftdt::DPMVector<IplImage *> scores;
	zftdt::DPMVector<IplImage *> argmaxes;
	zftdt::DPMVector<zftdt::Detection> detections;
};

extern zftdt::MemAllocDPM *g_DPM_memory;

class DeformablePartModel
{
public:
    //! ���캯��, ��ģ���ļ�����ģ��
    /*!
        \param[in] modelPath ģ���ļ���·��
     */
    Z_LIB_EXPORT DeformablePartModel(const std::string& modelPath);
    //! �ж�ģ���Ƿ�Ϸ�
    /*!
        \return ���ģ�ͷǿ�, ���Ҷ����ڸ�ģ�ͺͲ���ģ��, �򷵻� true, ���򷵻� false
     */
    Z_LIB_EXPORT bool isValid(void) const;
    //! ȫģ��ȫ��⺯��
    /*!
        \param[in] image ����ͼƬ
        \param[in] padx ���� HOG ����ͼʱ�������Ҳ����Ŀհ�����������, ������� 0
        \param[in] pady ���� HOG ����ͼʱ���Ϸ����·����Ŀհ�����������, ������� 0
        \param[in] interval ���� HOG ����������ʱ��������ֱ��ʵ�ͼƬ�м��㼸�㲻ͬ�߶ȵ� HOG ����
        \param[in] minSideLen ���� HOG ����������ʱ��С image ����С�߳�
        \param[in] maxNumLevels ���� HOG ������������������,
                                ����� minSideLen ����õ��Ľ���������� maxNumLevels С, ����� minSideLen �õ��Ľ���������
        \param[in] thresh ���ֵ��ֵ, ���ڸ���ֵ������������п������
        \param[in] overlap �������������� image �е��ص�����������ֵ, ���ֵ��С�Ľ������
        \param[out] results �����
        \return true ����ִ�гɹ�, false ����ִ��ʧ��
     */

    //! ���ټ�⺯��, ���ø�ģ�ͼ��ֵ��λ�����λ��, ���ò���ģ�ͼ��ֵ���������λ��
    /*!
        \param[in] image ����ͼƬ
        \param[in] padx ���� HOG ����ͼʱ�������Ҳ����Ŀհ�����������, ������� 0
        \param[in] pady ���� HOG ����ͼʱ���Ϸ����·����Ŀհ�����������, ������� 0
        \param[in] interval ���� HOG ����������ʱ��������ֱ��ʵ�ͼƬ�м��㼸�㲻ͬ�߶ȵ� HOG ����
        \param[in] minSideLen ���� HOG ����������ʱ image ��С����С�߳�
        \param[in] maxNumLevels ���� HOG ������������������
        \param[in] rootThresh ��ģ�ͼ��ֵ��ֵ, ���ڸ���ֵ���������������һ�ּ��
        \param[in] fullThresh ȫģ�ͼ��ֵ��ֵ, ���ڸ���ֵ������������п������
        \param[in] overlap �������������� image �е��ص�����������ֵ, ���ֵ��С�Ľ������
        \param[out] results �����
        \return true ����ִ�гɹ�, false ����ִ��ʧ��
     */

	Z_LIB_EXPORT bool detectFast(const IplImage* image, int minSideLen, int maxNumLevels, const HOGPyramid& pyramid,
    double rootThresh, double fullThresh, double overlap, DPMVector<Result>& results) const;

    //! ȫģ��ȫ��⺯��
    /*!
        \param[in] image ����ͼƬ
        \param[in] padx ���� HOG ����ͼʱ�������Ҳ����Ŀհ�����������, ������� 0
        \param[in] pady ���� HOG ����ͼʱ���Ϸ����·����Ŀհ�����������, ������� 0
        \param[in] interval ���� HOG ����������ʱ��������ֱ��ʵ�ͼƬ�м��㼸�㲻ͬ�߶ȵ� HOG ����
        \param[in] minSideLen ���� HOG ����������ʱ��С image ����С�߳�
        \param[in] rootLevels ��ģ�����ڵ� HOG �����������Ĳ���,
                              ����� minSideLen ����õ��Ľ���������� rootLevels�е����ֵҪС, ����� minSideLen �õ��Ľ���������
        \param[in] thresh ���ֵ��ֵ, ���ڸ���ֵ������������п������
        \param[in] overlap �������������� image �е��ص�����������ֵ, ���ֵ��С�Ľ������
        \param[out] results �����
        \return true ����ִ�гɹ�, false ����ִ��ʧ��
     */

    //! ���ټ�⺯��, ���ø�ģ�ͼ��ֵ��λ�����λ��, ���ò���ģ�ͼ��ֵ���������λ��
    /*!
        \param[in] image ����ͼƬ
        \param[in] padx ���� HOG ����ͼʱ�������Ҳ����Ŀհ�����������, ������� 0
        \param[in] pady ���� HOG ����ͼʱ���Ϸ����·����Ŀհ�����������, ������� 0
        \param[in] interval ���� HOG ����������ʱ��������ֱ��ʵ�ͼƬ�м��㼸�㲻ͬ�߶ȵ� HOG ����
        \param[in] minSideLen ���� HOG ����������ʱ image ��С����С�߳�
        \param[in] rootLevels ��ģ�����ڵ� HOG �����������Ĳ���,
                              ����� minSideLen ����õ��Ľ���������� rootLevels�е����ֵҪС, ����� minSideLen �õ��Ľ���������
        \param[in] rootThresh ��ģ�ͼ��ֵ��ֵ, ���ڸ���ֵ���������������һ�ּ��
        \param[in] fullThresh ȫģ�ͼ��ֵ��ֵ, ���ڸ���ֵ������������п������
        \param[in] overlap �������������� image �е��ص�����������ֵ, ���ֵ��С�Ľ������
        \param[out] results �����
        \return true ����ִ�гɹ�, false ����ִ��ʧ��
     */

	//return max filters' width or height
	Z_LIB_EXPORT CvSize getMaxSizeOfFilters(void) const;
private:
    DeformablePartModel(const DeformablePartModel&);
    DeformablePartModel& operator=(const DeformablePartModel&);
	Mixture* mixture;
	//HOGPyramid pyramid;
};


}

CvRect rectOverlap(const CvRect &a, const CvRect &b);
int fw_matrix_32FC1(const CvMat *src, const char* name);
int fw_matrix_8UC3(const IplImage *src, const char* name);
int copyMatWithRoi(const CvMat *src, CvMat *dst, const CvRect &sRoi, const CvRect &dRoi);
IplImage* cvLoadImageFromFile(const char* filename, int flags = 0);
IplImage* cvLoadImageFromArray( const char* filedata, int flags);
#endif /* DPMDETECTOR0_H_ */
