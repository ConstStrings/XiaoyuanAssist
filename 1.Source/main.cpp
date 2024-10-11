#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <windows.h>
#include<direct.h>    
#include<io.h>  

using namespace std;
using namespace cv;

struct Match {
    cv::Rect rect;
    double score;
    int num;
};

Mat Screenshoot;

std::vector<Match> detect_digits(cv::Mat targetImage)
{

    // ת��Ŀ��ͼ��Ϊ�Ҷ�ͼ�񲢶�ֵ��
    cv::Mat grayTarget;
    cv::cvtColor(targetImage, grayTarget, cv::COLOR_BGR2GRAY);
    cv::threshold(grayTarget, grayTarget, 127, 255, cv::THRESH_BINARY);

    // ģ��ͼ���ļ����б�
    std::vector<std::string> templateFiles = {
        "0.png", "1.png", "2.png", "3.png", "4.png",
        "5.png", "6.png", "7.png", "8.png", "9.png"
    };

    // ƥ����ֵ���ص���ֵ
    double matchThreshold = 0.8;
    double overlapThreshold = 0.5;

    // �洢����ƥ����
    std::vector<Match> matches;

    for (int num = 0; num <= 9; ++num) {
        cv::Mat templateImage = cv::imread(std::string("./src/") + templateFiles[num]);
        if (templateImage.empty()) {
            std::cerr << "Can not load Templete!" << templateFiles[num] << std::endl;
            continue;
        }

        // ת��ģ��ͼ��Ϊ�Ҷ�ͼ�񲢶�ֵ��
        cv::Mat grayTemplate;
        cv::cvtColor(templateImage, grayTemplate, cv::COLOR_BGR2GRAY);
        cv::threshold(grayTemplate, grayTemplate, 127, 255, cv::THRESH_BINARY);

        // ģ��ƥ��
        cv::Mat result;
        cv::matchTemplate(grayTarget, grayTemplate, result, cv::TM_CCOEFF_NORMED);

        // ��������ƥ��ֵ������ֵ������
        for (int y = 0; y < result.rows; ++y) {
            for (int x = 0; x < result.cols; ++x) {
                double score = result.at<float>(y, x);
                if (score >= matchThreshold) {
                    cv::Rect matchRect(x, y, templateImage.cols, templateImage.rows);
                    matches.push_back({ matchRect, score, num });
                }
            }
        }
    }

    // ��ƥ�������зǼ���ֵ����
    std::vector<Match> finalMatches;
    std::vector<bool> suppressed(matches.size(), false);

    for (size_t i = 0; i < matches.size(); ++i) {
        if (suppressed[i]) continue;
        finalMatches.push_back(matches[i]);

        for (size_t j = i + 1; j < matches.size(); ++j) {
            if (suppressed[j]) continue;

            double intersectionArea = (matches[i].rect & matches[j].rect).area();
            double unionArea = matches[i].rect.area() + matches[j].rect.area() - intersectionArea;
            double iou = intersectionArea / unionArea;

            if (iou > overlapThreshold) {
                suppressed[j] = true;
            }
        }
    }

    // ���պ������꣨x ���꣩����
    std::sort(finalMatches.begin(), finalMatches.end(), [](const Match& a, const Match& b) {
        return a.rect.x < b.rect.x;
        });

    // ���ƽ�������
    for (const auto& match : finalMatches) {
        cv::rectangle(targetImage, match.rect, cv::Scalar(0, 255, 0), 2);
        std::string label = "Digit " + std::to_string(match.num);
        cv::putText(targetImage, label, match.rect.tl(), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
        std::cout << "Number: " << match.num << " Position: (" << match.rect.x << ", " << match.rect.y << ")\n";
    }

    cv::imshow("Detected Numbers Sorted", targetImage);
    
    return finalMatches;
}

int merge_vector(std::vector<int> v)
{
    int num = 0;
    for (int i = 0; i < v.size(); i++)
    {
        num += v[i] * pow(10, v.size() - 1 - i);
    }
    return num;
}

int compare_match(std::vector<Match> result)
{
    static int pre_num1;
    static int pre_num2;
    int repeat_flag = 0;
    
    std::vector<int> num1,num2;
    for (const auto& match : result) {
        if (match.rect.x < 200)
            num1.push_back(match.num);
        else 
            num2.push_back(match.num);
    }
    int result1 = merge_vector(num1);
    int result2 = merge_vector(num2);

    std::cout << "Numbe merge1: " << result1 << std::endl;
    std::cout << "Numbe merge2: " << result2 << std::endl;

    if (result1 == pre_num1 && result2 == pre_num2)
        repeat_flag = 1;

    pre_num1 = result1;
    pre_num2 = result2;

    if (repeat_flag == 1)
    {
        repeat_flag = 0;
        return 0;
    }

    if (result1 > result2)
        return 1;
    else if (result1 < result2)
        return -1;
    else
        return 0;
}

void refreshSrcImage(string Path) {
    system("adb -s 127.0.0.1:7555 shell screencap -p /sdcard/ScreenCatch.png");

    string file_path = Path;
    file_path.append("\\adb\\temp");
    if (_access(file_path.c_str(), 0) == -1)//����ֵΪ-1����ʾ������
    {
        cout << "Path target is created at " << file_path << endl;
        system(string("md ").append(file_path).c_str());
    }

    string msg = "adb -s 127.0.0.1:7555 pull /sdcard/ScreenCatch.png ";
    msg.append(Path);
    msg.append("\\adb\\temp");
    system(msg.c_str());

    string real_path = Path;
    real_path.append("\\adb\\temp\\ScreenCatch.png");
    Screenshoot = imread(real_path.c_str());

    resize(Screenshoot, Screenshoot, Size(786, 1398));
}

std::string GetProgramDir()
{
    wchar_t exeFullPath[MAX_PATH]; // Full path 
    std::string strPath = "";

    GetModuleFileName(NULL, exeFullPath, MAX_PATH);
    char CharString[MAX_PATH];
    size_t convertedChars = 0;
    wcstombs_s(&convertedChars, CharString, MAX_PATH, exeFullPath, _TRUNCATE);

    strPath = (std::string)CharString;    // Get full path of the file 

    int pos = strPath.find_last_of('\\', strPath.length());
    return strPath.substr(0, pos);  // Return the directory without the file name 
}

int process(void)
{
    //cv::Mat srcImage = cv::imread("./test/shoot2.png");
    //if (srcImage.empty()) {
    //    std::cerr << "Can not load image��" << std::endl;
    //    return -1;
    //}
    cv::Mat srcImage = Screenshoot;

    cv::resize(srcImage, srcImage, cv::Size(786, 1398));

    cv::Rect roiRect(200, 360, 400, 200);

    // ��� ROI �Ƿ񳬳�ͼ��߽�
    if (roiRect.x >= srcImage.cols || roiRect.y >= srcImage.rows ||
        roiRect.x + roiRect.width > srcImage.cols || roiRect.y + roiRect.height > srcImage.rows) {
        std::cerr << "ROI Flow Out��" << std::endl;
        return -1;
    }

    // ��ԭͼ�ϻ��� ROI ����
    cv::rectangle(srcImage, roiRect, cv::Scalar(0, 255, 0), 2);
    cv::Mat roiImage = srcImage(roiRect);

    // ��ʾ���ͼ��
    //cv::imshow("ScreenShot", srcImage);

    std::vector<Match> result = detect_digits(roiImage);

    int cmp = compare_match(result);

    std::cout << "Result: " << cmp << std::endl;

    return cmp;
}

void action_bigger(void)
{
    char buffer[100];
    sprintf_s(buffer, "adb shell input swipe %d %d %d %d 1", 664, 773, 802, 844);
    system(buffer);
    sprintf_s(buffer, "adb shell input swipe %d %d %d %d 1", 802, 844, 696, 942);
    system(buffer);

    std::cout << "Excute action: Write bigger." << std::endl;
}

void action_smaller(void)
{
    char buffer[100];
    sprintf_s(buffer, "adb shell input swipe %d %d %d %d 1", 805, 768, 652, 835);
    system(buffer);
    sprintf_s(buffer, "adb shell input swipe %d %d %d %d 1", 652, 835, 781, 938);
    system(buffer);

    std::cout << "Excute action: Write smaller." << std::endl;
}

void action(int cmp)
{
    if (cmp == 1)
        action_bigger();
    if (cmp == -1)
        action_smaller();
}

int main() {
   
    string Path = GetProgramDir();
    system("adb connect 127.0.0.1:7555");
    
    while (true)
    {
        refreshSrcImage(Path);

        int cmp = process();

        action(cmp);

        char c = waitKey(1);
        if (c == 27)
        {
            break;
        }
    }

    return 0;
}



