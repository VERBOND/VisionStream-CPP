#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std::chrono;


struct Frame
{
    cv::Mat data;
    steady_clock::time_point timestamp;
};


class CameraSource
{
private:
    cv::VideoCapture capture;
    std::string devicePath;
    int config[2]; //w/h
    int targetFps;

public:
    CameraSource(std::string dev, int w, int h, int fps)
    {
        devicePath = dev;
        config[0] = w;
        config[1] = h;
        targetFps = fps;
    }

    bool tryOpen()
    {
        if (std::isdigit(devicePath[0])) capture.open(std::stoi(devicePath));
        else capture.open(devicePath);

        if (!capture.isOpened()) return false;
        //!!!!!!!!!!!!!!!!
        capture.set(cv::CAP_PROP_FRAME_WIDTH, config[0]);
        capture.set(cv::CAP_PROP_FRAME_HEIGHT, config[1]);
        capture.set(cv::CAP_PROP_FPS, targetFps);

        int realSize[2] =
                {
                        static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH)),
                        static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT))
                };

        std::cout << "!!!!CAMERA INITIALIZED: " << realSize[0] << "x" << realSize[1] << std::endl;
        return true;
    }

    bool fetch(cv::Mat& outFrame)
    {
        return capture.read(outFrame);
    }

    void stop()
    {
        capture.release();
    }
};


class Renderer
{
private:
    std::string winName;

public:
    Renderer(std::string name)
    {
        winName = name;
        cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);
    }

    void draw(cv::Mat& img, int fps, long long lat, int dropped)
    {
        std::string info = "FPS: " + std::to_string(fps) +
                           " | LATENCY: " + std::to_string(lat) + "ms" +
                           " | DROPPED: " + std::to_string(dropped);

        cv::putText(img, info, cv::Point(20, 40), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
        cv::imshow(winName, img);
    }
};


class App
{
private:
    CameraSource camera;
    Renderer renderer;

    struct
    {
        cv::Mat frame;
        steady_clock::time_point time;
        std::mutex mtx;
        std::atomic<bool> ready{false};
        std::atomic<int> dropped{0};
    } shared;

public:
    App(std::string dev, int w, int h, int f)
            : camera(dev, w, h, f), renderer(dev) {}

    void exec()
    {

        std::jthread cap_thread([&](std::stop_token stoken)
                                {
                                    while (!stoken.stop_requested())
                                    {
                                        if (!camera.tryOpen())
                                        {
                                            std::cerr << "!!!!CAMERA NOT FOUND. RECONNECTING..." << std::endl;
                                            std::this_thread::sleep_for(std::chrono::seconds(2));
                                            continue;
                                        }

                                        while (!stoken.stop_requested())
                                        {
                                            cv::Mat tmp;
                                            if (!camera.fetch(tmp))
                                            {
                                                std::cerr << "!!!!STREAM INTERRUPTED" << std::endl;
                                                break; // Спроба перепідключення
                                            }

                                            {
                                                std::lock_guard<std::mutex> lock(shared.mtx);
                                                if (shared.ready) shared.dropped++; // DROP КАДРІВ ДЛЯ MIN LATENCY
                                                shared.frame = std::move(tmp);
                                                shared.time = steady_clock::now();
                                                shared.ready = true;
                                            }
                                        }
                                        camera.stop();
                                    }
                                });


        auto last_fps_check = steady_clock::now();
        int frame_count = 0;
        int current_fps = 0;

        while (true)
        {
            cv::Mat frame_to_draw;
            steady_clock::time_point c_time;
            bool has_data = false;

            {
                std::lock_guard<std::mutex> lock(shared.mtx);
                if (shared.ready)
                {
                    frame_to_draw = shared.frame.clone();
                    c_time = shared.time;
                    shared.ready = false;
                    has_data = true;
                }
            }

            if (has_data)
            {
                frame_count++;
                auto now = steady_clock::now();
                long long latency = duration_cast<milliseconds>(now - c_time).count();

                auto elapsed = duration_cast<milliseconds>(now - last_fps_check).count();
                if (elapsed >= 1000)
                {
                    current_fps = frame_count;
                    frame_count = 0;
                    last_fps_check = now;
                    std::cout << "!!!!STATS: FPS=" << current_fps << " LAT=" << latency << "ms" << std::endl;
                }

                renderer.draw(frame_to_draw, current_fps, latency, shared.dropped);
            }

            char key = static_cast<char>(cv::waitKey(1));
            if (key == 'q' || key == 27)
            {
                std::cout << "!!!!TERMINATING BY USER" << std::endl;
                break;
            }
        }
    }
};


void parseArgs(int argc, char** argv, std::string& dev, int* s, int& f)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--device" && i + 1 < argc) dev = argv[++i];
        else if (arg == "--width" && i + 1 < argc) s[0] = std::stoi(argv[++i]);
        else if (arg == "--height" && i + 1 < argc) s[1] = std::stoi(argv[++i]);
        else if (arg == "--fps" && i + 1 < argc) f = std::stoi(argv[++i]);
    }
}

int main(int argc, char** argv)
{
    std::string device = "0";
    int size[2] = {
            1280, 720
    };
    int fps = 30;

    parseArgs(argc, argv, device, size, fps);

    App app(device, size[0], size[1], fps);
    app.exec();

    return 0;
}