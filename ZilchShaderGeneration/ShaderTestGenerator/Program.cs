using Shared;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ShaderTestGenerator
{
  class Program
  {

    static bool EvolvePermutation(List<int> indices, int count, int upperBound)
    {
      // Increment the first index
      ++indices[0];

      // Now we need to check if we need to overflow into the next slot
      int innerIndex = 0;

      // Loop while we're overflowing and our index hasn't gone outside the 
      while (indices[innerIndex] == upperBound)
      {
        // If we hit the last index, and it's at the upper bound (confirmed above)
        if (innerIndex == (count - 1))
          return false;

        // Reset the current counter
        indices[innerIndex] = 0;

        // Increment the index so we can increment the next value (overflow)
        ++innerIndex;
        ++indices[innerIndex];
      }

      // We need to continue the permutation!
      return true;
    }


    static void GenerateVectorSwizzles(ExtendedStringBuilder builder, String baseType, int vectorDim)
    {
      List<String> componentNames = new List<String>() {"X", "Y", "Z", "W"};
      List<String> vectorTypeNames = new List<String>();
      vectorTypeNames.Add(baseType);
      vectorTypeNames.Add(String.Format("{0}2", baseType));
      vectorTypeNames.Add(String.Format("{0}3", baseType));
      vectorTypeNames.Add(String.Format("{0}4", baseType));

      builder.AppendLine(String.Format("function Test{0}()", vectorTypeNames[vectorDim - 1]));
      builder.AppendLine("{");
      builder.PushScope();

      var maxCount = 4;

      builder.AppendLine(String.Format("var v = {0}();", vectorTypeNames[vectorDim - 1]));
      List<String> vectorNames = new List<String>();
      for (var i = 1; i <= maxCount; ++i)
      {
        String varName = String.Format("v{0}", i);
        vectorNames.Add(varName);

        builder.AppendLine(String.Format("var v{0} = {1}();", i, vectorTypeNames[i - 1]));
      }
      builder.AppendLine();

      for (var count = 1; count <= maxCount; ++count)
      {
        List<int> indices = new List<int>() { 0, 0, 0, 0 };

        do
        {
          StringBuilder nameBuilder = new StringBuilder();

          for (int i = 1; i <= count; ++i)
          {
            var index = indices[i - 1];
            nameBuilder.Append(componentNames[index]);
          }

          String name = nameBuilder.ToString();
          builder.AppendLine(String.Format("{0} = v.{1};", vectorNames[count - 1], name));
        } while (EvolvePermutation(indices, count, vectorDim));
      }
      
      
      builder.PopScope();
      builder.AppendLine("}");
      builder.AppendLine();
    }

    

    static void GenerateVectorSwizzlesForType(ExtendedStringBuilder builder, String baseType)
    {
      builder.AppendLine("[Pixel]");
      builder.AppendLine(String.Format("struct {0}VectorSwizzles", baseType));
      builder.AppendLine("{");
      builder.PushScope();

      for (var i = 1; i <= 4; ++i)
      {
        GenerateVectorSwizzles(builder, baseType, i);
      }

      builder.PopScope();
      builder.AppendLine("}");
      builder.AppendLine();
    }

    static void GenerateMatrixSwizzles(ExtendedStringBuilder builder, String baseType, int yDim, int xDim)
    {
      String matrixTypeName = String.Format("{0}{1}x{2}", baseType, yDim, xDim);
      builder.AppendLine(String.Format("function Test{0}()", matrixTypeName));
      builder.AppendLine("{");
      builder.PushScope();

      builder.AppendLine(String.Format("var m = {0}();", matrixTypeName));
      builder.AppendLine(String.Format("var s = {0}();", baseType));
      //ExtendedStringBuilder test = new ExtendedStringBuilder();
      
      for (var y = 1; y <= yDim; ++y)
      {
        for (var x = 1; x <= xDim; ++x)
        {
          var member = String.Format("s = m.M{0}{1};", y - 1, x - 1);
          builder.AppendLine(member);
        }
      }

      builder.PopScope();
      builder.AppendLine("}");
      builder.AppendLine();
    }

    static void GenerateMatrixSwizzlesForType(ExtendedStringBuilder builder, String baseType)
    {
      var maxDim = 4;
      builder.AppendLine("[Pixel]");
      builder.AppendLine(String.Format("struct {0}MatrixSwizzles", baseType));
      builder.AppendLine("{");
      builder.PushScope();

      for (var y = 2; y <= maxDim; ++y)
      {
        for (var x = 2; x <= maxDim; ++x)
        {
          GenerateMatrixSwizzles(builder, baseType, y, x);
        }
      }

      builder.PopScope();
      builder.AppendLine("}");
      builder.AppendLine();
    }

    [STAThread]
    static void Main(string[] args)
    {
      ExtendedStringBuilder builder = new ExtendedStringBuilder();

      GenerateVectorSwizzlesForType(builder, "Real");
      GenerateVectorSwizzlesForType(builder, "Integer");
      GenerateVectorSwizzlesForType(builder, "Boolean");

      GenerateMatrixSwizzlesForType(builder, "Real");

      String result = builder.ToString();
      Clipboard.SetText(result);
    }
  }
}
