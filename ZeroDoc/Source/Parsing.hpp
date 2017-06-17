#pragma once
#include "String\String.hpp"
#include "Engine\Documentation.hpp"
#include "Support\StringReplacement.hpp"

namespace Zero
{

String BuildDoc(ClassDoc& classDoc, Replacements& replacements);
void Check(String& className, String& element, String& string);

}//namespace Zero
