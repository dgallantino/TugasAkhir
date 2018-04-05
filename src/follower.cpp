/*
 * follwer.cpp
 *
 *  Created on: Jul 4, 2017
 *      Author: gallantino
 */

#include "follower.hpp"

LineFollower::LineFollower(const cv::Mat& inputFeed)
{
	double roi_posYpercent = 0.7;
	double roi_heightpercent = 0.05;
	cv::Size roi_size = cv::Size(inputFeed.cols, (int)(inputFeed.rows * roi_heightpercent));
	cv::Point roi_position = cv::Point(0, (int)((inputFeed.rows * roi_posYpercent) - roi_size.height));
	_ROI = cv::Rect(roi_position, roi_size);
	// _refVec = cv::Point(inputFeed.cols/2, inputFeed.rows);
	_refVec = cv::Point(inputFeed.cols/2, inputFeed.rows + (int)(inputFeed.rows*0.25));
	_followedTrack = std::make_tuple(_ROI, getRectMidPoint(_ROI), 90, DIRECTION::STRAIGHT);
}

cv::Point LineFollower::getRectMidPoint(const cv::Rect& rect_)
{
	return cv::Point(rect_.x + rect_.width/2, rect_.y + rect_.height/2);
}

/**
 * calculate the 2D vector angel of @param point_
 * with _refVec as the the point of origin
 * instead of the orginila (0,0) point on top left corner of the image
 * @param point_ 2D vector
 * @return angle theta
 */
int LineFollower::calculateAngel(const cv::Point& point_)
{
	double normalize_x = point_.x - _refVec.x;
	double normalize_y = _refVec.y - point_.y;
	return atan2(normalize_y, normalize_x)/_pi*180;
}

/**
 * labeling detected track(s) as direction by its angle
 * @param rects	bounding rectangle of detected tracks
 */
void LineFollower::calculateTracks(const std::vector<cv::Rect>& rects)
{
	std::vector<track_t> temp_tracks;
	//initializing vetor of tracks while ignoring the direction lable
	for(cv::Rect rect : rects)
	{
		cv::Point temp_point = getRectMidPoint(rect);
		int temp_angle = calculateAngel(temp_point);
		track_t temp_track = std::make_tuple(rect,
		                                     temp_point,
		                                     temp_angle,
		                                     DIRECTION::STRAIGHT);
		temp_tracks.push_back(temp_track);
	}
	//labeling the 'straight' track
	auto straight_it = std::min_element(temp_tracks.begin(),
	                                    temp_tracks.end(),
	                                    [](const track_t &lhs, const track_t &rhs)
	                                    {return abs(std::get<2>(lhs) - 90) < abs(std::get<2>(rhs)-90);});
	std::get<3>(*straight_it) = DIRECTION::STRAIGHT;
	int straight_angle = std::get<2>(*straight_it);

	//labeling the other tracks
	//using the straight track as refrence
	for(auto itr_1 = temp_tracks.begin(); itr_1 != temp_tracks.end();itr_1++)
	{
		if(itr_1 == straight_it)
			continue;
		int itr_1_angle = std::get<2>(*itr_1);
		if(itr_1_angle > straight_angle)
			std::get<3>(*itr_1) = DIRECTION::LEFT;
		else
			std::get<3>(*itr_1) = DIRECTION::RIGHT;
	}
	_tracks = temp_tracks;
}

/**
 * main function of this class
 * process the input feed to detect line follower track(s)
 * calculate track(s) trajectory and chose which track to follow
 * @param inputFeed	image from camera feed
 * @param direction	which direction to follow
 * @return	angle of the chosen track to follow
 */
int LineFollower::follow(const cv::Mat &inputFeed, DIRECTION direction)
{
	//pre-processing
	cv::Mat processed;
	cv::GaussianBlur(inputFeed(_ROI), processed, cv::Size(5,5), 2);
    cv::threshold(processed, processed, 50, 255, cv::THRESH_BINARY_INV);
 //    cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(5,5));
	// cv::morphologyEx(processed, processed, cv::MORPH_OPEN, element);
	contours_t contours;
	cv::findContours(processed, contours, cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);

	rects_t tracking_rects;
	if (!contours.empty())
	{
		for(std::vector<cv::Point> contour : contours)
		{
			double area = cv::contourArea(contour);
			if (area < 15000 && area > 100)
			{
				cv::Rect temp_rect = cv::boundingRect(contour);
				temp_rect = cv::Rect(temp_rect.x + _ROI.x,
				                     temp_rect.y + _ROI.y,
				                     temp_rect.width,
				                     temp_rect.height);
				tracking_rects.push_back(temp_rect);
			}
		}
		if(!tracking_rects.empty())
		{
			calculateTracks(tracking_rects);
			auto found_it = std::find_if(_tracks.begin(),
			                             _tracks.end(),
			                             [&direction](const track_t &a)
			                             {return std::get<3>(a) == direction;});
			if (found_it != _tracks.end())
				_followedTrack = *found_it;
			else
				_followedTrack = *std::find_if(_tracks.begin(),
				                               _tracks.end(),
				                               [](const track_t &a)
				                               {return std::get<3>(a) == DIRECTION::STRAIGHT;});
		}
	}
	else
	{
		return 255;
	}
	return int(std::get<2>(_followedTrack));
}

void LineFollower::drawDebugDisplay(cv::Mat& inputOutputArr)
{
	const cv::Scalar arr_color[2] = {{255,0,0}, {0,0,255}};
	cv::Scalar color;

	const int font = CV_FONT_HERSHEY_DUPLEX, font_thick = 1;
	const double font_scale = 1;
	cv::Size txt_size = cv::getTextSize("SOME.. BODY", font, font_scale, font_thick, NULL);
	cv::putText(inputOutputArr,
	            "FOLLOW",
	            cv::Point(inputOutputArr.cols - txt_size.width, txt_size.height*2),
	            font,
	            font_scale,
	            arr_color[0],
	            font_thick);
	cv::putText(inputOutputArr,
	            "IGNORE",
	            cv::Point(inputOutputArr.cols - txt_size.width, txt_size.height*3+5),
	            font,
	            font_scale,
	            arr_color[1],
	            font_thick);

	for(track_t track : _tracks)
	{
		if(track == _followedTrack)
			color = arr_color[0];
		else
			color = arr_color[1];

		cv::arrowedLine(inputOutputArr, _refVec, std::get<1>(track), color, 1);
		cv::rectangle(inputOutputArr, std::get<0>(track), color, 2);
		cv::putText(inputOutputArr,
		            std::to_string(std::get<2>(track)),
		            std::get<1>(track) - cv::Point(0,30),
		            font,
		            font_scale,
		            color,
		            font_thick);
	}
}
