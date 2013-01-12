#include "Precompiled.hpp"

#include "TinyXmlHelpers.hpp"
#include "..\TinyXml\tinyxml.h"
#include "Parsing.hpp"
#include "Platform\Windows\FileSystem.hpp"

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

void ExtractMethodDocs(StringParam doxyPath, String& className, Array<Replacement>& replacements,
                       HashMap<String,PropertyDoc>& properties, HashMap<String,MethodDoc>& methods)
{
  //try to open the class file
  TiXmlDocument doc(doxyPath.c_str());
  bool loadOkay = doc.LoadFile();

  if(!loadOkay)
  {
    //__debugbreak();
    return;
  }

  TiXmlElement* doxygenElement = doc.FirstChildElement("doxygen");
  TiXmlElement* compounddef = doxygenElement->FirstChildElement("compounddef");

  className = GetElementValue(compounddef, "compoundname");
  uint nameStart = className.FindLastOf(':') + 1;
  className = className.sub_string(nameStart, className.size() - nameStart);

  TiXmlNode* pSection;
  for(pSection = compounddef->FirstChild(); pSection != 0; pSection = pSection->NextSibling()) 
  {
    TiXmlNode* pMemberDef;
    for(pMemberDef = pSection->FirstChild(); pMemberDef != 0; pMemberDef = pMemberDef->NextSibling()) 
    {
      TiXmlElement* memberElement = pMemberDef->ToElement();
      if(!memberElement)
        continue;

      //only parse member definitions (both members and methods in doxy)
      if(strcmp(memberElement->Value(),"memberdef") != 0)
        continue;

      //variables have the mutable attribute while methods don't, use this to differentiate the two
      const char* isVariable = memberElement->Attribute("mutable");
      String name = GetElementValue(memberElement, "name");
      String argsstring = GetElementValue(memberElement, "argsstring");
      String briefdescription = DoxyToString(memberElement, "briefdescription");
      String returnValue  = ToTypeName(memberElement, "type");

      name = Replace(replacements,name);
      argsstring = Replace(replacements,argsstring);
      briefdescription = Replace(replacements,briefdescription);
      returnValue = Replace(replacements,returnValue);

      PropertyDoc propDoc;
      propDoc.Name = name;
      propDoc.Description = briefdescription;
      MethodDoc metDoc;
      metDoc.Name = name;
      metDoc.Description = briefdescription;
      metDoc.ReturnValue = returnValue;

      if(isVariable)
      {
        //See if this is an m'VarName' member variable.
        if(name[0] == 'm')
        {
          //strip the 'm' off of the name
          String mName = name.size() > 1 ? name.sub_string(1, name.size() - 1) : String();

          PropertyDoc* propertyDoc = properties.findPointer(mName);
          if(!propertyDoc)
            properties[mName] = propDoc;
          else
          {
            if(propertyDoc->Description.empty())
              *propertyDoc = propDoc;
          }
        }

        //See if this is a property that is not a Getter and doesn't start with m, such as Translation.
        PropertyDoc* propertyDoc = properties.findPointer(name);
        if(!propertyDoc)
          properties[name] = propDoc;
        else
        {
          if(propertyDoc->Description.empty())
            *propertyDoc = propDoc;
        }
      }
      else
      {
        //See if this is a Get 'Property' function.
        if(name[0] == 'G')
        {
          //strip the 'Get' off of the name
          String getName = name.size() > 3 ? name.sub_string(3, name.size() - 3) : String();
      
          properties[getName] = propDoc;
        }
        //this will right now make Set 'Property' functions actually be methods, but I don't think that's a huge deal now.
        //Not adding an if for setters because a set might not be a get for a property.
        else
        {
          //otherwise it's just a method
          methods[name] = metDoc;
        }
      }
    }
  }
}

