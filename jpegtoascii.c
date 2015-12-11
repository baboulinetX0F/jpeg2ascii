#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <string.h>
#include <math.h>


// Prototype de fonctions

unsigned char* read(char* filename);
double* RGBtoHSL(double r, double g, double b);
unsigned char* RGBtoHSLArray(unsigned char* RGBinput);

unsigned char* read(char* filename)
{
    struct jpeg_decompress_struct cinfo; // Declaration des structures
    struct jpeg_error_mgr jerr;          // utilisé par libjpeg pour la lecture
    
    long size;                  // Taille du tableau en sortie
    
    unsigned char *buffer[1];   // Buffer utilisé pour la lecture
    unsigned char *data;        // Données de l'image recuperés en sortie (Valeurs RGB)
    
    FILE* img = fopen(filename,"r");
    if (img == NULL)
    {
        fprintf(stderr,"ERREUR : Impossible d'ouvrir le fichier %s \n",filename);
        exit(EXIT_FAILURE);
    }
    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo,img);
    jpeg_read_header(&cinfo,TRUE);
    jpeg_start_decompress(&cinfo);
    
    size = cinfo.output_width * cinfo.output_height * cinfo.output_components;
    data = (unsigned char*)malloc(size);
    
    while(cinfo.output_scanline < cinfo.output_height)
    {
        buffer[0] = (unsigned char*)data + 3 * cinfo.output_width * cinfo.output_scanline;
        jpeg_read_scanlines(&cinfo, buffer, 1);
    }
    
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    
    fclose(img);
    
    return data;
    
}

unsigned char* RGBtoHSLArray(unsigned char* RGBinput)
{
    unsigned char* output;
    
    // CODE TO BE ADDED HERE
    
    return output;
}

double* RGBtoHSL(double r, double g, double b)
{
    double* output = (double*) malloc (sizeof(double)*3);
    
    r /= 255, g /= 255, b /= 255;
    double max = fmax(r,fmax(g,b));
    double min = fmin(r,fmin(g,b));    
    double h, s, l = (max + min) / 2;
   
    if(max == min){
        h = s = 0; // achromatic
    }
    else
    {
        double d = max - min;
        if(l>0.5) s = d / (2 - max - min);
        if (max == r)
            h = (g - b) / d + (g < b ? 6 : 0);
        else if (max == g)
            h = (b - r) / d + 2;
        else if (max == b)
            h = (r - g) / d + 4;
    }
    h /= 6;      
    
    output[0] = h; output[1] = s; output[2] = l;
    return output;
}

int main(int argc, char* argv[])
{    
    if (argc != 2)
    {
        fprintf(stderr,"ERREUR : Requiert un argument (nom du fichier) \n");
        return 1;
    }
    
    unsigned char* data;
    
    data = read(argv[1]);
    
    // FOR DEBUG    
    double* test = RGBtoHSL(data[0], data[1], data[2]);
    printf("R:%d\nG:%d\nB:%d\n", data[0], data[1], data[2]);
    printf("H:%f\nS:%f\nL:%f\n", test[0], test[1], test[2]);
      
    
    return 0;
}