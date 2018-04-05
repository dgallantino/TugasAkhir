/*
 * DetectorClass.cpp
 *
 *  Created on: Sep 25, 2017
 *      Author: gallantino
 */
#include "DetectorClass.hpp"

/**
 * geometric rules to classify BW image as symbol or not
 * @param symbolCandidate
 * @return true if @param symbolCandidate is a symbol; false otherwise
 */
bool Detector::verifySymbol(const cv::Rect& boundingRect,
                            const std::vector<cv::Point> &contour)
{

	float imageArea = boundingRect.area();
	float extent = (float)cv::contourArea(contour)/imageArea;
	std::vector<cv::Point> hull;
	cv::convexHull(contour, hull);
	float solidity = (float) cv::contourArea(contour)/cv::contourArea(hull);
	float apect = float(boundingRect.width)/float(boundingRect.height);

	return ((extent > 0.3) && (extent < 0.92)) &&
			((solidity > 0.4) && (solidity < 95)) &&
			((apect > 0.30) && (apect < 1.3));

}

//find object area by non-black pixels
bool Detector::verifySymbol(const cv::Mat& symbolImage,
                            const std::vector<cv::Point> &contour)
{
	int whites = cv::countNonZero(symbolImage);
	std::vector<cv::Point> hull;
	cv::convexHull(contour, hull);



	float extent = (float)whites / (symbolImage.rows*symbolImage.cols);
	float solidity = (float) whites / cv::contourArea(hull);
	float apect_s = float(symbolImage.cols)/float(symbolImage.rows);


	return ((extent > 0.3) && (extent < 0.90)) &&
			((solidity > 0.4) && (solidity < 0.95)) &&
			((apect_s > 0.30) && (apect_s < 1.3));
}

/**
 * verify sign object
 * @param inputImage object image
 * @return vector of symbol images if true positive, empty vector if false positive
 */
