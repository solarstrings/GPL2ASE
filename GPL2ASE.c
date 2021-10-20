/*
** GPL2ASE Version 1.0
** Author: Johan Forsblom
**
** GIMP is free software anyone can use, Adobe Photoshop not so much.
**
** This converter was written for converting GIMP .gpl palettes to Adobe .ase palettes, for various reasons:
**  - Sharing custom palettes with those who use Photoshop as their daily driver.
**  - converting custom .gpl palettes to .ase for importing into blender for texture painting
**  - Any other .ase need anyone might have
**
** The converter supports up to 2048 colors in a palette (way more than enough for pixelart)
**
** Want more palette colors? Crank the #define MAX_PALETTE_COLORS 2048 up higher & recompile.
** Or rewrite the paletteT struct to dynamically allocate memory for the colors.
**
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include "aseWriter.h"

int loadGimpPalette(char *filename, paletteT *palette)
{
    char *splitString;   // string that has been splitted
    int currentColor=0;

    // Max 1048 characters / line - more than enough for parsing a .gpl palette file
    char chunk[1048];
    // try to open the file
    FILE *fp = fopen(filename, "r");
    // If it does not exist
    if(fp == NULL){
        // print the error to screen
        printf("Error: The file '%s' does not exist!\n", filename);
        // exit the program
        exit(1);
    }
    // read the first line from the file
    fgets(chunk, sizeof(chunk), fp);

    // check if it's a GIMP palette file
    if(strncmp(chunk,"GIMP Palette",sizeof(char)*12)!=0){
        // if not, tell the user so
        printf("The file:'%s' is not a Gimp Palette file!\n", filename);
        // exit the program
        exit(1);
    }

    // Read file line by line
    while(fgets(chunk, sizeof(chunk), fp) != NULL) {
        // If the line begins with a comment #, N(Name) or C (Columns) or is a string termintaor, skip it
        if(chunk[0] != '#' && chunk[0]!= 'N' && chunk[0] != 'C' && chunk[0] != '\0' ){

            // if max number of colors was reached
            if(palette->numColors > MAX_PALETTE_COLORS-1){
                // tell the user so
                printf("warning, max palette colors is %d.\n", MAX_PALETTE_COLORS);
                // break out of the while loop.
                break;
            }
            else{
                // read the Red value
                splitString = strtok(chunk," \t");
                // convert & store the red color in the palette struct
                palette->colors[currentColor].r = atoi (splitString);
                // read the Green value
                splitString = strtok (NULL, " \t");
                // convert & store the green color in the palette struct
                palette->colors[currentColor].g = atoi (splitString);
                // read the Blue value
                splitString = strtok (NULL, " \t");
                // convert & store the blue color in the palette struct
                palette->colors[currentColor].b = atoi (splitString);

                // If a color value is out of range
                if (palette->colors[currentColor].r < 0 || palette->colors[currentColor].r > 255
                ||  palette->colors[currentColor].g < 0 || palette->colors[currentColor].g > 255
                ||  palette->colors[currentColor].b < 0 || palette->colors[currentColor].b > 255){
                    // tell the user so
                    printf("Error! One of the RGB values is out of range! (all values must be between 0 and 255).\n");
                    // close the file
                    fclose(fp);
                    // exit the program
                    exit(1);
                }
                // increase the number of colors in the palette.
                palette->numColors++;
                // go to the next color index
                currentColor++;
            }
        }
     }

    fclose(fp);
    return 1;
}

void addAseFileEndingIfMmissing(char *filename, char *newFilename)
{
    char fileNameEnding[5] = {0};   // char array for file name ending
    int fileNamelength;             // length of the file name
    int fileNameEndingPosition;     // position where the .ase starts in the string

    // if the file name and newFile parameters are valid
    if(filename != NULL && newFilename != NULL){

        // get the length of the file name
        fileNamelength = strlen(filename);

        // calculate the starting position for the file name ending
        fileNameEndingPosition = fileNamelength-4;
        // copy the last four characters in the file name to the fileNameEnding string
        sprintf(fileNameEnding,filename+fileNameEndingPosition);

        // if the file name does not end with ".ase"
        if(strcmp(fileNameEnding,".ase") != 0 && strcmp(fileNameEnding,".ASE") != 0){
            // add the .ase to the end of the new file name
            sprintf(newFilename,"%s.ase",filename);
        }
        // if the file name already ends with .ase
        else{
            // copy the file name as is
            sprintf(newFilename,"%s",filename);
        }
    }
}

void printProgramInfo(int tooFewParameters){

    #ifdef __linux__
    system("clear");        // linux terminal uses clear
    #elif _WIN32
    system("cls");          // windows cmd uses "cls"
    #endif

    printf("*********************************************\n");
    printf("*  GPL2ASE Version 1.0                      *\n");
    printf("*  Gimp GPL -> Adobe ASE Palette converter  *\n");
    printf("*  Writen by: Johan Forsblom                *\n");
    printf("*********************************************\n");
    // if there was too few parameters passed in from the command line
    if(tooFewParameters == 1){
        // write out usage
        printf("This program converts a GIMP palette .gpl to an\nAdobe .ASE RGB palette.\n");
        printf("\nUsage: GPL2ASE input.gpl output.ase\n");
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    char *paletteName;  // pointer for the palette name
    paletteT palette;   // the palette

    // If there is not enough parameters
    if(argc <=2){
        // Write program info with usage text
        printProgramInfo(1);

    }
    // allocate memory for the file name (add 4 extra characters for .ase file ending)
    paletteName = (char *) malloc((strlen(argv[2])+4)*sizeof(char));
    // memory allocation failed
    if(paletteName == NULL){
        // tell the user so
        printf("Error! Could not allocate memory for new palette name!\n");
        // exit the program
        exit(1);
    }
    // add .ase to the adobe ase file name if missing
    addAseFileEndingIfMmissing(argv[2],paletteName);

    // print program info without usage text
    printProgramInfo(0);

    // load the gimp palette
    loadGimpPalette(argv[1],&palette);
    // tell the user what's happening
    printf("\n> Converting GIMP gpl palette: '%s' to Adobe ase palette '%s'\n",argv[1],paletteName);
    // if something went wrong during the palette converting process
    if(!saveAse(paletteName,&palette)){
        printf("\n> Failed to convert the palette!!\n");
    }else{
        printf("> Done!\n");
        printf("> Palette saved as: '%s'\n\n\n",paletteName);
    }
    free(paletteName);
    return 0;
}


