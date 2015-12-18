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
int write(unsigned char* dataRGB, int width, int height, int components);
float* RGBtoHSL(float r, float g, float b);
float* RGBtoHSLArray(unsigned char* RGBinput,int* width, int* height, int* components);
unsigned char ***conv3D (unsigned char* input, int w, int h);
void convASCII(unsigned char*** data, int width_img, int height_img, int width_lines, int nb_lines);
char GetASCII(char ASCIITable[6][7], int* RGB);
void fillASCIITable (char ASCIITable [6][7]);

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

// fillASCIITable : Remplit un tableau de char passé en paramètre en valeur ASCII utilisé
// ensuite de la conversion HSL vers ASCIII
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
    ASCIITable[1][6] = '*';
    
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

// GetASCII : Renvoie un caractère ASCII basé sur le triplé RGB passé en paramètre.
// Il ira piocher dans un tableau de caractère passé également en paramètre
char GetASCII(char ASCIITable[6][7], int* RGB)
{
    char output;
    
    // On convertit le triplé RGB en valeur HSL
    float* tmp = RGBtoHSL((float) RGB[0], (float) RGB[1], (float) RGB[2]);
    
    // Pour retourner une valeur ASCII suivant un triplé HSL, on va choisir un caractère
    // dans le tableau de caractère passé en paramètre suivant la valeur de la luminesence et de la couleur
    // Pour recuperer une valeur dans le tableau a partir d'une valeur, on va la multiplier par le nombre de colonne (dans le cas de la couleur)
    // et l'arrondir a l'entier le plus proche (avec floor() ) afin de savoir quel caractère prendre. (On effectue la même manipulation avec les lignes pour la luminesence)
    output = ASCIITable[(int)floor(tmp[0]*6)][(int)floor(tmp[2]*7)];
    
    return output;
}



// getRGBAvgBlock : Retourne les valeurs RGB moyenne d'un bloc de l'image (ou le contenu est stocké dans data)
// On assume qu'on connait deja la position du bloc (position X et Y)  et sa taille (Hauteur + Longueur d'un bloc)
int* getRGBAvgBlock(unsigned char*** data, int block_x, int block_y, int width_block, int height_block)
{
    int* output =(int*) malloc(sizeof(int)*3);
    int totalR,totalG,totalB,cmp_pixel,x,y;
    totalR = totalG = totalB = cmp_pixel = 0;
    
    // On parcours tous les pixels du bloc de l'ialge dont la position et la taille sont passé en paramètre
    for (y=block_y*height_block;y<(height_block*block_y)+height_block;y++)
    {
        for (x=block_x*width_block;x<width_block + (width_block*block_x);x++)
        {
            totalR += data[y][x][0];
            totalG += data[y][x][1];
            totalB += data[y][x][2];
            cmp_pixel++;            
        }
    }
    
    // Après avoir additionné toutes les valeurs RGB de chaque pixel du bloc, on retourne un triplé
    // avec la moyenne de chaque valeur du triplé RGB
     output[0] = totalR/cmp_pixel;
     output[1] = totalG/cmp_pixel;
     output[2] = totalB/cmp_pixel;

    
    return output;
}

// convASCII : Fonction qui s'occupe d'afficher une image dont le contenu RGB est passé en paramètre
// en ASCII dans le terminal suivant les paramètres d'affichages données (nombre de lignes et nombre de caractère par ligne)
void convASCII(unsigned char*** data, int width_img, int height_img, int width_lines, int nb_lines)
{
     
    int width_block = width_img / width_lines;
    int height_block = height_img / nb_lines;
    
    int rst_width = width_img % width_lines;
    int rst_height = height_img % nb_lines;
    
    int *tmp;
    int x,y;
    
    char ctmp;
    
    char ASCIITable[6][7];
    fillASCIITable(ASCIITable);
    
    // On parcourt tous les blocks de l'image
    for (y=0;y<nb_lines;y++)
    {
        for (x=0;x<width_lines;x++)
        { 
            // Lors qu'on arrive au dernier bloc en x
            //On inclut le reste des pixels hors bloc dans la moyenne du dernier bloc 
            if (x == width_lines-1)
            {
                tmp = getRGBAvgBlock(data,x,y,width_block+rst_width,height_block);                
            }
            else
            {

                // Pour chaque bloc on utilise la fonction getRGBAvgBlock pour recuperer les valeurs RGB
                // moyenne du bloc que l'on passera ensuite a la fonction GetASCII
                tmp = getRGBAvgBlock(data,x,y,width_block,height_block);
            
                // A partir du triplé de RGB passé en paramètre et le tableau de caractère, la fonction
                // va retourner le caractère ASCII corréspondant.
                ctmp = GetASCII(ASCIITable, tmp);
                printf("%c",ctmp);
            }
        }
        printf("\n");
    }
    
    
}


