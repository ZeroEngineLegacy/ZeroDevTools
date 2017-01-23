using Shared;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ShaderExtensionGeneration
{
  class Program
  {
    static void GenerateLengthSq(ExtendedStringBuilder results, String type)
    {
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function LengthSq(value : {0}) : Real", type));
      results.AppendLine("{");
      results.AppendLine("  return Math.Dot(value, value);");
      results.AppendLine("}");
    }

    static void GenerateDistanceSq(ExtendedStringBuilder results, String type)
    {
      String varName = type.ToLower();
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function DistanceSq({0}0 : {1}, {0}1 : {1}) : Real", varName, type));
      results.AppendLine("{");
      results.AppendLine(String.Format("  var vector = {0}0 - {0}1;", varName));
      results.AppendLine("  return Math.LengthSq(vector);");
      results.AppendLine("}");
    }

    static void GenerateReflectAcrossVector(ExtendedStringBuilder results, String type)
    {
      String varName = type.ToLower();
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function ReflectAcrossVector(toBeReflected : {0}, vector : {0}) : {0}", type));
      results.AppendLine("{");
      results.AppendLine("  return Math.ReflectAcrossPlane(-toBeReflected, vector);");
      results.AppendLine("}");
    }

    static void GenerateProjectOnVector(ExtendedStringBuilder results, String type)
    {
      String varName = type.ToLower();
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function ProjectOnVector(toBeProjected : {0}, normalizedVector : {0}) : {0}", type));
      results.AppendLine("{");
      results.AppendLine("  return normalizedVector * Math.Dot(toBeProjected, normalizedVector);");
      results.AppendLine("}");
    }

    static void GenerateProjectOnPlane(ExtendedStringBuilder results, String type)
    {
      String varName = type.ToLower();
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function ProjectOnPlane(toBeProjected : {0}, planeNormal : {0}) : {0}", type));
      results.AppendLine("{");
      results.AppendLine("  return toBeProjected - Math.ProjectOnVector(toBeProjected, planeNormal);");
      results.AppendLine("}");
    }

    static void GenerateGetAxis(ExtendedStringBuilder results, String type)
    {
      String varName = type.ToLower();
      results.AppendLine(String.Format("[Static][Extension(typeid({0}))][Implements]", type));
      results.AppendLine(String.Format("function GetAxis(value : Integer) : {0}", type));
      results.AppendLine("{");
      results.AppendLine(String.Format("  var axis = {0}();", type));
      results.AppendLine("  axis[value] = 1;");
      results.AppendLine("  return axis;");
      results.AppendLine("}");
    }

    static void GenerateSaturate(ExtendedStringBuilder results, String type)
    {
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function Saturate(value : {0}) : {0}", type));
      results.AppendLine("{");
      results.AppendLine(String.Format("  return Math.Clamp(value, {0}(0), {0}(1));", type));
      results.AppendLine("}");
    }

    static void GenerateCeilPlaces(ExtendedStringBuilder results, String type)
    {
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function Ceil(value : {0}, places : Integer) : {0}", type));
      results.AppendLine("{");
      results.AppendLine(String.Format("  return Math.Ceil(value, places, 10);", type));
      results.AppendLine("}");
    }

    static void GenerateCeilPlacesBase(ExtendedStringBuilder results, String type)
    {
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function Ceil(value : {0}, places : Integer, numericalBase : Integer) : {0}", type));
      results.AppendLine("{");
      results.AppendLine("  var scale = Math.Pow(numericalBase, places);");
      results.AppendLine("  return Math.Ceil(value / scale) * scale;");
      results.AppendLine("}");
    }

    static void GenerateFloorPlaces(ExtendedStringBuilder results, String type)
    {
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function Floor(value : {0}, places : Integer) : {0}", type));
      results.AppendLine("{");
      results.AppendLine(String.Format("  return Math.Floor(value, places, 10);", type));
      results.AppendLine("}");
    }

    static void GenerateFloorPlacesBase(ExtendedStringBuilder results, String type)
    {
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function Floor(value : {0}, places : Integer, numericalBase : Integer) : {0}", type));
      results.AppendLine("{");
      results.AppendLine("  var scale = Math.Pow(numericalBase, places);");
      results.AppendLine("  return Math.Floor(value / scale) * scale;");
      results.AppendLine("}");
    }

    static void GenerateRoundPlaces(ExtendedStringBuilder results, String type)
    {
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function Round(value : {0}, places : Integer) : {0}", type));
      results.AppendLine("{");
      results.AppendLine(String.Format("  return Math.Round(value, places, 10);", type));
      results.AppendLine("}");
    }

    static void GenerateRoundPlacesBase(ExtendedStringBuilder results, String type)
    {
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function Round(value : {0}, places : Integer, numericalBase : Integer) : {0}", type));
      results.AppendLine("{");
      results.AppendLine("  var scale = Math.Pow(numericalBase, places);");
      results.AppendLine("  return Math.Round(value / scale) * scale;");
      results.AppendLine("}");
    }

    static void GenerateTruncatePlaces(ExtendedStringBuilder results, String type)
    {
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function Truncate(value : {0}, places : Integer) : {0}", type));
      results.AppendLine("{");
      results.AppendLine(String.Format("  return Math.Truncate(value, places, 10);", type));
      results.AppendLine("}");
    }

    static void GenerateTruncatePlacesBase(ExtendedStringBuilder results, String type)
    {
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function Truncate(value : {0}, places : Integer, numericalBase : Integer) : {0}", type));
      results.AppendLine("{");
      results.AppendLine("  var scale = Math.Pow(numericalBase, places);");
      results.AppendLine("  return Math.Truncate(value / scale) * scale;");
      results.AppendLine("}");
    }

    static void GenerateFMod(ExtendedStringBuilder results, String type)
    {
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function FMod(numerator : {0}, denominator : {0}) : {0}", type));
      results.AppendLine("{");
      results.AppendLine("  return numerator - denominator * Math.Truncate(numerator / denominator);");
      results.AppendLine("}");
    }

    static void GenerateAngleBetween(ExtendedStringBuilder results, String type)
    {
      String varName = type.ToLower();
      String var0Name = varName + "0";
      String var1Name = varName + "1";
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function AngleBetween({0} : {2}, {1} : {2}) : Real", var0Name, var1Name, type));
      results.AppendLine("{");
      results.AppendLine(String.Format("  var dotVal = Math.Dot({0}, {1});", var0Name, var1Name));
      results.AppendLine("  dotVal = Math.Clamp(dotVal, -1.0, 1.0);");
      results.AppendLine("  return Math.ACos(dotVal);");
      results.AppendLine("}");
    }

    static void GenerateRotateTowards(ExtendedStringBuilder results, String type)
    {
      String varName = type.ToLower();
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function RotateTowards(p0 : {0}, p1 : {0}, maxRadians : Real) : {0}", type));
      results.AppendLine("{");
      results.AppendLine(String.Format("  return MathGenericRotateTowards[{0}].RotateTowards(p0, p1, maxRadians);", type));
      results.AppendLine("}");
    }
    
    static void GenerateLog10(ExtendedStringBuilder results, String type)
    {
      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function Log10(value : {0}) : {0}", type));
      results.AppendLine("{");
      results.AppendLine(String.Format("  return Math.Log(value) / Math.Log(10);", type));
      results.AppendLine("}");
    }

    static void GenerateGetByIndex(ExtendedStringBuilder results, String type)
    {
      results.AppendLine(String.Format("[Extension(typeid({0}))][Implements]", type));
      results.AppendLine("function GetByIndex(index : Integer) : Real");
      results.AppendLine("{");
      results.AppendLine("  var indexX = index % this.CountX;");
      results.AppendLine("  var indexY = index / this.CountX;");
      results.AppendLine("  return this[indexY][indexX];");
      results.AppendLine("}");
    }

    static void GenerateSetByIndex(ExtendedStringBuilder results, String type)
    {
      results.AppendLine(String.Format("[Extension(typeid({0}))][Implements]", type));
      results.AppendLine("function SetByIndex(index : Integer, value : Real)");
      results.AppendLine("{");
      results.AppendLine("  var indexX = index % this.CountX;");
      results.AppendLine("  var indexY = index / this.CountX;");
      results.AppendLine("  this[indexY][indexX] = value;");
      results.AppendLine("}");
    }

    static void GenerateMultiplyUpNoDivide(ExtendedStringBuilder results, String fnName, String matrixType, String scalarType, int sizeX, int sizeY, String promotionValue)
    {
      String inputVectorType = String.Format("{0}{1}", scalarType, sizeY - 1);
      if (sizeY - 1 == 1)
        inputVectorType = scalarType;

      String multiplyVectorType = String.Format("{0}{1}", scalarType, sizeY);
      String memberAccessStr = "X";
      if (sizeY > 2)
        memberAccessStr += "Y";
      if (sizeY > 3)
        memberAccessStr += "Z";
      if (sizeY > 4)
        memberAccessStr += "W";

      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function {0}(by : {1}, the : {2}) : {2}", fnName, matrixType, inputVectorType));
      results.AppendLine("{");
      results.AppendLine(String.Format("  var promotedVector = {0}(the, {1});", multiplyVectorType, promotionValue));
      results.AppendLine(String.Format("  return Math.Multiply(by, promotedVector).{0};", memberAccessStr));
      results.AppendLine("}");
    }

    static void GenerateMultiplyNormal(ExtendedStringBuilder results, String matrixType, String scalarType, int sizeX, int sizeY)
    {
      GenerateMultiplyUpNoDivide(results, "MultiplyNormal", matrixType, scalarType, sizeX, sizeY, "0");
    }

    static void GenerateMultiplyPointNoDivision(ExtendedStringBuilder results, String matrixType, String scalarType, int sizeX, int sizeY)
    {
      results.AppendLine("// Multiplies the given vector as a point without performing the homogeneous division");
      GenerateMultiplyUpNoDivide(results, "MultiplyPointNoDivide", matrixType, scalarType, sizeX, sizeY, "1");
    }

    static void GenerateMultiplyUpWithDivision(ExtendedStringBuilder results, String fnName, String matrixType, String scalarType, int sizeX, int sizeY, String promotionValue)
    {
      String inputVectorType = String.Format("{0}{1}", scalarType, sizeY - 1);
      if (sizeY - 1 == 1)
        inputVectorType = scalarType;

      String multiplyVectorType = String.Format("{0}{1}", scalarType, sizeY);
      var memberAccessStrings = new List<string>() { "X", "Y", "Z", "W" };
      String memberAccessStr = "";
      for (var i = 0; i < sizeY - 1; ++i)
        memberAccessStr += memberAccessStrings[i];

      results.AppendLine("[Static][Extension(typeid(Math))][Implements]");
      results.AppendLine(String.Format("function {0}(by : {1}, the : {2}) : {2}", fnName, matrixType, inputVectorType));
      results.AppendLine("{");
      results.AppendLine(String.Format("  var promotedVector = {0}(the, {1});", multiplyVectorType, promotionValue));
      results.AppendLine(String.Format("  var transformedVector = Math.Multiply(by, promotedVector);"));
      results.AppendLine(String.Format("  var result = transformedVector.{0} / transformedVector.{1};", memberAccessStr, memberAccessStrings[sizeY - 1]));
      results.AppendLine("  return result;");
      results.AppendLine("}");
    }

    static void GenerateMultiplyPoint(ExtendedStringBuilder results, String matrixType, String scalarType, int sizeX, int sizeY)
    {
      results.AppendLine("// Multiplies the given vector as a point while performing the homogeneous division");
      GenerateMultiplyUpWithDivision(results, "MultiplyPoint", matrixType, scalarType, sizeX, sizeY, "1");
    }

    delegate void FnCall(ExtendedStringBuilder results, String type);
    delegate void MatrixFnCall(ExtendedStringBuilder results, String matrixType, String scalarType, int sizeX, int sizeY);

    static void GenerateClass(ExtendedStringBuilder results, String className, List<String> types, FnCall fnCall)
    {
      results.AppendLine("[Implements]");
      results.AppendLine(String.Format("struct {0}", className));
      results.AppendLine("{");

      results.PushScope();
      foreach (var type in types)
        fnCall(results, type);
      results.PopScope();

      results.AppendLine("}");
    }

    static void GenerateSquareMatrixClass(ExtendedStringBuilder results, String className, MatrixFnCall fnCall)
    {
      results.AppendLine("[Implements]");
      results.AppendLine(String.Format("struct {0}", className));
      results.AppendLine("{");

      results.PushScope();
      fnCall(results, "Real2x2", "Real", 2, 2);
      fnCall(results, "Real3x3", "Real", 3, 3);
      fnCall(results, "Real4x4", "Real", 4, 4);
      results.PopScope();

      results.AppendLine("}");
    }

    static void AddCodeFromFile(ExtendedStringBuilder results, String filePath)
    {
      results.AppendLine(File.ReadAllText(filePath));
    }

    [STAThread]
    static void Main(string[] args)
    {
      ExtendedStringBuilder builder = new ExtendedStringBuilder();


      List<String> realVectorTypes = new List<String>();
      for (var i = 2; i <= 4; ++i)
        realVectorTypes.Add(String.Format("Real{0}", i));
      List<String> real23VectorTypes = new List<String>();
      real23VectorTypes.Add(realVectorTypes[0]);
      real23VectorTypes.Add(realVectorTypes[1]);

      List<String> realMatrixTypes = new List<String>();
      for (var y = 2; y <= 4; ++y)
      {
        for (var x = 2; x <= 4; ++x)
        {
          realMatrixTypes.Add(String.Format("Real{0}x{1}", y, x));
        }
      }
      List<String> realVectorAndScalarTypes = new List<String>();
      realVectorAndScalarTypes.Add("Real");
      realVectorAndScalarTypes.AddRange(realVectorTypes);
      List<String> realVectorAndMatrixTypes = new List<String>();
      realVectorAndMatrixTypes.AddRange(realVectorTypes);
      realVectorAndMatrixTypes.AddRange(realMatrixTypes);
      List<String> allRealTypes = new List<String>();
      allRealTypes.Add("Real");
      allRealTypes.AddRange(realVectorAndMatrixTypes);
        
      

      GenerateClass(builder, "MathLengthSq", realVectorTypes, GenerateLengthSq);
      builder.AppendLine();
      GenerateClass(builder, "MathDistanceSq", realVectorTypes, GenerateDistanceSq);
      builder.AppendLine();
      GenerateClass(builder, "MathReflectAcrossVector", realVectorTypes, GenerateReflectAcrossVector);
      builder.AppendLine();
      GenerateClass(builder, "MathProjectOnVector", realVectorTypes, GenerateProjectOnVector);
      builder.AppendLine();
      GenerateClass(builder, "MathProjectOnPlane", realVectorTypes, GenerateProjectOnPlane);
      builder.AppendLine();
      GenerateClass(builder, "MathAngleBetween", realVectorTypes, GenerateAngleBetween);
      builder.AppendLine();
      GenerateClass(builder, "MathRotateTowards", real23VectorTypes, GenerateRotateTowards);
      builder.AppendLine();
      GenerateClass(builder, "MathGetAxis", realVectorTypes, GenerateGetAxis);
      builder.AppendLine();
      GenerateClass(builder, "MathSaturate", realVectorAndScalarTypes, GenerateSaturate);
      builder.AppendLine();

      GenerateClass(builder, "MathCeilPlaces", realVectorAndScalarTypes, GenerateCeilPlaces);
      builder.AppendLine();
      GenerateClass(builder, "MathCeilPlacesBase", realVectorAndScalarTypes, GenerateCeilPlacesBase);
      builder.AppendLine();
      GenerateClass(builder, "MathFloorPlaces", realVectorAndScalarTypes, GenerateFloorPlaces);
      builder.AppendLine();
      GenerateClass(builder, "MathFloorPlacesBase", realVectorAndScalarTypes, GenerateFloorPlacesBase);
      builder.AppendLine();
      GenerateClass(builder, "MathRoundPlaces", realVectorAndScalarTypes, GenerateRoundPlaces);
      builder.AppendLine();
      GenerateClass(builder, "MathRoundPlacesBase", realVectorAndScalarTypes, GenerateRoundPlacesBase);
      builder.AppendLine();
      GenerateClass(builder, "MathTruncatePlaces", realVectorAndScalarTypes, GenerateTruncatePlaces);
      builder.AppendLine();
      GenerateClass(builder, "MathTruncatePlacesBase", realVectorAndScalarTypes, GenerateTruncatePlacesBase);
      builder.AppendLine();

      GenerateClass(builder, "MathFMod", realVectorAndScalarTypes, GenerateFMod);
      builder.AppendLine();
      GenerateClass(builder, "MathLog10", realVectorAndScalarTypes, GenerateLog10);
      builder.AppendLine();
      GenerateClass(builder, "MathMatrixGetByIndex", realMatrixTypes, GenerateGetByIndex);
      builder.AppendLine();
      GenerateClass(builder, "MathMatrixSetByIndex", realMatrixTypes, GenerateSetByIndex);
      builder.AppendLine();
      GenerateSquareMatrixClass(builder, "MathMultiplyPointNoDivide", GenerateMultiplyPointNoDivision);
      builder.AppendLine();
      GenerateSquareMatrixClass(builder, "MathMultiplyNormal", GenerateMultiplyNormal);
      builder.AppendLine();
      GenerateSquareMatrixClass(builder, "MathMultiplyPoint", GenerateMultiplyPoint);

      AddCodeFromFile(builder, "ExtraCode.zilchFrag");

      String result = builder.ToString();
      Clipboard.SetText(result);
    }
  }
}
