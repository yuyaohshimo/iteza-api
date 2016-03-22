#include <iostream>
#include <list>
#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <tesseract/baseapi.h>

using namespace cv;

const int AREA_THRETHOLD_MIN = 500;
const int AREA_THRETHOLD_MAX = 15000;
const int NEAR_BY_INTERSECTIONS_THRETHOLD = 80;
const int NEAR_BY_COUT_THRETHOLD_RATE = 80;


// 正規化画像のサイズ
const int NORMALIZED_WIDTH = 3840;
const int NORMALIZED_HEIGHT = 2160;

const int MARKER_LEFT_CENTER = 200;
const int MARKER_RIGHT_CENTER = NORMALIZED_WIDTH - MARKER_LEFT_CENTER;
const int MARKER_TOP_CENTER = 180;
const int MARKER_BOTTOM_CENTER = NORMALIZED_HEIGHT - MARKER_TOP_CENTER;

// メディアンフィルタのサイズ
const int MEDIAN_BLUR_SIZE = 3;
// HSVの設定
const int HSV_H_CENTER = 130;
const int HSV_H_RANGE = 30;
const int HSV_S_MIN = 100;
const int HSV_S_MAX = 255;
const int HSV_V_MIN = 0;
const int HSV_V_MAX = 200;

const int LINE_SIZE_MAX = 150;

const int LINE_THREATHOLD_MIN = 150;
const int LINE_THREATHOLD_STEP = 20;

const int INTERSECTION_VOTE_RANGE = 5;

// マーカからのオフセット
const float ID_OFFSET_X = 450;
const float ID_OFFSET_Y = 1450;
const float ID_WIDTH = 1500;
const float ID_HEIGHT = 250;

Point2f calcIntersection(double rho1, double theta1, double rho2, double theta2 );

bool isNearByNode(const double x, const double y, std::vector<Point2f> &intersections);
double pointToLineDistance(Point2f p, double rho, double theta);
void writeLine(Mat mat, Vec2f line, const Scalar& color, int thickness);

typedef std::pair<int, int> ass_arr;
bool sort_greater(const ass_arr& left,const ass_arr& right){
    return left.second > right.second;
}

inline void debugWrite(const std::string &outDir, const std::string &name, const Mat image) {
	imwrite(outDir + "/" + name + ".png", image);
}

