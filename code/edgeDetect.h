#ifndef EDGEDETECT_H
#define EDGEDETECT_H

#include <iostream>
#include <vector>
#include "CImg.h"

using namespace std;
using namespace cimg_library;

#define GRADLIMIT 50
#define THRESHOLD 250
#define DIFF 100

const double PI_VALUE = 3.141592653 / 180;

typedef struct node {
    float x, y, pixel;
    node(float _x, float _y, float _p) {
        x = _x; y = _y; pixel = _p;
    }
} NODE;

typedef struct point {
	int x, y;
	point(int _x, int _y) {
		x = _x;
		y = _y;
	}
} Point;

class EdgeDetect {
 private:
 	const int CrossX(int theta, int r, int x);
 	const int CrossY(int theta, int r, int x);
 	const double dis(int x, int y);
 	// 获得的定点保存在_points里，按照 左上-0、右下-1、右上-2、左下-3 存储
 	void getPoints(CImg<unsigned char> &image, vector<NODE*> &peaks, vector<Point> &_points);
 	void printLines(vector<NODE*> &peaks);
 	void toGrey(CImg<unsigned char> &rgb, CImg<unsigned char> &grey); // 灰度转换
 	void canny(CImg<unsigned char> &grey, CImg<unsigned char> &edge); // 边缘提取
 	void toMap(CImg<unsigned char> &edge, CImg<float> &hough); // 将边缘提取图映射为hough图
 	void getPeaks(CImg<float> &hough, vector<NODE*> &peaks, int w, int height); // 寻找峰值
 public:
 	void start(CImg<unsigned char> image, vector<Point> &points, string saveName); // 处理并返回边缘节点检测结果
};

#include "edgeDetect.cpp"

#endif