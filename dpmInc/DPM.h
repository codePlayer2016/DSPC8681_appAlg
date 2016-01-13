/*
 * DPM.h
 *
 *  Created on: 2015-3-13
 *      Author: LinZW
 */

#ifndef DPM_H_
#define DPM_H_

#include <iostream>
#include "HOG.h"
#include "DPMDetector.h"
//#include <fftw3.h>
#include <set>
#include <list>

#define FFTW_ENABLE (1)//TODO2.0->should modify fftw to dsp fft


namespace zftdt
{

//! Model definition
struct Model
{
    /*!
        Get the convolutions of root filter and each possible pyramid level.
        \param[in] pyramid HOG pyramid.
        \param[out] convolutions Root filter scores at each pyramid level,
                                 scores.size() = pyramid.levels.size(),
                                 because root will not locate at the first pyramid.interval levels,
                                 scores[0] to scores[pyramid.interval] should not be used.
     */

    /*!
        Get the convolutions of pyramid levels and model parts.
        \param[in] pyramid HOG pyramid.
        \param[out] convolutions Root filter scores at each pyramid level,
                                 the size of which is parts.size() x pyramid.levels.size(),
                                 because root will not locate at the first pyramid.interval levels,
                                 scores[0] to scores[pyramid.interval] should not be used.
     */

    /*!
        Given the root filter level and its top left corner at the root level,
        find the best locations of each part filter that maximize the convolution score.
        \param[in] pyramid HOG pyramid.
        \param[in] rootLevel Root filter level index in pyramid.levels.
        \param[in] rootTopLeft Root filter position in pyramid.levels[rootLevel].
        \param[out] partTopLefts Each part filter top left position at pyramid.levels[rootLevel - pyramid.interval],
                                 that maximize its convolution score.
        \return The sum of all the part filter scores.
     */
	Z_LIB_EXPORT float findBestParts(const HOGPyramid& pyramid,
        int rootLevel, const CvPoint& rootTopLeft, zftdt::DPMVector<CvPoint>& partTopLefts) const;
    /*!
        Given the convolutions of pyramid levels and model parts,
        find the best locations of each part filter that maximize score plus cost,
        and add them to the corresponding root location.
        \param[in] pyramid HOG pyramid
        \param[in,out] convolutions Convolutions of pyramid levels and model parts,
                                    the size of which is parts.size() x pyramid.levels.size(),
                                    this param will be modified and transfered to the next param.
        \param[out] scores Scores at each root location, size of which is pyramid.levels.size(),
                           because root will not locate at the first pyramid.interval levels,
                           scores[0] to scores[pyramid.interval] should not be used.
        \param[out] positions Part filter top left corner positions (at the part filter level)
                              that make the score of each root location's score largest,
                              type: CV_32SC2, size: parts.size() x pyramid.levels.size()
     */
    /*!
        Get the max width and height among all the filters.
     */
    //! Part definition
    struct Part
    {
        CvMat *filter;//cv::Mat filter;          ///< Part filter
        CvPoint offset;        ///< Part filter top left relative to the root top left at the parts level.
        float deformation[4];//cv::Vec4f deformation;   ///< Deformation cost, def(0) * x^2 + def(1) * x + def(2) * y^2 + def(3) * dy.
        enum {halfSize = 4};
        float cost[halfSize * 2 + 1][halfSize * 2 + 1];  ///< Pre-computed cost
    };

	DPMVector<Part> parts;     ///< Root and parts information, parts[0] is the root.
	//Model(int maxParts);
    float bias;                  ///< Value added after convolution between parts and level.
#if OPT_LEVEL_MAXFLTSIZE
	CvSize maxSizeOfFilters;
#endif
	Model();
	~Model();
};

//! Serializes a model to a stream.
Z_LIB_EXPORT std::ostream & operator<<(std::ostream & os, const Model & model);

//! Unserializes a model from a stream.
Z_LIB_EXPORT std::istream & operator>>(std::istream & is, Model & model);

//! Mixture of models
struct Mixture
{
	friend struct MemAllocDPM;
    /*!
        Get the max scores at each possible pyramid level, among all the models.
        \param[in] pyramid HOG pyramid.
        \param[out] scores Max total scores, containing root filter and part filters,
                           at each pyramid level, among all the models,
                           scores.size() = pyramid.levels.size(),
                           because root will not locate at the first pyramid.interval levels,
                           scores[0] to scores[pyramid.interval] should not be used.
        \param[out] argMaxes Model indexes that make the score largest, argMaxes.size() = models.size()
        \param[out] positions Part filter top left corner positions (at the part filter level)
                              that make the score of each root location's score largest,
                              type: CV_32SC2, size: numModels x numParts x numLevels.
     */

    /*!
        Get the max root filter scores at each possible pyramid level, among all the models.
        \param[in] pyramid HOG pyramid
        \param[out] scores Max root filter scores,
                           at each pyramid level, among all the models,
                           scores.size() = pyramid.levels.size(),
                           because root will not locate at the first pyramid.interval levels,
                           scores[0] to scores[pyramid.interval] should not be used.
        \param[out] argMaxes Model indexes that make the score largest, argMaxes.size() = models.size().
     */

