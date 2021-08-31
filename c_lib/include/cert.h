#include <string.h>

struct CBytes {
    const unsigned char* bytes;
    int len;
};

struct CBytes get_certificate(const char* organisation_name);
