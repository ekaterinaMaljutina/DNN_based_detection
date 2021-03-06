#include "Detector.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <iostream>

using namespace cv;
using namespace std;

void Detector::Preprocessing(Mat &img) {
    float mean[] = {0.40559885502486, -0.019621851500929, 0.026953143125972};
    float std[] = {0.26126178026709, 0.049694558439293, 0.071862255292542};
    img.convertTo(img, CV_32F, 1.0f/255.0f);
    cvtColor(img, img, COLOR_BGR2YCrCb);

    for (int x = 0; x < img.cols; x++) {
        for (int y = 0; y < img.rows; y++) {
            Vec3f pixel = img.at<Vec3f>(y, x);
            for (int z = 0; z < 3; z++) {
                pixel[z] = (pixel[z] - mean[z]) / std[z];
            }
        }
    }
}

Detector::Detector(std::shared_ptr<Classifier> classifier_,
                   cv::Size window_size_, int dx_ = 1, int dy_ = 1,
                   double scale_ = 1.2, int min_neighbours_ = 3,
                   bool group_rect_ = false)
    : classifier(classifier_),
      window_size(window_size_),
      dx(dx_),
      dy(dy_),
      scale(scale_),
      min_neighbours(min_neighbours_),
      group_rect(group_rect_)
{}

void Detector::Detect(const Mat &img, vector<int> &labels,
                      vector<double> &scores, vector<Rect> &rects) {
    CV_Assert(scale > 1.0 && scale <= 2.0);

    vector<Mat> imgPyramid;
    Mat base_img(img);
    imgPyramid.push_back(base_img);

    Mat tmp(base_img);

    while (tmp.cols > window_size.width && tmp.rows > window_size.height) {
        Mat tmp2;
        resize(tmp, tmp2, Size((int)(tmp.cols / scale), (int)(tmp.rows / scale)), 0, 0, INTER_LINEAR);
        tmp2.copyTo(tmp);
        imgPyramid.push_back(tmp2);
    }

    float newScale = 1;
    //for every layer of pyramid

    for (uint i = 0; i < imgPyramid.size(); i++) {
        Mat layer = imgPyramid[i];
        vector<Rect> layerRect;

        for (int y = 0; y < layer.rows - window_size.height + 1; y += dy) {
            for (int x = 0; x < layer.cols - window_size.width + 1; x += dx) {
                Rect rect(x, y, window_size.width, window_size.height);
                Mat window = layer(rect);

                Classifier::Result result = classifier->Classify(window);
                if (fabs(result.confidence) < DETECTOR_THRESHOLD && result.label == 1) {
                    labels.push_back(result.label);
                    scores.push_back(result.confidence);

                    layerRect.push_back( Rect(cvRound(rect.x * newScale), cvRound(rect.y * newScale),
                                              cvRound(rect.width * newScale), cvRound(rect.height * newScale)) );
                }
            }
        }
        if (group_rect) {
            groupRectangles(layerRect, min_neighbours, 0.2);
        }
        rects.insert(rects.end(), layerRect.begin(), layerRect.end());
        newScale *= scale;
    }
}
