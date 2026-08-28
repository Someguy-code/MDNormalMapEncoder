# Normal map encoder for the Sega Megadrive / Genesis

Utility to encode normals maps in a format usable by the Sega Megadrive / Genesis. It can combine the information from several input images into one or more 4-bit images + a material file. The resulting images can be loaded directly into the Megadrive VRAM and displayed as sprites or scroll planes. They do not need to be modified to apply lighting to them. Instead, the material files describe the lighting data associated with every color in the image. Modifying the palette entries assined to them is enough to change the image lighting.

To check the Megadrive-side implementation, please check [this tech-demo](https://github.com/Someguy-code/MDNormalMapTest)

## MAT file format

All the generated material data is packed in a MAT binary file that can be attached to you Megadrive ROM as a BIN resource using SGDK's rescomp.
The file contains the following fields:

* Include pure darkness flag (unsigned 8 bits): Specifies whether an additional non-shaded color must be reserved (only needed when using ambient occlusion)
* Albedo colors:
  * Colors count (usigned 8 bits)
  * List of colors: RGB333 values, 3 x unsigned 8 bits
* Normals groups:
  * Normals groups count (unsigned 8 bit)
  * List of normals groups: Pairs first-last indices defining a color range. 2 x unsigned 8 bit
* Normals:
  * Normals count (unsigned 8 bit)
  * List of normals: 3D directions normalized in the [127, -128] range. 3 x signed 8 bit
* Diffuse component:
  * Colors written as RGB33 flag (unsigned 8 bit). 0 = Packed big-endian 16bit VDP colors, 1 = RGB333
  * Log 2 of front shades count (unsigned 8 bit)
  * Log 2 of back shades count (unsigned 8 bit)
  * Padding byte (only present if needed to align the following data to 16-bit)
  * List of front shade colors. Can be 16bit packed VDP colors or RGB333
  * List of back shade colors. Can be 16bit packed VDP colors or RGB333
* Has specular component flag (unsigned 8 bit)
* Specular component (if available)
  * Log 2 of front shades count (unsigned 8 bit)
  * Log 2 of back shades count (unsigned 8 bit)
  * List of front shade colors. RGB333 values, 3 x unsigned 8 bits
  * List of back shade colors. RGB333 values, 3 x unsigned 8 bits

## Supported lighting model

The data generated with this tool supports the following features:
* A single directional (it's not possible to encode positional information) lighting with a fixed color (the color of the light is backed-in) for both front and back-facing sides.
* Diffuse component.
* Specular component (Blinn-Phong model).
* Normal map.
* Albedo.
* Ambient occlusion.

## Color conversion

The Megadrive uses a custom 3-bit per component master palette. There are some discrepancies about the specific equivalent RGB values, but this tool uses the Blastem emulator criterion. Thus, these are the possible values for each component: 0, 49, 87, 119, 146, 174, 206, 255.

In order to find the closest match between RGB values an a color in the Megadrive master palette, the colors are compared in the CIELAB space. This helps obtain a colser preceptual equivalence rather than a raw numerical one.

## Color quantization

The Megadrive palette includes 60 usable palette entries. Moreover, it's split in 4 palettes, each with 15 available entries. A single 8x8 pixels tile can reference only one of these sub-palettes. With these constraints in mind, it's not hard to imagine limiting the number of used colors is a must.

This tool is able to automatically quantize the input images to a specific numbre of colors specified by the user. In order to mitigate the quality degradation, the error introduced is dispersed through the image using the Floyd-Steinberg dithering algorithm. Color reduction is handled differently depending on the input image type:

### Normal map

Normal map colors are interpreted as normalized directions of an hemispehere.
The user can define a reduced palette by specifying the number of horizontal and vertical divisions of this hemispfere.
Each normal map direction is matched with the closest one in the palette, using the angular distance of the vectors for the comparison.

### Albedo

The user can specify a maximum number of colors. The k-means algorithm is used to obtain a palette with up to the specified number of colors.

### Ambient occlusion

The ambient occlusion image is quantized to a 1bpp image. The dark pixel will remain always as such, unaffected by lighting.

## Usage

This is a command-line utility. It supports the following arguments (all perceded by the '-' character and followed by the value):
* **im**: Input mask file. A BMP file defining the shape cut-out. Black pixels will point to the transparent color.
* **in**: Input normal map file. BMP file containing a regular normal map image.
* **ia**: Input albedo file. BMP containing the image albedo.
* **io**: Input ambient occlusion file. BMP file containig a grey-scale image defining the ambient occlusion of the image.
* **nhs**: Number of vertical division for the normal pallete dome. Must be at least 2.
* **nvs**: Number of horizontal division for the normal pallete dome. Must be at least 1.
* **amc**: Maximum number of albedo colors. Albedo image will be quantized if needed.
* **lsc**: Log base 2 of the shades count.
* **lfc**: Color of the front light.
* **lbc**: Color of the back light.
* **ldc**: Color of the back light.
* **lsi**: Specular component intensity (in the [0, 1] range). Use 0 for no specular component.
* **lsh**: Specular component light strength power exponent. Higher values produce more compact and defined highlights.
* **lssc**: Log base 2 of the shades count of specular shades.
* **ot**: Output BMP enconded filename. The index will be appended if multiple output needed.
* **om**: Output material filename (.MAT).

**NOTE:** All colors in quotes, 3 8-bit RGB separated by commas.

Example: MDNormalMapEncoder -im "torus_mask.bmp" -in "torus_normal.bmp" -ia "torus_albedo.bmp" -ot "torus_out.bmp" -om "torus_out.mat" -nhs 6 -nvs 3 -amc 2

## TODO

* Add a graphical interface with live preview.
* Add the ability to specify hand-crafted normal and albedo palettes.
* Add support for more dithering algorithms.
* Add support for glossiness texture.
* Discard unused attribute combinations (ie.: not all albedo colors will necessarily use all nomrals in the palette).

## Special thanks

This tool was inspired by [Dither it!](https://ditherit.com) by Alex Harris.
