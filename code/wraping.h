#ifndef WRAPING_H
#define WRAPING_H

#include <iostream>
#include <cmath>
// #include <armadillo>
#include "CImg.h"
#include "edgeDetect.h"

using namespace cimg_library;
// using namespace arma;
using namespace std;

class Wraping {
 private:
 	static EdgeDetect edgeDetect;
 	CImg<unsigned char> input;  // 输入图像
 	CImg<unsigned char> output; // 目标图像
 	double matrix[9]; // 透视变换矩阵
 	void setOuptutSize(vector<Point> &points); // 设置目标图像大小
 	void calcOutput(vector<Point> &points); // 计算目标图像
 	void calcMatrix(vector<Point> &points); // 计算透视变换矩阵
 public:
 	Wraping() {}
 	void start(string filename);
};

#include "wraping.cpp"

#endif