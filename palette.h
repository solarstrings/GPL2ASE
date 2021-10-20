#ifndef PALETTE_H
#define PALETTE_H

// Increase MAX_PALETTE_COLORS to support more than 2048 colors, or rewrite the paletteT struct
// to handle dynamic palette colors using malloc: paletteColorT *colors;
#define MAX_PALETTE_COLORS 2048

// Palette color struct
typedef struct paletteColorT
{
    int r;      // red
    int g;      // green
    int b;      // blue
}paletteColorT;

// Palette struct
typedef struct paletteT
{
    int numColors;                              // number or colors in the palette
    paletteColorT colors[MAX_PALETTE_COLORS];   // pointer to all palette entries
}paletteT;
#endif
