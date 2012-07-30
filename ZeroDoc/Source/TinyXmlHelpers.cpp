#include "Precompiled.hpp"

#include "TinyXmlHelpers.hpp"
#include "..\TinyXml\tinyxml.h"
#include "Parsing.hpp"

namespace Zero
{

String GetDoxygenName(String name)
{
  char buffer[1000];
  const char* input = name.c_str();

  char* output = buffer;

  while(*input != '\0')
  {
    if( isupper(*input) )
    {
      *output = '_';
      ++output;
    }

    *output = tolower(*input);
    ++output;
    ++input;
  }

  *output = '\0';

  return buffer;
}

String ToTypeName(TiXmlElement* parent, cstr name)
{
  String typeName = GetElementValue(parent, "type", "ref");
  if(!typeName.empty())
    return typeName;
  return GetElementValue(parent, "type");
}

TiXmlElement* FindElementWithAttribute(TiXmlElement* element, cstr attribute, cstr value)
{
  TiXmlElement* pChild = NULL;
  for ( pChild = element->FirstChildElement(); pChild != 0; pChild = pChild->NextSiblingElement()) 
  {
    cstr elementAttValue = pChild->Attribute(attribute);
    if(elementAttValue && strcmp(elementAttValue,value)==0)
    {
      return pChild;
    }
  }
  return NULL;
}

void RecursiveExtract(StringBuilder& builder, TiXmlNode* node)
{
  if(TiXmlText* text = node->ToText())
    builder << " " << text->Value();

  if(TiXmlElement* element = node->ToElement())
  {
    TiXmlNode* child = element->FirstChild();
    while(child)
    {
      RecursiveExtract(builder, child);

      child = child->NextSibling();
    }
    builder << " ";
  }

}

String DoxyToString(TiXmlElement* parent, cstr name)
{
  StringBuilder builder;

  TiXmlElement* mainElement = parent->FirstChildElement(name);

  if(mainElement)
  {
    RecursiveExtract(builder, mainElement);
  }

  return builder.ToString();
}

String GetElementValue(TiXmlElement* parent, cstr name, cstr second)
{
  TiXmlElement* element = parent->FirstChildElement(name);
  if(element)
  {
    if(second)
      element = element->FirstChildElement(second);

    if(element)
    {
      const char* text = element->GetText();
      if( text )
      {
        return text;
      }
    }
  }
  return String();
}

void ExtractMethodDocs(ClassDoc& classDoc, DocumentationLibrary& library, ClassDoc& currentClass, StringParam doxyPath, Array<Replacement>& replacements)
{
  //if(classDoc.Name != "KeyboardEvent")
  //  return;

  if(!currentClass.BaseClass.empty())
  {
    if(ClassDoc* parentClass = library.ClassMap.findValue(currentClass.BaseClass, NULL))
    {
      ExtractMethodDocs(classDoc, library, *parentClass, doxyPath, replacements);
    }
  }

  String fileName = BuildString(doxyPath.c_str(),"\\xml\\class_zero_1_1", GetDoxygenName(currentClass.Name).c_str(), ".xml");
  TiXmlDocument doc(fileName.c_str());
  bool loadOkay = doc.LoadFile();
  if(!loadOkay)
  {
    String fileName = BuildString(doxyPath.c_str(),"\\xml\\struct_zero_1_1", GetDoxygenName(currentClass.Name).c_str(), ".xml");
    loadOkay = doc.LoadFile(fileName.c_str());
  }

  if (loadOkay)
  {
    TiXmlElement* doxygenElement = doc.FirstChildElement("doxygen");
    TiXmlElement* compounddef = doxygenElement->FirstChildElement("compounddef");

    if( &classDoc == &currentClass )
    {
      String classDesc = DoxyToString(compounddef, "briefdescription");
      classDoc.Description = classDesc;
    }

    TiXmlNode* pSection;
    for ( pSection = compounddef->FirstChild(); pSection != 0; pSection = pSection->NextSibling()) 
    {
      TiXmlNode* pMemberDef;
      for ( pMemberDef = pSection->FirstChild(); pMemberDef != 0; pMemberDef = pMemberDef->NextSibling()) 
      {
        if(TiXmlElement* memberElement = pMemberDef->ToElement())        
        {
          if( strcmp(memberElement->Value(),"memberdef") == 0)
          {
            String name = GetElementValue(memberElement, "name");
            String argsstring = GetElementValue(memberElement, "argsstring");
            String briefdescription = DoxyToString(memberElement, "briefdescription");
            String returnValue  = ToTypeName(memberElement, "type");

            name = Replace(replacements,name);
            argsstring = Replace(replacements,argsstring);
            briefdescription = Replace(replacements,briefdescription);
            returnValue = Replace(replacements,returnValue);


            //std::cout << name << " " << returnValue << argsstring << " " << briefdescription << "\n";

            ///Check for Get'Property' function.
            if(name[0] == 'G')
            {
              String getName = name.size() > 3 ? name.sub_string(3, name.size() - 3) : String();

              if(PropertyDoc* propertyDoc = classDoc.PropertyMap.findValue(getName, NULL) )
              {
                propertyDoc->Description = briefdescription;
                propertyDoc->Type = returnValue;
              }
            }

            ///Check for m'Property' member variable.
            if(name[0] == 'm')
            {
              String mName = name.size() > 1 ? name.sub_string(1, name.size() - 1) : String();

              if(PropertyDoc* propertyDoc = classDoc.PropertyMap.findValue(mName, NULL) )
              {
                if(propertyDoc->Description.empty())
                {
                  propertyDoc->Description = briefdescription;
                  propertyDoc->Type = returnValue;
                }
              }


            }

            if(PropertyDoc* propertyDoc = classDoc.PropertyMap.findValue(name, NULL) )
            {
              if(propertyDoc->Description.empty())
              {
                propertyDoc->Description = briefdescription;
                propertyDoc->Type = returnValue;
              }
            }

            if(MethodDoc* methodDoc = classDoc.MethodMap.findValue(name, NULL) )
            {
              methodDoc->Description = briefdescription;
              methodDoc->Arugments = argsstring;
              methodDoc->ReturnValue = returnValue;
            }
          }
        }
      }
    }
  }
  else
  {
    // __debugbreak();
  }
}

void ExtractMethodDocs(ClassDoc& classDoc, DocumentationLibrary& library, StringParam doxyPath, Array<Replacement>& replacements)
{
  ExtractMethodDocs(classDoc, library, classDoc, doxyPath, replacements);
}

}//namespace Zero
