#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "dcmtk/config/osconfig.h"
#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/core/mat.hpp>
#include <opencv4/opencv2/core/hal/interface.h>
#include <opencv2/imgproc/types_c.h>

#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmimgle/dcmimage.h"
#include "dcmtk/dcmimage/diregist.h" /*to support color image*/

#include <experimental/filesystem>
#include <filesystem>


using namespace std;
namespace stdfs = std::filesystem;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    stdfs::path path = "/home/dana/search";
    const stdfs::directory_iterator end{};
    for (stdfs::directory_iterator iter{ path }; iter != end; ++iter)
    {
        DicomImage* image = new DicomImage(iter->path().string().c_str());
       int nWidth = image->getWidth();
       int nHeight = image->getHeight();
       int depth = image->getDepth();
       cout << nWidth << ", " << nHeight << endl;
       cout << depth << endl;

       cv::Mat dst;
       image->setWindow(100, 400);
       if (image != NULL)
       {
           if (image->getStatus() == EIS_Normal)
           {
               Uint16* pixelData = (Uint16*)(image->getOutputData(16));
               if (pixelData != NULL)
               {
                   dst = cv::Mat(nHeight, nWidth, CV_16UC1, pixelData);
                   /*imshow("image2", dst);
                   cv::waitKey(0);
                   system("pause");*/
               }
           }
           else
               cerr << "Error: cannot load DICOM image (" << DicomImage::getString(image->getStatus()) << ")" << endl;
       }

       dst.convertTo(dst, CV_8UC3, 1.0 / 256);
       cv::Mat img = cv::Mat(nHeight, nWidth, CV_8UC3);
       //img = dst;

       //Mat img=Mat::zeros(100,100,CV_8UC3);
      // randu(img, Scalar(0, 0, 0), Scalar(255, 255, 255));

       //gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY);

       //gray_three = cv2.merge([gray,gray,gray]);
       cv::Mat rgb = cv::Mat(nHeight, nWidth, CV_8UC3);
       cvtColor(dst, rgb, CV_GRAY2BGR);
       cvtColor(dst, img, CV_GRAY2BGR);
       //rgb = dst;
       //dst = rgb;


       //create brightness mask
       cv::Mat mask1 = cv::Mat(nHeight, nWidth, CV_8UC3);
       cv::inRange(img, cv::Scalar(110, 110, 110), cv::Scalar(215, 215, 215), mask1);
       vector<vector<cv::Point>> contours;
       cv::findContours(mask1, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

       //exclude outer bones and big area objects
       vector<vector<cv::Point>> suspected_contours;
       vector<vector<cv::Point>>::iterator contour;
       for (contour = contours.begin(); contour != contours.end(); contour++)
           if (cv::contourArea(*contour) > 80 and cv::contourArea(*contour) < 600 and cv::arcLength(*contour, true) <= 300)
               suspected_contours.push_back(*contour);
       cv::Mat mask2 = cv::Mat::zeros(img.rows, img.cols, CV_8UC3);
       cv::drawContours(mask2, suspected_contours, -1, (255), -1);
       cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
       cv::erode(mask2, mask2, kernel);
       cv::cvtColor(mask2, mask2, cv::COLOR_BGR2GRAY);

       //exclude dark areas
       cv::Mat mask = cv::Mat(nHeight, nWidth, CV_8UC3);
       cv::bitwise_and(mask1, mask2, mask);

       //exclude small inner bones
       cv::Mat mask3 = cv::Mat(nHeight, nWidth, CV_8UC3);
       cv::inRange(img, cv::Scalar(215, 215, 215), cv::Scalar(255, 255, 255), mask3);
       kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
       cv::dilate(mask3, mask3, kernel);
       mask = mask - mask3;

       cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

       //exclude elongate objects
       vector<vector<cv::Point>> correct_contours;
       for (contour = contours.begin(); contour != contours.end(); contour++)
       {
           if ((*contour).size() >= 3 and cv::contourArea(*contour) > 80 and cv::contourArea(*contour) < 600)
           {
               cv::Moments M = cv::moments(*contour);
               double cx = M.m10 / M.m00;
               double cy = M.m01 / M.m00;
               double w = 10000;
               double h = 0;
               for (vector<cv::Point>::iterator point = (*contour).begin(); point != (*contour).end(); point++)
               {
                   double distance = sqrt(pow(((*point).x - cx), 2.0) + pow(((*point).y - cy), 2.0));
                   if (distance < w)
                       w = distance;
                   if (distance > h)
                       h = distance;
               }
               if (h / w <= 10)
                   correct_contours.push_back(*contour);
           }
       }

       cv::drawContours(rgb, correct_contours, -1, cv::Scalar(0, 255, 0), 3);
       cv::imshow("suspected", rgb);

       int k = cv::waitKey(0);
       if (k == 27)  //Esc
           break;

       delete image;
   }
}

MainWindow::~MainWindow()
{
    delete ui;
}




