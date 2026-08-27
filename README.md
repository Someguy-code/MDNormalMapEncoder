# Normal map encoder for the Sega Megadrive / Genesis

Utility to encode normals maps in a format usable by the Sega Megadrive / Genesis. It can combine the information from several input images into one or more 4-bit images + a material file. The resulting images can be directly loaded into the Megadrive VRAM and displayed as sprites of scroll planes. They do not need to be modified to apply lighting to them. Instead, the material files describe the lighting data associated with every color in the image. Modifying the palette entries is enough to change the image lighting.

## Supported lighting model

The data generated with this tool supports the following features:
* A single <span title="It's not possible to encode positional information">directional</span> lighting with [a fixed color]("The limitation stems from light color being backed-in") for both front and back-facing sides.
* Diffuse component
* Specular component

## Usage

This is a command-line utility. It supports the forllowing arguments:
