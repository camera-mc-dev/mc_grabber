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

Once you have captured the calibration sequence, you can make use of the calibration tools from either the command line, or from the session manager.

### Calibrating from the session manager

<div style="text-align: center">
![Session manager](imgs/manager/c0.png){style="width: 45%; margin: auto;"}
![session / trial selection](imgs/manager/c1.png){style="width: 45%; margin: auto;"}
![Debayer controls](imgs/manager/c2.png){style="width: 45%; margin: auto;"}
![Calibration view](imgs/manager/c3.png){style="width: 45%; margin: auto;"}
</div>

  1) Start the session manager.
  2) Select the session and calibration trial that you are interested in.
  3) If you did *not* do live grid detection:
     1) Debayer the trial (select the trial, click on "queue trial", click on "launch jobs", check demon is running, wait for job to complete)
     2) Change to the calibration view
     3) *Disable* all of: "use matches", "use bundle adjust", "use existing grids"
     4) Click "Run Calibration" - the main purpose of this is grid detection, we'll ignore the calibration result.
  4) Click "Run Pointmatcher" and get manual alignment matches, plus any extras as needed. (see the next section ("Calibrating from the command line") for instructions)
  5) *Enable* "use matches", "use bundle adjust", "use existing grids"
  6) Click "Run Calibration" - check that the grid error is less than 1 pixel, watch our for large maximum errors.
  7) Click "Check Calibration" - to visualise the result. The origin will be near a camera, you should see magenta "frustums" drawn at each camera. Annotated points should show their annotation and reprojection (yellow, green) very close to each other. If not, see trouble shooting.
  8) Check settings for the manual alignment annotations. 
  9) Click "Run Alignment"
  10) Click "Check Calibration" to visualise the result. If the previous check was good, you're now mostly checking the origin is as you expect it. Check yor alignmenent annotations and alignment settings if not. 
  11) Click "Make Groundplane Image" to project all camera views to the x-y ground plane making a "top down" view of the scene. If the ground plane is important to you, check that features on the ground line up neatly between different viewpoints. If not, adjust your alignment annotations, try making your x and y alignment annotations as far from the origin as possible, try adding auxilliary matches using features on the ground plane.
  12) If you *did* do live grid detection:
     1) Debayer the trial
     2) click the "Raw -> RGB" button to move all calibration data to the processed output. 
     3) If "mirroring" your data to a remote host, re-run the debayer to ensure calibration files are mirrored.

Note that each time you run the calibration tool ("Run Calib") you be shown the final calibration output in a text editor (Kate). The calibration errors are always written to a file `/tmp/mc_calib_err`. 

### Calibrating from the command line

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
  5) Run the `manualAlignNetwork` tool to set the origin and ground plane. (The tool will render a 6 m by 6m area starting at (-3, -3) - if you need a different region, consider running the tool from the command line.
  6) Run the `calibCheck` tool to visualise the calibration result. You _should_ expect to see an axis drawn on the origin aligned to your desired x-axis and y-axis. You should also see your manual annotatations and the reconstructions of those as green and yellow circles, which should be very closely aligned. You should also see purple frustums drawn over (or very near to) each camera that is visible.
  7) If a robust ground plane is critical then you can use the tool `makeGroundPlaneImage` to project all camera views onto the ground. If the calibration is good and the ground plane alignment is good, you should find that the resulting image neatly looks like a top-down view of your scene. Ghosting of lines and features will indicate imperfect ground plane alignment.
  
 So, for example:

```bash 
     $> cd /data/ssd0/recording/2023-02-02-test/calib_01
     $> /opt/mc_bin/mc_core/pointMatcher calib.cfg
     $> /opt/mc_bin/mc_core/circleGridCamNetwork calib.cfg
     $> /opt/mc_bin/mc_core/manualAlignNetwork calib.cfg
     $> /opt/mc_bin/mc_core/calibCheck calib.cfg
     $> /opt/mc_bin/mc_core/makeGroundPlaneImage calib.cfg -3000 -3000 6000 1000
```

NOTE: The auto-generated `calib.cfg` file is set up on the assumption that you have used the Qualisys L-Frame to indicate your origin. As such, it is configured such that:

  1) The annotated y-axis point is assumed to be on the _negative_ y-axis
  2) The annotated x-axis, y-axis, and origin points are 50 mm _above_ the ground plane.

If this is not the case, then you can edit the `calib.cfg` file and adjust the settings `targetDepth`, `alignXisNegative` and `alignYisNegative` to better suit your data.

NOTE: The arguments to `makeGroundPlaneImage` are `<min x> <min y> <world size> <image size>`, thus the above command will show a square map of 6000 mm starting from (-3000,-3000).

For a more complete discussion of calibration see the documentation in `mc_core`.

## Example of the calibration tools

### Point matcher

<div style="text-align: center">
![Point Matcher](imgs/manager/pm.png){style="width: 45%; margin: auto;"}
</div>

Here we see the point matcher interface after the user has annotated the three main alignment points (an x-axis point, the origin, and a y-axis point). In this case there was no Qualisys L-frame so a suitable feature on the ground was used.

### Calib check

<div style="text-align: center">
![Point Matcher](imgs/manager/check00.png){style="width: 45%; margin: auto;"}
![Point Matcher](imgs/manager/check01.png){style="width: 45%; margin: auto;"}
</div>

Viewing the calibration result after successful calibration, but before alignment...

<div style="text-align: center">
![Point Matcher](imgs/manager/check10.png){style="width: 45%; margin: auto;"}
![Point Matcher](imgs/manager/check11.png){style="width: 45%; margin: auto;"}
</div>

... and after successful alignment.

### Ground plane image

<div style="text-align: center">
![Point Matcher](imgs/manager/gp.png){style="width: 45%; margin: auto;"}
</div>

This is the "top down" view resulting from the projection of all camera views to the ground plane. This shows strong alignment of ground plane features near the origin, but away from the origin features do not align. This suggests that:
  
   1) the real world ground is not actually flat
   2) the alignment annotations resulted in a z = 0 plane that is not ideally aligned to the real ground
   3) the calibration is imperfect away from the origin. (but this would tend to show itself in other ways too, such as poor calibration errors, large distortion parameters, etc)

In this case the answer was (2) and an improvement of the x-axis point annotations greatly improved the ground plane calibration:

<div style="text-align: center">
![Point Matcher](imgs/manager/gp1.png){style="width: 45%; margin: auto;"}
</div>
