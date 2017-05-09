using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;

namespace SharedBuildHelpers
{
  // Represents a property to output for the build machine/server
  public class BuildProperty
  {
    public String mType;
    public String mName;
    public String mValue;

    public BuildProperty() { }

    public BuildProperty(String type, String name, String value)
    {
      mType = type;
      mName = name;
      mValue = value;
    }
  }

  // Represents meta information for the build needed for the build server
  public class BuildMeta
  {
    public Dictionary<String, BuildProperty> mProperties = new Dictionary<String, BuildProperty>();

    public BuildMeta()
    {
    }

    public void SetupLauncher()
    {
      mProperties.Add("LauncherMajorVersion", new BuildProperty("int", "LauncherMajorVersion", "1"));
      SetupCommon();
    }

    public void SetupEditor()
    {
      mProperties.Add("MajorVersion", new BuildProperty("int", "MajorVersion", "0"));
      SetupCommon();
    }

    void SetupCommon()
    {
      // Populate a whole bunch of properties with default values. We only update properties if it already exists here
      mProperties.Add("MinorVersion", new BuildProperty("int", "MinorVersion", "0"));
      mProperties.Add("PatchVersion", new BuildProperty("int", "PatchVersion", "0"));
      mProperties.Add("RevisionId", new BuildProperty("int", "RevisionId", "0"));
      mProperties.Add("ExperimentalBranchName", new BuildProperty("string", "ExperimentalBranchName", ""));
      mProperties.Add("BuildId", new BuildProperty("string", "BuildId", "0.0.0.0"));
      mProperties.Add("ShortChangeSet", new BuildProperty("string", "ShortChangeSet", "0"));
      mProperties.Add("ChangeSet", new BuildProperty("string", "ChangeSet", "\"0\""));
      mProperties.Add("ChangeSetDate", new BuildProperty("string", "ChangeSetDate", "\"0-0-0\""));
      mProperties.Add("Platform", new BuildProperty("string", "Platform", "\"Win32\""));
    }

    public bool GetBuildInfoFromExe(String exePath, String sourcePath, int timoutInSeconds = 10)
    {
      // Run the engine with the command "WriteBuildInfo" to have it output all build meta information. 
      // Also pass it -newProject to force it to not open a project from the recent project list to help it load faster.
      var tempFile = Path.GetTempFileName();
      ProcessStartInfo startInfo = new ProcessStartInfo();

      startInfo.Arguments = String.Format("-WriteBuildInfo {0} -newProject", tempFile);
      startInfo.FileName = exePath;

      String exeName = Path.GetFileNameWithoutExtension(exePath);
      Console.WriteLine("Invoking " + exeName + " at: " + startInfo.FileName);
      Console.WriteLine("With arguments: " + startInfo.Arguments);

      var process = Process.Start(startInfo);

      // Old versions of the engine won't understand this argument, so wait a
      // max amount of time and then kill the process
      process.WaitForExit(timoutInSeconds * 1000);
      if (!process.HasExited)
      {
        try { process.Kill(); }
        catch (Exception) { }
      }

      var result = true;
      // Try to read all lines from the output file.
      var metaData = File.ReadAllText(tempFile);
      if (metaData.Length == 0)
      {
        // If the file doesn't exist then parse information for the revision from mercurial
        // into the a string that has the same format as what zero should output, this way
        // we can run the same update logic as normal.
        metaData = Mercurial.GetRevisionInfo(sourcePath);

        Console.WriteLine("BuildInfo file did not exist. Falling back to build information from mercurial.");
        result = false;
      }

      Parse(metaData);
      return result;
    }
    
    public void Parse(String data)
    {
      var lines = data.Split(new string[] { "\n" }, StringSplitOptions.RemoveEmptyEntries);

      // Load each line as a "property"
      var splitTokens = new string[] { " " };
      foreach (var line in lines)
      {
        // A property should be a name-value pair separated by a space (spaces in strings is not supported for now)
        var entries = line.Split(splitTokens, StringSplitOptions.RemoveEmptyEntries);
        if (entries.Length != 2)
          continue;

        var propertyName = entries[0];
        var propertyValue = entries[1];
        // Update the property if it existed
        if (mProperties.ContainsKey(propertyName))
          mProperties[propertyName].mValue = propertyValue;
      }
    }

    public String GetProperty(String name, String defaultValue)
    {
      if (!mProperties.ContainsKey(name))
        return defaultValue;
      return mProperties[name].mValue;
    }

    public String GetProperty(String name)
    {
      return GetProperty(name, "");
    }

    public String StripQuotes(String str)
    {
      return str.Trim('\"');
    }

    public String GetBuildId()
    {
      var str = GetProperty("BuildId");
      return StripQuotes(str);
    }

    public String GetBuildNameWithoutExtension(String rootName)
    {
      return rootName + "." + GetBuildId();
    }

    public void SaveMeta(String filePath)
    {
      var builder = new StringBuilder();
      foreach (var property in mProperties.Values)
      {
        // Skip experimental branch if it's an emtpy string
        if (property.mName == "ExperimentalBranchName" && property.mValue.Length == 0)
          continue;

        String propStr = String.Format("{0} {1} = {2},", property.mType, property.mName, property.mValue);
        builder.AppendLine(propStr);
      }
      File.WriteAllText(filePath, builder.ToString());
    }
  }
}
