## Fake Grabber Dev

We don't hugely need to think about `sisoRec` as this stage - though I can't guarantee that `basicRec` is currently functional.

With that in mind, we can approach the task in hand.

We currently have the `SiSoGrabber` which at its most basic can be considered as a collection of cameras with some common mechanism of controlling those cameras. 

  - We want to make some class `AbstractGrabber` which provides the basic interface of the minimum set of controls we would need for controlling a group of cameras.
  - We then want to change things so that `SiSoGrabber` inherits and specialises `AbstractGrabber`.
  - We then want some class `FakeGrabber` which uses some on-disk sources of images and otherwise looks like a live camera.

  

