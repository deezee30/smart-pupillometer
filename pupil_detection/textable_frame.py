import cv2 # Computer vision
import numpy as np
from numpy.core.fromnumeric import shape

# Text format
TEXT_MARGIN = 15
TEXT_FONT_FACE = cv2.FONT_HERSHEY_DUPLEX
TEXT_FONT_SIZE = 0.5

# Text positioning
TOP_LEFT     = 0
TOP_RIGHT    = 1
BOTTOM_LEFT  = 2
BOTTOM_RIGHT = 3

class TextableFrame(object):
    ''' Helper class to print out text to CV2 '''

    def __init__(self, frame, text_margin = TEXT_MARGIN,
                 text_lines = None, square = False):
        self.frame = frame
        self.text_margin = text_margin
        self.text_lines  = np.zeros(4) if text_lines  == None else text_lines

        # Adjust margins for square aspect ratios
        self.sq_margin = (frame.shape[1] - frame.shape[0])//2 if square else 0
    
    def add_text(self, pos, text, col = (255, 255, 255)):        
        if type(pos) == tuple:
            org = pos
        else:
            self.text_lines[pos] += 1

            x_off = int(self.frame.shape[1] - (320*TEXT_FONT_SIZE+self.sq_margin))
            y_off = self.text_margin * self.text_lines[pos]

            x = x_off if (pos == TOP_RIGHT or pos == BOTTOM_RIGHT) else self.text_margin + self.sq_margin
            y = y_off if (pos == TOP_LEFT  or pos == TOP_RIGHT)    else self.frame.shape[0] - y_off

            org = (int(x), int(y))
        
        cv2.putText(self.frame, text, org, TEXT_FONT_FACE, TEXT_FONT_SIZE, col)