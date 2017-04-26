#include "canny.cpp"

void EdgeDetect::toGrey(CImg<unsigned char> &rgb, CImg<unsigned char> &grey) {
    int x, y;
    grey.assign(rgb.width(), rgb.height(), 1, 1, 0);
    cimg_forXY(rgb, x, y) {
        grey(x, y) = (rgb(x, y, 0) * 299 + \
                      rgb(x, y, 1) * 587 + \
                      rgb(x, y, 2) * 114 + \
                      500) / 1000;
    }
    CImg<unsigned char> temp(3, 3, 1, 1, 1);
    grey.dilate(temp);
    grey.blur(8);
    cimg_forXY(grey, x, y) {
        if (grey(x,y) < 150) grey(x, y) = 0;
    }
    // grey.display("without 阈值");
}

void EdgeDetect::canny(CImg<unsigned char> &grey, CImg<unsigned char> &edge) {
	edge.assign(grey.width(), grey.height(), 1, 1, 0);
	edge._data = cannyparam(grey, grey.width(), grey.height(), 2.5f, 7.5f, 6.0f, 15, 0);
    int x, y;
    cimg_forXY(edge, x, y) {
        edge(x, y) *= 255;
    }
}

void EdgeDetect::toMap(CImg<unsigned char> &edge, CImg<float> &hough) {
	int x, y, angle, polar;
    double theta;
    hough.assign(360, (int)sqrt(edge.width() * edge.width() + edge.height() * edge.height()), 1, 1, 0);
    cimg_forXY(edge, x, y) {
        if (edge(x, y) == 255) {
            cimg_forX(hough, angle) {
                theta = angle * PI_VALUE; // PI_VALUE=3.141592657 / 180，此处是将角度转为弧度
                polar = x * cos(theta) + y * sin(theta);
                if (polar >= 0 && polar < hough.height())
                    hough(angle, polar) += 1;
            }
        }
    }
}

// Polar coordinate intersection at x  
const int EdgeDetect::CrossX(int theta, int r, int x) {  
    double angle = (double)theta * PI_VALUE;  
    double m = -cos(angle) / sin(angle);  
    double b = (double)r / sin(angle);  
    return m*x + b;  
}  
  
// Polar coordinate intersection at y  
const int EdgeDetect::CrossY(int theta, int r, int y) {  
    double angle = (double)theta * PI_VALUE;  
    double m = -cos(angle) / sin(angle);  
    double b = (double)r / sin(angle);  
    return ((double)(y - b) / m);  
}

// 计算两点间距离  
const double EdgeDetect::dis(int x, int y) {  
    return sqrt(x * x + y * y);  
}  

void EdgeDetect::getPeaks(CImg<float> &hough, vector<NODE*> &peaks, int width, int height) {
    bool flag;
    int theta, r;
    int x0, x1, y0, y1;

    const int ymin = 0, ymax = height - 1;
    const int xmin = 0, xmax = width - 1;

    cimg_forXY(hough, theta, r) {
        // if (hough(theta, r) > 400)
        //     cout << hough(theta, r) << endl;
        if (hough(theta, r) > THRESHOLD) {
            flag = false; 
            x0 = CrossY(theta, r, ymin);  
            x1 = CrossY(theta, r, ymax);  

            y0 = CrossX(theta, r, xmin);  
            y1 = CrossX(theta, r, xmax);  
  
            if (x0 >= 0 && x0 <= xmax || // 表示的直线是否在图像内  
                x1 >= 0 && x1 <= xmax ||  
                y0 >= 0 && y0 <= ymax ||  
                y1 >= 0 && y1 <= ymax) {  
                for (int i = 0; i < peaks.size(); ++i) {    // 遍历数组，找相邻峰值
                    if (dis(peaks[i]->y - r, peaks[i]->x - theta) < DIFF) {   // 存在相邻峰值  
                        flag = true;  
                        if (peaks[i]->pixel < hough(theta, r)) {  // 如果比相邻峰值还大
                            peaks[i]->x = theta;   // 替换为当前峰值
                            peaks[i]->y = r;
                            peaks[i]->pixel = hough(theta, r);
                        }  
                    }  
                }  
                if (flag == false) { // 当前峰值无相邻峰值  
                    peaks.push_back(new NODE(theta, r, hough(theta, r)));  // 加入新峰值  
                }  
            }  
        }
    }

    // 将点转成直线
    double temp, _cos, _sin;
    for (int i = 0; i < peaks.size(); ++i) {
        temp = peaks[i]->x * PI_VALUE;
        _cos = -cos(temp);
        _sin = sin(temp);
        peaks[i]->x = _cos / _sin; // m
        cout << "m=" << peaks[i]->x << ", cos=" << _cos << ", _sin=" << _sin << endl;
        peaks[i]->y /= _sin; // b
    }
}

void EdgeDetect::printLines(vector<NODE*> &peaks) {
    cout << "Lines:" << endl;
    for (int i = 0; i < peaks.size(); ++i) {
        cout << "y = (" << peaks[i]->x << ") x + (" << peaks[i]->y << ")" << endl;
    }
    cout << endl;
}

bool comp1(const Point &a, const Point &b) {
    return a.x < b.x;
}

bool comp2(const Point &a, const Point &b) {
    return a.y < b.y;
}

