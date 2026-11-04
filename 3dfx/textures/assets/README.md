# Cathedral materials

`materials.png` is an original four-material atlas generated with the built-in
imagegen tool for this demo. `STONE.RGB`, `ROOF.RGB`, `GRASS.RGB` and
`PAVING.RGB` are 256 x 256 RGB24 crops of its quadrants, converted with FFmpeg.
These raw files are the renderer inputs; no generation or image tools are
needed to build or run the DOS executable.

The supplied cathedral photographs, Google Maps views, plan and section were
used only as visual references. None was copied into this folder or embedded
in an asset. The building is a procedural interpretation, not a survey.
Pointed window/portal cutouts, mullions, checkerboard and font are generated
deterministically by C code at startup.

## Exact generation prompt

Use case: photorealistic-natural. Asset type: a square 2 by 2 game texture atlas for a 1990s 3D reconstruction inspired by Winchester Cathedral, England. Generate original material surfaces, not a building picture. Four equal perfectly square quadrants meeting exactly at image centre, no borders, margins, gaps, labels or text. TOP LEFT: pale grey-beige English medieval limestone ashlar masonry, small rectangular blocks in horizontal staggered courses, subtle worn mortar, occasional brown-grey weathering, flat front-on view. TOP RIGHT: aged dark grey lead roofing with straight closely spaced vertical standing seams and subtle horizontal joints, flat front-on view. BOTTOM LEFT: muted olive-green close-mown English cathedral lawn, top-down flat grass surface with soft organic variation, no objects. BOTTOM RIGHT: worn grey-tan rectangular paving slabs in staggered rows, fine joints, top-down. Each quadrant fills its entire allocated square and should tile seamlessly on its own. Consistent soft diffuse neutral daylight; albedo only, no cast shadows, no perspective, no directional illumination, no decorative objects, no buildings, no watermarks, no maps. Fine but readable detail that survives reducing each quadrant to 256 by 256 pixels. Restrained natural colours.
