# DDA Asset Extractor
Destruction Derby Arenas PS2 Asset extractor / ripper.

<img width="1534" height="853" alt="image" src="https://github.com/user-attachments/assets/13c46aef-3db7-470c-b895-2d1ff320b040" />

Link to the documentation: https://fewnity.github.io/DDA-Asset-Extractor/

If you want to see some research made on files (GLADIATO.UBR, SPRITES.UBR, INGAME.UBR, DD4FRONT.UBR), put the `imhex_dda_project.hexproj` file in the root folder where the game assets files are.<br>
And then open it with [ImHex](https://imhex.werwolv.net/).

Feel free to make a pull request!

## Usage
`DDA_Extractor.exe <input_path> <output_path>`<br>
Example: `DDA_Extractor.exe "C:\path\to\dda_folder" "C:\path\to\output"`

For each file you will get PNG textures and meshes in output.fbx

Currently:<br>
- Power ups and car wheels meshes are not extracted.<br>
- Extracted meshes are splitted into many triangles batches, it's how the PS2 works. I have to find a way to group them.<br>
- Skybox meshes are not extracted.<br>
- Dynamic objects/animated objects position will be wrong.

This project works with the SCES_507.81 version (Europe), the tool will probably crash at the end with other version of the game. Only FLASH files (menus) should be missing.

### Blender fix

Meshes have their triangles in the wrong side.<br>
To fix meshes in blender, you have to follow these steps:
- A (Select all meshes)
- Tab (Use edit mode)
- Shift + D (duplicate the mesh)
- Right click (abort the move meshes action)
- Alt + N (Open the Mesh->Normals menu)
- F (Flip normals)

## Gallery

<img width="1536" height="854" alt="image" src="https://github.com/user-attachments/assets/02b4a505-6fa8-4600-a6ab-45b97a26ae01" />
<img width="1536" height="855" alt="image" src="https://github.com/user-attachments/assets/758a97e3-73ea-4e78-aaa2-d087c385b6cb" />

## Used libraries
stb_image_write: https://github.com/nothings/stb

Assimp: https://github.com/assimp/assimp/

## License

The code of this repository is under the MIT license.
