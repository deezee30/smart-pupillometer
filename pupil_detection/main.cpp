#include <stdio.h>

#include <map>
#include <cmath>
#include <deque>
#include <filesystem>
#include <numeric>
#include <sstream>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

// Definitions
#define RECORDING_DIR  "recordings"     // location at which PD recordings are saved
#define WINDOW_TITLE   "Pupil Detector" // camera window title

using namespace cv;
using namespace std;
using namespace std::chrono;
using namespace std::filesystem;

// Constants
static const int    MONITOR_FRAMES  = 15; // no. of frames to monitor for FPS calculation
static const double ZOOM            = 2.; // zoom amount

// Internal variables
int text_lines = 0;

int terminate(const string& msg) {
    cout << msg << endl;
    cout << "Press any key to close..." << endl;
    cin.get(); // wait for any key press
    destroyAllWindows();
    return EXIT_FAILURE;
}

void add_text(Mat frame, int tot_rows, const string& text,
              Scalar col = CV_RGB(255, 255, 255)) {
    text_lines += 1;
    putText(frame, text, Point(15, (int) (tot_rows - 15*text_lines)),
            FONT_HERSHEY_DUPLEX, 0.4, col);
}

long long now() {
    auto time_pt = high_resolution_clock::now().time_since_epoch();
    milliseconds ms = duration_cast<milliseconds>(time_pt);
    return ms.count();
}

string d2s(double d, int i = 0) {
    if (i == 0) return to_string((int) d);
    stringstream stream;
    stream << std::fixed << std::setprecision(i) << d;
    return stream.str();
}

int main(int, char**) {
    // TODO: Handle arguments

    // Set up input stream and configure
    VideoCapture cap(0); // set up stream
    cap.set(CAP_PROP_BUFFERSIZE, 2);
    cap.set(CAP_PROP_FPS, 25);
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));
    cap.set(CAP_PROP_FRAME_WIDTH, 320);
    cap.set(CAP_PROP_FRAME_HEIGHT, 240);

    // Ensure video is successfully opened
    if (!cap.isOpened()) return terminate("Failed to open camera!");

    // Define width and height of video and zoom properties
    int width0  = cap.get(CAP_PROP_FRAME_WIDTH);  // original width of frames of video
    int height0 = cap.get(CAP_PROP_FRAME_HEIGHT); // original height of frames of video
    int width   = width0 /ZOOM;         // zoomed width
    int height  = height0/ZOOM;         // zoomed height
    int roi_x   = (width0  - width )/2; // zoomed roi x pos
    int roi_y   = (height0 - height)/2; // zoomed roi y pos

    // Handle FPS queue
    deque<double> q(0); // render FPS
    double fps_camera = cap.get(CAP_PROP_FPS); // camera FPS
    if (fps_camera == 0.) fps_camera = 30.; // 30 is default if input FPS unavailable

    // Handle codec
    int ex = static_cast<int>(cap.get(CAP_PROP_FOURCC)); // get codec type - int form
    char EXT[] = {(char) (ex & 0XFF), // transform from int to char[] via bitwise operators
                  (char) ((ex & 0XFF00) >> 8),
                  (char) ((ex & 0XFF0000) >> 16),
                  (char) ((ex & 0XFF000000) >> 24), 0};
    
    // Recording
    bool rec = false;
    optional<long long> rec_init;
    map<int, int> rec_pd;

    namedWindow(WINDOW_TITLE);

    // Temp vars
    string path;
    VideoWriter out;

    for (;;) {
        long long start = now(); // record timing for current tick to determine FPS

        if (rec && !rec_init) {
            rec_init = {start};
        
            // Generate recording folder and parent folder(s)
            path = RECORDING_DIR + ('/' + d2s(rec_init.value())); // timestamp of recording
            create_directories(path);

            string path_video = path + "/output.avi";

            // Write processed video
            out.open(path_video, // output path
                     //VideoWriter::fourcc('M', 'J', 'P', 'G'),
                     ex, // codec type (int form)
                     fps_camera, // input camera FPS
                     Size(width, height), // output roi size
                     true); // use colour = true
            
            // Check if ok
            if (!out.isOpened()) return terminate("Could not open the output video " + path_video);

            cout << "Generated " + path_video << endl;
        }

        long long t1 = now();
        Mat frame; // current frame
        bool ok = cap.read(frame); // read frame of video
        cout << (now() - t1) << "ms" << endl;
        if (ok) { // if read is successful
            // Zoom
            Mat roi = frame(Rect(roi_x, roi_y, width, height)); // region of image

            // TODO: Finish processing
            //Mat roi_gr;
            //cvtColor(roi, roi_gr, COLOR_BGR2GRAY);
            //GaussianBlur(roi_gr, roi_gr, Size(11, 11), 0);
            //medianBlur(roi_gr, roi_gr, 3);

            //double th = threshold(roi_gr, roi_gr, 15, 255, THRESH_BINARY_INV);
            //findContours

            // Pre-computations
            double fps_render = q.empty() ? 0. : accumulate(q.begin(), q.end(), 0.)/q.size();
            //cout << accumulate(q.begin(), q.end(), 0.) << endl;
            //cout << q.size() << endl;

            add_text(roi, height, "[Press Q to Exit]", CV_RGB(200, 255, 0)); // exit message
            add_text(roi, height, "Render FPS: " + d2s(fps_render, 1));
            add_text(roi, height, "Camera FPS: " + d2s(fps_camera, 1));
            add_text(roi, height, "Zoom: " + d2s(ZOOM, 1));
            add_text(roi, height, "Codec: " + string(EXT));
            if (rec) {
                int ms = now() - rec_init.value(); // get elapsed time for tick in milliseconds
                string s = d2s(ms/1000);
                add_text(roi, height, "[R] " + s + 's', CV_RGB(255, 0, 0)); // [R] status

                // Save pupil diameter when recording
                rec_pd[ms] = 0; // FIXME: Use actual PD
            } else if (rec_init) {
                string id = d2s(rec_init.value()); // ID = timestamp of the recording

                // TODO: Save PD history to CSV file

                // Write out to file
                out.write(roi);

                // Reset
                rec_init.reset(); // reset recording timer
                rec_pd.clear(); // reset recording PD
            }

            text_lines = 0; // reset text positioning

            imshow(WINDOW_TITLE, roi);
        } else {
            cap.set(CAP_PROP_POS_FRAMES, 0); // restart frame decoding
        }

        // Record FPS
        if (q.size() == MONITOR_FRAMES) q.pop_back(); // erase out-of-bound tick
        double s = (now() - start) / 1000.; // get elapsed time for tick in seconds
        q.push_front(1./s); // inverse elapsed time to get FPS

        // Hot keys
        char k = waitKey(1) & 0xFF;
        if      (k != -1)  cout << "> " << k << endl;
        if      (k == 'q') break;      // press 'Q' on the keyboard to exit the playback
        else if (k == 'r') rec = !rec; // toggle recording
    }

    cap.release();
    out.release();
    destroyAllWindows();

    return EXIT_SUCCESS;
}