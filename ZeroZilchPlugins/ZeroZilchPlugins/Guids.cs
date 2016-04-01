// Guids.cs
// MUST match guids.h
using System;

namespace DigiPenInstituteofTechnology.ZeroZilchPlugins
{
	static class GuidList
	{
		public const string guidZeroZilchPluginsPkgString = "672a28d0-2848-40fd-ade9-1b10239ad02a";
		public const string guidZeroZilchPluginsCmdSetString = "3dbbeaab-be66-4433-921e-04327d3c3dca";

		public static readonly Guid guidZeroZilchPluginsCmdSet = new Guid(guidZeroZilchPluginsCmdSetString);
	};
}
