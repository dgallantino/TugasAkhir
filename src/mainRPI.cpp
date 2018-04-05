//============================================================================
// Name        : LineFollower.cpp
// Author      : dgallantino
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>


#include <iostream>
#include <fstream>
#include <ostream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sstream>

#include "follower.hpp"
#include "GPIOcontrol.hpp"
#include "DetectorClass.hpp"
#include "TypeDefinitions.hpp"

using namespace std;
using namespace std::chrono;

const int font = CV_FONT_HERSHEY_PLAIN;


void cameraBufferFlusher(cv::VideoCapture &cap)
{
	if (cap.isOpened())
		for (int i = 0; i < 7; ++i)
			cap.grab();	
}
int frame_counter(time_point<steady_clock> &timeBegin)
{
	static int framecounter = 0, fps = 0;
	time_point<steady_clock> timeNow = steady_clock::now();
	if (timeNow - timeBegin >= seconds{1})
	{
		/* Do something with the fps in frame_counter */
		fps = framecounter;
		framecounter = 0;
		timeBegin = timeNow;
		return fps;
	}
	else
	{
		++framecounter;
		return fps;
	}
}
static void showUsage(std::string name)
{
    std::cerr << "Usage: " << name << " <option(s)> SOURCES"
              << "Options:\n"
              << "\t-h, \t HELP \tShow this help message\n"
              << "\t-f <file> \t Database input \tSpecify the database file path of available parking spots\n"
              << "\t-m 'parking' \t Manual input \tOne argument needed example '1B 2A'\n"
              << "\t-p \t Parking mode \tPut program in parking mode (this is the default mode)\n"
              << "\t-e \t Exit mode \tPut program in exit mode\n"
              << std::endl;
}

void addParking(const char* parkingNumbers, std::vector<std::pair<int, direction_t>> & parking_spots)
{
    std::istringstream sstream(parkingNumbers);
    std::string s_str;
    while(sstream >> s_str)
    {
        // std::cout<<s_str<<std::endl;
        std::string::size_type sz;
        int temp_int = std::stoi(s_str, &sz);
        direction_t direction;
        switch( s_str[sz])
        {
        	case 'A':
        		direction = DIRECTION::LEFT;
        		std::cout<<"adding "<<temp_int<<"A"<<" to list"<<std::endl;
        		break;
        	case 'B':
        		direction = DIRECTION::RIGHT;
        		std::cout<<"adding "<<temp_int<<"B"<<" to list"<<std::endl;
        		break;
        	default:
        		continue;
        }

        parking_spots.push_back(std::make_pair(temp_int,direction));

    }
}
void readParkingFile(const char* filePath, std::vector<std::pair<int, direction_t> >&parking_spots)
{
    std::ifstream database(filePath);
    std::string str;
    while(std::getline(database,str))
        addParking(str.c_str(), parking_spots);
    database.close();
}

void ledSignalOne( GPIOClass* gpioOne, GPIOClass* gpioTwo, int blink = 3)
{
	std::cout<<"signal one"<<std::endl;
	for(int i = blink; i > 0; --i)
	{
		gpioOne->setval_gpio("1");
		gpioTwo->setval_gpio("1");
		std::this_thread::sleep_for(std::chrono::milliseconds((1000/blink) * i));		
		gpioOne->setval_gpio("0");
		gpioTwo->setval_gpio("0");
		std::this_thread::sleep_for(std::chrono::milliseconds(1000/blink));	
	}
}


bool ctrl_c_pressed = false;
void sig_handler(int sig)
{
	std::cout<<"\nsignal cought"<<std::endl;
    ctrl_c_pressed = true;
}

