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
  7)  Video check - Start grabbing; check camera aim, aperture, exposure, gain, focus; Stop grabbing.
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


 
## Details

### Recording system

The "Grabbie" recording computer is a powerful Linux system with 128 GB of system RAM, fast CPU, and (at time of writing) super powerful workstation GPU. It lives in the ABS lab to facilitate recording high(ish) framerate, hardware synchronised video for markerless motion capture experiments.

As a bonus, the system can send out synchronisation signals to other systems such as the Qualisys motion capture system - but that is an advanced discussion.

As it is treated like a server, the system is usually already powered up. Make sure a monitor, mouse and keyboard are connected, wake the machine up, and wait for the login screen.

Login as the user named "Framey Grabber" - who has username `grabberuser`. You should have the password if you have permission to use the system. See Murray/Steffi/Logan.

The desktop will show several shortcuts to the tools that are relevant to you:

  1) `Konsole`: KDE's terminal/console for command line input.
  2) `sisoRec 3`: Start the capture tool.
  3) `sessionManager`: Start the session manager.
  4) `powerCycle`: Power cycle the framegrabbers - basically, reboots the cameras.

There are also a few other shortcuts:

  1) `sisoRec 1`: Start the capture tool using only the top framegrabber
  2) `sisoRec 2`: Start the capture tool using the top 2 framegrabbers
  3) `sisoRec 3`: Start the capture tool using all 3 framegrabbers
  





### Setting up cameras

The 9 JAI cameras use the _CoaXpress_ interface to connect to the computer. There are 9 20m cables (blue), and 9(?) 30m cables (lighter blue) available for use, note that some may already be routed around the ABS lab. The cables provide power, data connection and timing connection.

Each camera has 2 little bronze "spigot" like connectors on the back to which the cable can affix. You can connect 1 cable to _either_ connector on the camera. The other end of the cable connects to a simillar connector on one of the frame grabbers at the back of the capture computer "Grabbie".

The connectors on the cable are simple to connect - just push onto the "spigot". To remove the cable, push the sprung shield _towards_ the camera, and then pull back on the cable gently. It should come away without much force. Pulling hard on the cable could damage the cable, connector, or worse, the camera or framegrabbers, so treat with respect.

When connecting the cameras to the computer, keep in mind which cable goes to which camera. It can be useful to connect the cameras to specific framegrabber ports so that the cameras are numbered in a logical order. (*NOTE*: You don't have to do this - but it can make things easier later).

Cameras will be numbered starting from the top-most framegrabber first, and from _left_ to _right_ on each grabber.

When the cameras are connected correctly you will see the orange power light on the back illuminate (flash?).

If the cameras are connected but no light appears, you may need to _power cycle_ the framegrabbers.

You can do this using the desktop shortcut, or by running:

  1) Open the `Konsole`
  2) At the prompt enter:
  3) `/opt/mc_bin/mc_grabber/powerCycle 3`

If cameras _still_ fail to connect, try different grabber ports, a different cable, or restart the computer. (Check (Special Notices)[#specNotes] section above incase there are any special notices regarding starting the computer).

There are 7 6mm lenses, which give about 90-degrees or so across the diagonal at 1080p, and 2 8mm lenses, which are a little less wide but still wide. This was to facilitate the original camera configuration for skeleton track recordings. If you absolutely _need_ all cameras to have the exact same imaging parameters, you will either need to buy new lenses or stick to 7 cameras.

### The recording tool: `sisoRec`

The main tool for recording video trials is the `sisoRec` tool. (The framegrabbers are originally from (Si)licon (So)ftware and our recording tool is based around using the SiSo runtime - hence `sisoRec`). 

You can start the tool using 1,2 or all 3 framegrabbers using the shortcuts on the desktop or from the command line as:

  1) Open the `Konsole`
  2) At the prompt enter:
  3) `/opt/mc_bin/mc_grabber/sisoRec 3`

(Replace the 3 with the number of grabbers you want to use). The tool assumes that the first grabber is the top grabber, which matters in regards to timing signals and internal cabling - see (advanced setups)[#advSet] ).

#### Controls window

When the tool opens you will be presented with the "controls" window, which allows you to set session name, trial names and camera parameters.

<div style="text-align: center">
![Controls window](imgs/controls-2023-02.png){style="width: 90%; margin: auto;"}
</div>

By default, a new session is created for you when you start the tool and given a name with today's date. You can instead create a session with a custom name using the `file` menu.

The _acquisition_ frame lets you set the image size, framerate and capture duration for the captures. This applies to all cameras. Some things to *NOTE*:

  1) `fps` doesn't control a camera setting, but instead controls a signal generator on the first framegrabber that instructs all cameras to capture a frame.
     - it is possible for the cameras to not match this rate if
        - image size is too large
        - exposure is too slow
     - you will be shown a highly visible warning if the cameras are not matching your desired capture rate.
  2) Capture duration is limited by available memory on the computer.
     - The computer has 128 GB of RAM
     - Each captured image is `(width)*(height)` bytes
     - Maximum capture time is such that `(width)*(height)*(fps)*(duration) < 128 GB`
     - The tool is currently set to a limit of 40 seconds.
  3) "observed fps" gives an estimate of current capture rate.

The acquisition frame also contains the buttons to start and stop grabbing, as well as check-boxes for camera calibration options (see later).

Beneath the acquisition frame is a grid showing exposure controls for individial cameras. Exposure time is shown using `1/x` format, where you set `x`. It is done this way for clarity - For a 200 fps capture, you will need to keep `x` _larger_ than 200.

