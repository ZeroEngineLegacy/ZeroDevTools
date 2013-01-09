#pragma once

#include "Engine\Documentation.hpp"
#include "WikiOperations.hpp"

namespace Zero
{

//the only function that should be called. This will check and undocumented items.
void WarnAndLogUndocumented(Array<ClassDoc>& classes, StringParam doxyPath,
                            StringParam docPath, bool verbose, String log);

void WarnAndLogUndocumentedProperties(Array<ClassDoc>& classes, StringBuilder& builder);
void WarnUndocumentedClasses(Array<ClassDoc>& classes, StringParam doxyPath,
                             StringParam docPath, StringBuilder& builder);

void WarnNeedsWikiPage(Array<WikiUpdatePage>& pagesToUpdate, Array<ClassDoc>& documentedClasses,
                       StringParam doxyPath, StringParam docPath, 
                       bool verbose, StringParam log);

}//namespace Zero
