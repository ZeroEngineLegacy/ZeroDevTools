#include "Precompiled.hpp"

#include "TypeBlackList.hpp"
#include "Serialization/Simple.hpp"
#include "Platform/FileSystem.hpp"

namespace Zero
{
  ZilchDefineType(TypeBlacklist, builder, type)
  {
  }

  TypeBlacklist::TypeBlacklist(StringParam filename) : Object()
  {
    LoadFromFile(filename);
  }

  void TypeBlacklist::operator=(TypeBlacklist& blacklist)
  {
    this->mTypeNames = blacklist.mTypeNames;
  }

  void TypeBlacklist::Serialize(Serializer& stream)
  {
    SerializeName(mTypeNames);
  }


  bool TypeBlacklist::LoadFromFile(StringParam fileName)
  {
    Status status;

    DataTreeLoader loader;

    loader.OpenFile(status, fileName);

    if (status.Failed())
    {
      Error(status.Message.c_str());
      return false;
    }

    PolymorphicNode dummyNode;
    loader.GetPolymorphic(dummyNode);

    loader.SerializeField("HashSet", mTypeNames);

    loader.Close();

    return true;
  }

  bool TypeBlacklist::SaveToFile(StringParam fileName)
  {
    Status status;

    TextSaver saver;

    saver.Open(status, fileName.c_str());
    if (status.Failed())
    {
      Error(status.Message.c_str());
      return false;
    }


    saver.StartPolymorphic("TypeBlacklist");

    saver.SerializeField("HashSet", mTypeNames);

    saver.EndPolymorphic();

    saver.Close();

    return true;
  }


  bool TypeBlacklist::isOnBlacklist(StringParam typeName)
  {
    return mTypeNames.Contains(typeName.ToLower());
  }

}