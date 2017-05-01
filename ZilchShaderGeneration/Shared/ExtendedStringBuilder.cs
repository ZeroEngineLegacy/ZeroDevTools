using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Shared
{
  public class ExtendedStringBuilder
  {
    public int mScopes;



    String mIndentation;

    public StringBuilder mBuilder = new StringBuilder();

    public ExtendedStringBuilder()
    {
      mScopes = 0;
      mIndentation = "  ";
    }

    public void AppendLine(String str)
    {
      for (var i = 0; i < mScopes; ++i)
        mBuilder.Append(mIndentation);
      mBuilder.AppendLine(str);
    }

    public void AppendLine()
    {
      mBuilder.AppendLine();
    }

    public String ToString()
    {
      return mBuilder.ToString();
    }

    public void PushScope()
    {
      ++mScopes;
    }

    public void PopScope()
    {
      --mScopes;
    }

  }
}
