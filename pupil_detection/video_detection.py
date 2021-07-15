import cv2 # Computer vision
import numpy as np
import argparse # Command parser
import time # Timing
from pathlib import Path # file handling
import csv # For diameter history analysis
from collections import deque

# Constants
MONITOR_FRAMES  = 30            # number of frames to monitor for FPS calculation
RECORDING_DIR   = "recordings"  # location at which PD recordings are saved

# Internal variables
text_lines = 0

def add_text(img, rows, text, col = (255, 255, 255)):
    ''' Helper function to print out text to CV2 '''

    global text_lines
    text_lines += 1
    cv2.putText(img, # Render FPS
                text = text,
                org = (15, int(rows - 15*(text_lines))),
                fontFace = cv2.FONT_HERSHEY_DUPLEX,
                fontScale = 0.4,
                color = col)

if __name__ == "__main__":

    # Handle arguments
    parser = argparse.ArgumentParser(description="Pupil detector")
    parser.add_argument("--src", default=0, required=False,
                        help="Location of video source, or 0 for camera device.")
    args = parser.parse_args()

    # Ensure recording dir exists
    # Path(RECORDING_DIR).mkdir(parents=True, exist_ok=True)

    # Handle FPS queue
    q = deque(maxlen = MONITOR_FRAMES)
    
    # Recording
    rec = False
    rec_init = 0
    rec_pd = {}

    # Set up stream
    cap = cv2.VideoCapture(args.src)

    # Temp vars
    path = ""
    fps_camera = 0
    out = None

    while cap.isOpened():
        start = time.time() # record timing for current tick to determine FPS
        if rec and rec_init == 0:
            rec_init = start

            path = f"{RECORDING_DIR}/{str(int(rec_init))}" # timestamp of the recording

            # Generate recording folder and parent folder(s)
            Path(path).mkdir(parents=True, exist_ok=True)

            # Camera FPS
            fps_camera = cap.get(cv2.CAP_PROP_FPS)

            # Write processed video
            out = cv2.VideoWriter(f"{path}/out.avi",
                                  cv2.VideoWriter_fourcc("M", "J", "P", "G"),
                                  fps_camera,
                                  (500, 450))

        ok, frame = cap.read() # capture and read frames of the video
        if ok: # if read is successful
            roi = frame[50:500, 300:800] # zoom
            rows, cols, _ = roi.shape
            
            gray_roi = cv2.cvtColor(roi, cv2.COLOR_BGR2GRAY)
            gray_roi = cv2.GaussianBlur(gray_roi, (11, 11), 0)
            gray_roi = cv2.medianBlur(gray_roi, 3)

            threshold = cv2.threshold(gray_roi, 15, 255, cv2.THRESH_BINARY_INV)[1]
            contours = cv2.findContours(threshold, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)[0]
            contours = sorted(contours, key=lambda x: cv2.contourArea(x), reverse=True)

            # Pre-computations
            fps_render = 0 if len(q) == 0 else np.mean(q) # Rendering FPS

            for cnt in contours:
                (x, y, w, h) = cv2.boundingRect(cnt) # minimum bounding box around binary contour
                pd = int(h/2)
                cv2.circle(roi,  (x + int(w/2),         y + int(h/2)), pd,   (0, 0, 255),  2)
                cv2.line(roi,    (x + int(w/2), 0),    (x + int(w/2), rows), (50, 200, 0), 1)
                cv2.line(roi, (0, y + int(h/2)), (cols, y + int(h/2)),       (50, 200, 0), 1)

                add_text(roi, rows, "[Press Q to Exit]", (0, 200, 255)) # Exit message
                if rec:
                    secs = time.time() - rec_init # get elapsed time for tick in seconds
                    add_text(roi, rows, "[R] {s}s".format(s=int(secs)), (0, 0, 255)) # [R] status

                    # save pupil diameter when recording
                    rec_pd[int(secs*1000)] = pd # save PD at millisecond point
                elif rec_init != 0:
                    id = str(int(rec_init)) # ID = timestamp of the recording

                    # Save PD history to CSV file
                    with open(f"{RECORDING_DIR}/{id}/pd_history.csv", "w", newline="") as csv_file:
                        writer = csv.writer(csv_file)
                        writer.writerow(["Time [ms]", "Relative PD"])
                        for ms in rec_pd:
                            writer.writerow([ms, rec_pd[ms]])
                    
                    # Write out to file
                    out.write(roi)

                    # Reset
                    rec_init = 0 # reset recording timer
                    rec_pd = {} # reset recording PD
                add_text(roi, rows, "Render FPS: {:.1f}".format(fps_render)) # Render FPS
                add_text(roi, rows, "Camera FPS: {:.1f}".format(fps_camera)) # Camera FPS
                text_lines = 0

                break
                
            cv2.imshow("Pupil Detector", roi)
            #cv2.imshow("Threshold", threshold)
            #cv2.imshow("gray_roi", gray_roi)
        else:
            cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
        
        secs = time.time() - start # get elapsed time for tick in seconds
        q.append(1/secs) # inverse elapsed time to get FPS
        
        # Hot keys
        k = cv2.waitKey(30) & 0xFF
        if k == ord("q"):
            break # Press 'Q' on the keyboard to exit the playback
        elif k == ord("r"):
            rec = ~rec # toggle recording

    cap.release()
    out.release()
    cv2.destroyAllWindows()