import cv2 # Computer vision
import numpy as np
import argparse # Command parser
import time # Timing
from pathlib import Path # file handling
import csv # For diameter history analysis
from collections import deque
from threading import Thread
from datetime import datetime

# Constants
MONITOR_FRAMES      = 15                # no. of frames to monitor for FPS calculation
ZOOM                = 2.5               # zoom amount
RECORDING_DIR       = "recordings"      # location at which PD recordings are saved
WINDOW_TITLE        = "Pupil Detector"  # camera window title

# Text positioning
TOP_LEFT     = 0
TOP_RIGHT    = 1
BOTTOM_LEFT  = 2
BOTTOM_RIGHT = 3

# Helper functions
def now():
    return int(time.time()*1000)
class TextableFrame(object):
    ''' Helper class to print out text to CV2 '''

    def __init__(self, frame, text_margin = None, text_lines = None):
        self.frame = frame
        self.text_margin = 10          if text_margin == None else text_margin
        self.text_lines  = np.zeros(4) if text_lines  == None else text_lines
    
    def add_text(self, pos, text, col = (255, 255, 255)):
        self.text_lines[pos] += 1

        x_off = int(self.frame.shape[1] - 100)
        y_off = self.text_margin * self.text_lines[pos]

        x = x_off if (pos == TOP_RIGHT or pos == BOTTOM_RIGHT) else self.text_margin
        y = y_off if (pos == TOP_LEFT  or pos == TOP_RIGHT)    else self.frame.shape[0] - y_off

        cv2.putText(self.frame,
                    text = text,
                    org = (int(x), int(y)),
                    fontFace = cv2.FONT_HERSHEY_DUPLEX,
                    fontScale = 0.3,
                    color = col)

