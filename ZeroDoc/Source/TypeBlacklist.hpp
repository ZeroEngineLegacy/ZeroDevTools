#pragma once

namespace Zero
{
  class TypeBlacklist : public Object
  {
  public:
    ZilchDeclareType(TypeCopyMode::ReferenceType);

    TypeBlacklist() : Object(){}

    void operator=(TypeBlacklist& blacklist);

    TypeBlacklist(StringParam filename);

    void Serialize(Serializer& stream);

    bool LoadFromFile(StringParam fileName);

    bool SaveToFile(StringParam fileName);

    bool isOnBlacklist(StringParam typeName);
  //private:
    HashSet<String> mTypeNames;
  };
}