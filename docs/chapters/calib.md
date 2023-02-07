## Calibration

`sisoRec` is a video capture tool, not a calibration tool. However, if you do a recording using the `live detect grid` enabled then the tool will set things up ready for the external calibration tools.

Full documentation of the calibration process is provided in the `mc_core` documentation, but here is a brief step-by-step.

First, you need to perform a capture:

  1) Place the Qualisys L-frame at your desired origin - (optional)
  2) Enable `live detect grid` and `calibration mode`
  3) Start grabbing
  4) press `r` to start recording and detecting
  5) Walk the calibration board through the scene.
     1) aim to cover the full view of each camera with detections, even if those detections are unique to the camera.
     2) aim to connect pairs and triples of cameras with shared grid observations
     3) Ensure there is a "path" connecting all the cameras through shared grids.
     4) detections are colour coded based on how many cameras saw that grid
        1) red == 1 camera
        2) purple == 2 cameras
        3) green  == 3 or more cameras.
  6) Press `r` to stop recording. (If needs be, you can press `r` again to continue the capture for this calibration trial)
  7) Stop grabbing. This finishes the calibration session, saving out the grid detections and auto-generating a calibration config file.

Once you have captured the calibration sequence:

  1) Open up the `Konsole` command line tool.
  2) Navigate to the directory containing your recording. By default, calibration recordings will be saved to `/data/ssd0/recording/<session>/calib_<trial number>`.
  3) Run the `pointMatcher` tool to annotate origin and ground plane points.
     - `a` and `s` keys to change views on the left
     - `z`, `x` for the right
     - `up`  and `down` arrows to change point ID.
     - Click the mouse to annotate a point, and you will be shown a cropped/enlarged view to refine your annotation.
     - Use the buttons at the top of the tool to delete annotations.
     - You _must_ annotate (in as many views as reliably possible) at least 3 points (the markers on the Qualisys L-frame if using):
        - point 0: Any point on the desired x-axis of the scene
        - point 1: The origin point of the scene
        - point 2: Any point on the desired y-axis of the scene.
  4) Run the `circleGridCamNetwork` tool to actually calibrate.
     - The tool will report its reconstruction errors for the grid detections and for the auxilliary (manually annotated) point matches.
     - You should expect sub-pixel errors on grid reconstructions.
  5) Run the `manualAlignNetwork` tool to set the origin and ground plane.
  6) Run the `calibCheck` tool to visualise the calibration result. You _should_ expect to see an axis drawn on the origin aligned to your desired x-axis and y-axis. You should also see your manual annotatations and the reconstructions of those as green and yellow circles, which should be very closely aligned. You should also see purple frustums drawn over (or very near to) each camera that is visible.
  7) If a robust ground plane is critical then you can use the tool `makeGroundPlaneImage` to project all camera views onto the ground. If the calibration is good and the ground plane alignment is good, you should find that the resulting image neatly looks like a top-down view of your scene. Ghosting of lines and features will indicate imperfect ground plane alignment.
  
 So, for example:

```bash 
     $> cd /data/ssd0/recording/2023-02-02-test/calib_01
     $> /opt/mc_bin/mc_core/pointMatcher calib.cfg
     $> /opt/mc_bin/mc_core/circleGridCamNetwork calib.cfg
     $> /opt/mc_bin/mc_core/manualAlignNetwork calib.cfg
     $> /opt/mc_bin/mc_core/checkCalib calib.cfg
     $> /opt/mc_bin/mc_core/makeGroundPlaneImage calib.cfg -3000 -3000 6000 1000
```

NOTE: The auto-generated `calib.cfg` file is set up on the assumption that you have used the Qualisys L-Frame to indicate your origin. As such, it is configured such that:

  1) The annotated y-axis point is assumed to be on the _negative_ y-axis
  2) The annotated x-axis, y-axis, and origin points are 50 mm _above_ the ground plane.

If this is not the case, then you can edit the `calib.cfg` file and adjust the settings `targetDepth`, `alignXisNegative` and `alignYisNegative` to better suit your data.

For a more complete discussion of calibration see the documentation in `mc_core`.
