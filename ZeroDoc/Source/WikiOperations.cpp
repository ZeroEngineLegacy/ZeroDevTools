#include "Precompiled.hpp"

#include "WikiOperations.hpp"

#define CURL_STATICLIB

#include <curl/curl.h>
#include "Serialization/CharacterTraits.hpp"
#include "Support/StringMap.hpp"
#include "Engine/EngineContainers.hpp"
#include "Utility/FilePath.hpp"
#include "Serialization/Text.hpp"
#include "../TinyXml/tinyxml.h"

/*
#if _DEBUG
#pragma comment(lib, "libcurld.lib") 
#else
#pragma comment(lib, "libcurl.lib") 
#endif
*/

#pragma comment(lib, "ws2_32.lib") 
#pragma comment(lib, "Wldap32.lib") 

namespace Zero
{

//------------------------------------------------------------------- Write Data
struct WriteData
{
  virtual void Start() {};
  virtual void End() {};

  virtual void Parse(const char* data, uint sizeOfChar, uint numOfElements)
  {
    if(mVerbose)
      printf("%s",data);
  }

  bool mVerbose;
};

//---------------------------------------------------------------- Buffered Data
struct BufferedData : public WriteData
{
  void Start() override{};
  void End() override{};

  void Parse(const char* data, uint sizeOfChar, uint numOfElements) override
  {
    WriteData::Parse(data,sizeOfChar,numOfElements);

    String str = String(data,numOfElements * sizeOfChar);
    mBuilder.Append(str);
  }

  Zero::StringBuilder mBuilder;
};

//----------------------------------------------------------------- Token Parser
struct TokenParser : public WriteData
{
  void Start() override
  {
    mValid = false;
  };

  void End() override {};

  void Parse(const char* data, uint sizeOfChar, uint numOfElements) override
  {
    WriteData::Parse(data,sizeOfChar,numOfElements);

    Zero::String str = Zero::String::Format("%s",data);
    if(str.size() < 8)
      return;

    if(str.sub_string(0,7) == Zero::String("<token>"))
    {
      mToken = str.sub_string(16,30);
      mValid = true;
    }
  }

  Zero::String mToken;
  bool mValid;
};

static size_t write_data(void *ptr, size_t sizeOfChar, size_t numOfElements, void *stream)
{
  //int written = fwrite(ptr, size, nmemb, (FILE *)stream);
  //return written;
  const char* data = (const char*)ptr;
  if(stream != NULL)
  {
    WriteData* writeData = static_cast<WriteData*>(stream);
    writeData->Parse(data,sizeOfChar,numOfElements);
  }

  return sizeOfChar*numOfElements;
}

void LogOn(String& token, bool verbose)
{
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();

  TokenParser tokenParser;
  tokenParser.mVerbose = verbose;

  String loginUrl = "http://zeroengine0.digipen.edu/api.asp?"
                    "cmd=logon&email=LevelDesigner2D&password=letmein";
  if(verbose)
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

  curl_easy_setopt(curl, CURLOPT_URL,loginUrl.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,&tokenParser);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  res = curl_easy_perform(curl);

  if(res != CURLE_OK)
    printf("Failed to log in.\n");

  if(tokenParser.mValid)
    token = tokenParser.mToken;
  else
    printf("Failed to parse a valid token after logging in.\n");

  /* always cleanup */ 
  curl_easy_cleanup(curl);
}

void LogOff(StringParam token, bool verbose)
{
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();

  String logoffUrl = Zero::BuildString("http://zeroengine0.digipen.edu/api.asp?"
                                       "cmd=logoff&token=", token.c_str());

  WriteData parser;
  parser.mVerbose = verbose;

  if(verbose)
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

  curl_easy_setopt(curl, CURLOPT_URL,logoffUrl.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,&parser);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  res = curl_easy_perform(curl);

  if(res != CURLE_OK)
    printf("Failed to log off.\n");

  /* always cleanup */ 
  curl_easy_cleanup(curl);
}

void GetWikiArticleIds(StringParam token, StringMap& wikiIndices, bool verbose)
{
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();

  String url = String::Format("http://zeroengine0.digipen.edu/api.asp?"
                              "cmd=listArticles&ixWiki=1&token=%s", 
                              token.c_str());

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  if(verbose)
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

  BufferedData parser;
  parser.mVerbose = false;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &parser);

  res = curl_easy_perform(curl);

  String str = parser.mBuilder.ToString();

  TiXmlDocument doc;
  doc.Parse(str.c_str());

  TiXmlElement* response = doc.FirstChildElement("response");
  TiXmlElement* articles = response->FirstChildElement("articles");

  TiXmlNode* article;
  for(article = articles->FirstChild(); article != 0; article = article->NextSibling())
  {
    TiXmlNode* wikiIdTag = article->FirstChildElement("ixWikiPage");
    TiXmlText* wikiIdData = (TiXmlText*)wikiIdTag->FirstChild();

    TiXmlNode* headlineTag = wikiIdTag->NextSiblingElement("sHeadline");
    TiXmlText* headlineData = (TiXmlText*)headlineTag->FirstChild();

    String id = wikiIdData->Value();
    String pageName = headlineData->Value();

    wikiIndices[pageName] = id;
  }