int main(int argc, char *argv[]) {


	//argument handling
    if (argc < 2) 
    {
        showUsage(argv[0]);
        return 0;
    }
    int opt,e_flag, detect_once;
    detect_once=0;
    e_flag=0;

    std::vector<std::pair<int, direction_t> > parking_spots;

    while( ( opt = getopt (argc, argv, "hpef:m:") ) != -1)
    {
        switch(opt)
        {
            case 'h':
                showUsage(argv[0]);
                return 0;
            case 'p':
                e_flag = 0;
                std::cerr<<"Parking mode"<<std::endl;
                break;
            case 'e':
                e_flag = 1;
                std::cerr<<"Exiting mode"<<std::endl;
                break;
            case 'f':
                if(optarg)
                    readParkingFile(optarg, parking_spots);
                else
                {
                	std::cerr<<"Err : no argument was given for -f parameter"<<std::endl;
                    showUsage(argv[0]);
                    return 0;
                }
                break;
            case 'm':
                if(optarg)
                    addParking(optarg, parking_spots);
                else
                {
                	std::cerr<<"Err : no argument was given for -m parameter"<<std::endl;
                    showUsage(argv[0]);
                    return 0;
                }
                break;
            case '?':
                std::cerr<<"wrong option was chosen"<<std::endl;
                showUsage(argv[0]);
                return 0;
            default :
                std::cerr<<"no option was chosen"<<std::endl;
                showUsage(argv[0]);
                return 0;
        }            
    }
    if (e_flag == 0 && parking_spots.empty())
    {
    	std::cerr <<"no parking spot in the list \n"
    			  <<"exiting..."<<std::endl;

    	showUsage(argv[0]);
    	return -1;
    }
    
	cv::Mat input_feed, input_gray, input_rear, input_rear_gray, masking;

	cv::VideoCapture capture_main(0);
	capture_main.set(CV_CAP_PROP_FRAME_WIDTH,720);
	capture_main.set(CV_CAP_PROP_FRAME_HEIGHT,480);
	// capture_main.set(CV_CAP_PROP_EXPOSURE,0.5);
	if ( !capture_main.isOpened() )
	{
		std::cout << "Cannot open the main cam" << std::endl;
		return -1;
	}
	else
	{
		std::cerr << "warming up main camera ..." << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	} 
	
	cv::VideoCapture capture_rear;
	if (e_flag)
	{
		capture_rear.open(1);
		capture_rear.set(CV_CAP_PROP_FRAME_WIDTH,720);
		capture_rear.set(CV_CAP_PROP_FRAME_HEIGHT,480);
		if ( !capture_rear.isOpened() )
		{
			std::cout << "Cannot open rear cam" << std::endl;
			return -1;
		}
		else
		{
			std::cerr << "warming up rear camera ..." << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			capture_rear>>input_rear;

		}
	}

	capture_main>>input_feed;
	if(input_feed.empty())
	{
		std::cout<<"fail to grab initialization frame"<<std::endl;
	}

	cv::VideoWriter tulisVideo("out.avi",CV_FOURCC('M','J','P','G'),15, cv::Size(input_feed.cols,input_feed.rows));


	// const float roi_x_factor = 0.3;
	// const cv::Size roi_size = cv::Size(input_feed.cols, (int)input_feed.rows*(1-roi_x_factor));
	// const cv::Point roi_pos = cv::Point(0, input_feed.rows*roi_x_factor);
	// cv::Rect ROI = cv::Rect(roi_pos, roi_size);

	Detector detector("data/cascadev4-hexagon/cascade.xml");
	LineFollower tracker(input_feed);
	LineFollower tracker_rear(input_rear);
	FILE * serial_file;
	const std::vector<Sign> &signs = detector.getSigns();	//detected sign objects
  	direction_t direction  = direction_t::STRAIGHT;			//argument for linefollower

  	std::cerr<<"creating signal handler"<<std::endl;
	struct sigaction sig_struct;
    sig_struct.sa_handler = sig_handler;
    sig_struct.sa_flags = 0;
    sigemptyset(&sig_struct.sa_mask);
    if (sigaction(SIGINT, &sig_struct, NULL) == -1) {
        cout << "Problem with sigaction" << endl;
        exit(1);
    }

    std::cerr<<"enabling output pins"<<std::endl;
	//brake control
	GPIOClass* backward_control = new GPIOClass("27");
	backward_control->export_gpio();
	backward_control->setdir_gpio("out");
	if (e_flag)
		backward_control->setval_gpio("1");
	else
		backward_control->setval_gpio("0");

	//brake LED
	GPIOClass* brake_light = new GPIOClass("26");
	brake_light->export_gpio();
	brake_light->setdir_gpio("out");
	brake_light->setval_gpio("0");
	//tracking LED
	GPIOClass* tracking_light = new GPIOClass("19");
	tracking_light->export_gpio();
	tracking_light->setdir_gpio("out");
	tracking_light->setval_gpio("0");

    std::cerr<<"opening serial coms"<<std::endl;
	serial_file = fopen("/dev/serial0", "w");


	std::cerr<<"start"<<std::endl;
	time_point<steady_clock> begin_time = steady_clock::now();

	while(1){
		capture_main>>input_feed;

		if(input_feed.empty()){
			std::cout<<"Image accuisition failed"<<std::endl;
			return -1;
		}
		masking = cv::Mat::zeros(input_feed.rows, input_feed.cols, CV_8UC3);
		cv::cvtColor(input_feed, input_gray, cv::COLOR_RGB2GRAY);
		if(detector.detect(input_gray) > 0){
			// brake_light->setval_gpio("1");
			// std::this_thread::sleep_for(std::chrono::milliseconds(1100));
			// for (int i = 0; i < 7; ++i)
			// {
			// 	capture_main.grab();
			// }
			// capture_main.read(input_feed);
			// cv::imwrite("debug/debug_afterstop.jpg", input_feed);	
			// if(input_feed.empty())
			// {
			// 	std::cout<<"Image accuisition failed"<<std::endl;
			// 	return -1;
			// }
			// cv::cvtColor(input_feed, input_gray, cv::COLOR_RGB2GRAY);
			// detector.detect(input_gray, true);
			// masking = cv::Mat::zeros(input_feed.rows, input_feed.cols, CV_8UC3);

			if(detect_once == 0)
			{
				brake_light->setval_gpio("1");
				std::this_thread::sleep_for(std::chrono::milliseconds(330));
				for (int i = 0; i < 7; ++i)
				{
					capture_main.grab();
				}
				capture_main.read(input_feed);
				cv::imwrite("debug/debug_afterstop.jpg", input_feed);	
				if(input_feed.empty())
				{
					std::cout<<"Image accuisition failed"<<std::endl;
					return -1;
				}
				cv::cvtColor(input_feed, input_gray, cv::COLOR_RGB2GRAY);
				detector.detect(input_gray, true);
				cv::putText(masking, "STOP", cv::Point(200,80), font, 2.5,cv::Scalar(0,0,200), 2);
				if(!signs.empty())
				{
					for (auto s_itr = signs.begin();s_itr != signs.end(); s_itr++)
					{
						cv::rectangle(masking, s_itr->boundingRect, cv::Scalar(0,255,0));
						cv::putText(masking, std::to_string(s_itr->predictedNumber), s_itr->boundingRect.tl(), font, 1,cv::Scalar(0,200,0), 1);
					}
				}
				// masking += input_feed;
				// tulisVideo.write(masking);

			}
		}
		else
		{
	 		if(!signs.empty())
			{
				for (auto s_itr = signs.begin();s_itr != signs.end(); s_itr++)
				{
					cv::rectangle(masking, s_itr->boundingRect, cv::Scalar(0,255,0));
					cv::putText(masking, std::to_string(s_itr->predictedNumber), s_itr->boundingRect.tl(), font, 1,cv::Scalar(0,200,0), 1);
					auto ps_itr = std::find_if(parking_spots.begin(), 
											   parking_spots.end(),
											   [&s_itr](std::pair<int, direction_t>& spot)
											   {return spot.first == s_itr->predictedNumber;});
					if(ps_itr != parking_spots.end())
					{
						direction = ps_itr->second;
						tracking_light->setval_gpio("1");
						break;
					}
					else
					{
						direction = DIRECTION::STRAIGHT;
						tracking_light->setval_gpio("0");

					}

				}
				if(detect_once == 0 && e_flag)
				{
					std::cerr<<"stoping and onward"<<std::endl;
					detect_once = 1;
					backward_control->setval_gpio("0");
				}
			}
		}
		if (e_flag == 1 && detect_once == 0)
		{
			capture_rear>>input_rear;
			cv::cvtColor(input_rear, input_rear, cv::COLOR_RGB2GRAY);
			int angle = tracker_rear.follow(input_rear, direction);
			fprintf(serial_file, "%d\n", angle);
		}
		else
		{
			int angle = tracker.follow(input_gray, direction);
			fprintf(serial_file, "%d\n", angle);
			
		}
		// std::cerr<<framecounter(begin_time)<<std::endl;
		// std::cout << std::left << std::setw(5) << std::setfill(' ') << "\rFPS = " <<frame_counter(begin_time);
		masking += input_feed;
		tracker.drawDebugDisplay(masking);
		tulisVideo.write(masking);
		brake_light->setval_gpio("0");	//turn off brake light

		if(ctrl_c_pressed)//program stopped by ctrl+c signal
		{
			ledSignalOne(brake_light, tracking_light);
			cout << endl;
			cout << "closing capture device ..... " << endl;
			capture_main.release();
			capture_rear.release();
			tulisVideo.release();
			cout << "exiting program ..... " << endl;
			cout << "unexporting pins" << endl;
			backward_control->setval_gpio("0");
			brake_light->setval_gpio("0");
			tracking_light->setval_gpio("0");

			backward_control->unexport_gpio();
			brake_light->unexport_gpio();
			tracking_light->unexport_gpio();

			cout << "deallocating GPIO Objects" << endl;
			delete backward_control;
			delete brake_light;
			delete tracking_light;

			backward_control =0;
			brake_light =0;
			tracking_light =0;
			cout << "closing serial coms" << endl;
			fclose(serial_file);

			break;
		}
	}

	return 0;
}
