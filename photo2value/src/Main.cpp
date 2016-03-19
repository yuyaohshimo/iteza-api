#include <iostream>
#include <list>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <sys/stat.h>


namespace {

// 正規化画像のサイズ
const int NORMALIZED_WIDTH = 3840;
const int NORMALIZED_HEIGHT = 2160;

// 検出面積のスレッショールド
const int AREA_THRETHOLD_MIN = 1200;
const int AREA_THRETHOLD_MAX = 15000;
const int SQUARE_DIFF_THRETHOLD = 15;

// マーカの中心位置
const int MARKER_LEFT_CENTER = 200;
const int MARKER_RIGHT_CENTER = NORMALIZED_WIDTH - MARKER_LEFT_CENTER;
const int MARKER_TOP_CENTER = 180;
const int MARKER_BOTTOM_CENTER = NORMALIZED_HEIGHT - MARKER_TOP_CENTER;
// 傾きによるマーカずれの限界(大きすぎると、誤検知につながる)
const int MARKER_DIFF_RANGE_X = 300;
const int MARKER_DIFF_RANGE_Y = 1000;

const int TOPLEFT_MARKER_DETECT_RANGE = 20;

// offset

// 画像切り出しにつかう変数
// マーカからのオフセット
const float FACEVALUE_OFFSET_X = 500;
const float FACEVALUE_OFFSET_Y = 550;
const float FACEVALUE_WIDTH = 1500;
const float FACEVALUE_HEIGHT = 300;
//const float FACEVALUE_OFFSET_WIDTH = 200;
//const float FACEVALUE_OFFSET_HEIGHT = 60;

// マーカからのオフセット
const float ID_OFFSET_X = 500;
const float ID_OFFSET_Y = 1480;
const float ID_WIDTH = 1500;
const float ID_HEIGHT = 200;
}

using namespace std;



