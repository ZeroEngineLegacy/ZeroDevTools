#pragma once

namespace Zero
{

struct Replacement
{
  Replacement()
  {

  }
  Replacement(StringRef a, StringRef b)
    :Value(a), Replace(b)
  {
  }

  String Value;
  String Replace;
  char operator[](uint index)const{ return Value[index]; }
  uint size()const {return Value.size(); };
  bool operator<(const Replacement& right)const{return Value < right.Value;}
};

typedef Array<Replacement> Replacements;
typedef Replacements::range rrange;

}//namespace
