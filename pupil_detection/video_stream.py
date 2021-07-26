import cv2 # Computer vision
import numpy as np
import util
import textable_frame as frame
from collections import deque
from threading import Thread

# Constants
MONITOR_FRAMES  = 50               # no. of frames to monitor for FPS calculation
ZOOM            = 1                # zoom amount
WINDOW_TITLE    = "Pupil Detector" # camera window title
FOCUS_BOX_SCALE = 2                # bounding box scale with respect to zoomed resolution

class VideoStream(object):
    def __init__(self, src):
        # Set up input stream and configure
        self.cap = cv2.VideoCapture(src)
        
        # Ensure video is successfully opened
        if not self.cap.isOpened(): return self._terminate("Failed to open camera!")

        # Define width and height of video and zoom properties
        self.width0   = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))  # original width of video
        self.height0  = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT)) # original height of video
        # Zoomed video properties
        self.width    = int(self.width0  / ZOOM)               # zoomed width
        self.height   = int(self.height0 / ZOOM)               # zoomed height
        self.zoom_x   = int((self.width0  - self.width)  / 2)  # zoomed ROI x pos
        self.zoom_y   = int((self.height0 - self.height) / 2)  # zoomed ROI y pos
        # Focused bounding box properties
        self.width_f  = int(self.width  / FOCUS_BOX_SCALE)     # zoomed, focused width
        self.height_f = int(self.height / FOCUS_BOX_SCALE)     # zoomed, focused height
        self.x_f      = int((self.width  - self.width_f)  / 2) # zoomed, focused ROI x pos
        self.y_f      = int((self.height - self.height_f) / 2) # zoomed, focused ROI y pos
        # Compute bounding box properties
        self.bb_shape = np.zeros((4, 2, 2), dtype=np.uint16)
        self.bb_shape[0] = ((0,                     0),                         # top pt 1
                            (self.width,            self.y_f))                  # top pt 2
        self.bb_shape[1] = ((0,                     self.y_f+self.height_f),    # bottom pt 1
                            (self.width,            self.height))               # bottom pt 2
        self.bb_shape[2] = ((0,                     self.y_f),                  # left pt 1
                            (self.x_f,              self.y_f+self.height_f))    # left pt 2
        self.bb_shape[3] = ((self.x_f+self.width_f, self.y_f),                  # right pt 1
                            (self.width,            self.y_f+self.height_f))    # right pt 2

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
                start = util.now()

                # Read frame
                self.frame_ok, self.frame = self.cap.read()

                # Record FPS
                secs = (util.now()-start) / 1000 # get elapsed time for tick in seconds
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

        # Rendering FPS
        self.fps_render = 0 if len(self.q) == 0 else np.mean(self.q)

        # Zoom
        roi_z = self.frame[self.zoom_y:self.zoom_y+self.height,
                           self.zoom_x:self.zoom_x+self.width] # region of image
        roi = roi_z.copy() # create a copy not tied to post-processing
        
        # Compute and render a bounding focus box
        alpha = 0.4
        for i in range(4):
            ((x0, y0), (x1, y1)) = self.bb_shape[i]
            roi_sub = roi_z[y0:y1, x0:x1]
            roi_cover = np.ones(roi_sub.shape, dtype=np.uint8)
            roi[y0:y1, x0:x1] = cv2.addWeighted(roi_sub, 1-alpha, roi_cover, alpha, 1.0)
        roi_focus = roi_z[self.y_f:self.y_f+self.height_f,
                          self.x_f:self.x_f+self.width_f] # focused region

        tf = frame.TextableFrame(roi)

        # Top left
        tf.add_text(frame.TOP_LEFT, f"Image: {self.height} x {self.width}")
        tf.add_text(frame.TOP_LEFT, f"Zoom: {ZOOM:.1f}")
        tf.add_text(frame.TOP_LEFT, f"Codec: {self.ext}")
        # Top right
        tf.add_text(frame.TOP_RIGHT, f"Render FPS: {self.fps_render:.1f}")
        tf.add_text(frame.TOP_RIGHT, f"Camera FPS: {self.fps_camera:.1f}")
        # Bottom left
        if pd_pct      != -1: tf.add_text(frame.BOTTOM_LEFT, f"PD increase: {pd_pct:.1f}%")
        if rec_elapsed != -1: tf.add_text(frame.BOTTOM_LEFT, f"[R] {rec_elapsed:.3f}s", (0, 0, 255))
        # Bottom right
        tf.add_text(frame.BOTTOM_RIGHT, "[Press Q to Exit]", (0, 200, 255))
        
        # Finally: Try detect pupil
        centers, sizes = self._find_pupils(roi_focus)

        for i in range(len(sizes)):
            if i == 4: break # only allow max 4 detections

            cv2.circle(roi, tuple(centers[i]), sizes[i], (0,  0, 255), 2)
            cv2.line(roi, (centers[i, 0], 0), (centers[i, 0], self.height), (50, 200, 0), 1)
            cv2.line(roi, (0, centers[i, 1]), (self.width,  centers[i, 1]), (50, 200, 0), 1)
            tf.add_text(tuple(centers[i]-int(sizes[i]/2)), sizes[i], (0, 0, 255))
        
        cv2.imshow(WINDOW_TITLE, roi)

        return roi, pd

    def save_frame(self, roi):
        # Write out to file
        self.out.write(roi)
    
    def wait_key(self, ms):
        return cv2.waitKey(ms)
    
    def close(self):
        self.cap.release()
        if self.out is not None: self.out.release()
        cv2.destroyAllWindows()
    
    def _find_pupils(self, roi):
        gray_roi = cv2.cvtColor(roi, cv2.COLOR_BGR2GRAY) # convert roi to gray
        gray_roi = cv2.GaussianBlur(gray_roi, (11, 11), 0) # apply gaussian blur
        gray_roi = cv2.medianBlur(gray_roi, 3) # apply median blur

        threshold = cv2.threshold(gray_roi, 15, 255, cv2.THRESH_BINARY_INV)[1] # binary inv thresh
        contours = cv2.findContours(threshold, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)[0] # contours
        contours = sorted(contours, key=lambda x: cv2.contourArea(x), reverse=True)

        centers = np.zeros((len(contours), 2), dtype=np.uint16)
        sizes   = np.zeros(len(contours),      dtype=np.uint16)

        for i, cnt in enumerate(contours):
            (x, y, w, h) = cv2.boundingRect(cnt) # minimum bounding box around binary contour
            centers[i] = (self.x_f + x + w // 2, self.y_f + y + h // 2)
            sizes[i] = int(h/2) # relative pupil diameter
        
        #cv2.imshow("Threshold", threshold)
        
        return centers, sizes

    def _terminate(self, msg):
        print(msg)
        input("Press 'Enter' to close...")
        cv2.destroyAllWindows()
        return -1