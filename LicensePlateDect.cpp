#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace cv;
using namespace std;

//思路一：灰度图直接二值化
Mat Binary(Mat src)
{
    Mat img_grey;
    cvtColor(src,img_grey,COLOR_BGR2GRAY);//读取灰度图
    //imshow("img_grey",img_grey);
    
    Mat img_o;
    int thr=150;
    threshold(img_grey, img_o, 150, 255, CV_THRESH_OTSU);//threshold(img_grey, img, 150, 255, CV_THRESH_BINARY);
    return(img_o);        
}

/*
Mat Adapt_Binary(Mat src)
{
    Mat img_grey;
    cvtColor(src,img_grey,COLOR_BGR2GRAY);//读取灰度图
    imshow("img_grey",img_grey);
    Mat img_a;
    adaptiveThreshold(img_grey,img_a,150,ADAPTIVE_THRESH_GAUSSIAN_C,THRESH_BINARY,7,0);
    return(img_a);
}


    Mat adapt_bin=Adapt_Binary(img);
    imshow("adapt_bin",adapt_bin);
*/

/*思路二：由于车牌底色是固定的(蓝色、黄色、绿色)，是否可以只提取该具有特定颜色的连通区域
  本方案的难点在于车牌的HSV颜色难以确定,即使都是蓝牌，在不同的相机与光线下也有不同的色彩，可考虑更换色域空间或者使用平均值和方差求出合适的色域再修改像素*/
Mat colorFilter(Mat src)
{
    Mat img=src,img_hsv;
    Mat blur;
    medianBlur(src,blur,7);
    cvtColor(blur,img_hsv,CV_BGR2HSV); //转为HSV空间，方便提取颜色
    for (int i = 0; i < img_hsv.cols; i++)
    {
        for (int j = 0; j < img_hsv.rows; j++)
        {
            int h=img_hsv.at<Vec3b>(j,i)[0];        
            int s=img_hsv.at<Vec3b>(j,i)[1];
            int v=img_hsv.at<Vec3b>(j,i)[2];
            if(h>=100 && h<=120 && s>=80 && s<=240 && v>=80 && v<=240)
            {
                img.at<Vec3b>(j,i)=Vec3b(0,0,0);
            }
            else
            {
                img.at<Vec3b>(j,i)=Vec3b(255,255,255);
            }
        }        
    }
    return(img);
}

vector<vector<Point>> rectC(Mat src)
{
    vector<vector<Point>> contours;
    findContours(src,contours,noArray(),RETR_LIST,CHAIN_APPROX_SIMPLE);
    //提取除边框外面积最大的轮廓
    int maxArea=-1;
    for (int i = 0; i < contours.size(); i++)
    {
        if(contourArea(contours[i])<0.5*src.rows*src.cols && contourArea(contours[i])>0.005*src.rows*src.cols && contourArea(contours[i])>contourArea(contours[maxArea]))//0.5作为系数可以更改，但不能为1
        {
            maxArea=i;
        }
    }
    //近似出矩形
    vector<vector<Point>> rectContours(1);    
    approxPolyDP(contours[maxArea],rectContours[0],10,true);//10是近似截止的阈值，越大计算量越小且可以滤去无用的边，但要注意src中车牌的大小，以免损失重要的边 
                                                         //TODO:可以考虑将该值设置为随总边长变化(我国车牌的长宽比是固定的，可以通过计算边长得到车牌的宽，从而让该值不超过车牌的宽)
    if(maxArea>=0)
    {
        return rectContours;
    }
     
    
}
    
Mat cutImg(Mat src, vector<vector<Point>> rectContours)
{
    //提取凸包
    vector<int> hull;
    convexHull(rectContours[0],hull,false);
    //排序
    int xmin=rectContours[0][0].x,
        xmax=rectContours[0][0].x,
        ymin=rectContours[0][0].y,
        ymax=rectContours[0][0].y;
    for (int i = 0; i < rectContours[0].size(); i++)
    {
        if(rectContours[0][i].x>=xmax) xmax=rectContours[0][i].x;
        if(rectContours[0][i].y>=ymax) ymax=rectContours[0][i].y;
        if(rectContours[0][i].x<=xmin && rectContours[0][i].x!=0) xmin=rectContours[0][i].x;
        if(rectContours[0][i].y<=ymin && rectContours[0][i].y!=0) ymin=rectContours[0][i].y;
    }
    //裁剪出车牌
    int width=xmax-xmin;
    int height=ymax-ymin;
    Rect lp (xmin,ymin,width,height);
    Mat LP=src(lp);
    return LP;
}

int main()
{

    Mat img=imread("C:/Users/25793/Desktop/OpenCV/LicensePlateDetection/negative/test.jpg",1);
    Mat imgC=imread("C:/Users/25793/Desktop/OpenCV/LicensePlateDetection/negative/test.jpg",1);
    imshow("oringin",img);    

    Mat color_bin=colorFilter(img);
    Mat rectArea=Binary(color_bin);//TODO:colorFilter已经是二值化过程，为何此处还需要二值化？
    imshow("rectArea",rectArea);

    vector<vector<Point>> rectContours(1);
    rectContours=rectC(rectArea);
    Mat rect_contour=Mat::zeros(rectArea.rows,rectArea.cols,CV_8U);
    drawContours(rect_contour,rectContours,-1,Scalar::all(255));
    imshow("contours",rect_contour);
    
    Mat LP=cutImg(imgC,rectContours);    
    imshow("LP",LP);

    /*
    Mat img_bin=Binary(imgC);
    imshow("img_bin",img_bin);
    */

    /*
    Mat img_canny;
    Canny(img_bin,img_canny,800,1000,3,0);
    */

    /*
    int index=contours.size();
    vector<vector<Point>> rectContours(index);
    for(int i=0; i<index;i++)
    {
        approxPolyDP(contours[i],rectContours[i],5,1);
    }
    Mat img_rect=Mat::zeros(img.rows,img.cols,CV_8U);
    drawContours(img_rect,rectContours,-1,Scalar::all(255));
    imshow("rectContours",img_contour);
    */

    waitKey(0);
    return(0);
}