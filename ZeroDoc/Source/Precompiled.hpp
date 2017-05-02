//#include "Common/Utility/Standard.hpp"
//#include "Common/CommonStandard.hpp"
//#include "Support/SupportStandard.hpp"
//#include "Math/MathStandard.hpp"
//#include "Geometry/GeometryStandard.hpp"
//#include "Meta/MetaStandard.hpp"
//#include "Content/ContentStandard.hpp"
//#include "Platform/PlatformStandard.hpp"
#include "Zilch/Project/Zilch/Zilch.hpp"
#include "Engine/EngineStandard.hpp"
#include "Serialization/SerializationStandard.hpp"
#include "Common/Utility/Standard.hpp"


#pragma message("Including Windows...")


#ifdef _WINDOWS_
#error Windows has already been included!
#endif

//Prevent including winsock1.
#define _WINSOCKAPI_

//Only include frequently used elements.
#define WIN32_LEAN_AND_MEAN

//Minimum version is Windows XP.
#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

//Prevent MIN ans MAX macros from being defined.
#define NOMINMAX

#include <windows.h>


#include <cmath>
#include <vector>
#include <list>
#include <map>

#include <algorithm>
#include <string>
#include <fstream>

#include <WinSock2.h>

//#include <unordered_map>
//#include <hash_map>


