/*
 * DetectorClass.hpp
 *
 *  Created on: Sep 25, 2017
 *      Author: gallantino
 */

#ifndef DETECTORCLASS_HPP_
#define DETECTORCLASS_HPP_

//#ifdef __cplusplus
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/ml.hpp"
#include "opencv2/video.hpp"
#include "ClassifierClass.hpp"
#include "TypeDefinitions.hpp"
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <vector>
//#endif // include guard

class Detector
{
private:
	cv::CascadeClassifier _cascadeModel;
	bool _stopFlag;
	bool _trackingflag;
	cv::Ptr<cv::MSER> _MSERDetectorPtr;

	const cv::String _SVMModelFile = "data/SVM_MODEL.yml";
	Predictor predictor = Predictor(_SVMModelFile);

	cv::Mat _prevFrame;
	signs_t _signs;
	void tracksigns(cv::Mat& inputFrame);
	bool verifySymbol(const cv::Rect &boundingRect, const std::vector<cv::Point> &contour);
	bool verifySymbol(const cv::Mat& symbolImage, const std::vector<cv::Point> &contour);
	std::vector<cv::Mat> verifyObject(	cv::Mat &inputFeed);

	int counter = 0;



public:
	Detector(cv::String cascadeModel ="data/cascadev4-hexagon/cascade.xml");
	int detect(cv::Mat &inputFrame, bool predict = false);
//	bool operator ()(const cv::Mat &inputFrame);

	const signs_t& getSigns() const;
};

#endif /* DETECTORCLASS_HPP_ */
