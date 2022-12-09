#include "Line.h"

#include <stdlib.h>
#include "Assert.h"

void initStrings(Strings *strings)
{
    assert(strings);

    strings->originBuffer  = nullptr;
    strings->size          = 0;
    strings->sequence      = nullptr;
    strings->stringsCount  = 0;
}

void destroyStrings(Strings *strings)
{
    assert(strings);

    strings->size         = 0;
    strings->stringsCount = 0;

    free(strings->originBuffer);
    free(strings->sequence);
}
