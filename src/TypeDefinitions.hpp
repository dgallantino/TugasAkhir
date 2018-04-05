/*
 * TypeDefinitions.hpp
 *
 *  Created on: Oct 9, 2017
 *      Author: gallantino
 */

#ifndef TYPEDEFINITIONS_HPP_
#define TYPEDEFINITIONS_HPP_
#ifdef __cplusplus
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/ml.hpp"
#include "opencv2/video.hpp"
#include <vector>
#endif // include guard


typedef enum class DIRECTION : uchar
{
	STRAIGHT,
	RIGHT,
	LEFT
} direction_t, *direction_p;


struct Sign{
	cv::Point2f midpoint;
	cv::Rect boundingRect;
	int predictedNumber;
	Sign(){}
	Sign(cv::Rect &rec,
		 int prediction):
			 boundingRect(rec), predictedNumber(prediction){
		midpoint = cv::Point2f(rec.x + rec.width/2, rec.y + rec.width/2);

	}
	void update(cv::Point2f pt){
		midpoint = pt;
		boundingRect = cv::Rect(pt.x - boundingRect.width/2,
								pt.y - boundingRect.height/2,
								boundingRect.width,
								boundingRect.height);
		}
};

typedef std::vector<cv::Rect> rects_t, *rects_p;
typedef std::vector<std::vector<cv::Point>> contours_t, *contours_p;
typedef std::vector<Sign> signs_t;
typedef std::tuple<cv::Rect, cv::Point, int ,DIRECTION> track_t, *track_p;



#endif /* TYPEDEFINITIONS_HPP_ */
