#pragma once
#include "String\StringBuilder.hpp"
#include "Engine\Documentation.hpp"
#include "Replacement.hpp"

class TiXmlElement;
class TiXmlNode;

namespace Zero
{

String GetDoxygenName(String name);
String ToTypeName(TiXmlElement* parent, cstr name);
TiXmlElement* FindElementWithAttribute(TiXmlElement* element, cstr attribute, cstr value);
void RecursiveExtract(StringBuilder& builder, TiXmlNode* node);
String DoxyToString(TiXmlElement* parent, cstr name);
String GetElementValue(TiXmlElement* parent, cstr name, cstr second=NULL);
void ExtractMethodDocs(ClassDoc& classDoc, DocumentationLibrary& library, ClassDoc& currentClass, StringParam doxyPath, Array<Replacement>& replacements);
void ExtractMethodDocs(ClassDoc& classDoc, DocumentationLibrary& library, StringParam doxyPath, Array<Replacement>& replacements);

}//namespace Zero
