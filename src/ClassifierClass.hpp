/*
 * ClassifierFunctions.hpp
 *
 *  Created on: Sep 26, 2017
 *      Author: gallantino
 */

#ifndef CLASSIFIERCLASS_HPP_
#define CLASSIFIERCLASS_HPP_

#ifdef __cplusplus
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/ml.hpp"
#include "opencv2/objdetect.hpp"
#include <iostream>
#endif //include guard

class Predictor{
private:

	cv::Mat getSquareImage(	const cv::Mat& img,
							int target_width);
	cv::Mat HOGFeatureExtractor(const cv::Mat &inputImage);
	void deskewSample(cv::Mat& inputOutputVector);

	cv::HOGDescriptor _HOG;
	cv::Ptr<cv::ml::SVM> _SVMPtr;

public:
	Predictor(cv::String svmfile = "data/SVM_MODEL.yml");
	int predict(std::vector<cv::Mat> &symbols);

};



#endif /* CLASSIFIERCLASS_HPP_ */