class VideoStream(object):
    def __init__(self, src):
        # Set up input stream and configure
        self.cap = cv2.VideoCapture(src)
        
        # Ensure video is successfully opened
        if not self.cap.isOpened(): return self._terminate("Failed to open camera!")

        # Define width and height of video and zoom properties
        self.width0  = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH));  # original width of video
        self.height0 = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT)); # original height of video
        self.width   = int(self.width0  / ZOOM);              # zoomed width
        self.height  = int(self.height0 / ZOOM);              # zoomed height
        self.roi_x   = int((self.width0  - self.width ) / 2); # zoomed roi x pos
        self.roi_y   = int((self.height0 - self.height) / 2); # zoomed roi y pos

        # Handle FPS
        self.q = deque(maxlen = MONITOR_FRAMES) # render FPS
        self.fps_camera = self.cap.get(cv2.CAP_PROP_FPS) # camera FPS

        # Handle codec
        self.ex = cv2.VideoWriter_fourcc(*"MJPG") # get codec type - int form
        self.ext = "".join([chr((int(self.ex) >> 8 * i) & 0xFF) for i in range(4)]) # transform to str

        cv2.namedWindow(WINDOW_TITLE)

        # Video output
        self.out = None

        # Start the thread to read frames from the video stream
        self.frame_ok = False
        self.thread = Thread(target=self.update, args=())
        self.thread.daemon = True
        self.thread.start()

    def update(self):
        # Read the next frame from the stream in a different thread
        while True:
            if self.cap.isOpened():
                # Record timing for current tick to determine FPS [ms]
                start = now()

                # Read frame
                self.frame_ok, self.frame = self.cap.read()

                # Record FPS
                secs = (now()-start) / 1000 # get elapsed time for tick in seconds
                if secs > 0: self.q.append(1./secs) # inverse elapsed time to get FPS
    
    def setup_output(self, path):
        self.out = cv2.VideoWriter(path, # output path
                                   self.ex, # codec type (int form)
                                   self.fps_camera, # input camera FPS
                                   (self.width, self.height), # output roi size
                                   isColor = True) # use colour = true
        
        # Check if ok
        if not self.out.isOpened(): return self._terminate(f"Could not open output video {path}")

    def render_frame(self, pd, pd_pct = -1, rec_elapsed = -1):        
        # Await until read is successful
        while not self.frame_ok: continue

        # Zoom
        roi = self.frame[self.roi_y:self.roi_y+self.height,
                         self.roi_x:self.roi_x+self.width] # region of image
        
        gray_roi = cv2.cvtColor(roi, cv2.COLOR_BGR2GRAY)
        gray_roi = cv2.GaussianBlur(gray_roi, (11, 11), 0)
        gray_roi = cv2.medianBlur(gray_roi, 3)

        threshold = cv2.threshold(gray_roi, 15, 255, cv2.THRESH_BINARY_INV)[1]
        contours = cv2.findContours(threshold, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)[0]
        contours = sorted(contours, key=lambda x: cv2.contourArea(x), reverse=True)

        # Pre-computations
        self.fps_render = 0 if len(self.q) == 0 else np.mean(self.q) # Rendering FPS

        tf = TextableFrame(roi)

        # Top left
        tf.add_text(TOP_LEFT, f"Image: {self.height} x {self.width}")
        tf.add_text(TOP_LEFT, f"Zoom: {ZOOM:.1f}")
        tf.add_text(TOP_LEFT, f"Codec: {self.ext}")
        # Top right
        tf.add_text(TOP_RIGHT, f"Render FPS: {self.fps_render:.1f}")
        tf.add_text(TOP_RIGHT, f"Camera FPS: {self.fps_camera:.1f}")
        # Bottom left
        if pd_pct      != -1: tf.add_text(BOTTOM_LEFT, f"PD increase: {pd_pct:.1f}%")
        if rec_elapsed != -1: tf.add_text(BOTTOM_LEFT, f"[R] {rec_elapsed:.3f}s", (0, 0, 255))
        # Bottom right
        tf.add_text(BOTTOM_RIGHT, "[Press Q to Exit]", (0, 200, 255))

        if len(contours) > 0:
            cnt = contours[0]
            (x, y, w, h) = cv2.boundingRect(cnt) # minimum bounding box around binary contour
            pd = int(h/2) # update PD
            cv2.circle(roi,  (x + w//2,      y + h//2), pd,           (0,  0, 255), 2)
            cv2.line(roi,    (x + w//2, 0), (x + w//2,  self.height), (50, 200, 0), 1)
            cv2.line(roi, (0, y + h//2),    (self.width, y + h//2),   (50, 200, 0), 1)
            
        cv2.imshow(WINDOW_TITLE, roi)

        return roi, pd

    def save_frame(self, roi):
        # Write out to file
        self.out.write(roi)
    
    def close(self):
        self.cap.release()
        if self.out is not None: self.out.release()
        cv2.destroyAllWindows()

    def _terminate(self, msg):
        print(msg)
        input("Press 'Enter' to close...")
        cv2.destroyAllWindows()
        return -1

# Main exec
def main():
    # Handle arguments
    parser = argparse.ArgumentParser(description="Pupil detector")
    parser.add_argument("--src", default=0, required=False,
                        help="Location of video source, or 0 for camera device.")
    args = parser.parse_args()

    # out
    path = ""

    # Recording
    rec = False
    rec_init = 0
    rec_pd = {}
    rec_elapsed = -1

    # Pupil diameter
    pd_std = 0 # calibrated pupil diameter
    pd_cal = False # whether or not due to calibrate pupil diameter
    pd = 0 # current pupil diameter
    perc = -1

    # Set up video stream and run in a separate thread
    vs = VideoStream(args.src)
    while True:
        if rec and rec_init == 0: # start of recording
            rec_init = now()

            # Generate recording folder and parent folder(s)
            path = "{rd}/{ts}".format(rd=RECORDING_DIR, # timestamp of the recording
                    ts=datetime.fromtimestamp(rec_init//1000).strftime("%Y-%m-%d %H.%M.%S"))
            Path(path).mkdir(parents=True, exist_ok=True)

            path_video = f"{path}/output.avi"

            # Open processed output video
            vs.setup_output(path_video)

            print(f"Generated '{path_video}'")
        
        try:
            frame, pd = vs.render_frame(pd, perc, rec_elapsed) # render and obtain ROI and PD
        except Exception as err:
            return vs._terminate("Error caught in main loop:\n" + str(err))

        #### Post-processing for next frame ####

        # Get elapsed time for tick
        if rec: rec_elapsed = round((now()-rec_init)/1000, 3) # [s]

        # Detection of pupil
        if pd > 0:
            if pd_cal: # if pupil diameter is due a calibration
                pd_std = pd # set new pupil diameter standard
                pd_cal = False # toggle calibration off

            # Update PD history if recording is enabled
            if rec: rec_pd[rec_elapsed] = pd # [s]

            # Adjust relative percentage if a standard has been set
            if pd_std != 0: perc = 100*(pd-pd_std)/pd_std
        
        # Write recorded video to file
        if rec: vs.save_frame(frame)

        # Reset parameters if recording has been toggled off
        if not rec and rec_init != 0:
            # Save PD history to CSV file
            with open(f"{path}/pd_history.csv", "w", newline="") as csv_file:
                writer = csv.writer(csv_file)
                writer.writerow(["Time [ms]", "Relative PD"])
                for ms in rec_pd:
                    writer.writerow([ms, rec_pd[ms]])

            # Reset
            rec_init = 0 # reset recording timer
            rec_pd = {} # reset recording PD
            rec_elapsed = -1 # reset elapsed recorded seconds
        
        # Hot keys
        k = cv2.waitKey(1) & 0xFF
        if   k != 0xFF:     print(f"> {chr(k)}") # display pressed key
        if   k == ord("q") and ~rec: break # [Q] exit playback when not recording
        elif k == ord("r"): rec = ~rec     # [R] toggle recording
        elif k == ord("c"): pd_cal = True  # [C] toggle pupil diameter calibration

    vs.close()

if __name__ == "__main__": main()