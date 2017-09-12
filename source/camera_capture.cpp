#include "opencv_tool.hpp"
#include <iostream>  
#include <signal.h>
#include <stdint.h>

using namespace std;
using namespace cv;

#define max(x,y) (x>y?x:y)
#define min(x,y) (x<y?x:y)
#define y(r,g,b) (((66 * r + 129 * g + 25 * b + 128) >> 8) + 16)
#define u(r,g,b) (((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128)
#define v(r,g,b) (((112 * r - 94 * g - 18 * b + 128) >> 8) + 128)
#define color(x) ((unsigned char)((x < 0) ? 0 : ((x > 255) ? 255 : x)))

#define RGBA_YUV420SP 0x00004012
#define BGRA_YUV420SP 0x00004210
#define RGBA_YUV420P 0x00014012
#define BGRA_YUV420P 0x00014210
#define RGB_YUV420SP 0x00003012
#define RGB_YUV420P 0x00013012
#define BGR_YUV420SP 0x00003210
#define BGR_YUV420P 0x00013210

void rgbaToYuv(int width, int height, unsigned char * rgb, unsigned char * yuv, int type)
{
    const int frameSize = width * height;
    const int yuvType = (type & 0x10000) >> 16;
    const int byteRgba = (type & 0x0F000) >> 12;
    const int rShift = (type & 0x00F00) >> 8;
    const int gShift = (type & 0x000F0) >> 4;
    const int bShift = (type & 0x0000F);
    const int uIndex = 0;
    const int vIndex = yuvType; //yuvType为1表示YUV420p,为0表示420sp

    int yIndex = 0;
    int uvIndex[2] = { frameSize, frameSize + frameSize / 4 };

    unsigned char R, G, B, Y, U, V;
    unsigned int index = 0;
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            index = j * width + i;

            R = rgb[index*byteRgba + rShift] & 0xFF;
            G = rgb[index*byteRgba + gShift] & 0xFF;
            B = rgb[index*byteRgba + bShift] & 0xFF;

            Y = y(R, G, B);
            U = u(R, G, B);
            V = v(R, G, B);

            yuv[yIndex++] = color(Y);
            if (j % 2 == 0 && index % 2 == 0) {
                yuv[uvIndex[uIndex]++] = color(U);
                yuv[uvIndex[vIndex]++] = color(V);
            }
        }
    }
}

IplImage *camFrame = NULL;
CvCapture *cam = NULL;

//-------信号函数------------
void process(int)
{
    cvReleaseCapture(&cam);//释放CvCapture结构
    exit(0);
}

int main()
{
    FILE *out = fopen("out_video.yuv", "wb");
    int32_t YUV_buffersize = 640 * 480 * 1.5;
    uint8_t *YUV_buffer = (uint8_t *)malloc(YUV_buffersize);
    Mat rgbImg;
    Mat yuvImg;
    signal(SIGINT, process);//关闭信号（此处的SIGINT 为中断信号，即是键盘上的Ctrl + c）
    namedWindow("视频窗口");
    while (1) {
        //----------图片采样-------------
        cam = cvCreateCameraCapture(0);//初始化从摄像头中获取视频
        camFrame = cvQueryFrame(cam);//从摄像头或者文件中抓取并返回一帧
        rgbImg = Mat(camFrame);//图片转换

        waitKey(5);//时间等待

        /*采用opencv自带函数*/
        //cvtColor(rgbImg, yuvImg, CV_BGR2YUV_I420);
        //memcpy(YUV_buffer, yuvImg.data, YUV_buffersize * sizeof(unsigned char));//YUV_buffer即为所获取的YUV420数据  

        /*快速转换算法*/
        rgbaToYuv(rgbImg.cols, rgbImg.rows, rgbImg.data, YUV_buffer, BGR_YUV420P);

        fwrite(YUV_buffer, YUV_buffersize, 1, out);
        imshow("视频窗口", rgbImg);  //显示图片
    }
    free(YUV_buffer);
    fclose(out);
    return 0;
}