void FindClassesWithBase(StringParam doxyPath, HashSet<String>& classes, HashSet<String>& baseClassesToInclude,
                         HashSet<String>& baseClassesToIgnore, HashSet<String>& classesToIgnore)
{
  typedef HashMap<String,String> ClassBaseMap;
  ClassBaseMap classToBase;

  FileRange files(BuildString(doxyPath.c_str(),"\\xml\\"));
  
  //iterate through all of the files searching for classes
  for(; !files.empty(); files.popFront())
  {
    String fileName = files.front();
  
    String classHeader = "class_zero_1_1";
    String structHeader = "struct_zero_1_1";
  
    //only open class or struct documentation files
    if(String(fileName.sub_string(0,classHeader.size())) != classHeader &&
      String(fileName.sub_string(0,structHeader.size())) != structHeader)
      continue;
  
    String fullFileName = BuildString(doxyPath.c_str(),"\\xml\\",fileName.c_str());

    //try to open the class file
    TiXmlDocument doc(fullFileName.c_str());
    bool loadOkay = doc.LoadFile();

    //should never happen since we were told this file exists, but check to be safe
    if(!loadOkay)
      return;

    TiXmlElement* doxygenElement = doc.FirstChildElement("doxygen");
    TiXmlElement* compounddef = doxygenElement->FirstChildElement("compounddef");

    //get the name of the class (strip out the Zero:: parts)
    String className = GetElementValue(compounddef, "compoundname");
    uint nameStart = className.FindLastOf(':') + 1;
    className = className.sub_string(nameStart, className.size() - nameStart);

    //get the name of the base class (this doesn't have the namespace,
    //but it will have templates so strip out anything after the first <)
    String baseName = GetElementValue(compounddef, "basecompoundref");
    uint templateStart = baseName.FindFirstOf('<');
    if(templateStart < baseName.size())
      baseName = baseName.sub_string(0,templateStart);
    
    //mark that this class has the given base
    classToBase[className] = baseName;
  }

  //now we want to iterate through all of the classes and see if this class should be documented
  //it should be document if it has one of the bases to include but doesn't have an ignore base first.
  //Also, if it itself is marked as ignore then don't add it
  ClassBaseMap::range r = classToBase.all();
  for(; !r.empty(); r.popFront())
  {
    String className = r.front().first;

    //see if we ignore this class such as random events (e.g. ChatEvent)
    if(classesToIgnore.findValue(className,"").empty() == false)
      continue;

    //iterate through the bases until we have no base
    String baseName = r.front().second;
    while(!baseName.empty())
    {
      //if we ignore this base then stop. e.g. Widget
      if(baseClassesToIgnore.findValue(baseName,"").empty() == false)
        break;

      //if this base should be included then insert the class and go to the next one. e.g. Component
      if(baseClassesToInclude.findValue(baseName,"").empty() == false)
      {
        classes.insert(className);
        break;
      }

      baseName = classToBase.findValue(baseName,"");
    }
  }
}

