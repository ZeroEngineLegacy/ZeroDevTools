using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using FogLampz;
using System.Net;
using System.IO;
using System.Xml.Linq;
using FogLampz.Model;
using System.Xml;

namespace BuildMaker
{
  class ReleaseNoteBuilder
  {
    public void Run()
    {
      GetLoginToken();
      UpdateTheInstallPage();
      GetFogBugzCases();
      Console.ReadKey();
    }

    bool LoginToFogBugz(bool useMyCredentials)
    {
      //Use the LevelDesigner2D account.
      FogBugzClient.LogOn("http://zeroengine0.digipen.edu/api.asp",
                          "LevelDesigner2D", "letmein");
      Console.WriteLine("Logged in!");

      return true;
    }

    void GetFogBugzCases()
    {
      //Login as LevelDesigner2D to get a list of cases. This account has the 
      //"every case resolved in the last 2 months" filter.
      LoginToFogBugz(false);

      IEnumerable<Filter> filters = FogBugzClient.GetFilters();

      //Find the ID of the "Release Notes" filter so we can get all the cases
      //that matter to us.
      //Filter releaseNotes = filters.FirstOrDefault(filter => filter.Name == "Release Notes");
      Filter testFilter = filters.FirstOrDefault(filter => filter.Name == "TestFilter");
      FogBugzClient.SetFilter(testFilter.Content);

      IEnumerable<Case> cases = FogBugzClient.GetAllCases();
    }

    void GetLoginToken()
    {
      String loginUrl = "http://zeroengine0.digipen.edu/api.asp?cmd=logon&email=LevelDesigner2D&password=letmein";
      XDocument doc = XmlFromUrl(loginUrl);

      //Get the token value from the new document.
      mLoginToken = doc.Root.Element("token").Value;
      Console.WriteLine("Login token is: " + mLoginToken);
    }

    void UpdateTheInstallPage()
    {
      //const String pageToFind = "TestArticle";
      const String pageToFind = "Installer";
      String installUrl = FindTheInstallerPage(pageToFind);
      XDocument installerPage = XmlFromUrl(installUrl);
      XElement pageBody = installerPage.Element("response")
                                       .Element("wikipage")
                                       .Element("sBody");
      if (pageBody == null)
      {
        Console.WriteLine("wikipage could not be found.");
      }
      String pageBodyContent = pageBody.Value;
      pageBodyContent.First()
      Console.WriteLine(pageBody.Value);
      //Console.WriteLine(installerPage.FirstNode.ToString());
      installerPage.Save("temp.xml");

    }

    String FindTheInsidePage()
    {
      //Get the first wiki page on the fogbugz wiki.
      Console.WriteLine("Get the main wiki page...");
      String wikiUrl = "http://zeroengine0.digipen.edu/api.asp?cmd=listWikis&token=";
      wikiUrl += mLoginToken;
      XDocument mainPage = XmlFromUrl(wikiUrl);

      //Find the "Inside" page as this contains the installer page.
      XElement page = (from p in mainPage.Descendants("wiki")
                       where p.Element("sWiki").Value == "Inside"
                       select p).FirstOrDefault();
      if (page == null)
      {
        Console.WriteLine("\"Inside\" page could not be found.");
        return null;
      }

      //Get the ID of the "Inside" page for further use.
      String insideId = page.Element("ixWiki").Value;

      Console.WriteLine(insideId);

      String insideUrl = "http://zeroengine0.digipen.edu/api.asp?cmd=listArticles&ixWiki=";
      insideUrl += insideId + "&token=" + mLoginToken;
      return insideUrl;
    }

    String FindTheInstallerPage(String pageTitle)
    {
      String insideUrl = FindTheInsidePage();
      XDocument insidePage = XmlFromUrl(insideUrl);

      //Find the "Install" page for further use.
      XElement page = (from p in insidePage.Descendants("article")
                       where p.Element("sHeadline").Value == pageTitle
                       select p).FirstOrDefault();

      String installerId = page.Element("ixWikiPage").Value;

      String installerUrl = "http://zeroengine0.digipen.edu/api.asp?cmd=viewArticle&ixWikiPage=";
      installerUrl += installerId + "&token=" + mLoginToken;

      return installerUrl;
    }

    XDocument XmlFromUrl(String url)
    {
      WebRequest urlPage = WebRequest.Create(url);
      
      //Get the response from the server.
      Console.WriteLine("Get response from server...");
      HttpWebResponse response = (HttpWebResponse)urlPage.GetResponse();

      //Get the stream containing content returned by the server.
      Console.WriteLine("Get stream with server response...");
      Stream dataStream = response.GetResponseStream();

      //Open the stream using a StreamReader for easy access.
      Console.WriteLine("Open the stream...");
      StreamReader reader = new StreamReader(dataStream);

      //Read the content.
      Console.WriteLine("Read the content...");
      String responseFromServer = reader.ReadToEnd();

      //Save off the response from the server into an XML object.
      XDocument xmlPage = XDocument.Parse(responseFromServer);

      reader.Close();
      dataStream.Close();
      response.Close();

      return xmlPage;
    }

    String mLoginToken;
  }
}
