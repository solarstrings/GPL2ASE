/*
** .ase RGB palette file format description:
**
** The following palette is a two color .ase palette in RGB format, without palette name
** which is not needed for saving a valid .ase palette.
**
** offset(h)   00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
**
** 00000000    41 53 45 46 00 01 00 00 00 00 00 02 00 01 00 00      ASEF............
** 00000010    00 22 00 07 00 62 00 38 00 63 00 32 00 62 00 39      ."...b.8.c.2.b.9
** 00000020    00 00 52 47 42 20 3F 38 B8 B9 3F 42 C2 C3 3F 39      ..RGB ?8¸¹?BÂÃ?9
** 00000030    B9 BA 00 00 00 01 00 00 00 22 00 07 00 33 00 38      ¹º......."...3.8
** 00000040    00 32 00 62 00 32 00 36 00 00 52 47 42 20 3E 60      .2.B.2.6..RGB >`
** 00000050    E0 E1 3E 2C AC AD 3E 18 98 99 00 00                  àá>,¬­>."˜˜˜˜˜™..
**
** Byte description:
**          -----------------FILE HEADER BLOCK----------------- //
** 00000000    41 53 45 46                                      // File signature 4*char (ASEF)
** 00000000                00 01 00 00                          // Version number 2*int16 (1.0)
** 00000000                            00 00 00 02              // Number of colors in the palette 1*int32
**          --------------------COLOR BLOCK------------------- //
** 00000000                                        00 01        // COLOR_START 00 01
** 00000000                                              00 00  // RGB_PAL_LENGTH 00 00 00 22 (the next 34 bytes holds the RGB color information)
** 00000010    00 22                                            // RGB_PAL_LENGTH continued
** 00000010          00 07                                      // Color hexcode string length 7 characters (incl. string terminator)
** 00000010                00 62 00 38 00 63 00 32 00 62 00 39  // Color hexcode 6 characters unit16 double byte = 14 hex bytes
**                                                              // Color code #b8c2b9 = .b.8.c.2.b.9)
** 00000020    00 00                                            // String terminator 00 00
** 00000020          52 47 42 20                                // Color mode "RGB " 4*char (the one we care about)
**                                                              // Other modes are "CMYK", "LAB " and "Gray"
** 00000020                     3F 38 B8 B9 3F 42 C2 C3 3F 39   // 3x float values in big endian IEEE754 format. Value between 0 and 1 per channel
** 00000030    B9 BA                                            // IEEE754 format continued. 3*uint32_t
** 00000030          00 00                                      // color type Color type 1*int16 (00 00 Global, 00 01 Spot, 00 02 Normal)
**          --------------------------------------------------- //
**     NEXT COLOR STARTS HERE AND REPEATS THE SAME STRUCTURE    //
**           FOR EVERY COLOR UNTIL THE END OF THE FILE          //
** 00000030                00 01                                // Color start 0001
** 00000030                      00 00 00 22                    // RGB_PAL_LENGTH 00 00 00 22 (34 bytes)
** 00000030                                  00 07              // Color hexcode string length 7 characters (incl. string terminator)
** 00000030                                        00 33 00 38  // ...
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "palette.h"

#define pack754_32(f) (pack754((f), 32, 8))     // macro for packing a float32 to IEEE754 format ( From Beej's guide to network programming)

#define COLOR_START             "\x00\x01"          // color start bytes 00 01
#define RGB_PAL_LENGTH          "\x00\x00\x00\x22"  // RGB palette length 00 00 00 22 (34 bytes)
#define STR_TERMINATE           "\x00\x00"          // string terminate 00 00
#define COLOR_HEXCODE_LENGTH    "\x00\x07"          // color hexcode string length
#define VERSION                 "\x00\x01\x00\x00"  // 00 01 00 00 (1.0)


// Function prototypes
static int machineIsLittleEndian();
static void createHexString(char *str,int value);
static inline int32_t int32ToBigEndian(int32_t value);
static inline int32_t int32ToBigEndian(int32_t value);
static void writeRGBColorNameHexString(FILE *fp,int r,int g,int b);
static void writeRBGPaletteValue(FILE *fp,int red, int green, int blue);
static unsigned long long int pack754(long double f, unsigned bits, unsigned expbits);
/*
** pack754() -- pack a floating point number into IEEE-754 format
** ( From Beej's guide to network programming)
*/
static unsigned long long int pack754(long double f, unsigned bits, unsigned expbits)
{
	long double fnorm;
	int shift;
	long long sign, exp, significand;
	unsigned significandbits = bits - expbits - 1; // -1 for sign bit

	if (f == 0.0) return 0; // get this special case out of the way

	// check sign and begin normalization
	if (f < 0) { sign = 1; fnorm = -f; }
	else { sign = 0; fnorm = f; }

	// get the normalized form of f and track the exponent
	shift = 0;
	while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
	while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
	fnorm = fnorm - 1.0;

	// calculate the binary form (non-float) of the significand data
	significand = fnorm * ((1LL<<significandbits) + 0.5f);

	// get the biased exponent
	exp = shift + ((1<<(expbits-1)) - 1); // shift + bias

	// return the final answer
	return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significand;
}
/*
** Big endian stores the most significant byte first.
** Little endian stores the least significant byte first.
**
** For example, if you are trying to store 0x12345678
** then Little Endian will store 0x78 first, and Big Endian
** will store 0x12 first
**
** An unsigned int with the value of 1 has 4 bytes and look like the following in hex:
** Big Endian format:    00 00 00 01
** Little Endian format: 01 00 00 00
**
** By checking the first hex value, whether it is 01 or 00, we can
** determine if the machine is big endian or little endian.
*/
static int machineIsLittleEndian()
{
    unsigned int x = 1;     // 00 00 00 01 (Big Endian) 01 00 00 00 (Little Endian)
    char *c = (char*) &x;   // Get the first value
    return (int)*c;         // c if it's a 01, then the machine is Little Endian
}
static void writeASEHeader(FILE *fp, int32_t numberOfPalettEntries)
{
    numberOfPalettEntries = int32ToBigEndian(numberOfPalettEntries);
    // write file signature
    fwrite("ASEF",4,1,fp);
    // write version information (00 01 00 00)
    fwrite(VERSION,4,1,fp);
    // write number of palette entries
    fwrite(&numberOfPalettEntries,sizeof(int32_t),1,fp);
}