void EdgeDetect::getPoints(CImg<unsigned char> &image, vector<NODE*> &peaks, vector<Point> &_points) {
    const int ymin = 0, xmin = 0;
    const int ymax = image.height() - 1, xmax = image.width() - 1;
    int x0, x1, y0, y1;
    // [1] 找出所有交点
    vector<Point> temp;
    for (int i = 0; i < peaks.size(); ++i)
        for (int j = i + 1; j < peaks.size(); ++j) {
            x0 = (peaks[i]->y - peaks[j]->y) / (peaks[j]->x - peaks[i]->x);
            y0 = (peaks[i]->y * peaks[j]->x - peaks[i]->x * peaks[j]->y) / (peaks[j]->x - peaks[i]->x);
            if (x0 >= 0 && y0 >= 0 && x0 < image.width() && y0 < image.height()) {
                temp.push_back(Point(x0, y0));
            }
        }

    // 找出相距离最远的两个点
    int m, n;
    double distance, maxDistance = 0;
    for (int i = 0; i < temp.size(); ++i) {
        for (int j = i + i; j < temp.size(); ++j) {
            distance = pow(temp[i].x - temp[j].x, 2) + pow(temp[i].y - temp[j].y, 2);
            if (distance > maxDistance) {
                maxDistance = distance;
                m = i; n = j;
            }
        }
    }

    // [2] 从所有交点中找出四个顶点
    vector<Point> points;
    if (temp.size() <= 4) {
        points.push_back(temp[m]);
        points.push_back(temp[n]);
        for (int i = 0; i < temp.size(); ++i) {
            if (i == m || i == n) continue;
            points.push_back(temp[i]);
        }
    } else {

        // 找出斜线两边最远的两个点
        int a = 0, b = 0;
        bool isVertical = false; // 标识最远两点是否垂直
        double a_dis = 0, b_dis = 0;
        double xielu ,jieju, fenmu;
        if (temp[m].x - temp[n].x == 0) { // 垂直
            isVertical = true;
        } else { // 非垂直
            xielu = (temp[m].y - temp[n].y) / double(temp[m].x - temp[n].x);
            jieju = temp[m].y - xielu * temp[m].x;
            fenmu = sqrt(pow(xielu, 2) + 1);
        }
        double xxx;
        for (int i = 0; i < temp.size(); ++i) {
            if (i == m || i == n) continue;
            if (!isVertical) {
                xxx = xielu * temp[i].x + jieju - temp[i].y;
                distance = fabs(xxx) / fenmu;
                if (xxx > 0 && distance > a_dis) {
                    a_dis = distance;
                    a = i;
                } else if (xxx < 0 && distance > b_dis) {
                    b_dis = distance;
                    b = i;
                }
            } else {
                distance = temp[i].x - temp[m].x;
                if (distance > 0 && fabs(distance) > a_dis) {
                    a_dis = fabs(distance);
                    a = i;
                } else if (distance < 0 && fabs(distance) > b_dis) {
                    b_dis = fabs(distance);
                    b = i;
                }
            }
        }
        points.push_back(temp[m]);
        points.push_back(temp[n]);
        points.push_back(temp[a]);
        points.push_back(temp[b]);
    }

    // [3] 对这四个顶点进行位置确定（及左上-0、右下-1、右上-2、左下-3）
    int pt[2], ptr;
    // [3.1] y轴上最小的点
    ptr = 0;
    for (int j = 0; j < points.size(); ++j) {
        if (points[ptr].y > points[j].y)
            ptr = j;
    }
    pt[0] = ptr;
    // [3.2] y轴上次小的点
    ptr = (++ptr) % points.size();
    for (int j = 0; j < points.size(); ++j) {
        if (j != pt[0] && points[ptr].y > points[j].y)
            ptr = j;
    }
    pt[1] = ptr;
    // [3.3] 找出左上点
    if (points[pt[0]].x < points[pt[1]].x) {
        ptr = 0;
    } else {
        ptr = 1;
    }
    // [3.4] 将points中的点按照 左上 右下 右上 左下 顺序存放
    for (int i = 0; i < 2; ++i) {
        _points.push_back(points[pt[ptr]]);
        if (pt[ptr] % 2) {
            _points.push_back(points[pt[ptr] - 1]); 
        } else {
            _points.push_back(points[pt[ptr] + 1]);
        }
        ptr = ++ptr % 2;
    }

    // [4] 绘点
    const double color[][4] = {{ 255, 0, 0 }, { 2, 255, 0 }, \
                               { 0, 0, 255 }, { 0, 0, 0 }};
    for (int i = 0; i < _points.size(); ++i) {
            cout << "[" << _points[i].x << "," << _points[i].y << "]"<< endl;
            image.draw_circle(_points[i].x, _points[i].y, 20, color[i]);
        }

    image.display("result");
}

void EdgeDetect::start(CImg<unsigned char> image, vector<Point> &points, string saveName) {
    // [1] 灰度化处理
    CImg<unsigned char> grey;
    toGrey(image, grey);
    // grey.display("grey");

    // [3] 计算灰度图的梯度，提取边缘
    CImg<unsigned char> edge(grey.width(), grey.height(), 1, 1, 0);
    canny(grey, edge);
    // edge.display("edge");

    // [4] 梯度图映射到Hough图
    CImg<float> hough;
    toMap(edge, hough);
    // hough.display("hough");

    // [5] 寻找峰值
    vector<NODE*> peaks;
    getPeaks(hough, peaks, edge.width(), edge.height());

    // [6] 打印直线方程
    printLines(peaks);
    getPoints(image, peaks, points);

    // [7] 保存图像
    image.save(saveName.c_str());
}
