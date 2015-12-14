#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <string.h>
#include <math.h>

//Macros pour la conversion HSL->RGB
#define R(buf) *(buf)
#define G(buf) *((buf)+1)
#define B(buf) *((buf)+2)

#define H(buf) *(buf)
#define S(buf) *((buf)+1)
#define L(buf) *((buf)+2)


// Prototype de fonctions

unsigned char* read(char* filename,int* width,int* height,int* components);
float* RGBtoHSL(float r, float g, float b);
float* RGBtoHSLArray(unsigned char* RGBinput,int* width, int* height, int* components);

/* Fonction générique de conversion hsl to rgb */
float hue2rgb(float v1,float v2,float vH)
{
  if (vH<0) vH+=1;
  if (vH>1) vH-=1;
  if ((6*vH)<1) return (v1+(v2-v1)*6*vH);
  if ((2*vH)<1) return (v2);
  if ((3*vH)<2) return (v1+(v2-v1)*((2./3)-vH)*6);
  return(v1);
}

/* Fonction qui convertie le triplet hsl pointé par in en son triplé rvb correspondant
   pointé par out.
   Formule et algo issu de http://www.easyrgb.com/index.php?X=MATH&H=18#text18
*/
void hsl2rgb(float * in, unsigned char * out)
{float x,y;
  if (S(in)==0) {               /* On est en monochrome */
    R(out)=L(in)*255;
    G(out)=L(in)*255;
    B(out)=L(in)*255;
    return;
  }
  if (L(in)<0.5) y = L(in)*(1+S(in));
  else y=(L(in)+S(in))-(S(in)*L(in));
  x=2*L(in)-y;

  R(out)=255*hue2rgb(x,y,H(in)+1./3);
  G(out)=255*hue2rgb(x,y,H(in));
  B(out)=255*hue2rgb(x,y,H(in)-1./3);
}

/* fonction qui prend un buffer image HSL et retourne un buffer image RVB où
   chaque pixel est encodé par trois unsigned char */
unsigned char * convhsl2rgb(float * hsl,unsigned int width, unsigned int height)
{ int i,j;
  unsigned char *rvb=(unsigned char *)malloc(height*width*3*sizeof(char));

  for (i=0;i<height;i++)
    for (j=0;j<width;j++)
      hsl2rgb(hsl+(i*width*3)+j*3,rvb+(i*width*3)+j*3);
  return rvb;
}

