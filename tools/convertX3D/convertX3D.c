/*
   Simple test with libxml2 <http://xmlsoft.org>. It displays the name
   of the root element and the names of all its children (not
   descendents, just children).

   On Debian, compiles with:
   gcc -Wall -o read-xml2 $(xml2-config --cflags) $(xml2-config --libs) \
                    read-xml2.c

*/

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>

#define MESH_FLAG       0x1
#define POLYGON_FLAG    0x2

static xmlNode* getNodeNamed(xmlNode * root, const char * name) {
  if (root->children == NULL) return NULL;
  for (xmlNode *node = root->children; node; node = node->next) {
    if ((node->type == XML_ELEMENT_NODE) && (strcmp(node->name, name)==0)) return node;
    else {
      xmlNode *temp = getNodeNamed(node, name);
      if (temp != NULL) return temp;
    }
  }
  return NULL;
}


static xmlNode* getShapeInGroup(xmlNode *root) {
  if (root == NULL) return NULL;
  for (xmlNode *node = root; node; node = node->next) {
    if ((node->type == XML_ELEMENT_NODE) && (strcmp(node->name, "Shape")==0)) return node;
  }
  return NULL;
}

static void printChild(xmlNode* root) {
  if (root->children == NULL) return;
  for (xmlNode *node = root->children; node; node = node->next) {
    printf("%s contains %s\n", root->name, node->name);
  }
}

//Besoin de gerer le USE vs DEF. Pour le moment pas plus besoin que ca...

#define FIX16(x) ((uint32_t)(((x) >= 0)                                         \
    ? ((x) * 65536.0f + 0.5f)                                                  \
    : ((x) * 65536.0f - 0.5f)))

typedef struct {
  char name[32];
  uint32_t flag;
  uint32_t nbFaces;
  uint32_t facesOffset;
  uint32_t textureOffset;
}geometry;

typedef struct {
  char name[32];
  uint32_t vertexNb;
  uint32_t vertexOffset;
  uint32_t normalsOffset;
  uint32_t nbGeometry;
  uint32_t totalSectorNb;
  geometry geometry[32];
} model;

typedef struct{
  uint32_t vertex_id[4]; //A,B,C,D
  uint16_t RGB;
  uint16_t padding;
} face;

typedef struct{
  uint16_t w; //Shall be a multiple of 8
  uint16_t h;
  uint32_t format;
  uint32_t offset; //Address in the full texture.
} texture;

// NAME 32
// VERTEX SIZE  4
// VERTEX OFFSET 4
// NB_GEOMETRY 4
// NB_SECTORS 4
// {
//   NAME 28
//   FLAG 4
//   FACE SIZE 4
//   FACE OFFSET 4 //Contains color
//   TEXTURE SIZE 4
//   TEXTURE OFFSET 4
//   NORMAL SIZE 4
//   NORMAL OFFSET 4
// }

// FACE
// A 2
// B 2
// C 2
// D 2
// RGB 2
// PADDING 2

//
// TEXTURE
// W 2
// H 2
// FORMAT 1
// PADDING 3
// DATA
//
// VERTEX //fixed16
// X 4
// Y 4
// Z 4
//
// NORMALS //fixed16
// X 4
// Y 4
// Z 4

