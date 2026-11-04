# Helmet material assets

- `reference.png`: the user's photograph supplied in this conversation,
  identified by the user as a diving helmet photographed at Winchester
  Cathedral. Preserved as a modeling/material reference, not sampled as a
  complete photograph across the model.
- `copper.png`: generated material tile guided by that photograph, using
  the built-in imagegen tool and the imagegen skill. The original generated
  output was copied into this project; no external generated-image path is
  required to build or run it.
- `COPPER.RGB`: 256×256 RGB24 renderer input, converted from `copper.png`.
  The DOS loader converts this to RGB565 and derives brass and rubbed-metal
  color variations. These are three ordinary Glide textures, not normal,
  roughness or shader maps. No image-generation service is used at runtime.

To regenerate the renderer input from the saved tile:

```powershell
ffmpeg -y -i assets/copper.png -vf scale=256:256 -f rawvideo -pix_fmt rgb24 assets/COPPER.RGB
```

Final generation prompt (built-in mode; the photograph was the sole reference):

> Use case: photorealistic-natural game material texture. Input image is a REFERENCE ONLY: the user's own museum photograph of an antique copper-and-brass diving helmet. Generate a single square, seamless, flat albedo texture tile of the aged brown copper on its main dome, for UV mapping onto a procedural 3D diving helmet in a DOS Glide demo. Match reference's muted warm chestnut/copper, small mottled dark oxidized spots, very fine scuffs, faint tiny verdigris in a few crevices, subtle rubbed areas. Low contrast medium-bright material with realistic tiny surface detail. Uniform diffuse lighting; no bright baked reflections, no strong shadows, no gradients across whole tile. Fill the entire square with continuous copper material. No helmet shape, windows, rings, fittings, text, labels, borders, collage or transparency. Tileable on all edges. This is a texture map, not a picture of a helmet.
