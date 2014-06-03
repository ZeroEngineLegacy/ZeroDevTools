#include "Precompiled.hpp"
#include "Parsing.hpp"
#include "Common/String/CharacterTraits.hpp"

namespace Zero
{

String BuildDoc(ClassDoc& classDoc, Replacements& replacements)
{
  StringBuilder builder;

  if(!classDoc.BaseClass.empty())
  {
    builder << "<h4 class=\"FunctionDoc\">";
    builder << "Base Class - ";
    Replace(builder, replacements, classDoc.BaseClass);
    builder << "</h4>";
  }

  builder << "<h2> Description </h2>";
  builder << "<p>";
  Check(classDoc.Name, "Class Desc", classDoc.Description);
  Replace(builder, replacements, classDoc.Description);
  builder << "</p>";

  builder << "<h2> Events </h2>";
  if(!classDoc.EventsSent.empty())
  {
    forRange(EventDoc& eventDoc, classDoc.EventsSent.all())
    {
      builder << "<h4 class=\"FunctionDoc\">";
      builder << "On Events." << eventDoc.EventName << " Sends ";
      Replace(builder, replacements, eventDoc.EventType);
      builder << "</h4>";
    }
  }
  else
  {
    builder << "<h3> No Events </h3>";
  }

  builder << "<h2> Properties </h2>";

  if(!classDoc.Properties.empty())
  {
    forRange(PropertyDoc& propertyDoc, classDoc.Properties.all())
    {
      builder << "<h4 class=\"FunctionDoc\">";
      builder << propertyDoc.Name;
      builder << " - ";
      Replace(builder, replacements, propertyDoc.Type);

      builder << "</h4>";

      builder << "<p>";
      Check(classDoc.Name, propertyDoc.Name, propertyDoc.Description);
      Replace(builder, replacements, propertyDoc.Description);
      builder << "</p>";

    }
  }
  else
  {
    builder << "<h3> No Properties </h3>";
  }


  builder << "<h2> Methods </h2>";
  if(!classDoc.Methods.empty())
  {
    forRange(MethodDoc& methodDoc, classDoc.Methods.all())
    {
      builder << "<h4 class=\"FunctionDoc\"> ";
      Replace(builder, replacements, methodDoc.ReturnValue);

      builder << " " << methodDoc.Name << " ";
      Replace(builder, replacements, methodDoc.Arguments);
      builder << "</h4>";

      builder << "<p>";
      Check(classDoc.Name, methodDoc.Name, methodDoc.Description);
      Replace(builder, replacements, methodDoc.Description);
      builder << "</p>";
    }
  }
  else
  {
    builder << "<h3> No Methods </h3>";
  }

  return builder.ToString();
}



void Check(StringRef className, StringRef element, StringRef string)
{
  bool nonWhitespace = false;
  forRange(char c,  string.all())
  {
    if(!IsSpace(c))
      nonWhitespace = true;
  }

  if(!nonWhitespace)
  { 
    OutputDebugStringA(className.c_str());
    OutputDebugStringA("  Missing ");
    OutputDebugStringA(element.c_str());
    OutputDebugStringA("\n");
  }
}

}//namespace Zero
