/*
 * ClassifierFunctions.cpp
 *
 *  Created on: Sep 26, 2017
 *      Author: gallantino
 */

#include "ClassifierClass.hpp"


void Predictor::deskewSample(cv::Mat& inputOutputVector) {
	int sz = 28;

	cv::Moments moment = cv::moments(inputOutputVector);

	if (std::abs(moment.mu02) < 1e-2)
	{

	}
	else
	{

		float skew = moment.mu11/moment.mu02;
		float skew2 =-0.5*sz*skew;
		float fM[6] = {1,skew,skew2,0,1,0};

		cv::Mat M = cv::Mat(2,3,CV_32F,fM);

		cv::warpAffine(inputOutputVector, inputOutputVector, M, cv::Size(sz,sz), cv::WARP_INVERSE_MAP);
	}

}

cv::Mat Predictor::getSquareImage(const cv::Mat& img, int target_width)
{

	int width = img.cols,height = img.rows;

	cv::Mat square = cv::Mat::zeros( target_width, target_width, img.type() );

	int max_dim = ( width >= height ) ? width : height;
	float scale = ( ( float ) target_width ) / max_dim;
	cv::Rect roi;
	if ( width >= height )
	{
		roi.width = target_width;
		roi.x = 0;
		roi.height = height * scale;
		roi.y = ( target_width - roi.height ) / 2;
	}
	else
	{
		roi.y = 0;
		roi.height = target_width;
		roi.width = width * scale;
		roi.x = ( target_width - roi.width ) / 2;
	}

	cv::resize( img, square( roi ), roi.size() );

	return square;
}

cv::Mat Predictor::HOGFeatureExtractor(const cv::Mat &inputImage)
{

	std::vector<float> descriptor;
	_HOG.compute(inputImage, descriptor);
	cv::Mat descriptorMat(1,descriptor.size(), CV_32FC1, descriptor.data());

	return descriptorMat;
}

int Predictor::predict(std::vector<cv::Mat> &symbols)
{

	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3));
	int result = 0;
	int counter =0;
	for(cv::Mat temp_symbol : symbols)
	{
		// cv::imwrite("debug/"+std::to_string(++counter)+".jpg", temp_symbol);
//		cv::dilate(temp_symbol, temp_symbol, element,cv::Point(0,0),1);
		cv::erode(temp_symbol, temp_symbol, element,cv::Point(0,0),1);
		cv::GaussianBlur(temp_symbol, temp_symbol, cv::Size(3,3), 0.3, 0.3);
		temp_symbol = getSquareImage(temp_symbol, 28);
		deskewSample(temp_symbol);

		cv::Mat feature_vector = HOGFeatureExtractor(temp_symbol);

		int tempInt = _SVMPtr->predict(feature_vector);

		result = result * 10 + tempInt;

		// cv::imwrite(std::to_string(++counter)+"_"+std::to_string(tempInt)+".jpg", temp_symbol);
	}
	std::cout<<result<<std::endl;
	return result;

}

Predictor::Predictor(cv::String svmfile)
{
	this->_SVMPtr = cv::ml::SVM::load(svmfile);
	this->_HOG = cv::HOGDescriptor(cv::Size(28,28), 					//winSize
	                              cv::Size(14,14), 					//blocksize
	                              cv::Size(7,7), 					//blockStride,
	                              cv::Size(7,7), 					//cellSize,
	                              9, 								//nbins,
	                              1, 								//derivAper,
	                              -1, 								//winSigma,
	                              cv::HOGDescriptor::L2Hys, 			//histogramNormType,
	                              0.2, 								//L2HysThresh,
	                              false,									//gamma correction,
	                              cv::HOGDescriptor::DEFAULT_NLEVELS,	//nlevels=64
	                              1);								//Use signed gradients

}

