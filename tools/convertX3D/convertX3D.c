/*
   Simple test with libxml2 <http://xmlsoft.org>. It displays the name
   of the root element and the names of all its children (not
   descendents, just children).

   On Debian, compiles with:
   gcc -Wall -o read-xml2 $(xml2-config --cflags) $(xml2-config --libs) \
                    read-xml2.c

*/

#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>

int
main(int argc, char **argv)
{
    xmlDoc         *document;
    xmlNode        *root, *first_child, *node, *scene;
    char           *filename;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s filename.xml\n", argv[0]);
        return 1;
    }
    filename = argv[1];

    document = xmlReadFile(filename, NULL, 0);
    root = xmlDocGetRootElement(document);
    // fprintf(stdout, "Root is <%s> (%i)\n", root->name, root->type);
    first_child = root->children;
    scene = NULL;
    for (node = first_child; node; node = node->next) {
        // fprintf(stdout, "\t Child is <%s> (%i)\n", node->name, node->type);
        if ((node->type == XML_ELEMENT_NODE) && (strcmp(node->name, "Scene")==0))
        {
          scene = node;
          break;
        }
    }
    if (scene == NULL) {
      fprintf(stdout, "No scene found\n");
      return -1;
    }
    // fprintf(stdout, "...\n");
    return 0;
}