int main(int argc, char **argv) {

	Mat srcImage = imread(argv[1]);
	Mat dest;
//	Mat dest = srcImage.clone();
	std::string infile = argv[1];
	infile = infile.substr(infile.find_last_of("/") + 1, infile.length());
	std::string outDir;
	outDir += "out";
	outDir += "/";
	outDir += infile;

	mkdir(outDir.c_str(), 0777);
    debugWrite(outDir, "src", srcImage);

	Mat hsv;
	cvtColor(srcImage, hsv, CV_BGR2HSV);

	Mat blur;
//	medianBlur(hsv, blur, MEDIAN_BLUR_SIZE);
	hsv.copyTo(blur);

	Mat mask;
	inRange(blur, Scalar(HSV_H_CENTER - HSV_H_RANGE, HSV_S_MIN, HSV_V_MIN)
			, Scalar(HSV_H_CENTER + HSV_H_RANGE, HSV_S_MAX, HSV_V_MAX), mask);
//	inRange(blur, Scalar(100 - 30, 100, 0), Scalar(100 + 30, 255, 255), mask);

	debugWrite(outDir, "filtered", mask);
	srcImage.copyTo(dest, mask);

	// 背景白にして、検出率アップ
	Mat masked(dest.rows, dest.cols, dest.type(), Scalar(255, 255, 255));
	dest.copyTo(masked, mask);

    Mat gray;
	cvtColor(dest, gray, CV_BGR2GRAY);

	Mat canny;
	Canny(gray, canny, 10, 10);
	debugWrite(outDir, "canny", canny);
	std::vector<Vec2f> lines;

	// 検出される線の量を調節
	for (int i = 0; i < 100; i++) {
		HoughLines(canny, lines, 1, CV_PI / 180.0, LINE_THREATHOLD_MIN + LINE_THREATHOLD_STEP * i);
		if (lines.size() < LINE_SIZE_MAX) {
			break;
		}
	}

	if (lines.size() >= LINE_SIZE_MAX) {
		std::cerr << "lines too big "<<std::endl;
		return 1;
	}

	for (std::vector<Vec2f>::iterator it = lines.begin(); it != lines.end(); ++it) {

		writeLine(dest, *it, Scalar(0, 255, 0), 3);
		writeLine(canny, *it, Scalar(0, 255, 0), 3);
	}
	std::vector<Point2f> intersections;

	for (std::vector<Vec2f>::iterator it1 = lines.begin(); it1 != lines.end(); ++it1) {
		double rho1 = (*it1)[0];
		double theta1 = (*it1)[1];

		for (std::vector<Vec2f>::iterator it2 = it1; it2 != lines.end(); ++it2) {

			double rho2 = (*it2)[0];
			double theta2 = (*it2)[1];

			if (abs(theta1 - theta2) < (60 * CV_PI / 180)) {
				// 角度がゆるいのは交点として認めません
				continue;
			}

			Point2f p = calcIntersection(rho1, theta1, rho2, theta2);

			// 枠外の交点は排除
			if (p.x > dest.cols || p.x < 0.0
					|| p.y > dest.rows || p.y < 0.0) {
				continue;
			}

//			std::cerr << "theta1 = " << theta1 << std::endl;
			intersections.push_back(p);

			circle(dest, p, 10, Scalar(255, 0, 0), 5);
		}
	}
    // 左上を取得

    Point2f leftTopPos;
    Point2f rightBottomPos;
    int leftTopLen = INT_MAX;
    int rightBottomLen = 0;

    // TODO 頂点候補の選択はもっと綺麗にできるかもしれない。
    for (int i = 0; i < intersections.size(); i++) {
    	Point2f pos1 = intersections[i];
    	int len = sqrt(pow(pos1.x, 2) + pow(pos1.y, 2));
    	int nearByPoints = 0;
    	if (len < leftTopLen) {
    		for (int j = 0; j < intersections.size(); j++) {
    			Point2f pos2 = intersections[j];
    			// 縦横xxピクセルに5個以上頂点がなければ、誤検出とする

    			if (abs(pos1.x - pos2.x) < INTERSECTION_VOTE_RANGE &&
    					abs(pos1.y - pos2.y) < INTERSECTION_VOTE_RANGE) {
    				nearByPoints++;
    			}
    			if (nearByPoints >= 5) {
    				break;
    			}
    		}
    		if (nearByPoints >= 5) {
				leftTopPos = pos1;
				leftTopLen = len;
    		}
    	}

    	if (len > rightBottomLen) {
    		for (int j = 0; j < intersections.size(); j++) {
    			Point2f pos2 = intersections[j];
    			// 縦横xxピクセルに5個以上頂点がなければ、誤検出とする

    			if (abs(pos1.x - pos2.x) < INTERSECTION_VOTE_RANGE &&
    					abs(pos1.y - pos2.y) < INTERSECTION_VOTE_RANGE) {
    				nearByPoints++;
    			}
    			if (nearByPoints >= 5) {
    				break;
    			}
    		}
    		if (nearByPoints >= 5) {
				rightBottomPos = pos1;
				rightBottomLen = len;
    		}
    	}
    }

    if (leftTopLen == INT_MAX || rightBottomLen == 0) {
    	std::cerr << "can't find left top or right bottom.." << std::endl;
    	std::cerr << "intersections = " << intersections.size() << std::endl;
    	std::cerr << "lines = " << lines.size() << std::endl;
    	debugWrite(outDir, "dest", dest);
    	return 1;
    }
    cv::Point2f prevEdge[4];

    prevEdge[0] = leftTopPos;
	prevEdge[2] = rightBottomPos;
    // 上下、左右の線を選択
	Vec2f leftVLine;
	Vec2f topHLine;
	Vec2f rightVLine;
	Vec2f bottomHLine;
	double leftVLineDist = INFINITY;
	double topHLineDist = INFINITY;
	double rightVLineDist = INFINITY;
	double bottomHLineDist = INFINITY;


	// 左上と右下に近い線を探す
	for (int i = 0; i < lines.size(); i++) {
		double rho = lines[i][0];
		double theta = lines[i][1];

		writeLine(dest, lines[i], Scalar(255,255,0), 2);

		double dist1 = pointToLineDistance(prevEdge[0], rho, theta);
		double dist2 = pointToLineDistance(prevEdge[2], rho, theta);

		bool isVLine = abs(sin(theta)) < (1/ sqrt(2));

		if (isVLine) {
			// 縦線
			if (dist1 < leftVLineDist) {
				leftVLineDist = dist1;
				leftVLine = lines[i];
			}
			if (dist2 < rightVLineDist) {
				rightVLineDist = dist2;
				rightVLine = lines[i];
			}
		} else {
			// 横線
			if (dist1 < topHLineDist) {
				topHLineDist = dist1;
				topHLine = lines[i];
			}
			if (dist2 < bottomHLineDist) {
				bottomHLineDist = dist2;
				bottomHLine = lines[i];
			}
		}
	}
	writeLine(dest, leftVLine, Scalar(255, 255, 255), 20);
	writeLine(dest, topHLine, Scalar(255, 255, 255), 5);
	writeLine(dest, rightVLine, Scalar(255, 255, 255), 5);
	writeLine(dest, bottomHLine, Scalar(255, 255, 255), 5);

	double rightTopDist = INFINITY;
	double leftBottomDist = INFINITY;

	for (int i = 0; i < intersections.size(); i++) {
		if (intersections[i] == prevEdge[0] || intersections[i] == prevEdge[2]) {
			continue;
		}

		double topDist = pointToLineDistance(intersections[i], topHLine[0], topHLine[1]);
		double leftDist = pointToLineDistance(intersections[i], leftVLine[0], leftVLine[1]);
		double bottomDist = pointToLineDistance(intersections[i], bottomHLine[0], bottomHLine[1]);
		double rightDist = pointToLineDistance(intersections[i], rightVLine[0], rightVLine[1]);
		if (topDist + rightDist < rightTopDist) {
			// 右上候補
			prevEdge[3] = intersections[i];
			rightTopDist = topDist + rightDist;
		}
		if (bottomDist + leftDist < leftBottomDist) {
			// 左下候補
			prevEdge[1] = intersections[i];
			leftBottomDist = bottomDist + leftDist;
		}
	}
//	std::cerr << "rightTopDist " << rightTopDist << std::endl;
//	std::cerr << "leftBottomDist " << leftBottomDist << std::endl;

	float imageResizeRate = (float)srcImage.cols / NORMALIZED_WIDTH;

	cv::Point2f postEdge[4];
	postEdge[0] = cv::Point2f(MARKER_LEFT_CENTER, MARKER_TOP_CENTER) * imageResizeRate; // 左上
	postEdge[1] = cv::Point2f(MARKER_LEFT_CENTER, MARKER_BOTTOM_CENTER) * imageResizeRate; // 左下
	postEdge[2] = cv::Point2f(MARKER_RIGHT_CENTER, MARKER_BOTTOM_CENTER) * imageResizeRate; // 右下
	postEdge[3] = cv::Point2f(MARKER_RIGHT_CENTER, MARKER_TOP_CENTER) * imageResizeRate; // 右上

	circle(dest, prevEdge[0], 10, Scalar(0, 0, 255), 5);
	circle(dest, prevEdge[1], 10, Scalar(0, 0, 255), 5);
	circle(dest, prevEdge[2], 10, Scalar(0, 0, 255), 5);
	circle(dest, prevEdge[3], 10, Scalar(0, 0, 255), 5);
//	std::cerr << "prevEdge[0] = " << prevEdge[0] << std::endl;
//	std::cerr << "prevEdge[1] = " << prevEdge[1] << std::endl;
//	std::cerr << "prevEdge[2] = " << prevEdge[2] << std::endl;
//	std::cerr << "prevEdge[3] = " << prevEdge[3] << std::endl;
//
	// TODO slack.jpg が検出できるように
//	std::cerr << "postEdge[0] = " << postEdge[0] << std::endl;
//	std::cerr << "postEdge[1] = " << postEdge[1] << std::endl;
//	std::cerr << "postEdge[2] = " << postEdge[2] << std::endl;
//	std::cerr << "postEdge[3] = " << postEdge[3] << std::endl;

	debugWrite(outDir, "masked", masked);
	debugWrite(outDir, "dest", dest);

	cv::Mat warpMatrix = cv::getPerspectiveTransform(prevEdge, postEdge);
	// 位置補正後の画像
	cv::Mat normarized(masked.rows, masked.cols, masked.type());
	cv::warpPerspective(masked, normarized, warpMatrix, normarized.size());

	debugWrite(outDir, "normalized", normarized);

//	std::cerr << "srcImage.cols:" << srcImage.cols << std::endl;
//	std::cerr << "srcImage.rows:" << srcImage.rows << std::endl;
//	std::cerr << "postEdge[0].x * imageResizeRate:" << postEdge[0].x * imageResizeRate << std::endl;
//	std::cerr << "postEdge[0].y * imageResizeRate:" << postEdge[0].y * imageResizeRate << std::endl;
//	std::cerr << "(postEdge[2].x - postEdge[0].x) * imageResizeRate:" << (postEdge[2].x - postEdge[0].x) * imageResizeRate << std::endl;
//	std::cerr << "(postEdge[2].y - postEdge[0].y) * imageResizeRate:" << (postEdge[2].y - postEdge[0].y) * imageResizeRate << std::endl;
	// マーカ内のみ切取り
	cv::Mat inMarker(normarized,
			cv::Rect(
					postEdge[0].x ,
					postEdge[0].y ,
					(postEdge[2].x - postEdge[0].x) ,
					(postEdge[2].y - postEdge[0].y) ));
	resize(normarized, normarized, normarized.size(), 1 / imageResizeRate, 1 / imageResizeRate);

	debugWrite(outDir, "merker-rect", inMarker);

	// ID
	cv::Mat id(inMarker,
			cv::Rect(
					ID_OFFSET_X * imageResizeRate,
					ID_OFFSET_Y * imageResizeRate,
			ID_WIDTH * imageResizeRate,
			ID_HEIGHT * imageResizeRate));

	debugWrite(outDir, "id", id);
	tesseract::TessBaseAPI tess;
	tess.Init(NULL, "eng", tesseract::OEM_DEFAULT);
	tess.SetImage((uchar*)id.data, id.size().width, id.size().height, id.channels(), id.step1());
	tess.Recognize(0);

	std::string value = tess.GetUTF8Text();

	std::cout << value << std::endl;
}

