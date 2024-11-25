#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

// 定义颜色范围（HSV格式）
void getHSVRange(vector<Mat> &ranges) {
    Mat red_lower1 = (Mat_<uchar>(1, 3) << 0, 150, 100);
    Mat red_upper1 = (Mat_<uchar>(1, 3) << 10, 255, 255);
    Mat red_lower2 = (Mat_<uchar>(1, 3) << 170, 150, 100);
    Mat red_upper2 = (Mat_<uchar>(1, 3) << 180, 255, 255);
    Mat green_lower = (Mat_<uchar>(1, 3) << 40, 50, 50); 
    Mat green_upper = (Mat_<uchar>(1, 3) << 80, 255, 255); 

    ranges.push_back(red_lower1);
    ranges.push_back(red_upper1);
    ranges.push_back(red_lower2);
    ranges.push_back(red_upper2);
    ranges.push_back(green_lower);
    ranges.push_back(green_upper);
}

// 检测圆形
bool isCircle(const vector<Point>& contour) {
    double area = contourArea(contour);
    double perimeter = arcLength(contour, true);
    if (perimeter == 0) return false;
    double circularity = 4 * CV_PI * area / (perimeter * perimeter);
    return (circularity > 0.7 && area > 100); // 调整阈值以适应实际情况
}

// 识别红绿灯
pair<string, Mat> detectTrafficLight(Mat &hsv, vector<Mat> &ranges, Mat &result) {
    Mat mask;
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    for (size_t i = 0; i < ranges.size(); i += 2) {
        inRange(hsv, ranges[i], ranges[i + 1], mask);

        //先腐蚀后膨胀，减少噪声
        erode(mask, mask, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)));
        dilate(mask, mask, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)));

        findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        // 显示掩码图像
        imshow("Mask", mask);

        for (const auto& contour : contours) {
            if (isCircle(contour)) {
                Rect boundRect = boundingRect(contour);
                rectangle(result, boundRect.tl(), boundRect.br(), Scalar(0, 0, 255), 2); // 画出矩形框
                if (i < 4) {
                    return make_pair("RED", result);
                } else {
                    return make_pair("GREEN", result);
                }
            }
        }
    }
    return make_pair("NOT", result);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <video_file_or_camera_index>" << endl;
        return -1;
    }

    string source = argv[1];
    VideoCapture cap;
    Mat frame, hsv, result;
    vector<Mat> hsvRanges;
    getHSVRange(hsvRanges);

    
    if (isdigit(source[0])) {
        int cameraIndex = stoi(source);
        cap.open(cameraIndex); // 打开摄像头
    } else {
        cap.open(source); // 打开视频文件
    }

    if (!cap.isOpened()) {
        cerr << "无法打开视频文件或摄像头: " << source << endl;
        return -1;
    }

    while (true) {
        cap >> frame; // 获取一帧
        if (frame.empty()) break; // 如果帧为空，跳出循环

        // 图像预处理
        GaussianBlur(frame, frame, Size(5, 5), 0);

        result = frame.clone(); // 用于绘制结果
        cvtColor(frame, hsv, COLOR_BGR2HSV); // 转换到HSV颜色空间

        // 识别红绿灯
        pair<string, Mat> detectionResult = detectTrafficLight(hsv, hsvRanges, result);
        string lightStatus = detectionResult.first;
        result = detectionResult.second;

        // 在画面左上角标注当前模式
        putText(result, lightStatus, Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 0), 2);

        imshow("Traffic Light Detection", result);
        if (waitKey(30) >= 0) break; // 按任意键退出
    }

    cap.release();
    destroyAllWindows();
    return 0;
}