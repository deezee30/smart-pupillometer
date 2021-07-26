import cv2 # Computer vision
import numpy as np
import argparse # Command parser
import time # Timing
from pathlib import Path # file handling
import csv # For diameter history analysis
from collections import deque

# Constants
MONITOR_FRAMES   = 15                # no. of frames to monitor for FPS calculation
ZOOM             = 2.5               # zoom amount
RECORDING_DIR    = "recordings"      # location at which PD recordings are saved
WINDOW_TITLE     = "Pupil Detector"  # camera window title

# Text positioning
TOP_LEFT     = 0
TOP_RIGHT    = 1
BOTTOM_LEFT  = 2
BOTTOM_RIGHT = 3

# Internal variables
text_lines = np.zeros(4)

# Helper functions
def now():
    return int(time.time()*1000)

def terminate(msg):
    print(msg)
    input("Press 'Enter' to close...")
    cv2.destroyAllWindows()
    return 1

def add_text(frame, pos, text, col = (255, 255, 255)):
    ''' Helper function to print out text to CV2 '''

    global text_lines
    text_lines[pos] += 1

    x_off = int(frame.shape[1] - 100)
    y_off = 10*text_lines[pos]

    x = x_off if (pos == TOP_RIGHT or pos == BOTTOM_RIGHT) else 10
    y = y_off if (pos == TOP_LEFT  or pos == TOP_RIGHT)    else frame.shape[0] - y_off

    cv2.putText(frame,
                text = text,
                org = (int(x), int(y)),
                fontFace = cv2.FONT_HERSHEY_DUPLEX,
                fontScale = 0.3,
                color = col)