std::vector<cv::Mat> Detector::verifyObject(cv::Mat& inputImage)
{


	/*
	 * pre proc
	 */
	cv::Mat sign_orig;
	// cv::bilateralFilter(inputImage, sign_orig, 5,300,1);
	// sign_orig.convertTo(sign_orig, CV_8UC1, 1.5, 0);
	// cv::equalizeHist(sign_orig,sign_orig);


	cv::medianBlur(inputImage,sign_orig, 7);					//median blur
	sign_orig.convertTo(sign_orig, CV_16UC1, 3, 0);				//contras with alpha
	double min, max;
	cv::minMaxLoc(sign_orig, &min, &max);						//finding minimum and maximum value for rescaling
	if (min != max)
	{
		sign_orig.convertTo(sign_orig,
		                    CV_8UC1,
		                    255.0/(max-min),
		                    -255.0*min/(max-min));				//rescale intentsity value to 0 - 255
	}

	/*
	 * Segmentation
	 */
	cv::Mat mser_mask = cv::Mat(sign_orig.rows,
	                            sign_orig.cols,
	                            CV_8UC1,
	                            cv::Scalar(0));
	contours_t mser_regions;
	rects_t mser_rects;
	/*
	 * mser detect region
	 */
	_MSERDetectorPtr->detectRegions(	sign_orig,
	                                	mser_regions,
	                                	mser_rects);

	//getting rid all  nested region
	auto cnt_itr = mser_regions.begin();
	for(auto it1 = mser_rects.begin(); it1 != mser_rects.end();)
	{
		auto cnt_itr2 = std::next(cnt_itr);
		bool it1_is_nested = false;
		for(auto it2 = it1+1; it2 != mser_rects.end();)
		{
			if((*it1 & *it2) == *it2)
			{
				it2 = mser_rects.erase(it2);
				cnt_itr2 = mser_regions.erase(cnt_itr2);
			}
			else if((*it1 & *it2) == *it1)
			{
				it1_is_nested = true;
				it2++;cnt_itr2++;
			}
			else
			{
				it2++;cnt_itr2++;
			}
		}
		if(it1_is_nested)
		{
			cnt_itr = mser_regions.erase(cnt_itr);
		}
		else
		{
			cnt_itr++;
		}
		it1++;
	}

	if(mser_rects.empty())
	{
		return std::vector<cv::Mat>();
	}


	for(std::vector<cv::Point> region : mser_regions)
	{//getting mser  masking
		for(cv::Point point : region)
		{
			mser_mask.at<uchar>(point) = 255;
		}
	}
	std::vector<cv::Mat> symbols;
	contours_t mser_contours;
	rects_t accepted_rects;
	cv::findContours(mser_mask, mser_contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	/*
	 * candidate selection by geometric rules
	 */
	for(std::vector<cv::Point> contour : mser_contours)
	{
		cv::Rect bounding_box = cv::boundingRect(contour);

		if(verifySymbol(bounding_box,contour))
		{					//candidate follows the rules
			rects_t::iterator it =  std::lower_bound(accepted_rects.begin(),
			                                         accepted_rects.end(),
			                                         bounding_box,
			                                         [](const cv::Rect &lhs, const cv::Rect &rhs)
			                                         {return lhs.x < rhs.x;});
			accepted_rects.insert(it,bounding_box);
		}

	}


	for(cv::Rect r : accepted_rects){
		try
		{
			cv::Rect outter_bounding_rect = cv::Rect(	r.x-6,
			                                         	r.y-7,
			                                         	r.width+14,
			                                         	r.height+10);
			cv::Mat accepted_symbols = mser_mask(outter_bounding_rect);
			symbols.push_back(accepted_symbols);
		}
		catch (cv::Exception &e)
		{
			// continue;
		}

	}

	if(!symbols.empty())
	{//sign is true positive return vector of symbol images
		return symbols;
		// cv::imwrite("debug/debug_orig"+std::to_string(counter)+".jpg",inputImage);
		// cv::imwrite("debug/debug_segmented"+std::to_string(counter)+".jpg",mser_mask);
		// for (int i = 0; i < symbols.size(); ++i)	
		// 	cv::imwrite("debug/symbol"+std::to_string(counter)+"_"+std::to_string(i)+".jpg",symbols[i]);
		// counter++;

	}
	else
	{//false positive return empty vector
		return std::vector<cv::Mat>();
	}
}

/**
 * update the position of old detected object
 * to tell function detect which area to ignore
 * @param inputFrame current feed from camera
 */
void Detector::tracksigns(cv::Mat& inputFrame)
{
	std::vector<cv::Point2f> current_points;
	std::vector<cv::Point2f> prev_points;
	std::vector<uchar> status;
	cv::Mat err;
	for(Sign sign_obj : _signs)
	{
		prev_points.push_back(sign_obj.midpoint);
	}

	cv::calcOpticalFlowPyrLK(_prevFrame,
	                         inputFrame,
	                         prev_points,
	                         current_points,
	                         status,
	                         err);

	/*
	 * update objs acording to status
	 * if stat = 0;	no flow found	erase objs element
	 * if stat = 1;	flow found		objs.midpoint = current point
	 */
	auto itr = std::begin(_signs);
	for(unsigned int idx = 0;idx < status.size();idx++)
	{
		if(!status[idx])
		{
			itr = _signs.erase(itr);
		}
		else
		{
			itr->update(current_points[idx]);
			++itr;
		}
	}
	_prevFrame = inputFrame.clone();
}

/**
 * main function of this class
 * detect object using cascade classifier
 *  then calls prediction funtion if true positif object detected
 * @param inputFrame input image from camera
 */
int Detector::detect(cv::Mat &inputFrame, bool predict)
{

	/*
	 * do tracking if tracking mode is on
	 */
	if(!_signs.empty())
	{
		tracksigns(inputFrame);
	}


	/*
	 * detect object in input frame
	 */
	rects_t detected_signs;
	_cascadeModel.detectMultiScale(inputFrame,			//observed frame
	                               detected_signs,		//output rectangles
	                               1.1,					//scale factor
	                               3,					//min rect neighbour
	                               0,					//flag
	                               cv::Size(100,100),	//min size
	                               cv::Size(300,300));	//max size
	/*
	 * erase overlaping rects
	 */
	if (!detected_signs.empty() && !_signs.empty())
	{
		auto itr = std::begin(detected_signs);
		for (Sign tracked :  _signs)
		{
			while(itr != std::end(detected_signs))
			{
				if((tracked.boundingRect & *itr).area() > 0)
					itr = detected_signs.erase(itr);
				else
					++itr;
			}
			itr = std::begin(detected_signs);
		}

	}
	

	/*
	 * detect function main process
	 */
	if(!detected_signs.empty())
	{ //sign object detected
		if(predict)
		{ //robot is stoped
			/*
			 * validate each detected sign
			 */
			for(cv::Rect sign : detected_signs)
			{
				cv::Mat sign_mat = inputFrame(sign);
				std::vector<cv::Mat> symbols =  verifyObject(sign_mat);
				if(!symbols.empty())
				{ //object is true positive
					/*
					 * do prediction
					 */
					int prediction =  predictor.predict(symbols);
					_signs.push_back(Sign(sign, prediction));
					_prevFrame = inputFrame;
				}
			}
			return detected_signs.size();
		}
		else
		{ //sign(s) detected and robot is in motion(no prediction asked)
			return detected_signs.size();
		}

	}
	else
	{ //no object detected
		return 0;
	}

}

const signs_t& Detector::getSigns() const
{
	return _signs;
}

Detector::Detector(cv::String cascadeFile)
{
	_cascadeModel.load(cascadeFile);
	_MSERDetectorPtr = cv::MSER::create(5,		//delta 		5
	                                    100,	//min area		60
	                                    1000,	//max area  	14400
	                                    0.15);	//max variation	0.25
}
