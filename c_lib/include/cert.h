#include <string.h>

struct CBytes {
    const unsigned char* bytes;
    int len;
};

struct CBytes get_certificate(/*const char* name, const char* email_id*/);
