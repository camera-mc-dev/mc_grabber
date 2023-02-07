## The process in brief

The recording system consists of:

  1) The computer ("Grabbie" - replaced "Boromir" in late 2022)
  2) 9 JAI cameras
  3) 9 CoaxPress cables
  4) 7 6mm C-mount lenses
  5) 2 8mm C-mount lenses

A typical capture session consists of:

  1)  Position and approximately aim the cameras.
  2)  Lay out cables and connect each cable to "Grabbie" and to camera
  3)  Power up "Grabbie" (if not already done!)
  4)  Log in as "Framey Grabber" - (grabberuser)
  5)  Start the recording tool `sisoRec`
  6)  Set recording duration and framerate.
  7)  Video check 
      - Start grabbing and check:
          - camera aim,
          - camera aperture
          - camera exposure
          - camera gain
          - camera focus
      - Stop grabbing.
  8)  Capture calibration sequence.
  9)  Run calibration tools (if time permits).
  10) Capture trials.
  11) Repeat / finalise calibration.
  12) Post process recordings using the `session manager`
  13) Move recordings to longer-term storage
  14) Remove recordings from "Grabbie"

<div style="text-align: center">
![Capture process](imgs/Capture-process.drawio.svg){style="width: 90%; margin: auto;"}
</div>

## Special notes (valid as of 2023-02): {#specNotes}

 1) If you have to power up the computer, note that you will need to ensure the system boots to a specific Linux kernel. It should do this by default - but is worth noting here.  
    - The required kernel is: `Ubuntu, with Linux 5.4.0-137-generic` and can be found in the "Advanced options for Ubuntu" menu which appears shortly after power on.
    - You can check from the command prompt using: `uname -a`
    - The reason this is needed is because of the _module_ (driver) for the _framegrabbers_ (interface between cameras and computer). Further *NOTE*: The _module_ can be _hacked_ and built for newer kernels - but the better solution should be the newer release of the SiSo runtime, which Basler released just at the end of 2022 before they _stopped support for the grabbers_. However, this will require significant testing and possibly updating of the backend to the `sisoRec` tool. 
    
  
 2) Over-heating has always been an issue with the framegrabbers. A new system fan has helped, but it is probably worth leaving the glass side-door of the computer open.
 
 3) The top-most framegrabber has caused some problems (Feb 2023): while it detects and powers connected cameras correctly, images have not been properly captured from those cameras. Recommend using the bottom two grabbers for now. But note that the top grabber still provides the master timing signal and must still be available for the system. You can still connect up to 8 cameras so it is not a _major_ loss.


 