Point2f calcIntersection(double rho1, double theta1, double rho2, double theta2 ) {
	double sin1 = sin(theta1);
	double cos1 = cos(theta1);
	double sin2 = sin(theta2);
	double cos2 = cos(theta2);
	double m = 1/(sin1 * cos2 - sin2 * cos1);

	// thanx http://imura-lab.org/wp/wp-content/uploads/2015/10/hough-transform.pdf
	Mat mat1 = (Mat_<double>(2, 2) << cos2, sin2, cos1, sin1);
	Mat mat2 = (Mat_<double>(2, 1)
			<< (-rho1 * cos1 + rho2 * cos2),
				-rho1 * sin1 + rho2 * sin2);
	Mat t = m * mat1 * mat2;
	double t1 = t.at<double>(0, 0);
	Point2f p(t1 * sin1 + rho1 * cos1, (-1) * t1 * cos1 + rho1 * sin1);

	return p;
}

bool isNearByNode(const double x, const double y, std::vector<Point2f> &intersections) {
	int count = 0;

	for (int i = 0; i < intersections.size(); i++) {
		// 本当に距離はかってもしゃーないので、x,yの差異で調べる
		if (abs(x - intersections[i].x) < NEAR_BY_INTERSECTIONS_THRETHOLD
				&& abs(y - intersections[i].y) < NEAR_BY_INTERSECTIONS_THRETHOLD) {
			count++;
			if (count >= intersections.size() / NEAR_BY_COUT_THRETHOLD_RATE) {
//			if (count >= 3) {
				return true;
			}
		}
	}
//	std::cerr << "pos" << x << ":" << y << " near by count = " << count << std::endl;
	return false;
}

