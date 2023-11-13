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
    xmlNode        *root, *first_child, *node;
    char           *filename;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s filename.xml\n", argv[0]);
        return 1;
    }
    filename = argv[1];

    document = xmlReadFile(filename, NULL, 0);
    root = xmlDocGetRootElement(document);
    fprintf(stdout, "Root is <%s> (%i)\n", root->name, root->type);
    first_child = root->children;
    for (node = first_child; node; node = node->next) {
        fprintf(stdout, "\t Child is <%s> (%i)\n", node->name, node->type);
    }
    fprintf(stdout, "...\n");
    return 0;
}