EdgeDetect Wraping::edgeDetect;

int calcX(double *matrix, int x, int y) {
	return (matrix[0] * x + matrix[3] * y + matrix[6]) / (matrix[2] * x + matrix[5] * y + matrix[8]); 
}

int calcY(double *matrix, int x, int y) {
	return (matrix[1] * x + matrix[4] * y + matrix[7]) / (matrix[2] * x + matrix[5] * y + matrix[8]); 
}

double distance(Point first, Point second) {
	return sqrt(pow(first.x - second.x, 2) + pow(first.y - second.y, 2));
}

string getFilename(string &filename, string it) {
	string substr = filename.substr(13);
	cout << substr << endl;
	return "../output/" + it + substr;
}

void Wraping::start(string filename) {
	input.load(filename.c_str());
	vector<Point> points;
	// 提取四个顶点, 按照 左上-0、右下-1、右上-2、左下-3 顺序存储
	edgeDetect.start(input, points, getFilename(filename, "points_"));
	// 设定目标图像大小
	setOuptutSize(points);
	// 计算目标图
	calcOutput(points);
	// 显示目标图
	output.display("output");
	// 保存结果图像
	output.save(getFilename(filename, "result_").c_str());
}

void Wraping::calcOutput(vector<Point> &points) {
	// 计算矩阵
	calcMatrix(points);
	// 计算目标图像--反向仿射
	int x, y, v, w;
	cimg_forXY(output, x, y) {
		v = calcX(matrix, x, y);
		w = calcY(matrix, x, y);
		if (v >= 0 && w >= 0 && v < input.width() && w < input.height()) {
			output(x, y, 0) = input(v, w, 0); 
			output(x, y, 1) = input(v, w, 1); 
			output(x, y, 2) = input(v, w, 2); 
		}
	}
}

void Wraping::calcMatrix(vector<Point> &points) {
	int w = output.width() - 1, h = output.height() - 1;
	CImg<float> A(6, 6, 1, 1,
	   w , 0 , -w * points[2].x , 0 , 0 , 0,
	   w , 0 , -w * points[1].x , h , 0 , -h * points[1].x,
	   0 , 0 , 0 , h , 0 , -h * points[3].x,
	   0 , w , -w * points[2].y , 0 , 0 , 0,
	   0 , w , -w * points[1].y , 0 , h , -h * points[1].y,
	   0 , 0 , 0 , 0 , h , -h * points[3].y
	);
	CImg<float> B(1, 6, 1, 1,
	   points[2].x - points[0].x , points[1].x - points[0].x,
	   points[3].x - points[0].x , points[2].y - points[0].y,
	   points[1].y - points[0].y , points[3].y - points[0].y
	);
	CImg<float> X = B.solve(A);

	for (int i = 0; i < 6; ++i)
		matrix[i] = X(i, 0);
	matrix[6] = points[0].x;
	matrix[7] = points[0].y;
	matrix[8] = 1;

	return;
}

void Wraping::setOuptutSize(vector<Point> &points) {
	int width = (distance(points[0], points[2]) + \
				 distance(points[1], points[3])) / 2;
	int height = (distance(points[0], points[3]) + \
				  distance(points[1], points[2])) / 2;
	output.assign(width, height, 1, 3, 0);
}
