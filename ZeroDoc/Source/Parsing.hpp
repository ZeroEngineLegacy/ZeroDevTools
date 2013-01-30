#pragma once
#include "String\String.hpp"
#include "Engine\Documentation.hpp"
#include "Replacement.hpp"

namespace Zero
{

String BuildDoc(ClassDoc& classDoc, Replacements& replacements);
void Replace(StringBuilder& output, Replacements& replacements, String source);
String Replace(Replacements& replacements, String source);
void Check(StringRef className, StringRef element, StringRef string);

}//namespace Zero
