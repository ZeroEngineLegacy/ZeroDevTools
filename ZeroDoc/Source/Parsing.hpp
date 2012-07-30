#pragma once
#include "String\String.hpp"
#include "Engine\Documentation.hpp"
#include "Replacement.hpp"

namespace Zero
{

String BuildDoc(ClassDoc& classDoc, Replacments& replacements);
void Replace(StringBuilder& output, Replacments& replacements, String source);
String Replace(Replacments& replacements, String source);
void Check(StringRef className, StringRef element, StringRef string);

}//namespace Zero
