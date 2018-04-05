/*
 * follower.hpp
 *
 *  Created on: Jul 4, 2017
 *      Author: gallantino
 */

#ifndef FOLLOWER_HPP_
#define FOLLOWER_HPP_

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "TypeDefinitions.hpp"
#include <tuple>
#include <vector>
#include <math.h>

class LineFollower{
private:

	cv::Rect _ROI;
	cv::Point _refVec;
	const double _pi = std::atan(1.0) * 4;
	track_t _followedTrack;
	std::vector<track_t> _tracks;
	cv::Point getRectMidPoint(const cv::Rect &);
	int calculateAngel(const cv::Point&);
	void calculateTracks(const std::vector<cv::Rect>&);

public:
	LineFollower(const cv::Mat&);
	int follow(const cv::Mat &, DIRECTION );
	void drawDebugDisplay(cv::Mat&);

};



#endif /* FOLLOWER_HPP_ */