// read : Lit une image passé en paramètre et renvoie le contenu de l'image
// sous forme de tableau de valeurs RGB
unsigned char* read(char* filename,int* width,int* height,int* components)
{
    struct jpeg_decompress_struct cinfo; // Declaration des structures
    struct jpeg_error_mgr jerr;          // utilisé par libjpeg
    
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
    jpeg_read_header(&cinfo,TRUE);  // On lit le header
    jpeg_start_decompress(&cinfo);  // Lancement de la compression du JPEG
    
    
    // A partir des infos du header (stockés dans cinfo), on peut definir
    // la taille du tableau de sortie.
    size = cinfo.output_width * cinfo.output_height * cinfo.output_components;
    data = (unsigned char*)malloc(size);
    *width=cinfo.output_width;
    *height=cinfo.output_height;
    *components=cinfo.output_components;
    
    while(cinfo.output_scanline < cinfo.output_height)
    {
        buffer[0] = (unsigned char*)data + cinfo.output_components * cinfo.output_width * cinfo.output_scanline;
        jpeg_read_scanlines(&cinfo, buffer, 1);
    }
    
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    
    fclose(img);
    
    return data;
    
}
// RGBtoHSLArray : Convert un tableau de valeur RGB en tableau de valeurs HSL
float* RGBtoHSLArray(unsigned char* RGBinput,int* width, int* height, int* components)
{
    int taille = (*width) * (*height) * (*components);
    
    float* output = (float*) malloc (sizeof(float)* taille);
    float* tmp;
    
    for(int i = 0;i<taille;i+=*components)
    {
        // On recupere le triplet HSL de la fonction RGBtoHSL dans le pointeur tmp
        tmp=RGBtoHSL(RGBinput[i],RGBinput[i+1],RGBinput[i+2]);
        output[i]=tmp[0];   // Et on assigne chaque du valeur du triplet dans une case
        output[i+1]=tmp[1]; // du tableau de sortie
        output[i+2]=tmp[2];
    } 
    return output;
}
// RGBtoHSL : Convertit les valeurs RGB passés en paramètre en valeur HSL
// Renvoie les valeurs HSL en un triplé de float
float* RGBtoHSL(float r, float g, float b)
{
    float* output = (float*) malloc (sizeof(float)*3);
    
    r /= 255, g /= 255, b /= 255;
    float max = fmax(r,fmax(g,b));
    float min = fmin(r,fmin(g,b));    
    float h, s, l = (max + min) / 2;
   
    if(max == min){
        h = s = 0; // achromatic
    }
    else
    {
        float d = max - min;
        if(l>0.5) s = d / (2 - max - min);
        else s = d / (max +min);
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

// write : Ecrit dans une image out.jpg le contenu du tableau RGB passé en paramètre
// de la fonction.
int write(unsigned char* dataRGB, int width, int height, int components)
{
    struct jpeg_compress_struct cinfo; // Declaration des structures
    struct jpeg_error_mgr jer;         // utilisé par libjpeg
    
    unsigned char* buffer[1];          // Buffer utilisé pour l'écriture
    
    FILE* image_out = fopen("out.jpg","w+");	
    if(image_out == NULL)
    {
         printf("Erreur : Impossible d'écrire ou de créer le fichier out.jpg\n");
         return 1;        
    }
    cinfo.err=jpeg_std_error(&jer);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo,image_out); // Definition des infos du fichiers de sortie
    cinfo.image_width = width;         // (taille,colorspace,pointeur de fichier etc...) 
    cinfo.image_height = height;
    cinfo.input_components = components;    
    cinfo.in_color_space = JCS_RGB;    
    jpeg_set_defaults(&cinfo);
    
    jpeg_start_compress(&cinfo,TRUE);
    
    while (cinfo.next_scanline < cinfo.image_height)
    {
        buffer[0] = &dataRGB[cinfo.next_scanline * cinfo.input_components * cinfo.image_width];
        jpeg_write_scanlines(&cinfo, buffer, 1);
    }
    
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(image_out);
    
    return 0;
}

void fillASCIITable (char ASCIITable [6][7])
{
    ASCIITable[0][0] = '.';
    ASCIITable[1][0] = '\'';
    ASCIITable[2][0] = '`';
    ASCIITable[3][0] = ',';
    ASCIITable[4][0] = '^';
    ASCIITable[5][0] = ':';
    
    ASCIITable[0][1] = '-';
    ASCIITable[0][2] = '/';
    ASCIITable[0][3] = 'r';
    ASCIITable[0][4] = 'L';
    ASCIITable[0][5] = 'o';
    ASCIITable[0][6] = '*';
    
    ASCIITable[1][1] = '_';
    ASCIITable[1][2] = '|';
    ASCIITable[1][3] = 'c';
    ASCIITable[1][4] = 'C';
    ASCIITable[1][5] = 'a';
    ASCIITable[1][6] = '&';
    
    ASCIITable[2][1] = '+';
    ASCIITable[2][2] = '(';
    ASCIITable[2][3] = 'v';
    ASCIITable[2][4] = 'J';
    ASCIITable[2][5] = 'h';
    ASCIITable[2][6] = '%';
    
    ASCIITable[4][1] = 'i';
    ASCIITable[4][2] = 'l';
    ASCIITable[4][3] = 'n';
    ASCIITable[4][4] = 'Y';
    ASCIITable[4][5] = 'b';
    ASCIITable[4][6] = '#';
    
    ASCIITable[3][1] = '<';
    ASCIITable[3][2] = ')';
    ASCIITable[3][3] = 'u';
    ASCIITable[3][4] = 'U';
    ASCIITable[3][5] = 'k';
    ASCIITable[3][6] = '$';
    
    ASCIITable[5][1] = '?';
    ASCIITable[5][2] = ']';
    ASCIITable[5][3] = 'x';
    ASCIITable[5][4] = 'X';
    ASCIITable[5][5] = 'd';
    ASCIITable[5][6] = '@';
}

char GetASCII(char ASCIITable[6][7], float totalHue, float totalLum, int cmp)
{
    char output = '-';
    float avgHue, avgLum; 
    
    avgHue = totalHue / cmp;
    avgLum = totalLum / cmp;
    
    output = ASCIITable[(int)floor(avgLum*6)][(int)floor(avgHue*7)];
    
    return output;
}


// convASCII : Fonction qui convertit un tableau de données HSL pour un affichage ASCII
void convertASCII(float* dataHSL, int width_image, int height_image, int width_lines, int height_lines)
{
    // Calcul de la taille d'un bloc d'image
    int width_block, height_block;
    width_block = width_image / width_lines;
    height_block = height_image / height_lines;
    
    char ASCIITable[6][7];
    fillASCIITable(ASCIITable);
    
    // Position lors du parcours de l'image
    int x,y;
    int block_x,block_y;
    
    float totalHue,totalLum = 0;
    int cmp=0;
    
    for (y = 0; y < height_lines; y++)
    {
        for (x = 0; x < width_lines; x++)
        {
            for(block_y=0;block_y<=height_block*y;block_y++)
            {
                for(block_x=0;block_x<=width_block*x;block_x+=3)
                {
                    totalHue+=dataHSL[block_x + (block_y*width_image*3)];
                    totalLum+=dataHSL[(block_x+2) + (block_y*width_image*3)];
                    cmp++;
                } 
            }
            printf("%c", GetASCII(ASCIITable,totalHue,totalLum,cmp));
            totalHue=0;
            totalLum=0;
            cmp=0;
        }
        printf("\n");
    }   
    
}

int main(int argc, char* argv[])
{    
    if (argc < 2)
    {
        fprintf(stderr,"ERREUR : Requiert au moins un argument (nom du fichier) \n");
        return 1;
    }
    
    unsigned char* data;   
    float* dataHSL;
    int width,height,components;
    
    // On lit le fichier jpeg passé en arguments et recupere son contenu sous
    // forme d'un tableau d'unsigned char contenant les valeurs RGB de chaque pixels
    data = read(argv[1],&width,&height,&components);
    
    // On convertit le tableau RGB en HSL grace a la fonction RGBtoHSL
    dataHSL=RGBtoHSLArray(data,&width,&height,&components); 
    
    // On effectue la conversion inverse
    data=convhsl2rgb(dataHSL,width,height); 
    
    convertASCII(dataHSL, width, height, atoi(argv[2]), atoi(argv[3]));
    
    // Et l'on réecrit dans un fichier le tableau obtenu afin de voir la conversion fonctionne
    write(data,width,height,components);         
    
    return 0;
}