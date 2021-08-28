#include <string.h>

struct CString {
    const char* str;
    int len;
};

struct CString get_certificate(const char* name/*, const char* email_id*/);
