# IsoSpriteGenerator

Isometric sprite generator is a tool for taking GLTF models and creating sprite sheet's from different directions as well as supporting taking snapshots of skinned mesh's animation frame's.  IsoSpriteGenerator can output the final colormap of a model, or the normals/metallic+roughness/emissions sprites seperately to allow for 2D isometric games that can have full physically based rendering system.

## Usage

### File Propertys

Select File - Open GLTF/GLB file.  note: jpg images are not supported, be sure to convert all images to png or dds first.
Export - Save sprite sheet with render settings.  It is not recommended to go above 8kx8k sprite sheets.  A meta json file will also be generated that includes some of the settings of the model+generator, as well as a list of sprites offsets into the generated textures.

Export With: 
Default - Default Color map with all lighting enabled.
Normals - Normal map of sprite sheet, appends _normals to output file.
Emissions - Emission the emission components of the material, appends _emissions to output file.
Albedo - Exports just the albedo map of the model, appends _albedo to output file.
Metallic - Exports Metallic+Roughness map of the model, appends _metallic to output file.

TextureSize the expected final texture size (To the next 2n size) with current settings, it is not recommended to go above 8k x 8k images.

Packing:
Largest - Each sprite sheet tile is divided according to the largest sprite sheet in the entire animation.
Tight - Sprite sheet trys to keep size to minimum(note some restrictions on how sprites are organized may prevent sprite sheet from being completely tight).

Meta-Data:
Offset - adds an offset from bottom left of each sprite to the center of the world space.  applying this offset when rendering the sprite sheets should keep the animation and directions sync'd.
(Future meta-plans include adding bone offset's objects can be attached to specific points of the sprite sheet.

### Camera
The camera settings should match the intended game's camera.  if the game is full 2D, then ortho camera should be used, otherwise a perspective camera view is also applied for games using an adjustable iso-metric view.

Type:
Perpsective - Camera setting for a perspective view, adds FoV adjustment (0-90 degrees limited).
Ortho - Camera setting for orthographic view, adds width/height adjustments where left = -width, right = width, top = height, bottom = -height.

Output:
Default - Final lit output of model.
Emissions - Only emissions output of model.
Albedo - Only albedo output of model.
Metallic - Only Metallic+Roughness output of model.

Pitch - Adjustable vertical direction of the camera, should match intended game's pitch.
Rotation - Adjustable horizontal rotation of the camera, should match intended game's camera rotation.
Distance - Distance camera is from object(between 1-800 range).

Hotkeys:
Left Shift - enable mouse camera controls, horizontal mouse movement adjusts rotation, scroll adjusts distance

### Iso Props
Isometric propertys of the model

Directions - The number of directions that should be generated, from 1 direction, upto 32 directions, directions are divided equally in a full circle.
Offset - Offset to initial direction, to allow for precise views.
Next - Jumps model to the Next Direction rotation.
Prev - Jumps model to the prev Direction rotation.

### Animation
Animation controls for the model.
Time - CurrentTime / TotalTime
Play - Run current animation.
Rewind - Reset back to 0 time.
Next - Jump to next frame's animation time and stops playing if playing was enabled.
Prev - Jump to prev frame's animation time and stops playing if playing was enabled.

Frames - How many frames of the animation should be generated[1-64 max] divides frames equally into time.
Offset - Adds offset to each frame's time segment.

### Lighting
Lighting controls, can either use single directional sun, or Image based lighting.

Sun Controls:
Rotation - Like camera, controls the horizontal rotation around the model the sun is casting from.
Pitch - Like camera, controls the vertical height the sun is looking down at the model from.

Flags:
Shadow Caster - Casts shadows on model and any background's.
Draw Sun - Draws the sun to get a visual of it's location [Hotkey: Ctrl+L]

IBL Controls:

Open brdf - opens the brdf look up table texture to use.
Open DiffuseEnv - opens a diffuse environment texture cubemap to use(only supports dds files for the time being).
Open SpecularEnv - opens a specular environment texture cubemap to use(only supports dds files for the time being).


## Compiling

Currently only windows visual studio build has been setup.  IsoSpriteGenerator is built ontop of https://github.com/slicer4ever/Lightwave and must have lightwave built first.
Once Lightwave is built, building IsoSpriteGenerator should be straight forward with only adjustment in Library Directorys to Lightwave binarys directory, and Include directorys to Lightwave's include directorys.