static inline int32_t int32ToBigEndian(int32_t value)
{
    // only convert to big endian if the machine architechture is little endian
    if(machineIsLittleEndian())
    {
        // convert int to big endian
        value = ( value >> 24 ) | (( value << 8) & 0x00ff0000 ) | ((value >> 8) & 0x0000ff00) | ( value << 24)  ;
    }
    return value;
}
static void writeRBGPaletteValue(FILE *fp,int red, int green, int blue)
{
    // Each color is stored as a Big Endian IEEE754 floating point value, with the range between 0 and 1

    // Convert each color to 0..1 range
    float r = (float)red / 255;
    float g = (float)green / 255;
    float b = (float)blue / 255;
    // Convert each color to IEEE754 format
    uint32_t IEEE754Red = pack754_32(r);
    uint32_t IEEE754Green = pack754_32(g);
    uint32_t IEEE754Blue = pack754_32(b);
    // Convert each color to Big Endian
    IEEE754Red = int32ToBigEndian(IEEE754Red);
    IEEE754Green = int32ToBigEndian(IEEE754Green);
    IEEE754Blue = int32ToBigEndian(IEEE754Blue);

    // write palette start
    fwrite(COLOR_START,2,1,fp);
    // write RGB palette length
    fwrite(RGB_PAL_LENGTH,4,1,fp);
    // write RGB hex string
    writeRGBColorNameHexString(fp,red,green,blue);
    // write colormode RGB (other formats not supported by the converter: Gray, CMYK or LAB)
    fwrite("RGB ",4,1,fp);
    // write rgb colors in IEEE754 format
    fwrite(&IEEE754Red,4,1,fp);
    fwrite(&IEEE754Green,4,1,fp);
    fwrite(&IEEE754Blue,4,1,fp);

    // write string terminate
    fwrite(STR_TERMINATE,2,1,fp);
}

// Note that the file format wants string values for the characters.
// So for instance, a 00 in hex is stored as 30 (48 in decimal = character '0' in the ASCII table)
static void writeRGBColorNameHexString(FILE *fp,int r,int g,int b)
{
    // color name is stored as a string.
    char hexstring[4];
    // write color name length
    fwrite(COLOR_HEXCODE_LENGTH,2,1,fp);   // 0x7 = 7 color values, stored
    // write color hex code i.e: #bebc99, in double-byte characters becomes: 00 62, 00 65, 00 62, 00 63, 00 39, 00 39
    createHexString(hexstring,r);
    fwrite(&hexstring,4,1,fp);
    createHexString(hexstring,g);
    fwrite(&hexstring,4,1,fp);
    createHexString(hexstring,b);
    fwrite(&hexstring,4,1,fp);

    // write string terminate 00 00
    fwrite(STR_TERMINATE,2,1,fp);
}
static void createHexString(char *str,int value)
{
    char hexstring[3];                  // string to store the hex value
    sprintf(hexstring, "%x", value);    // write the value as hexadecimal into the hexstring
    str[0] = 0x00;                      // write 00 to the string
    str[1] = hexstring[0];              // write first hex value to the string
    str[2] = 0x00;                      // write 00 to the string
    str[3] = hexstring[1];              // write second hex value to the string
}

int saveAse(char *filename, paletteT *palette)
{
    int i=0;    // our trusty loop variable
    FILE *fp;   // Pointer to the file to write to

    // if the filename is not provided
    if(filename == NULL){
        // write the error to screen
        printf("> saveAse() Error! - parameter ""filename"" is NULL!\n");
        return 0;
    }

    //if the palette is not provided
    if(palette == NULL){
        // write the error to screen
        printf("> saveAse() Error! - parameter ""palette"" is NULL!\n");
        return 0;
    }
    // Try to open the file
    fp = fopen(filename, "wb");
    // if the file could not be opened
    if (fp == NULL) {
        // report so
        printf(">saveAse() Error! - Cannot open :%s for writing!",filename);
        // exit the program
        return 0;
    }
    //Write ASE header to the file, and the number of palette entries
    writeASEHeader(fp, palette->numColors);
    // loop through all the colors
    for(i=0;i<palette->numColors;++i){
        // save the color to the file
        writeRBGPaletteValue(fp,palette->colors[i].r, palette->colors[i].g, palette->colors[i].b);
    }
    // close the file
    fclose(fp);
    return 1;
}
