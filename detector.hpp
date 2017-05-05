//
//  detector.hpp
//  signdetector
//
//  Created by Dimas Gallantino on 5/4/17.
//  Copyright © 2017 Dimas Gallantino. All rights reserved.
//
//  
//

#ifndef detector_hpp
#define detector_hpp

#ifdef __cplusplus
#  include "opencv2/core.hpp"
#  include "opencv2/imgproc.hpp"
#  include "opencv2/highgui.hpp"
#endif

#include <stdio.h>

class SignDetector{
private:
    int detectionLine, detectionLineBuffer;
        
    int cannyTreshold;
    cv::Size morphKsize;
    cv::Size gaussKsize;
    double gaussSigmaX;
    
    std::vector<cv::Point> centroid;
    cv::Mat detectedSign;
    
    int signSizeUpper,signSizeLower;
        
    cv::Mat preProcess(const cv::Mat & inputFeed);

        
public:
    SignDetector();
    void setDetectionLine(int,int);
    void setSignSize(int signArea, int bufferArea);
    void setPreProcParams(cv::Size morphKernel, cv::Size blurKernel, double blurSigmaX, int cannyTresh);
        
        
    bool detect(const cv::Mat & inputFeed);
    
        
    std::vector<cv::Point> getCentroid();
    cv::Mat getDetectedSign();
};


#endif /* detector_hpp */