  /* always cleanup */ 
  curl_easy_cleanup(curl);
}

bool CreateWikiPage(StringParam token, StringParam pageName, 
                    StringParam parentPageIndex, StringMap& wikiIndices, bool verbose)
{
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();

  String url = String::Format("http://zeroengine0.digipen.edu/api.asp?"
                              "cmd=newArticle&ixWiki=1&sHeadline=%s&token=%s", 
                              pageName.c_str(), token.c_str());

  curl_easy_setopt(curl, CURLOPT_URL,url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  if(verbose)
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

  BufferedData parser;
  parser.mVerbose = false;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &parser);

  res = curl_easy_perform(curl);

  String str = parser.mBuilder.ToString();

  TiXmlDocument doc;
  doc.Parse(str.c_str());

  TiXmlElement* response = doc.FirstChildElement("response");
  if(response == NULL)
  {
    printf("Failed to find response tag for page %s\n",pageName.c_str());
    return false;
  }

  TiXmlElement* wikipage = response->FirstChildElement("wikipage");
  if(wikipage == NULL)
  {
    printf("Failed to find wikipage tag for page %s\n",pageName.c_str());
    return false;
  }
  TiXmlElement* wikiPageIndex = wikipage->FirstChildElement("ixWikiPage");
  if(wikiPageIndex == NULL)
  {
    printf("Failed to find ixWikiPage tag for page %s\n",pageName.c_str());
    return false;
  }

  TiXmlText* wikiIdData = (TiXmlText*)wikiPageIndex->FirstChild();
  if(wikiIdData == NULL)
  {
    printf("Failed to find wiki page id for new page %s\n",pageName.c_str());
    return false;
  }

  String id = wikiIdData->Value();
  wikiIndices[pageName] = id;

  /* always cleanup */ 
  curl_easy_cleanup(curl);

  return true;
}

void UploadClassDoc(StringParam index, ClassDoc& classDoc, Replacements& replacements, 
                    StringParam token, bool verbose)
{
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();

  String url = String::Format("http://zeroengine0.digipen.edu/api.asp?"
                              "cmd=editArticle&ixWikiPage=%s&token=%s", 
                              index.c_str(), token.c_str());

  curl_easy_setopt(curl, CURLOPT_URL,url.c_str());

  curl_httppost* post = NULL;
  curl_httppost* last = NULL;

  String doc = BuildDoc(classDoc, replacements);

  curl_formadd(&post, &last, CURLFORM_COPYNAME, "sHeadline", 
               CURLFORM_COPYCONTENTS, classDoc.Name.c_str(), CURLFORM_END);

  curl_formadd(&post, &last, CURLFORM_COPYNAME, "sBody", 
               CURLFORM_PTRCONTENTS, doc.c_str(),
               CURLFORM_CONTENTSLENGTH, doc.size(),
               CURLFORM_CONTENTTYPE, "text/html", CURLFORM_END);

  /* Set the form info */
  curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

  WriteData parser;
  parser.mVerbose = verbose;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &parser);

  if(verbose)
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

  res = curl_easy_perform(curl);

  if(res != CURLE_OK)
    printf("Failed to upload class doc %s.\n",classDoc.Name.c_str());

  /* always cleanup */ 
  curl_easy_cleanup(curl);
}

void UploadPageContent(StringParam pageIndex, StringParam pageTitle, 
                       StringParam pageContent, StringParam token, bool verbose)
{
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();

  String url = String::Format("http://zeroengine0.digipen.edu/api.asp?"
                              "cmd=editArticle&ixWikiPage=%s&token=%s", 
                              pageIndex.c_str(), token.c_str());

  curl_easy_setopt(curl, CURLOPT_URL,url.c_str());

  curl_httppost* post = NULL;
  curl_httppost* last = NULL;

  //String doc = BuildDoc(classDoc, replacements);


  curl_formadd(&post, &last, CURLFORM_COPYNAME, "sHeadline", 
               CURLFORM_COPYCONTENTS, pageTitle.c_str(), CURLFORM_END);

  curl_formadd(&post, &last, CURLFORM_COPYNAME, "sBody", 
               CURLFORM_PTRCONTENTS, pageContent.c_str(),
               CURLFORM_CONTENTSLENGTH, pageContent.size(),
               CURLFORM_CONTENTTYPE, "text/html", CURLFORM_END);

  /* Set the form info */
  curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

  WriteData parser;
  parser.mVerbose = verbose;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &parser);

  if(verbose)
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

  res = curl_easy_perform(curl);

  if(res != CURLE_OK)
    printf("Failed to upload class doc %s.\n", pageTitle.c_str());

  /* always cleanup */ 
  curl_easy_cleanup(curl);
}

}//namespace Zero