For convenience, a master control is provided to set all the cameras to the same exposure and gain all at once.

*NOTE*: The numbers on the sliders, especially when starting up the tool, may not match what is currently set on the camera. As such, be sure to apply the settings at least once.

*NOTE*: Gain vs. analog base gain. "Gain" is mostly a digital boost to the raw pixel values. Analog gain is instead applied electronically in the camera's signal chain.

The _sharing_ section is a bonus tool - `sisoRec` can send the image stream from one camera across the network, which may be useful for focussing a camera out of reach of the capture computer, for example. However, a tool to display the sent image has not been maintained as this feature has not been needed. Still, it exists, but is not maintained.

Finally, the second half of the controls window displays information about capture trials. You can set the trial name as well as the trial number here. There is also a list of previously captured trials. (Double?) Clicking on a captured trial should show the captured raw data. Note that captures are recorded in their raw format and as such it will look like grey-scale with a strange grid-like or even interference pattern - that's _fine_.



#### Starting aquisition (grabbing)

Once you have set the fps and image size, you can start grabbing images from the cameras using the `Start Grabbing` button.

This will open up a second window, which will display the most recent image available from each camera. (You can toggle whether a camera is shown in the grid using the `display` check-box on each per-camera control frame in the controls window - useful if you need to enlarge a specific camera).

<div style="text-align: center">
![Grabbing window](imgs/capture-2023-02.png){style="width: 90%; margin: auto;"}
</div>

While grabbing, you _cannot_ adjust recording duration or fps due to the way recording works. You also _cannot_ adjust the analog base gain. You _can_ however adjust the exposure and gain of individual cameras, as well as change the trial name and trial number.


#### Recording

When the camera window is open, recording can happen in one of 2 ways. Buffered recording, or live recording.

##### Buffered recording

The primary recording method is _buffered recording_. This is the only way to record data as full framerate.

To make a buffered recording simply perform the thing you want to record, and then "bang the spacebar" to save the recorded images to disk. To make life easier, the tool automatically increments the trial number after each recording.

Saving will typically take substantially longer than making the recording - this is pretty much unavoidable - system memory is the only memory fast enough to capture the amount of data produced by many cameras at high framerates. Grabbie is equiped with a pair of SSDs to help when writing to disk - giving about 4 TB of available recording space.

In a little more detail:

  - When you start grabbing, a large chunk of system RAM is allocated for recording images. Specifically, it allocates enough memory for exactly `(duration)*(fps)` images - meaning it always keeps `(duration)` seconds of images in memory.
  - In buffered recording, the operator (you) does not have to start recording a trial - the computer is constantly recording. Instead all the operator has to do is "bang the spacebar" at the end of a trial. This stops the cameras, pulls the recording out of memory, saves it to disk, increments the trial number, and starts the cameras going again.

*NOTE*: You must have the grabbing window active when "banging the spacebar" otherwise nothing much will happen.

*NOTE*: When saving data to disk the tool stops sending out the timing signal, meaning the cameras all stop grabbing images. If something you want to capture happens when saving, then you've missed it.

*NOTE*: If you don't leave `(duration)` seconds between saving and "banging the spacebar" you may get some odd effects - like part of a previous trial appearing in the new recording. Equally, you will get weird results if you capture too soon after opening the grabbing window.

##### Live recording

When the grabber window is open you can press `r` to start and stop _live recording_. When recording, you will see a pulsating green circle in the top left of the grabbing window. In this mode, the tool can not guarantee the framerate - it just saves images to disk as fast as it can. This is only realistic with quite low framerates. However, images that are captured should still be synchronised. Live recording is most useful for calibration sequences, or very long trials which can be captured with slower framerates.


#### Calibration modes.

In the controls window, next to the `Start Grabbing` button are two check boxes useful for capturing calibration data.

  - `calibration mode`: sets the framerate to 5 fps and the trial name to `calib`.
  - `live detect grid`: enables live detection of the calibration grid.

When in calibration mode, press `r` to perform live recording while you move through the scene with the calibration board.

If `live detect grid` is enabled then the tool will run the circle grid detector when live recording is enabled. The detector will not as fast as it can but detections will not appear instantly. Detections will be displayed as red, purple or green boxes in each camera view.

  - Red: Grid detected in a single camera
  - Purple: Grid detected in two cameras.
  - Green: Grid detected in more than two cameras.

When capturing grids, your aim should be to maximis the number of grid detections that are shared across multiple cameras, as well as to maximise the area in each image with grid detections. To help with this, you will see a small third window showing a matrix of squares. You will find that as you capture more grids, the diagonal of this matrix gets brighter (this indicates the number of grids captured in each view). You should also, hopefully, see off-diagonal elements get brighter, indicating shared detections between camera views. To get a robust calibration all the cameras need to share detections with other views, and it should be possible to get from one camera to any other camera using a series of shared detections. Detections, and the sharing matrix, should be visible in the example shwon in the "Grabbing window" image above.

*NOTE*: A live detection calibration session starts when you open the grabbing window, and only completes when you close the grabbing window - at which point the grid detections are written out to disk. As such, the trial number is not incremented when pressing `r`.

*NOTE*: If in calibration mode (but not live detecting) the trial number will be incremented when pressing `r`. 






### The session manager

## Advanced setups {#advSet}

### Board order

### Synchronising with Qualisys motion capture

### Dual connection to cameras for highest possible data rate

### Fake cameras

### Image sender