	Z_LIB_EXPORT void getRootLevelsRootScores(const HOGPyramid& pyramid, zftdt::DPMVector<IplImage*>& scores, zftdt::DPMVector<IplImage*>& argMaxes) const;

    /*!
        Get the max width and height among all the models' root filters and part filters.
     */
	DPMVector<Model> models;   ///< All the models
	//Mixture(int maxModels);
	CvSize maxSizeOfFilters;
	Mixture();
	~Mixture();
};

//! Serializes a mixture to a stream.
Z_LIB_EXPORT std::ostream & operator<<(std::ostream & os, const Mixture & mixture);

//! Unserializes a mixture from a stream.
Z_LIB_EXPORT std::istream & operator>>(std::istream & is, Mixture & mixture);

struct Detection
{

	Detection(void)
        : index(0), score(0), level(0),levelRects(DPM_MAX_MODEL_PARTS + 1),imageRects(DPM_MAX_MODEL_PARTS + 1) {}
    //Detection(int index_, float score_, int level_, const cv::Rect& rootLevelRect_, const cv::Rect& rootImageRect_)
    //    : index(index_), score(score_), level(level_),levelRects(DPM_MAX_MODEL_PARTS),imageRects(DPM_MAX_MODEL_PARTS)
    //{
    //    levelRects[levelRects.size++] = rootLevelRect_;
    //    imageRects[imageRects.size++] = rootImageRect_;
    //}
    int index;     ///< Model index
    float score;   ///< Convolution score
    int level;     ///< Root filter level
    //! Root filter and part filters positions in the hog pyramid level.
    /*!
        levelRects[0] is the root filter position in level,
        levelRects[i] (i = 1, 2, ...) are the part filter positions in level - interval.
     */
	zftdt::DPMVector<CvRect> levelRects;
    //! Root filter and part filters positions in the input image.
    /*!
        imageRects[0] is the root filter rect scaled to the image,
        imageRects[i] (i = 1, 2, ...) are the part filter rects scaled to the image.
     */

	zftdt::DPMVector<CvRect> imageRects;
	zftdt::Detection& assign(int index_, float score_, int level_, const CvRect& rootLevelRect_, const CvRect& rootImageRect_)
    {
        this->index = index_;
		this->score = score_;
		this->level = level_;
		this->levelRects.size = 1;
		this->imageRects.size = 1;
		this->levelRects[0] = rootLevelRect_;
        this->imageRects[0] = rootImageRect_;
		return *this;
    }
	Detection& operator=(Detection& in)
	{
        this->index = in.index;
		this->score = in.score;
		this->level = in.level;
		this->levelRects = in.levelRects;
        this->imageRects = in.imageRects;
		return *this;
	}
    bool operator <(const Detection & detection) const
	{
		return score > detection.score;
	}
    operator CvRect() const
    {
		return (imageRects.size == 0) ? cvRect(0,0,0,0) : imageRects[0];
    }
};

Z_LIB_EXPORT void detectFast(const Mixture & mixture, int width, int height, const HOGPyramid & pyramid,
    double rootThreshold, double fullThreshold, double overlap, zftdt::DPMVector<Detection> & detections);

class ConvMixtureFastHelper
{
public:

	static const zftdt::DPM2DVector<CvMat *>& run(const HOGPyramid& pyramid, const Mixture& mixture);
	const zftdt::DPM2DVector<CvMat *>& execute(const HOGPyramid& pyramid, const Mixture& mixture) const;
	//const zftdt::DPM2DVector<IplImage *>& getConvMixtureScores(void) const;
	static const ConvMixtureFastHelper* getHelper(const HOGPyramid& pyramid, const Mixture& mixture);
    ~ConvMixtureFastHelper(void);
private:
    ConvMixtureFastHelper(const HOGPyramid& pyramid, const Mixture& mixture);
    const Mixture* ptrMixture;

	zftdt::DPMVector<CvSize> levelSizes;
	zftdt::DPMVector<unsigned char> validLevelMask;
	//zftdt::DPMVector<cv::Mat> filtersFFT;
	zftdt::DPMVector<CvMat*> filtersFFT;
	zftdt::DPMVector<int> idxMapRectToLevel;
	zftdt::DPMVector<std::pair<CvRect, int> > mergeRects;

	zftdt::DPM2DVector<CvMat *> scores;
#if FFTW_ENABLE
//    fftwf_plan forward;
//    fftwf_plan inverse;
#endif
    int maxRows, maxCols;
    float normVal;

	CvMat *levelFFT;
    CvMat *prodFFT;
	CvMat *prod;

    int numPlanes;
    enum {MAX_ACTIVE_COUNT = 64};
    int activeCount;

	typedef std::list<ConvMixtureFastHelper*> HelperArr;

    static HelperArr arr;
};

}

#endif /* DPM0_H_ */
