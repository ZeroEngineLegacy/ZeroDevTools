#include "Precompiled.hpp"

#include "Parsing.hpp"
#include "Serialization\CharacterTraits.hpp"

namespace Zero
{

struct CompareIndex
{
  uint index;
  CompareIndex(uint i)
    :index(i)
  {}

  template<typename arrayType>
  bool operator()(const arrayType& a, char b)
  {
    if(index < a.size())
      return a[index] < b;
    else
      return true;
  }

  template<typename arrayType>
  bool operator()(char a, const arrayType& b)
  {
    if(index < b.size())
      return a < b[index];
    else
      return false;
  }
};

template<typename type, typename rangeType, typename predicate>
rangeType lowerBound(rangeType r, const type& value, predicate pred)
{
  rangeType newRange = r;
  int count = newRange.length();
  while(count > 0)
  {
    int step = count / 2;
    auto newValue = newRange[step];
    if( pred(newValue, value) )
    {
      newRange.mBegin = newRange.mBegin + step + 1;
      count -= step+1;
    }
    else
      count = step;
  }
  return newRange;
}

template<typename type, typename rangeType, typename predicate>
rangeType upperBound(rangeType r, const type& value, predicate pred)
{
  rangeType newRange = r;
  int count = newRange.length();
  while(count > 0)
  {
    int step = count / 2;
    auto newValue = newRange[step];
    if( !pred(value, newValue) )
    {
      newRange.mBegin = newRange.mBegin + step + 1;
      count -= step+1;
    }
    else
      count = step;
  }
  return newRange;
}

String BuildDoc(ClassDoc& classDoc, Replacments& replacements)
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
      Replace(builder, replacements, methodDoc.Arugments);
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

void Replace(StringBuilder& output, Replacments& replacements, String source)
{
  StringRange sourceText = source.all();
  while(!sourceText.empty())
  {
    rrange possibleMatches = replacements.all();
    int letterIndex = 0;

    while(!possibleMatches.empty())
    {
      //Ran out of letters
      if(letterIndex == sourceText.size())
      {
        output << sourceText;
        sourceText = StringRange();
        break;
      }

      char currentLetter = sourceText[letterIndex];
      rrange lower = lowerBound(possibleMatches, currentLetter, CompareIndex(letterIndex) );
      rrange upper = upperBound(possibleMatches, currentLetter, CompareIndex(letterIndex) );

      rrange refinedMatches = rrange(lower.begin(), upper.begin());


      if(refinedMatches.empty())
      {
        String& match = possibleMatches.front().Value;
        StringRange lastText = StringRange(sourceText.begin, sourceText.begin + letterIndex);
        //The front will always be the shortest
        if(match == lastText)
        {
          output << possibleMatches.front().Replace;
          sourceText = StringRange(sourceText.begin + match.size(), sourceText.end);
          break;
        }
        else
        {
          output << sourceText.front();
          sourceText = StringRange(sourceText.begin + 1, sourceText.end);
          break;
        }
      }
      else
      {
        if(refinedMatches.length() == 1)
        {
          //Only one possible match
          String& match = refinedMatches.front().Value;
          if(sourceText.size() >= match.size() && 
            match == StringRange(sourceText.begin, sourceText.begin + match.size()))
          {
            output << refinedMatches.front().Replace ;
            sourceText = StringRange(sourceText.begin + match.size(), sourceText.end);
            break;
          }
          else
          {
            output << sourceText.front();
            sourceText = StringRange(sourceText.begin + 1, sourceText.end);
            break;
          }
        }
        else
        {
          //Next letter
          possibleMatches = refinedMatches;
          ++letterIndex;
        }
      }
    }
  }
}

String Replace(Replacments& replacements, String source)
{
  StringBuilder output;
  Replace(output, replacements, source);
  return output.ToString();
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
