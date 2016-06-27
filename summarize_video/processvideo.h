#pragma once
#include<iostream>
#include<fstream>
#include<stdlib.h>
#include<vector>
#include<string>
#include<algorithm>
#include<functional>
#include<math.h>
#include<map>

using namespace std;

class processvideo
{
	//ImagePath
	string framepath;
	//height width of input video frames
	int h;
	int w;
	//Storing Y channel of every input frame
	vector<double **> yframes;	

	//Storing Subsequent frame difference i & i-1 frame by reconstructing ith frame using i-1th frame 
	vector<double> result_frame_diff;

	//Avg. color value for every frame in video
	vector<double> avg_frame_color;

	//Avg frame color value per second
	map<int, double> avg_frame_color_sec;

	//Reconstructed frame using prediction from previous frame
	double** remod;

	//Temporary datastructures
	double** final1;
	double** temp;
	double **ybuf1;
	double** ybuf2;

public:
	processvideo();
	processvideo(string,int,int);
	int getFileSize();
	string getPath();
	void processFrame2();
	int reconstructFrame(double ** prev, double **curr, double **remod, int row, int col, int count);
	void readFrames2(FILE* IN_FILE, int index,char**, char**, char**,double **);
	double frameDiff(double **data_prev, double **data);
	float getSmallBlockDiff(double **curr, double **temp, int row, int col);
	void clearLastFrame();
	void setLastFrame();
	vector<vector<int>> evaluateResult();
	double** getFirst();
	double** getSecond();
	void read_write(FILE*,FILE*,char**,char**,char**,vector<vector<int> >);
	int read_write_a(string, vector<vector<int> >);
	~processvideo();
};

