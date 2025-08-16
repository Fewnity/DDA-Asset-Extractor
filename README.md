# DDA-Asset-Extractor
Destruction Derby Arenas PS2 Asset extractor / ripper

Link to the documentation: https://fewnity.github.io/DDA-Asset-Extractor/

If you want to see some research made on files (GLADIATO.UBR, SPRITES.UBR, INGAME.UBR, DD4FRONT.UBR), put the imhex_dda_project.hexproj file in the root folder where the game assets files are.<br>
And then open it with [ImHex](https://imhex.werwolv.net/).

## Usage
`DDA_Extractor.exe <input_path> <output_path>`<br>
Example: `DDA_Extractor.exe "C:\path\to\dda_folder" "C:\path\to\output"`

### Blender fix

Meshes have their triangles in the wrong side.<br>
To fix meshes in blender, you have to follow these steps:
- A (Select all meshes)
- Tab (Use edit mode)
- Shift + D (duplicate the mesh)
- Right click (abort the move meshes action)
- Alt + N (Open the Mesh->Normals menu)
- F (Flip normals)