double inline abs(Vec2d vec) {
	return sqrt(pow(vec[0], 2) + pow(vec[1], 2));
}

double pointToLineDistance(Point2f p, double rho, double theta) {

	// thanx http://www.sousakuba.com/Programming/gs_dot_line_distance.html
	double sin1 = sin(theta);
	double cos1 = cos(theta);
	double x0 = rho * cos1;
	double y0 = rho * sin1;

	// 線上のベクトル
	Vec2d lineVec = Vec2d(x0 - 100000000 * sin1, y0 + 100000000 * cos1);

	// 線上の点から点までのベクトル
	Vec2d line2PointVec = Vec2d(x0 - p.x, y0 - p.y);

	// 外積
	double cross = (lineVec[0] * line2PointVec[1] - lineVec[1] * line2PointVec[0]);

	// 面積 d (外積の絶対値)
	double d = abs(cross);

	double distance = abs(lineVec);

	return d / distance;
}

void writeLine(Mat mat, Vec2f linevec, const Scalar& color, int thickness) {
	float rho = linevec[0];
	float theta = linevec[1];
	double cosTheta = cos(theta);
	double sinTheta = sin(theta);
	double x0 = cosTheta * rho;
	double y0 = sinTheta * rho;

	Point pt1(x0 + 10000 * (-sinTheta), y0 + 10000 * cosTheta);
	Point pt2(x0 - 10000 * (-sinTheta), y0 - 10000 * cosTheta);

	line(mat, pt1, pt2, color, thickness);
}