void ExtractMethodDocs(ClassDoc& classDoc, DocumentationLibrary& library, ClassDoc& currentClass, StringParam doxyPath, Array<Replacement>& replacements)
{
  //if(currentClass.Name == "Space")
  //  __debugbreak();

  //extract methods and properties from the base classes
  if(!currentClass.BaseClass.empty())
  {
    if(ClassDoc* parentClass = library.ClassMap.findValue(currentClass.BaseClass, NULL))
    {
      ExtractMethodDocs(classDoc, library, *parentClass, doxyPath, replacements);
    }
  }

  //try to open the class file
  String fileName = BuildString(doxyPath.c_str(),"\\xml\\class_zero_1_1", GetDoxygenName(currentClass.Name).c_str(), ".xml");
  TiXmlDocument doc(fileName.c_str());
  bool loadOkay = doc.LoadFile();
  //if loading the class file failed, search for a struct file
  if(!loadOkay)
  {
    String fileName = BuildString(doxyPath.c_str(),"\\xml\\struct_zero_1_1", GetDoxygenName(currentClass.Name).c_str(), ".xml");
    loadOkay = doc.LoadFile(fileName.c_str());

    if(!loadOkay)
    {
      String fileName = BuildString(doxyPath.c_str(),"\\xml\\struct_zero_1_1_physics_1_1", GetDoxygenName(currentClass.Name).c_str(), ".xml");
      loadOkay = doc.LoadFile(fileName.c_str());
    }
  }

  if(!loadOkay)
  {
    //__debugbreak();
    return;
  }
  
  TiXmlElement* doxygenElement = doc.FirstChildElement("doxygen");
  TiXmlElement* compounddef = doxygenElement->FirstChildElement("compounddef");

  if(&classDoc == &currentClass)
  {
    String classDesc = DoxyToString(compounddef, "briefdescription");
    classDoc.Description = classDesc;
  }

  TiXmlNode* pSection;
  for(pSection = compounddef->FirstChild(); pSection != 0; pSection = pSection->NextSibling()) 
  {
    //if this is the bried description for the class then pull out the class's description
    if(strcmp(pSection->Value(),"briefdescription") == 0)
    {
      classDoc.Description = DoxyToString(pSection->ToElement(), "para");
    }

    TiXmlNode* pMemberDef;
    for(pMemberDef = pSection->FirstChild(); pMemberDef != 0; pMemberDef = pMemberDef->NextSibling()) 
    {
      TiXmlElement* memberElement = pMemberDef->ToElement();
      if(!memberElement)
        continue;
  
      //only parse member definitions (both members and methods in doxy)
      if(strcmp(memberElement->Value(),"memberdef") != 0)
        continue;

      //variables have the mutable attribute while methods don't, use this to differentiate the two
      const char* isVariable = memberElement->Attribute("mutable");
      String name = GetElementValue(memberElement, "name");
      String argsstring = GetElementValue(memberElement, "argsstring");
      String briefdescription = DoxyToString(memberElement, "briefdescription");
      uint i = 0;
      while(briefdescription[i] == ' ')
        ++i;
      briefdescription = briefdescription.sub_string(i,briefdescription.size() - i);

      String returnValue  = ToTypeName(memberElement, "type");

      name = Replace(replacements,name);
      argsstring = Replace(replacements,argsstring);
      briefdescription = Replace(replacements,briefdescription);
      returnValue = Replace(replacements,returnValue);

      PropertyDoc propDoc;
      propDoc.Name = name;
      propDoc.Description = briefdescription;
      propDoc.Type = returnValue;
      MethodDoc metDoc;
      metDoc.Name = name;
      metDoc.Arugments = argsstring;
      metDoc.Description = briefdescription;
      metDoc.ReturnValue = returnValue;


      //See if this is a Get 'Property' function.
      if(name[0] == 'G')
      {
        //strip the 'Get' off of the name
        String getName = name.size() > 3 ? name.sub_string(3, name.size() - 3) : String();

        if(PropertyDoc* propertyDoc = classDoc.PropertyMap.findValue(getName, NULL))
        {
          *propertyDoc = propDoc;
          propertyDoc->Name = getName;
        }
      }

      //See if this is an m'VarName' member variable.
      if(name[0] == 'm')
      {
        //strip the 'm' off of the name
        String mName = name.size() > 1 ? name.sub_string(1, name.size() - 1) : String();

        if(PropertyDoc* propertyDoc = classDoc.PropertyMap.findValue(mName, NULL))
        {
          if(propertyDoc->Description.empty())
          {
            *propertyDoc = propDoc;
            propertyDoc->Name = mName;
          }
        }
      }

      //See if this is a property that is not a Getter and doesn't start with m, such as Translation.
      if(PropertyDoc* propertyDoc = classDoc.PropertyMap.findValue(name, NULL))
      {
        if(propertyDoc->Description.empty())
          *propertyDoc = propDoc;
      }

      //See if this was a method.
      if(MethodDoc* methodDoc = classDoc.MethodMap.findValue(name, NULL))
        *methodDoc = metDoc;
    }
  }
}

void ExtractMethodDocs(ClassDoc& classDoc, DocumentationLibrary& library, StringParam doxyPath, Array<Replacement>& replacements)
{
  ExtractMethodDocs(classDoc, library, classDoc, doxyPath, replacements);
}

}//namespace Zero
