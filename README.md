# Streamline Visualization Project

## Project Overview
This project is a 3D streamline visualization tool for exploring vector and tensor fields, with a focus on brain imaging and scientific visualization. The application allows interactive exploration of complex vector fields through streamline generation, rendering, and advanced visualization techniques.

### Streamline Parameter Sliders
The tool has multiple parameters that can be changed when generating the streamlines:
- Step size
  - The size of the integration step.
- Max Streamline length
  - The maximum length of the streamlines. Note that currently this beheaves very similarly to the max integration steps setting due to the constant step size.
- Max integration steps
  - Maximum number of steps the integration algorithm runs.
- Max angle between steps
  - The maximum angle between the direction vectors generated in each step.
- Line Width
  - Line width of the streamlines in the visualization. Note that the line width doesn't scale with zooming so this may beheave slightly unintuitive.
- Integration method
  - The method used for integrating the trajectory between steps. Currently the options are:
    - Euler forward.
    - Second order Runge-Kutta.
- Flip axis
  - Some datasets may have some components of the vectors flipped. These settings allow for manually flipping the components if this seems to be an issue for the tracing.


### Camera Navigation
You control the camera position by dragging while holding the right mouse button.
You can zoom in/out by scrolling the mouse wheel.

### Interactivity

#### Mouse seeding
In the UI you can turn on mouse seeding. This function allows you to click on the screen and seed random points in a sphere around the area clicked. You can control the radius of the sphere and how many seed points are to be generated through the UI.

#### Future work: volume rendering
Something that could have been nice to add would be volume rendering of the brain dataset to allow for more dynamic and interactive viewing. For this a seperate system would have to be made for mouse seeding like a 3D cursor.

## Visualization Techniques
This tool used multiple visualization techniques:

### Streamline rendering
The streamlines are rendered from a series of trajectories calculated by the tool, that are then flattened together into a single vertex buffer. The tool also uses a single element array buffer for all the streamlines together with the openGL primitive restart function to allow for rendering all streamlines in a single draw call. This is much more efficient than a seperate draw call and buffer for each streamline, since these are expensive operations. When filling the buffers (and vectors) the needed memory is also allocated ahead of time to prevent expensive memory allocation operations.
When calculating the streamlines, they are also cut off when reaching outside of the nonzero part of the volume. In the visualization this may not always seem to be the case, but this is due to the irregular shape of the volume and the 3D nature of the streamlines.

### Spatial coloring
The streamlines are color coded as dx -> r, dy -> g, dz -> b. This gives the streamlines some sense of spatial meaning, making it easier to see the trajectories.

### Background images
The tool utilizes background images from slices of the data volume to give more context for the streamlines and more dynamic seeding more precise.
The background images are also overlaid with a mask that sets each voxel where the vector field has a zero vector to transparent. This allows one to easier see the boundaries of the image.

### Orthographic camera with angle switching
The visualization uses an orthographic camera projection to make sure all the streamlines line up with their location in the background image. This makes it easier to see the actual trajectories of the streamlines through the volume. The camera can also switch between the view axis. 
A feature that might have been nice to add would be viewing all three axis's at the same time, like in some medical imaging software. This could also have given a nice different seeding option where the "seed point" would be the intersection of the three selected planes.

## Dependencies
- OpenGL
- GLFW3
- GLM
- GLAD (included in repo)
- Eigen (included in repo)
- ImGui (included in repo)

## Build Instructions
1. Ensure all dependencies are installed.
2. Use CMake to compile and run the code.

### Installing dependencies using Visual Studio on windows
When using visual studio and windows one can install the dependencies by using vcpkg. A usage tutorial by microsoft is given [here](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-vs?pivots=shell-powershell).

### Installing dependencies using Homebrew on Mac
When using Mac, we suggest that use Homebrew to install missing packages. Homebrew installion can be found [here](https://brew.sh/).
Other than that, the build instructions should be the same as the general ones.

## Potential Improvements
- Add more advanced filtering techniques.
- Implement tensor field visualization.
- Optimize performance for large datasets so that parameters can be adjusted in real time.
- Three axis view with seeding at the intersection of the planes.
- Volume rendering with orbit camera to better see streamlines.
