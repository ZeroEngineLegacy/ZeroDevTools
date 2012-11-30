#include "Precompiled.hpp"

#include "Logging.hpp"
#include "Support\FileSupport.hpp"

namespace Zero
{

struct ClassDocSorter
{
  bool operator()(ClassDoc& lhs, ClassDoc& rhs) const
  {
    return strcmp(lhs.Name.c_str(),rhs.Name.c_str()) < 0;
  }
};

void WarnAndLogUndocumentedProperties(Array<ClassDoc>& classes, bool verbose, String log)
{
  if(!verbose && log.empty())
    return;

  sort(classes.all(),ClassDocSorter());

  StringBuilder builder;

  forRange(ClassDoc& classDoc, classes.all())
  {
    for(uint i = 0; i < classDoc.Properties.size(); ++i)
    {
      if(classDoc.Properties[i].Description.empty())
      {
        String str = String::Format("%s: %s is not documented.\n",classDoc.Name.c_str(),classDoc.Properties[i].Name.c_str());
        builder.Append(str);
      }
    }

    for(uint i = 0; i < classDoc.Methods.size(); ++i)
    {
      if(classDoc.Methods[i].Description.empty())
      {
        String str = String::Format("%s: %s is not documented.\n",classDoc.Name.c_str(),classDoc.Methods[i].Name.c_str());
        builder.Append(str);
      }
    }
  }

  String outStr = builder.ToString();
  if(outStr.empty())
    return;
  
  if(verbose)
    printf("%s",outStr.c_str());

  if(!log.empty())
  {
    if(!verbose)
      printf("Warning: Properties are not documented. Check log file %s.\n",log.c_str());
    WriteStringRangeToFile(log, outStr);
  }
}

}//namespace Zero
