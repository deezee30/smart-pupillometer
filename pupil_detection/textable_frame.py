import cv2 # Computer vision
import numpy as np

# Text format
TEXT_MARGIN = 15
TEXT_FONT_FACE = cv2.FONT_HERSHEY_DUPLEX
TEXT_FONT_SIZE = 0.4

# Text positioning
TOP_LEFT     = 0
TOP_RIGHT    = 1
BOTTOM_LEFT  = 2
BOTTOM_RIGHT = 3

class TextableFrame(object):
    ''' Helper class to print out text to CV2 '''

    def __init__(self, frame, text_margin = None, text_lines = None):
        self.frame = frame
        self.text_margin = TEXT_MARGIN if text_margin == None else text_margin
        self.text_lines  = np.zeros(4) if text_lines  == None else text_lines
    
    def add_text(self, pos, text, col = (255, 255, 255)):
        self.text_lines[pos] += 1

        x_off = int(self.frame.shape[1] - 130)
        y_off = self.text_margin * self.text_lines[pos]

        x = x_off if (pos == TOP_RIGHT or pos == BOTTOM_RIGHT) else self.text_margin
        y = y_off if (pos == TOP_LEFT  or pos == TOP_RIGHT)    else self.frame.shape[0] - y_off

        cv2.putText(self.frame,
                    text = text,
                    org = (int(x), int(y)),
                    fontFace = TEXT_FONT_FACE,
                    fontScale = TEXT_FONT_SIZE,
                    color = col)