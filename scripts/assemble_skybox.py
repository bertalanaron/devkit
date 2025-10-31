from PIL import Image

directory = "../examples/data/textures/skyboxes/stars/"

# Load faces (must be same size)
# +X (right), -X (left), +Y (top), -Y (bottom), +Z (front), -Z (back)
faces = {
    'posx': Image.open(f"{directory}right.png"),
    'negx': Image.open(f"{directory}left.png"),
    'posy': Image.open(f"{directory}top.png"),
    'negy': Image.open(f"{directory}bottom.png"),
    'posz': Image.open(f"{directory}front.png"),
    'negz': Image.open(f"{directory}back.png")
}
w, h = faces["posx"].size

cubemap = Image.new("RGB", (4 * w, 3 * h))

# Paste faces into a vertical cross layout:
#     [    ][+Y][    ][    ]
#     [ -X ][+Z][ +X ][ -Z ]
#     [    ][-Y][    ][    ]
cubemap.paste(faces['posy'], (w, 0))
cubemap.paste(faces['negx'], (0, h))
cubemap.paste(faces['posz'], (w, h))
cubemap.paste(faces['posx'], (2 * w, h))
cubemap.paste(faces['negz'], (3 * w, h))
cubemap.paste(faces['negy'], (w, 2 * h))

cubemap.save("cubemap.png")
