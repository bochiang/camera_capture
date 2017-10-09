#include "opencv_tool.hpp"
#include <iostream>  
#include <signal.h>
#include <stdint.h>
#include <time.h>

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

int main()
{
    int width = 320;               //设置图像分辨率
    int height = 240;

    FILE *out = fopen("out_video.yuv", "wb");
    int32_t YUV_buffersize = width * height * 1.5;
    uint8_t *YUV_buffer = (uint8_t *)malloc(YUV_buffersize);
    VideoCapture capture(0);
    if (!capture.isOpened()) { //判断能够打开摄像头
        cout << "can not open the camera" << endl;
        cin.get();
        exit(1);
    }

    capture.set(CV_CAP_PROP_FRAME_WIDTH, width);
    capture.set(CV_CAP_PROP_FRAME_HEIGHT, height);
    capture.set(CV_CAP_PROP_FPS, 15);

    int count = 0;
    clock_t tm_start;
    clock_t tm_total;
    tm_start = clock();
    while (1) {
        clock_t tm_cur = clock();
        Mat frame;
        Mat yuvImg;
        capture >> frame; //载入图像

        if (frame.empty()) { //判断图像是否载入
            cout << "can not load the frame" << endl;
        }
        else {
            count++;
            if (count == 1) {
                cout << frame.cols << "  " << frame.rows << endl;
            }

            /*采用opencv自带函数*/
            //cvtColor(frame, yuvImg, CV_BGR2YUV_I420);
            //memcpy(YUV_buffer, yuvImg.data, YUV_buffersize * sizeof(unsigned char));//YUV_buffer即为所获取的YUV420数据  

            /*快速转换算法*/
            rgbaToYuv(width, height, frame.data, YUV_buffer, BGR_YUV420P);

            fwrite(YUV_buffer, YUV_buffersize, 1, out);
            imshow("camera", frame);
            char c = waitKey(62); //延时60毫秒
            if (c == 27) //按ESC键退出
                break;
        }
        tm_cur = clock() - tm_cur;
        tm_total = clock() - tm_start;
        printf("%6d: %6d/%8d ms, %8.3f/%8.3f fps\n", count,
            tm_cur, tm_total,
            1000.0 / (tm_cur + 0.001), 1000.0 * count / tm_total);
    }
    free(YUV_buffer);
    fclose(out);
}

