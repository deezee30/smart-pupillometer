import numpy as np
import argparse # Command parser
from pathlib import Path # file handling
import csv # For diameter history analysis
from datetime import datetime
import util
import video_stream

# Constants
RECORDING_DIR = "recordings" # location at which PD recordings are saved

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
    vs = video_stream.VideoStream(args.src)
    while True:
        if rec and rec_init == 0: # start of recording
            rec_init = util.now()

            # Generate recording folder and parent folder(s)
            path = "{rd}/{ts}".format(rd=RECORDING_DIR, # timestamp of the recording
                    ts=datetime.fromtimestamp(rec_init//1000).strftime("%Y-%m-%d %H.%M.%S"))
            Path(path).mkdir(parents=True, exist_ok=True)

            path_video = f"{path}/output.avi"

            # Open processed output video
            vs.setup_output(path_video)

            print(f"Generated '{path_video}'")
        
        # Render and obtain ROI and PD
        frame, pd = vs.render_frame(pd, perc, rec_elapsed)

        #### Post-processing for next frame ####

        # Get elapsed time for tick
        if rec: rec_elapsed = round((util.now()-rec_init)/1000, 3) # [s]

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
        k = vs.wait_key(1) & 0xFF
        if   k != 0xFF:     print(f"> {chr(k)}") # display pressed key
        if   k == ord("q") and ~rec: break # [Q] exit playback when not recording
        elif k == ord("r"): rec = ~rec     # [R] toggle recording
        elif k == ord("c"): pd_cal = True  # [C] toggle pupil diameter calibration

    vs.close()

if __name__ == "__main__": main()