#include <iostream>
#include <opencv2/opencv.hpp>
#include "qrReader.h"

using namespace std;
using namespace cv;

int main(int argc, char *argv[]) 
{
    if (argc == 1) {
        cout << "Usage: " << argv[0] << "<image>" << endl;
        exit(0);
    }

    Mat img = imread(argv[1]);
    Mat imgBW;
    cvtColor(img, imgBW, CV_BGR2GRAY);
    adaptiveThreshold(imgBW, imgBW, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 51, 0);

    qrReader qr = qrReader();
    bool found = qr.find(imgBW);
    if (found) {
        qr.drawFinders(img);
    }

    namedWindow("image", WINDOW_NORMAL);
    imshow("image", img);
    waitKey(0);

    return 0;
}