int
main(int argc, char **argv)
{
    xmlDoc         *document;
    xmlNode        *root, *group, *shape;
    char           *filename;

    model modelOut;
    uint32_t vertexOut[2048*3];
    uint32_t normalOut[2048*3];
    face faceOut[32][2048];

    if (argc < 2) {
        fprintf(stderr, "Usage: %s filename.xml\n", argv[0]);
        return 1;
    }
    filename = argv[1];

    document = xmlReadFile(filename, NULL, 0);
    root = xmlDocGetRootElement(document);
    // fprintf(stdout, "Root is <%s> (%i)\n", root->name, root->type);
    group = getNodeNamed(root, "Group");
    if (group == NULL) {
      fprintf(stdout, "No group found\n");
      xmlFreeDoc(document);
      return -1;
    }

    modelOut.nbGeometry = 0;
    for(shape = group->children; shape = getShapeInGroup(shape); shape = shape->next) {
      printf("New Geometry\n");
      geometry *geo = &modelOut.geometry[modelOut.nbGeometry];
      geo->flag = 0;
      xmlNode *material = getNodeNamed(shape, "Material");
      if (material == NULL) {
        fprintf(stdout, "No material found\n");
        xmlFreeDoc(document);
        return -1;
      }
      printf("Material name = %s\n", xmlGetProp(material, "DEF"));
      if (strcasestr(xmlGetProp(material, "DEF"), "colored") != NULL)  geo->flag |= POLYGON_FLAG;
      if (strcasestr(xmlGetProp(material, "DEF"), "fire") != NULL)     geo->flag |= MESH_FLAG;
      xmlNode *faceset = getNodeNamed(shape, "IndexedFaceSet");
      if (faceset == NULL) {
        fprintf(stdout, "No faceset found\n");
        xmlFreeDoc(document);
        return -1;
      }
      if(strcmp(xmlGetProp(faceset, "normalPerVertex"), "true")!=0) {
        printf("No normals - not supported\n");
        xmlFreeDoc(document);
        return -1;
      }
      if(strcmp(xmlGetProp(faceset, "colorPerVertex"), "false")!=0) {
        printf("Colors per vertex - not supported\n");
        xmlFreeDoc(document);
        return -1;
      }

      char* texIndex = xmlGetProp(faceset, "texCoordIndex");
      geo->nbFaces = 0;
      int nbVertexForFaces = 0;
      char *str;
      while((str=strsep(&texIndex, " ")) != NULL) {
        if ((int) strtol(str, NULL, 10) == -1) geo->nbFaces++;
        else if (strlen(str)>0) nbVertexForFaces++;
        //Add textureCoordinate to the face => nned a dedicated process.
      }

      char* coordIndex = xmlGetProp(faceset, "coordIndex");
      int nb_entry = 0;
      geo->nbFaces = 0;
      while((str=strsep(&coordIndex, " ")) != NULL) {
        int val = (int)strtol(str, NULL, 10);
        if (val == -1) {
          if (geo->nbFaces!=0) {
            //Ensure face has 4 vertex
            for (nb_entry; nb_entry <= 4; nb_entry++)
              faceOut[modelOut.nbGeometry][geo->nbFaces].vertex_id[nb_entry] = faceOut[modelOut.nbGeometry][geo->nbFaces].vertex_id[nb_entry-1];
          }
          geo->nbFaces++;
          nb_entry = 0;
        }
        else if (strlen(str)>0) faceOut[modelOut.nbGeometry][geo->nbFaces].vertex_id[nb_entry++] = val;
      }

      printf("nbFaces = %d with nbVertex %d\n", geo->nbFaces,nbVertexForFaces);

      xmlNode *coordNode = getNodeNamed(shape, "Coordinate");
      if (coordNode == NULL) {
        fprintf(stdout, "No Coordinate found\n");
        xmlFreeDoc(document);
        return -1;
      }

      modelOut.vertexNb = 0;
      char* DEFIndex = xmlGetProp(coordNode, "DEF");
      if (DEFIndex != NULL) {
        char* coordIndex = xmlGetProp(coordNode, "point");
        while((str=strsep(&coordIndex, " ")) != NULL) {
          if (strlen(str)>0) {
            //convert to fix 16.
            uint32_t val = FIX16(strtod(str, NULL));
            vertexOut[modelOut.vertexNb++] = val;
          }
        }
        if ((modelOut.vertexNb%3) != 0) {
          printf("Error with vertex number %d\n", modelOut.vertexNb);
          xmlFreeDoc(document);
          return -1;
        }
        modelOut.vertexNb = modelOut.vertexNb/3;
        printf("nbVertex = %d\n", modelOut.vertexNb);
      }

      xmlNode *normNode = getNodeNamed(shape, "Normal");
      if (normNode == NULL) {
        fprintf(stdout, "No Normal found\n");
        xmlFreeDoc(document);
        return -1;
      }

      int nbNormals = 0;
      if(xmlGetProp(normNode, "DEF") != NULL) {
        char* coordIndex = xmlGetProp(normNode, "vector");
        while((str=strsep(&coordIndex, " ")) != NULL) {
          if (strlen(str)>0) {
            //convert to fix 16.
            uint32_t val = FIX16(strtod(str, NULL));
            normalOut[nbNormals++] = val;
          }
        }
        if ((nbNormals%3) != 0) {
          printf("Error with vertex number %d\n", nbNormals);
          xmlFreeDoc(document);
          return -1;
        }
        printf("nbNormals = %d\n", nbNormals/3);
      }

      if (nbNormals/3 != modelOut.vertexNb) {
        printf("Not a normal per vertex\n");
        xmlFreeDoc(document);
        return -1;
      }

      xmlNode *texcoordNode = getNodeNamed(shape, "TextureCoordinate");
      if (texcoordNode == NULL) {
        fprintf(stdout, "No Coordinate found\n");
        xmlFreeDoc(document);
        return -1;
      }

      int nbUv = 0;
      char* texcoordIndex = xmlGetProp(texcoordNode, "point");
      while((str=strsep(&texcoordIndex, " ")) != NULL) {
        if (strlen(str)>0) nbUv++;
      }
      if ((nbUv%2) != 0) {
        printf("Error with uv number %d\n", nbUv);
        xmlFreeDoc(document);
        return -1;
      }
      printf("nbUv = %d\n", nbUv/2);

      xmlNode *colorNode = getNodeNamed(shape, "ColorRGBA");
      int faceId = 0;
      if (colorNode == NULL) {
        fprintf(stdout, "No Coordinate found\n");
        xmlFreeDoc(document);
        return -1;
      }

      int nbColor = 0;
      char* colorIndex = xmlGetProp(colorNode, "color");
      while((str=strsep(&colorIndex, " ")) != NULL) {
        if (strlen(str)>0) {
          nbColor++;
          uint16_t col = (uint16_t)(strtod(str, NULL) * 32.0 + 0.5);
          int component = nbColor%4;
          if (component != 3) faceOut[modelOut.nbGeometry][nbColor/4].RGB |= (col<<(component*5));
          faceOut[modelOut.nbGeometry][nbColor/4].RGB |= 0x8000;
        }
      }
      if ((nbColor%4) != 0) {
        printf("Error with color number %d\n", nbColor);
        xmlFreeDoc(document);
        return -1;
      }
      printf("nbColor = %d\n", nbColor/4);

      modelOut.nbGeometry++;
    }

    xmlFreeDoc(document);
    return 0;
}