# Main exec
def main(args):
    # Set up input stream and configure
    cap = cv2.VideoCapture(args.src)
    if args.src == 0:
        cap.set(cv2.CAP_PROP_BUFFERSIZE, 3) # no. of frames the internal buffer stores in memory
        cap.set(cv2.CAP_PROP_FPS, 30)
        cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*"mp4v"))
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    
    # Ensure video is successfully opened
    if not cap.isOpened(): return terminate("Failed to open camera!")

    # Define width and height of video and zoom properties
    width0  = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH));  # original width of frames of video
    height0 = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT)); # original height of frames of video
    width   = int(width0 /ZOOM);         # zoomed width
    height  = int(height0/ZOOM);         # zoomed height
    roi_x   = int((width0  - width )/2); # zoomed roi x pos
    roi_y   = int((height0 - height)/2); # zoomed roi y pos

    # Handle FPS queue
    q = deque(maxlen = MONITOR_FRAMES) # render FPS
    fps_camera = cap.get(cv2.CAP_PROP_FPS) # camera FPS
    if fps_camera == 0: fps_camera = 30. # 30 is default if input FPS unavailable

    # Handle codec
    ex = int(cap.get(cv2.CAP_PROP_FOURCC)) # get codec type - int form
    ext = "".join([chr((int(ex) >> 8 * i) & 0xFF) for i in range(4)]) # transform to str
    
    # Recording
    rec = False
    rec_init = 0
    rec_pd = {}

    cv2.namedWindow(WINDOW_TITLE)

    # Pupil diameter
    pd_std = 0 # calibrated pupil diameter
    pd_cal = False # whether or not due to calibrate pupil diameter
    pd = 0 # current pupil diameter

    # Temp vars
    path = ""
    out = None

    while True:
        start = now() # record timing for current tick to determine FPS [ms]

        if rec and rec_init == 0:
            rec_init = start

            # Generate recording folder and parent folder(s)
            path = f"{RECORDING_DIR}/{rec_init}" # timestamp of the recording
            Path(path).mkdir(parents=True, exist_ok=True)

            path_video = path + "/output.avi"

            # Write processed video
            out = cv2.VideoWriter(path_video, # output path
                                  ex, # codec type (int form)
                                  fps_camera, # input camera FPS
                                  (width, height), # output roi size
                                  isColor = True) # use colour = true
            
            # Check if ok
            if not out.isOpened(): return terminate("Could not open output video" + path_video)

            print("Generated " + path_video)

        ok, frame = cap.read() # read frame of the video
        if ok: # if read is successful
            # Zoom
            roi = frame[roi_y:roi_y+height, roi_x:roi_x+width] # region of image
            
            gray_roi = cv2.cvtColor(roi, cv2.COLOR_BGR2GRAY)
            gray_roi = cv2.GaussianBlur(gray_roi, (11, 11), 0)
            gray_roi = cv2.medianBlur(gray_roi, 3)

            threshold = cv2.threshold(gray_roi, 15, 255, cv2.THRESH_BINARY_INV)[1]
            contours = cv2.findContours(threshold, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)[0]
            contours = sorted(contours, key=lambda x: cv2.contourArea(x), reverse=True)

            # Pre-computations
            fps_render = 0 if len(q) == 0 else np.mean(q) # Rendering FPS

            # Top left
            add_text(roi, TOP_LEFT, f"Image: {height} x {width}")
            add_text(roi, TOP_LEFT, f"Zoom: {ZOOM:.1f}")
            add_text(roi, TOP_LEFT, f"Codec: {ext}")
            # Top right
            add_text(roi, TOP_RIGHT, f"Render FPS: {fps_render:.1f}")
            add_text(roi, TOP_RIGHT, f"Camera FPS: {fps_camera:.1f}")
            # Bottom left
            if pd_std != 0: # PD calibration and change
                add_text(roi, BOTTOM_LEFT, f"PD increase: {100*(pd-pd_std)/pd_std:.1f}%")
            if rec: # [R] status
                secs = int((now() - rec_init)/1000) # get elapsed time for tick in seconds
                add_text(roi, BOTTOM_LEFT, f"[R] {secs}s", (0, 0, 255))
            elif rec_init != 0:
                # Save PD history to CSV file
                with open(f"{path}/pd_history.csv", "w", newline="") as csv_file:
                    writer = csv.writer(csv_file)
                    writer.writerow(["Time [ms]", "Relative PD"])
                    for ms in rec_pd:
                        writer.writerow([ms, rec_pd[ms]])
                
                # Write out to file
                out.write(roi)

                # Reset
                rec_init = 0 # reset recording timer
                rec_pd = {} # reset recording PD
            # Bottom right
            add_text(roi, BOTTOM_RIGHT, "[Press Q to Exit]", (0, 200, 255))

            # Reset text positions
            global text_lines
            text_lines = np.zeros(4)

            if len(contours) > 0:
                cnt = contours[0]
                (x, y, w, h) = cv2.boundingRect(cnt) # minimum bounding box around binary contour
                pd = int(h/2)
                cv2.circle(roi,  (x + int(w/2),          y + int(h/2)), pd,      (0,  0, 255), 2)
                cv2.line(roi,    (x + int(w/2), 0),     (x + int(w/2),  height), (50, 200, 0), 1)
                cv2.line(roi, (0, y + int(h/2)), (width, y + int(h/2)),          (50, 200, 0), 1)

                if rec: # save pupil diameter when recording
                    rec_pd[now()-rec_init] = pd # save PD at millisecond point
                if pd_cal: # update pupil diameter standard for calibration
                    pd_std = pd
                    pd_cal = False
                
            cv2.imshow(WINDOW_TITLE, roi)
        else:
            cap.set(cv2.CAP_PROP_POS_FRAMES, 0) # restart frame decoding
        
        # Record FPS
        secs = (now()-start) / 1000 # get elapsed time for tick in seconds
        if secs > 0: q.append(1./secs) # inverse elapsed time to get FPS
        
        # Hot keys
        k = cv2.waitKey(1) & 0xFF
        if   k != 0xFF:     print(f"> {chr(k)}") # display pressed key
        if   k == ord("q"): break         # Press 'Q' on the keyboard to exit the playback
        elif k == ord("r"): rec = ~rec    # toggle recording
        elif k == ord("c"): pd_cal = True # toggle pupil diameter calibration

    cap.release()
    if out is not None: out.release()
    cv2.destroyAllWindows()



if __name__ == "__main__":
    # Handle arguments
    parser = argparse.ArgumentParser(description="Pupil detector")
    parser.add_argument("--src", default=0, required=False,
                        help="Location of video source, or 0 for camera device.")
    args = parser.parse_args()

    # Call main
    main(args)