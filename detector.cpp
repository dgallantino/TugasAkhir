//
//  detector.cpp
//  signdetector
//
//  Created by Dimas Gallantino on 5/4/17.
//  Copyright © 2017 Dimas Gallantino. All rights reserved.
//

#include "detector.hpp"

SignDetector::SignDetector(){
    
}

void SignDetector::setPreProcParams(cv::Size morphKernel, cv::Size blurKernel, double blurSigmaX, int cannyTresh){
    cannyTreshold = cannyTresh;
    gaussKsize = blurKernel;
    gaussSigmaX = blurSigmaX;
    morphKsize = morphKernel;
}

void SignDetector::setDetectionLine(int detectionYAxis, int detetionYAxisBuffer){
    detectionLine = detectionYAxis;
    detectionLineBuffer = detetionYAxisBuffer;
}

void SignDetector::setSignSize(int signArea, int bufferArea){
    signSizeUpper = signArea + bufferArea;
    signSizeLower = signArea - bufferArea;
}

cv::Mat SignDetector::preProcess(const cv::Mat &inputFeed){
    
    cv::Mat outputFeed;
    
    cv::cvtColor(inputFeed,
                 outputFeed,
                 cv::ColorConversionCodes::COLOR_RGB2GRAY);

    
//    cv::GaussianBlur(outputFeed,
//                     outputFeed,
//                     gaussKsize,
//                     gaussSigmaX);
    
    cv::Canny(outputFeed,
              outputFeed,
              cannyTreshold,
              cannyTreshold*3);
    
    
    cv::morphologyEx(outputFeed,
                     outputFeed,
                     cv::MorphTypes::MORPH_CLOSE,
                     cv::getStructuringElement(cv::MorphShapes::MORPH_ELLIPSE, morphKsize));
    
    return outputFeed;

    
}

bool SignDetector::detect(const cv::Mat & inputFeed){
    cv::Mat preProcessed = SignDetector::preProcess(inputFeed);
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Point> approx;
    
    
    cv::findContours(preProcessed, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    
    for (std::vector<cv::Point> contour : contours) {
        if ((cv::contourArea(contour) < signSizeUpper) & (cv::contourArea(contour) > signSizeLower)) {
            cv::approxPolyDP(contour, approx, cv::arcLength(contour, true)*0.1, true);
            if (approx.size() == 4 ) {
                cv::Moments moments = cv::moments(approx, false);
                cv::Point center = cv::Point(moments.m10/moments.m00, moments.m01/moments.m00);
//                centroid.emplace_back(center);
                
                if ((center.y > detectionLine) & (center.y < detectionLineBuffer)) {
                    cv::Rect r = cv::boundingRect(approx);
                    detectedSign = inputFeed(r);
                    return true;

                }
            }
        }
    }
    
    return false;
    
}

cv::Mat SignDetector::getDetectedSign(){
    return detectedSign;
}

std::vector<cv::Point> SignDetector::getCentroid(){
    return centroid;
}