// conv3D : Convert un tableau a 1 dimension de valeurs RGB passé en paramètre et renvoie
// un tableau a trois dimensions avec les mêmes valeurs
unsigned char ***conv3D (unsigned char* input, int w, int h)
{    
    // Allocation dynamique du tableau a trois dimensions
    
    unsigned char ***ptr; // Tableau 3d que l'on va retourner a la fin de la fonction
    
    // Variable utilisé pour le parcours pour l'allocation dynamique
    int x,y;
    
    ptr=malloc(h*sizeof(*ptr)); // On alloue la première dimension du tableau
    for (y=0; y<h; y++)
        ptr[y]=malloc(w*sizeof(**ptr)); // On alloue la deuxième dimension en effectuant un malloc a chaque case de la premère dimension
    for (y=0; y<h; y++)
    {
       for (x=0; x<w; x++)
        {
            ptr[y][x]=malloc(3*sizeof(***ptr)); // On alloue la troisième dimension
        } 
    }
    
    // Rempissage du tableau a partir de l'input
    for (y=0;y<h;y++)
    {
        for(x=0;x<w;x++)
        {
            // On remplit le tableau avec les triplés RGB de l'ancien tableau
            // que l'on stocke dans la troisième dimension du tableau
            ptr[y][x][0]=input[(x*3) + (y*w*3)];
            ptr[y][x][1]=input[(x*3) + 1 + (y*w*3)];
            ptr[y][x][2]=input[(x*3) + 2 + (y*w*3)];
        }
    }
    
    return ptr; 
}



int main(int argc, char* argv[])
{    
    if (argc < 4)
    {
        fprintf(stderr,"ERREUR : Requiert au moins 3 arguments \n");
        return 1;
    }    
    
    unsigned char* data;
    unsigned char*** data3D;        
    int width,height,components;
    
    // On lit le fichier jpeg passé en arguments et recupere son contenu sous
    // forme d'un tableau d'unsigned char contenant les valeurs RGB de chaque pixels
    data = read(argv[1],&width,&height,&components);
    
    // Verification si la taille a afficher sur le terminal est correcte
    if (atoi(argv[2]) > width || atoi(argv[2]) <= 0)
    {
        fprintf(stderr,"ERREUR : La longueur de l'image a affiché en ASCII est incorrect (Doit etre superieur a 0 et inferieur a la longueur de l'image originale) \n");
        return 1;
    }
    if (atoi(argv[3]) > height || atoi(argv[3]) <= 0)
    {
        fprintf(stderr,"ERREUR : La hauteur de l'image a affiché en ASCII est incorrect (Doit etre superieur a 0 et inferieur a la hauteur de l'image originale) \n");
        return 1;
    }
    
    // On convertit les données recuperé dans le tableau a une dimensions
    // en tableau a trois dimentions afin de les utiliser dans la fonction convASCII
    data3D=conv3D(data,width,height);
    
    // On libère le tableau a une dimension qui nous seras plus utile pour la suite
    // On utilisera le tableau a trois dimension crée juste au dessus.
    free(data);
    
    // On appele la fonction qui va afficher l'image précdemment lue et la convertir en caractère ASCII
    // On passe donc a la fonction le tableau des données RGB, la taille de l'image, et la taille de l'affichage de l'image ASCII sur le teminal
    convASCII(data3D, width, height,atoi(argv[2]),atoi(argv[3]));
    
    free(data3D); // Une fois l'affichage effectué, on peut liberer le tableau data3D également.

    return 0;
}