int main(int argc, char **argv) {
	cv::Mat srcImage = cv::imread(argv[1]);

	if (srcImage.rows > srcImage.cols) {
		// 縦画像は回転
		cv::flip(srcImage.t(), srcImage, 0);
	}

	string infile = argv[1];
	infile = infile.substr(infile.find_last_of("/") + 1, infile.length());
	string dir = "out/";
	dir += infile;
	mkdir(dir.c_str(), 0777);
	dir += "/";

	// サイズを横幅で正規化
	cv::Mat image(srcImage.rows * NORMALIZED_WIDTH/srcImage.cols, NORMALIZED_WIDTH, srcImage.type());
	cv::resize(srcImage, image, image.size(), cv::INTER_CUBIC);

//	cerr << "end resize" << endl;

	cv::Mat gray;
	//
	cv::cvtColor(image, gray, CV_BGR2GRAY);

//	cerr << "end gray" << endl;
//	cv::imwrite(dir + "/gray.png", gray);

	// ガウシアンフィルタでノイズ除去
	cv::Mat blur;
	cv::GaussianBlur(gray, blur, cv::Size(5,5), 10, 10);

//	cerr << "end blur" << endl;
//    cv::imwrite(dir + "/blur.png", blur);

	cv::Mat bin;
	cv::adaptiveThreshold(
			blur,
			bin,
			50,
			cv::ADAPTIVE_THRESH_MEAN_C,
			cv::THRESH_BINARY,
			11,
			2);
//	cv::threshold(blur, bin, 200.0, 500.0, cv::THRESH_BINARY | cv::THRESH_OTSU);
//			gray, bin, 0.0, 255.0, cv::THRESH_BINARY | cv::THRESH_OTSU);
    // ネガポジ変換
//    bin = ~bin;

//	cerr << "end binarize" << endl;

	cv::Mat labelImg;
	cv::Mat stats;
    cv::Mat centroids;
    cv::Mat dest(bin.size(), CV_8UC3);

    cv::imwrite(dir + "/bin.png", bin);

    int labelCount = cv::connectedComponentsWithStats(bin, labelImg, stats, centroids);

//    cerr << "end connect" << endl;

    std::vector<cv::Vec3b> colors(labelCount);
    colors[0] = cv::Vec3b(0, 0, 0);
    for (int i = 1; i < labelCount; ++i) {
        colors[i] = cv::Vec3b((rand() & 255), (rand() & 255), (rand() & 255));
    }

    vector<cv::Point2f> markerCenterPoints;

    // for debug
    cv::Mat targetsInImage = image.clone();
    cv::Mat filteredImage = image.clone();

    for (int i = 1; i < labelCount; ++i) {
        int *param = stats.ptr<int>(i);
//        double *center = centroids.ptr<double>(i);

        int x = param[cv::CC_STAT_LEFT];
        int y = param[cv::CC_STAT_TOP];
//        int centerx = center[0];
//        int centery = center[1];
        int height = param[cv::CC_STAT_HEIGHT];
        int width = param[cv::CC_STAT_WIDTH];
    	int area = stats.ptr<int>(i)[cv::CC_STAT_AREA];

//        cerr << "target , x, y\t" << area << "\t" << x << "\t" << y << endl;


//    	if (area > AREA_THRETHOLD_MIN && area < AREA_THRETHOLD_MAX
//    			&& abs(width - height) < 20) {
//    	}
		// 面積小さいの無視 & 正方形じゃないの無視
    	if (area > AREA_THRETHOLD_MIN && area < AREA_THRETHOLD_MAX
    			&& abs(width - height) < SQUARE_DIFF_THRETHOLD) {
	        cv::rectangle(targetsInImage, cv::Rect(x, y, width, height), cv::Scalar(0, 255, 0), 2);

			if(((abs(x - MARKER_LEFT_CENTER) < MARKER_DIFF_RANGE_X)
						|| (abs(x - MARKER_RIGHT_CENTER) < MARKER_DIFF_RANGE_X))
				&&((abs(y - MARKER_TOP_CENTER) < MARKER_DIFF_RANGE_Y)
						|| (abs(y - MARKER_BOTTOM_CENTER) < MARKER_DIFF_RANGE_Y))
				) {

//				markerCenterPoints.push_back(cv::Point2f(centerx, centery));
				cv::Point2f pos(x + width / 2, y + height / 2);
				if (pos.x > NORMALIZED_WIDTH) {
					pos.x = NORMALIZED_WIDTH;
				}
				if (pos.y > NORMALIZED_HEIGHT) {
					pos.y = NORMALIZED_HEIGHT;
				}

				markerCenterPoints.push_back(cv::Point2f(x + width / 2, y + height / 2));


				cv::rectangle(filteredImage, cv::Rect(x, y, width, height), cv::Scalar(0, 255, 0), 2);

//				cerr << "founded area , x, y\t" << area << "\t" << centerx << "\t" << centery << endl;
			}
    	}
    }

    int maxMatch = 0;
    vector<int> matched(markerCenterPoints.size());
    int mostMatchIndex;

    for (int i = 0; i < markerCenterPoints.size(); i++) {
    	cv::Point2f pos = markerCenterPoints[i];
    	for (int j = i + 1; j < markerCenterPoints.size(); j++) {

    		cv::Point2f target = markerCenterPoints[j];
    		if (abs(pos.x - target.x) < MARKER_DIFF_RANGE_X) {
    			matched[i]++;
    		}
    		if (abs(pos.y - target.y) < MARKER_DIFF_RANGE_Y) {
    			matched[i]++;
    		}
    	}

    	if (maxMatch < matched[i]) {
    		maxMatch = matched[i];
    		mostMatchIndex = i;
    	}
    }

    cerr << "markerCenterPoints = " << markerCenterPoints << endl;
	// 4点以外がマッチした場合、一番マッチしたものを基準にして、縦のズレ、横のズレが一番小さいものを残す
    // マーカがMUFJロゴにマッチしやすいための苦肉の策
    if (markerCenterPoints.size() > 5) {

    	cerr << "del marker" << endl;

        cv::Point2f mostMatchPoint = markerCenterPoints[mostMatchIndex];
        vector<cv::Point2f> tmp = markerCenterPoints;

        cv::Point2f xMatchPoint(INT_MAX, INT_MAX);
        cv::Point2f yMatchPoint(INT_MAX, INT_MAX);
        int xMatchIndex;
        int yMatchIndex;

        // 一番マッチしたところは消す
        tmp.erase(tmp.begin() + mostMatchIndex);
        // 一番マッチしたとこと同じくらいの点を2点探す
        for (int i = 0; i < tmp.size(); i++) {
        	if (abs(mostMatchPoint.x - tmp[i].x) < abs(mostMatchPoint.x - xMatchPoint.x)) {
        		xMatchPoint = tmp[i];
        		xMatchIndex = i;
        	}
        	if (abs(mostMatchPoint.y - tmp[i].y) < abs(mostMatchPoint.y - yMatchPoint.x)) {
        		yMatchPoint = tmp[i];
        		yMatchIndex = i;
        	}
        }

//        if (xMatchIndex == yMatchIndex) {
        	// 左上マーカなので、いったん消す
        	tmp.erase(tmp.begin() + yMatchIndex);

        	xMatchPoint = cv::Point2f(INT_MAX, INT_MAX);
        	yMatchPoint = cv::Point2f(INT_MAX, INT_MAX);
        	// 再度x,yがマッチする場所を探す
            for (int i = 0; i < tmp.size(); i++) {
            	if (abs(mostMatchPoint.x - tmp[i].x) < abs(mostMatchPoint.x - xMatchPoint.x)) {
            		xMatchPoint = tmp[i];
            		xMatchIndex = i;
            	}
            }

            // xで一番マッチした点は消す
			tmp.erase(tmp.begin() + xMatchIndex);

            for (int i = 0; i < tmp.size(); i++) {
				if (abs(mostMatchPoint.y - tmp[i].y) < abs(mostMatchPoint.y - yMatchPoint.y)) {
					yMatchPoint = tmp[i];
					yMatchIndex = i;
				}
            }
            // yで一番マッチした点は消す

			tmp.erase(tmp.begin() + yMatchIndex);

            cv::Point2f diagonalNodePoint(yMatchPoint.x, xMatchPoint.y);
            int xyDiff = INT_MAX;
            int lastNode = -1;
            // 最後に一番マッチした点の対角にある点を消す
            for (int i = 0; i < tmp.size(); i++) {
            	int diff = abs(diagonalNodePoint.x - tmp[i].x) + abs(diagonalNodePoint.y - tmp[i].y);
            	if (xyDiff > diff) {
            		lastNode = i;
            		xyDiff = diff;
            	}
            }
            tmp.erase(tmp.begin() + lastNode);

        	cerr << "del tmp" << tmp << endl;

        	// tmpに残っているものはゴミなので、markerCenterPointsから消す
            for (int i = 0; i < tmp.size(); i++) {
            	cv::Point2f target = tmp[i];
            	bool found = false;

            	for (int j = 0; j < markerCenterPoints.size(); j++) {
            		if (target.x == markerCenterPoints[j].x
            				&& target.y == markerCenterPoints[j].y) {
            			markerCenterPoints.erase(markerCenterPoints.begin() + j);
            			found = true;
            			break;
            		}
            		if (found) {
            			break;
            		}
            	}
            }
        	cerr << "del tmp end " << markerCenterPoints << endl;
//        } else {
//        	// この場合は、左上マーカの検出に失敗していると思われるので、あきらめる。
//        }
    }

    for (int i = 0; i < markerCenterPoints.size(); i++) {
        cv::circle(targetsInImage, markerCenterPoints[i], 10, cv::Scalar(0, 255, 0), 10);
    }
//    cerr << "markerCenterPoints find end" << endl;
//    cerr << "found size = " << markerCenterPoints.size() << endl;
//    cerr << "markerCenterPoints = " << markerCenterPoints << endl;

    cv::imwrite(dir + "/targets_in_image.png", targetsInImage);
    cv::imwrite(dir + "/filteredImage.png", filteredImage);

    if (markerCenterPoints.size() != 5 && markerCenterPoints.size() != 4) {
    	cerr << "marker search failed" << endl;
    	cerr << "markerCenterPoints.size:" << markerCenterPoints.size() << endl;
    	return 1;
    }

	cv::Point2f leftTop;
	// 左上を検出(左上か右下にあるはず)
	bool found = false;
	for (int i = 0; i < markerCenterPoints.size(); i++) {
		for (int j = i + 1; j < markerCenterPoints.size(); j++) {
			if (abs(markerCenterPoints[i].x - markerCenterPoints[j].x) < TOPLEFT_MARKER_DETECT_RANGE
					&& abs(markerCenterPoints[i].y - markerCenterPoints[j].y) < TOPLEFT_MARKER_DETECT_RANGE) {
				leftTop = markerCenterPoints[i];
				found = true;
				break;
			}
		}
		if (found) {
			// 2つある左上の中心を削除
			markerCenterPoints.erase(markerCenterPoints.begin() + i);
			break;
		}
	}

    if (markerCenterPoints.size() > 4) {
    	// 見つからなかったら、左上決めうち
    	for (int i = 0; i < markerCenterPoints.size(); i++) {
    		for (int j = i + 1; j < markerCenterPoints.size(); j++) {
    			if (abs(markerCenterPoints[i].x - MARKER_LEFT_CENTER) < MARKER_DIFF_RANGE_X
    					&& abs(markerCenterPoints[i].y - MARKER_TOP_CENTER) < MARKER_DIFF_RANGE_Y) {
    				leftTop = markerCenterPoints[i];
    				found = true;
    				break;
    			}
    		}
    		if (found) {
    			// 2つある左上の中心を削除
    			markerCenterPoints.erase(markerCenterPoints.begin() + i);
    			break;
    		}
    	}
    }

	vector<cv::Point2f> v(markerCenterPoints.begin(),
			markerCenterPoints.end());

	cv::Point2f prevEdge[4];
	for (int i = 0; i < markerCenterPoints.size(); i++) {
		if (abs(markerCenterPoints[i].x - MARKER_LEFT_CENTER) < MARKER_DIFF_RANGE_X) {
			if (abs(markerCenterPoints[i].y - MARKER_TOP_CENTER) < MARKER_DIFF_RANGE_Y) {
				prevEdge[0] = v[i]; // 左上
			} else {
				prevEdge[1] = v[i]; // 左下
			}
		} else {
			if (abs(markerCenterPoints[i].y - MARKER_TOP_CENTER) < MARKER_DIFF_RANGE_Y) {
				prevEdge[3] = v[i]; // 右上
			} else {
				prevEdge[2] = v[i]; // 右下
			}
		}
	}

	// 左上の検出が右下だった場合
	// TODO: 全体見直したら、たぶんもっとエレガントに書けるな・・・

	if (found && abs(leftTop.x - MARKER_LEFT_CENTER) > MARKER_DIFF_RANGE_X) {
		// 元の画像を180度回転しておく

		cv::flip(image, image, -1);
	    cv::imwrite(dir + "/fliped.png", image);

		// 入れ替え
		cv::Point2f tmp = prevEdge[0];
		prevEdge[0] = prevEdge[2];
		tmp = prevEdge[2] = tmp;
		tmp = prevEdge[1];
		prevEdge[1] = prevEdge[3];
		prevEdge[3] = tmp;

		// 座標の反転
		prevEdge[0].x = NORMALIZED_WIDTH - prevEdge[0].x;
		prevEdge[0].y = NORMALIZED_HEIGHT - prevEdge[0].y;
		prevEdge[1].x = NORMALIZED_WIDTH - prevEdge[1].x;
		prevEdge[1].y = NORMALIZED_HEIGHT - prevEdge[1].y;
		prevEdge[2].x = NORMALIZED_WIDTH - prevEdge[2].x;
		prevEdge[2].y = NORMALIZED_HEIGHT - prevEdge[2].y;
		prevEdge[3].x = NORMALIZED_WIDTH - prevEdge[3].x;
		prevEdge[3].y = NORMALIZED_HEIGHT - prevEdge[3].y;

		leftTop = prevEdge[0];
	}

	cv::Point2f postEdge[4];
	postEdge[0] = cv::Point2f(MARKER_LEFT_CENTER, MARKER_TOP_CENTER); // 左上
	postEdge[1] = cv::Point2f(MARKER_LEFT_CENTER, MARKER_BOTTOM_CENTER); // 左下
	postEdge[2] = cv::Point2f(MARKER_RIGHT_CENTER, MARKER_BOTTOM_CENTER); // 右下
	postEdge[3] = cv::Point2f(MARKER_RIGHT_CENTER, MARKER_TOP_CENTER); // 右上

//	cerr << "prevEdge[0] " << prevEdge[0] << endl;
//	cerr << "prevEdge[1] " << prevEdge[1] << endl;
//	cerr << "prevEdge[2] " << prevEdge[2] << endl;
//	cerr << "prevEdge[3] " << prevEdge[3] << endl;
//	cerr << "postEdge[0] " << postEdge[0] << endl;
//	cerr << "postEdge[1] " << postEdge[1] << endl;
//	cerr << "postEdge[2] " << postEdge[2] << endl;
//	cerr << "postEdge[3] " << postEdge[3] << endl;

	cv::Mat warpMatrix = cv::getPerspectiveTransform(prevEdge, postEdge);

	// 位置補正後の画像
	cv::Mat normarized(image.rows, image.cols, image.type());
	cv::warpPerspective(image, normarized, warpMatrix, normarized.size());

    cv::imwrite(dir + "/normalized.png", normarized);

	// マーカ内のみ切取り
	cv::Mat inMarker(normarized,
			cv::Rect(
					postEdge[0].x,
					postEdge[0].y,
					postEdge[2].x - postEdge[0].x,
					postEdge[2].y - postEdge[0].y));
    cv::imwrite(dir + "/marker-rect.png", inMarker);

	// 額面
	cv::Mat faceValue(inMarker,
			cv::Rect(
			FACEVALUE_OFFSET_X,
			FACEVALUE_OFFSET_Y,
			FACEVALUE_WIDTH,
			FACEVALUE_HEIGHT));
	cv::cvtColor(faceValue, faceValue, CV_BGR2GRAY);
    cv::GaussianBlur(faceValue, faceValue,cv::Size(5,5), 10, 10);
	cv::threshold(faceValue, faceValue, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    cv::imwrite(dir + "/facevalue.png", faceValue);

	// ID
	cv::Mat id(inMarker,
			cv::Rect(
					ID_OFFSET_X,
					ID_OFFSET_Y,
			ID_WIDTH,
			ID_HEIGHT));

	cv::cvtColor(id, id, CV_BGR2GRAY);

    cv::GaussianBlur(id, id,cv::Size(5,5), 10, 10);

	cv::threshold(id, id, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    cv::imwrite(dir + "/id.png", id);

//    cv::imshow("edges", faceValue);
//    cv::waitKey(0);
    // tesseract
	tesseract::TessBaseAPI tess;
	tess.Init(NULL, "eng", tesseract::OEM_DEFAULT);
	tess.SetImage((uchar*)faceValue.data, faceValue.size().width, faceValue.size().height, faceValue.channels(), faceValue.step1());
	tess.Recognize(0);
	string faceValueText = tess.GetUTF8Text();

	tess.SetImage((uchar*)id.data, id.size().width, id.size().height, id.channels(), id.step1());
	tess.Recognize(0);
	string idText = tess.GetUTF8Text();


	faceValueText.erase(faceValueText.find_last_not_of(" \n\r\t")+1);
	idText.erase(idText.find_last_not_of(" \n\r\t")+1);
//	cout << "faceValue:" << faceValueText << endl;
	replace(idText.begin(), idText.end(), 'I', '1');
	cout << idText << endl;

//	cv::imshow("1", bin);
//	cv::imshow("2", converted);
//	cv::imshow("3", faceValue);
//	cv::waitKey(0);

	return 0;
}



