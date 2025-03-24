# Streamline Visualization Project

## Project Overview
This project is a 3D streamline visualization tool for exploring vector and tensor fields, with a focus on brain imaging and scientific visualization. The application allows interactive exploration of complex vector fields through streamline generation, rendering, and advanced visualization techniques.



## Key Components

### 1. Data Reading and Management
- **DataReader.cpp/h**: Handles reading NIFTI format files
    - Key Functions:
        - `readData()`: Read scalar volumetric data
        - `readVectorData()`: Read vector field data
        - `readTensorData()`: Read tensor field data

### 2. Vector Field Representation
- **VectorField.cpp/h**: Manages 3D vector field data
    - Key Methods:
        - `getVector()`: Retrieve vector at integer grid coordinates
        - `interpolateVector()`: Interpolate vector at floating-point coordinates
        - `isInBounds()`: Check coordinate validity

### 3. Streamline Generation
- **StreamlineTracer.cpp/h**: Generates streamlines from vector fields
    - Key Methods:
        - `traceStreamline()`: Generate a complete streamline from a seed point
        - `generateSeedGrid()`: Create uniform seed point distribution
        - `generateUnifiedBrainSeeds()`: Specialized seeding for brain data
        - `rk4Integrate()`: Advanced numerical integration method

### 4. Streamline Rendering
- **StreamlineRenderer.cpp/h**: Manages OpenGL rendering of streamlines
    - Key Methods:
        - `prepareStreamlines()`: Prepare streamline data for rendering
        - `render()`: Draw streamlines with various color modes
        - `addStreamlines()`: Dynamically add new streamlines

### 5. Main Application
- **Source.cpp**: Primary application logic
    - Handles:
        - OpenGL initialization
        - Rendering loop
        - User interaction
        - Data loading and visualization

### 6. Shader Management
- **Shader.h**: OpenGL shader program wrapper
    - Manages shader compilation, linking, and uniform setting

### Streamline Seeding Modes

The application provides three distinct seeding strategies for generating streamlines:

1. **Grid Seeding - Creates this 3x3x3 grid for each direction**
    - Creates a uniform grid of seed points across the entire volume
    - Seed points are evenly distributed in X, Y, and Z directions
    - Best for:
        * Systematic exploration of vector field
        * Uniform sampling of vector directions
        * Initial understanding of field characteristics

2. **Unified Brain Seeding - I use this for the brain dataset**
    - Specialized algorithm for neuroimaging data
    - Intelligently selects seed points based on:
        * Scalar intensity thresholds
        * Vector field magnitude
        * Anatomical structure
    - Advantages:
        * Focuses on physiologically relevant regions
        * Reduces noise and irrelevant streamlines
        * Highlights key neural pathways

3. **Toy Dataset Seeding - doesn't work very well, needs improving**
    - Optimized for synthetic or simplified vector fields
    - Selects seed points to best represent simplified vector dynamics
    - Useful for:
        * Algorithmic visualization
        * Educational demonstrations
        * Testing streamline generation methods

### Streamline Parameter Sliders

#### Seed Density Slider
- Controls the number of seed points generated
- Range: 1 to 20
- Higher values = More seed points
- Impact:
    * Increases computational complexity
    * Provides more detailed visualization
    * Can reveal finer vector field structures

#### Step Size Slider
- Determines the integration step length for streamline tracing
- Range: 0.1 to 2.0
- Smaller steps = More accurate but slower tracing
- Larger steps = Faster computation but potentially less precise

#### Minimum Magnitude Slider
- Filters out vectors with magnitude below this threshold
- Range: 0.0001 to 0.1
- Lower values include more subtle vector field variations
- Higher values focus on stronger vector directions

#### Maximum Length Slider
- Limits the maximum length of generated streamlines
- Range: 10.0 to 100.0
- Prevents infinitely long or looping streamlines
- Helps manage computational resources

#### Maximum Steps Slider
- Constrains the number of integration steps per streamline
- Range: 100 to 2000
- Prevents excessive computational time
- Balances detail and performance

### Camera Navigation

#### Standard Camera Mode
- **Movement**:
    * W: Move forward
    * S: Move backward
    * A: Strafe left
    * D: Strafe right
    * Space: Move up
    * Left Ctrl/Shift: Move down

- **Rotation**:
    * Right-click and drag to rotate view
    * Mouse scroll wheel zooms in/out

#### Orbit Camera Mode
- Rotates the view around a fixed center point
- Useful for examining 3D vector field from different angles
- Left-click and drag to rotate around the data volume center

#### Camera Controls
- **Reset Camera**: Returns to initial view
- **Orbit Camera Mode Toggle**: Switch between free and orbit camera modes

### Interactive mode
- **Interactive Seeding**: Click to add seed points - it works but its generating a single streamline - I will fix this to generate 50/100 streamlines per click so its more visible.

## Visualization Techniques
- Vector field streamline generation
- Interactive seeding methods
- Multiple visualization modes
- Advanced numerical integration (RK4)
- Adaptive seeding for different dataset types

## Dependencies
- OpenGL
- GLFW
- GLAD
- GLM
- ImGui

## Build Instructions
1. Ensure all dependencies are installed
2. Use CMake or Visual Studio project file
3. Set up include and library paths
4. Compile and run

## Usage
- Use mouse and keyboard to navigate
- Adjust parameters in the ImGui control panel
- Switch between different datasets
- Customize streamline generation and rendering

## Potential Improvements
- Add more advanced filtering techniques
- Implement tensor field visualization
- Enhance interactive seeding methods, by this I mean fix to add be able to seed more with a single click
- Fix it for toy-dataset
- Optimize performance for large datasets
- Optimize for moving parameters
