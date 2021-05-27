// ArchiveCommandLine.cpp

#include "StdAfx.h"

#ifdef _WIN32
#include <io.h>
#endif
#include <stdio.h>

#include "Common/ListFileUtils.h"
#include "Common/StringConvert.h"
#include "Common/StringToInt.h"

#include "Windows/FileName.h"
#include "Windows/FileDir.h"
#ifdef _WIN32
#include "Windows/FileMapping.h"
#include "Windows/Synchronization.h"
#endif

#include "ArchiveCommandLine.h"
#include "UpdateAction.h"
#include "Update.h"
#include "SortUtils.h"
#include "EnumDirItems.h"

extern bool g_CaseSensitive;

#if _MSC_VER >= 1400
#define MY_isatty_fileno(x) _isatty(_fileno(x))
#else
#define MY_isatty_fileno(x) isatty(fileno(x))
#endif

#define MY_IS_TERMINAL(x) (MY_isatty_fileno(x) != 0);

using namespace NCommandLineParser;
using namespace NWindows;
using namespace NFile;

namespace NKey {
enum Enum
{
  kHelp1 = 0,
  kHelp2,
  kHelp3,
  kDisableHeaders,
  kDisablePercents,
  kArchiveType,
  kYes,
  #ifndef _NO_CRYPTO
  kPassword,
  #endif
  kProperty,
  kOutputDir,
  kWorkingDir,
  kInclude,
  kExclude,
  kArInclude,
  kArExclude,
  kNoArName,
  kUpdate,
  kVolume,
  kRecursed,
  kSfx,
  kStdIn,
  kStdOut,
  kOverwrite,
  kEmail,
  kShowDialog,
  kLargePages,
  kCharSet,
  kTechMode,
  kShareForWrite,
  kCaseSensitive
};

}


static const wchar_t kRecursedIDChar = 'R';
static const wchar_t *kRecursedPostCharSet = L"0-";

namespace NRecursedPostCharIndex {
  enum EEnum
  {
    kWildCardRecursionOnly = 0,
    kNoRecursion = 1
  };
}

static const char kImmediateNameID = '!';
static const char kMapNameID = '#';
static const char kFileListID = '@';

static const char kSomeCludePostStringMinSize = 2; // at least <@|!><N>ame must be
static const char kSomeCludeAfterRecursedPostStringMinSize = 2; // at least <@|!><N>ame must be

static const wchar_t *kOverwritePostCharSet = L"asut";

NExtract::NOverwriteMode::EEnum k_OverwriteModes[] =
{
  NExtract::NOverwriteMode::kWithoutPrompt,
  NExtract::NOverwriteMode::kSkipExisting,
  NExtract::NOverwriteMode::kAutoRename,
  NExtract::NOverwriteMode::kAutoRenameExisting
};

static const CSwitchForm kSwitchForms[] =
  {
    { L"?",  NSwitchType::kSimple, false },
    { L"H",  NSwitchType::kSimple, false },
    { L"-HELP",  NSwitchType::kSimple, false },
    { L"BA", NSwitchType::kSimple, false },
    { L"BD", NSwitchType::kSimple, false },
    { L"T",  NSwitchType::kUnLimitedPostString, false, 1 },
    { L"Y",  NSwitchType::kSimple, false },
    #ifndef _NO_CRYPTO
    { L"P",  NSwitchType::kUnLimitedPostString, false, 0 },
    #endif
    { L"M",  NSwitchType::kUnLimitedPostString, true, 1 },
    { L"O",  NSwitchType::kUnLimitedPostString, false, 1 },
    { L"W",  NSwitchType::kUnLimitedPostString, false, 0 },
    { L"I",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { L"X",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { L"AI", NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { L"AX", NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { L"AN", NSwitchType::kSimple, false },
    { L"U",  NSwitchType::kUnLimitedPostString, true, 1},
    { L"V",  NSwitchType::kUnLimitedPostString, true, 1},
    { L"R",  NSwitchType::kPostChar, false, 0, 0, kRecursedPostCharSet },
    { L"SFX", NSwitchType::kUnLimitedPostString, false, 0 },
    { L"SI", NSwitchType::kUnLimitedPostString, false, 0 },
    { L"SO", NSwitchType::kSimple, false, 0 },
    { L"AO", NSwitchType::kPostChar, false, 1, 1, kOverwritePostCharSet},
    { L"SEML", NSwitchType::kUnLimitedPostString, false, 0},
    { L"AD",  NSwitchType::kSimple, false },
    { L"SLP", NSwitchType::kUnLimitedPostString, false, 0},
    { L"SCS", NSwitchType::kUnLimitedPostString, false, 0},
    { L"SLT", NSwitchType::kSimple, false },
    { L"SSW", NSwitchType::kSimple, false },
    { L"SSC", NSwitchType::kPostChar, false, 0, 0, L"-" }
  };

static const CCommandForm g_CommandForms[] =
{
  { L"A", false },
  { L"U", false },
  { L"D", false },
  { L"T", false },
  { L"E", false },
  { L"X", false },
  { L"L", false },
  { L"B", false },
  { L"I", false }
};

static const int kNumCommandForms = sizeof(g_CommandForms) /  sizeof(g_CommandForms[0]);

static const wchar_t *kUniversalWildcard = L"*";
static const int kMinNonSwitchWords = 1;
static const int kCommandIndex = 0;

// ---------------------------
// exception messages

static const char *kUserErrorMessage  = "Incorrect command line";
static const char *kIncorrectListFile = "Incorrect item in listfile.\nCheck charset encoding and -scs switch.";
static const char *kIncorrectWildCardInListFile = "Incorrect wildcard in listfile";
static const char *kIncorrectWildCardInCommandLine  = "Incorrect wildcard in command line";
static const char *kTerminalOutError = "I won't write compressed data to a terminal";
static const char *kSameTerminalError = "I won't write data and program's messages to same terminal";

static void ThrowException(const char *errorMessage)
{
  throw CArchiveCommandLineException(errorMessage);
};

static void ThrowUserErrorException()
{
  ThrowException(kUserErrorMessage);
};

// ---------------------------

bool CArchiveCommand::IsFromExtractGroup() const
{
  switch(CommandType)
  {
    case NCommandType::kTest:
    case NCommandType::kExtract:
    case NCommandType::kFullExtract:
      return true;
    default:
      return false;
  }
}

NExtract::NPathMode::EEnum CArchiveCommand::GetPathMode() const
{
  switch(CommandType)
  {
    case NCommandType::kTest:
    case NCommandType::kFullExtract:
      return NExtract::NPathMode::kFullPathnames;
    default:
      return NExtract::NPathMode::kNoPathnames;
  }
}

bool CArchiveCommand::IsFromUpdateGroup() const
{
  return (CommandType == NCommandType::kAdd ||
    CommandType == NCommandType::kUpdate ||
    CommandType == NCommandType::kDelete);
}

static NRecursedType::EEnum GetRecursedTypeFromIndex(int index)
{
  switch (index)
  {
    case NRecursedPostCharIndex::kWildCardRecursionOnly:
      return NRecursedType::kWildCardOnlyRecursed;
    case NRecursedPostCharIndex::kNoRecursion:
      return NRecursedType::kNonRecursed;
    default:
      return NRecursedType::kRecursed;
  }
}

static bool ParseArchiveCommand(const UString &commandString, CArchiveCommand &command)
{
  UString commandStringUpper = commandString;
  commandStringUpper.MakeUpper();
  UString postString;
  int commandIndex = ParseCommand(kNumCommandForms, g_CommandForms, commandStringUpper,
      postString) ;
  if (commandIndex < 0)
    return false;
  command.CommandType = (NCommandType::EEnum)commandIndex;
  return true;
}

// ------------------------------------------------------------------
// filenames functions

static bool AddNameToCensor(NWildcard::CCensor &wildcardCensor,
    const UString &name, bool include, NRecursedType::EEnum type)
{
  bool isWildCard = DoesNameContainWildCard(name);
  bool recursed = false;

  switch (type)
  {
    case NRecursedType::kWildCardOnlyRecursed:
      recursed = isWildCard;
      break;
    case NRecursedType::kRecursed:
      recursed = true;
      break;
    case NRecursedType::kNonRecursed:
      recursed = false;
      break;
  }
  wildcardCensor.AddItem(include, name, recursed);
  return true;
}

static void AddToCensorFromListFile(NWildcard::CCensor &wildcardCensor,
    LPCWSTR fileName, bool include, NRecursedType::EEnum type, UINT codePage)
{
  UStringVector names;
  if (!ReadNamesFromListFile(fileName, names, codePage))
    throw kIncorrectListFile;
  for (int i = 0; i < names.Size(); i++)
    if (!AddNameToCensor(wildcardCensor, names[i], include, type))
      throw kIncorrectWildCardInListFile;
}

static void AddCommandLineWildCardToCensr(NWildcard::CCensor &wildcardCensor,
    const UString &name, bool include, NRecursedType::EEnum recursedType)
{
  if (!AddNameToCensor(wildcardCensor, name, include, recursedType))
    throw kIncorrectWildCardInCommandLine;
}

static void AddToCensorFromNonSwitchesStrings(
    int startIndex,
    NWildcard::CCensor &wildcardCensor,
    const UStringVector &nonSwitchStrings, NRecursedType::EEnum type,
    bool thereAreSwitchIncludes, UINT codePage)
{
  if (nonSwitchStrings.Size() == startIndex && (!thereAreSwitchIncludes))
    AddCommandLineWildCardToCensr(wildcardCensor, kUniversalWildcard, true, type);
  for (int i = startIndex; i < nonSwitchStrings.Size(); i++)
  {
    const UString &s = nonSwitchStrings[i];
    if (s[0] == kFileListID)
      AddToCensorFromListFile(wildcardCensor, s.Mid(1), true, type, codePage);
    else
      AddCommandLineWildCardToCensr(wildcardCensor, s, true, type);
  }
}

#ifdef _WIN32
static void ParseMapWithPaths(NWildcard::CCensor &wildcardCensor,
    const UString &switchParam, bool include,
    NRecursedType::EEnum commonRecursedType)
{
  int splitPos = switchParam.Find(L':');
  if (splitPos < 0)
    ThrowUserErrorException();
  UString mappingName = switchParam.Left(splitPos);
  
  UString switchParam2 = switchParam.Mid(splitPos + 1);
  splitPos = switchParam2.Find(L':');
  if (splitPos < 0)
    ThrowUserErrorException();
  
  UString mappingSize = switchParam2.Left(splitPos);
  UString eventName = switchParam2.Mid(splitPos + 1);
  
  UInt64 dataSize64 = ConvertStringToUInt64(mappingSize, NULL);
  UInt32 dataSize = (UInt32)dataSize64;
  {
    CFileMapping fileMapping;
    if (!fileMapping.Open(FILE_MAP_READ, false, GetSystemString(mappingName)))
      ThrowException("Can not open mapping");
    LPVOID data = fileMapping.MapViewOfFile(FILE_MAP_READ, 0, dataSize);
    if (data == NULL)
      ThrowException("MapViewOfFile error");
    try
    {
      const wchar_t *curData = (const wchar_t *)data;
      if (*curData != 0)
        ThrowException("Incorrect mapping data");
      UInt32 numChars = dataSize / sizeof(wchar_t);
      UString name;
      for (UInt32 i = 1; i < numChars; i++)
      {
        wchar_t c = curData[i];
        if (c == L'\0')
        {
          AddCommandLineWildCardToCensr(wildcardCensor,
              name, include, commonRecursedType);
          name.Empty();
        }
        else
          name += c;
      }
      if (!name.IsEmpty())
        ThrowException("data error");
    }
    catch(...)
    {
      UnmapViewOfFile(data);
      throw;
    }
    UnmapViewOfFile(data);
  }
  
  {
    NSynchronization::CManualResetEvent event;
    if (event.Open(EVENT_MODIFY_STATE, false, GetSystemString(eventName)) == S_OK)
      event.Set();
  }
}
#endif

static void AddSwitchWildCardsToCensor(NWildcard::CCensor &wildcardCensor,
    const UStringVector &strings, bool include,
    NRecursedType::EEnum commonRecursedType, UINT codePage)
{
  for (int i = 0; i < strings.Size(); i++)
  {
    const UString &name = strings[i];
    NRecursedType::EEnum recursedType;
    int pos = 0;
    if (name.Length() < kSomeCludePostStringMinSize)
      ThrowUserErrorException();
    if (::MyCharUpper(name[pos]) == kRecursedIDChar)
    {
      pos++;
      int index = UString(kRecursedPostCharSet).Find(name[pos]);
      recursedType = GetRecursedTypeFromIndex(index);
      if (index >= 0)
        pos++;
    }
    else
      recursedType = commonRecursedType;
    if (name.Length() < pos + kSomeCludeAfterRecursedPostStringMinSize)
      ThrowUserErrorException();
    UString tail = name.Mid(pos + 1);
    if (name[pos] == kImmediateNameID)
      AddCommandLineWildCardToCensr(wildcardCensor, tail, include, recursedType);
    else if (name[pos] == kFileListID)
      AddToCensorFromListFile(wildcardCensor, tail, include, recursedType, codePage);
    #ifdef _WIN32
    else if (name[pos] == kMapNameID)
      ParseMapWithPaths(wildcardCensor, tail, include, recursedType);
    #endif
    else
      ThrowUserErrorException();
  }
}

#ifdef _WIN32

// This code converts all short file names to long file names.

static void ConvertToLongName(const UString &prefix, UString &name)
{
  if (name.IsEmpty() || DoesNameContainWildCard(name))
    return;
  NFind::CFileInfoW fileInfo;
  if (NFind::FindFile(prefix + name, fileInfo))
    name = fileInfo.Name;
}

static void ConvertToLongNames(const UString &prefix, CObjectVector<NWildcard::CItem> &items)
{
  for (int i = 0; i < items.Size(); i++)
  {
    NWildcard::CItem &item = items[i];
    if (item.Recursive || item.PathParts.Size() != 1)
      continue;
    ConvertToLongName(prefix, item.PathParts.Front());
  }
}

static void ConvertToLongNames(const UString &prefix, NWildcard::CCensorNode &node)
{
  ConvertToLongNames(prefix, node.IncludeItems);
  ConvertToLongNames(prefix, node.ExcludeItems);
  int i;
  for (i = 0; i < node.SubNodes.Size(); i++)
    ConvertToLongName(prefix, node.SubNodes[i].Name);
  // mix folders with same name
  for (i = 0; i < node.SubNodes.Size(); i++)
  {
    NWildcard::CCensorNode &nextNode1 = node.SubNodes[i];
    for (int j = i + 1; j < node.SubNodes.Size();)
    {
      const NWildcard::CCensorNode &nextNode2 = node.SubNodes[j];
      if (nextNode1.Name.CompareNoCase(nextNode2.Name) == 0)
      {
        nextNode1.IncludeItems += nextNode2.IncludeItems;
        nextNode1.ExcludeItems += nextNode2.ExcludeItems;
        node.SubNodes.Delete(j);
      }
      else
        j++;
    }
  }
  for (i = 0; i < node.SubNodes.Size(); i++)
  {
    NWildcard::CCensorNode &nextNode = node.SubNodes[i];
    ConvertToLongNames(prefix + nextNode.Name + wchar_t(NFile::NName::kDirDelimiter), nextNode);
  }
}

static void ConvertToLongNames(NWildcard::CCensor &censor)
{
  for (int i = 0; i < censor.Pairs.Size(); i++)
  {
    NWildcard::CPair &pair = censor.Pairs[i];
    ConvertToLongNames(pair.Prefix, pair.Head);
  }
}

#endif

static NUpdateArchive::NPairAction::EEnum GetUpdatePairActionType(int i)
{
  switch(i)
  {
    case NUpdateArchive::NPairAction::kIgnore: return NUpdateArchive::NPairAction::kIgnore;
    case NUpdateArchive::NPairAction::kCopy: return NUpdateArchive::NPairAction::kCopy;
    case NUpdateArchive::NPairAction::kCompress: return NUpdateArchive::NPairAction::kCompress;
    case NUpdateArchive::NPairAction::kCompressAsAnti: return NUpdateArchive::NPairAction::kCompressAsAnti;
  }
  throw 98111603;
}

const UString kUpdatePairStateIDSet = L"PQRXYZW";
const int kUpdatePairStateNotSupportedActions[] = {2, 2, 1, -1, -1, -1, -1};

const UString kUpdatePairActionIDSet = L"0123"; //Ignore, Copy, Compress, Create Anti

const wchar_t *kUpdateIgnoreItselfPostStringID = L"-";
const wchar_t kUpdateNewArchivePostCharID = '!';


static bool ParseUpdateCommandString2(const UString &command,
    NUpdateArchive::CActionSet &actionSet, UString &postString)
{
  for (int i = 0; i < command.Length();)
  {
    wchar_t c = MyCharUpper(command[i]);
    int statePos = kUpdatePairStateIDSet.Find(c);
    if (statePos < 0)
    {
      postString = command.Mid(i);
      return true;
    }
    i++;
    if (i >= command.Length())
      return false;
    int actionPos = kUpdatePairActionIDSet.Find(::MyCharUpper(command[i]));
    if (actionPos < 0)
      return false;
    actionSet.StateActions[statePos] = GetUpdatePairActionType(actionPos);
    if (kUpdatePairStateNotSupportedActions[statePos] == actionPos)
      return false;
    i++;
  }
  postString.Empty();
  return true;
}

static void ParseUpdateCommandString(CUpdateOptions &options,
    const UStringVector &updatePostStrings,
    const NUpdateArchive::CActionSet &defaultActionSet)
{
  for (int i = 0; i < updatePostStrings.Size(); i++)
  {
    const UString &updateString = updatePostStrings[i];
    if (updateString.CompareNoCase(kUpdateIgnoreItselfPostStringID) == 0)
    {
      if (options.UpdateArchiveItself)
      {
        options.UpdateArchiveItself = false;
        options.Commands.Delete(0);
      }
    }
    else
    {
      NUpdateArchive::CActionSet actionSet = defaultActionSet;

      UString postString;
      if (!ParseUpdateCommandString2(updateString, actionSet, postString))
        ThrowUserErrorException();
      if (postString.IsEmpty())
      {
        if (options.UpdateArchiveItself)
          options.Commands[0].ActionSet = actionSet;
      }
      else
      {
        if (MyCharUpper(postString[0]) != kUpdateNewArchivePostCharID)
          ThrowUserErrorException();
        CUpdateArchiveCommand uc;
        UString archivePath = postString.Mid(1);
        if (archivePath.IsEmpty())
          ThrowUserErrorException();
        uc.UserArchivePath = archivePath;
        uc.ActionSet = actionSet;
        options.Commands.Add(uc);
      }
    }
  }
}

static const char kByteSymbol = 'B';
static const char kKiloSymbol = 'K';
static const char kMegaSymbol = 'M';
static const char kGigaSymbol = 'G';

static bool ParseComplexSize(const UString &src, UInt64 &result)
{
  UString s = src;
  s.MakeUpper();

  const wchar_t *start = s;
  const wchar_t *end;
  UInt64 number = ConvertStringToUInt64(start, &end);
  int numDigits = (int)(end - start);
  if (numDigits == 0 || s.Length() > numDigits + 1)
    return false;
  if (s.Length() == numDigits)
  {
    result = number;
    return true;
  }
  int numBits;
  switch (s[numDigits])
  {
    case kByteSymbol:
      result = number;
      return true;
    case kKiloSymbol:
      numBits = 10;
      break;
    case kMegaSymbol:
      numBits = 20;
      break;
    case kGigaSymbol:
      numBits = 30;
      break;
    default:
      return false;
  }
  if (number >= ((UInt64)1 << (64 - numBits)))
    return false;
  result = number << numBits;
  return true;
}

static void SetAddCommandOptions(
    NCommandType::EEnum commandType,
    const CParser &parser,
    CUpdateOptions &options)
{
  NUpdateArchive::CActionSet defaultActionSet;
  switch(commandType)
  {
    case NCommandType::kAdd:
      defaultActionSet = NUpdateArchive::kAddActionSet;
      break;
    case NCommandType::kDelete:
      defaultActionSet = NUpdateArchive::kDeleteActionSet;
      break;
    default:
      defaultActionSet = NUpdateArchive::kUpdateActionSet;
  }
  
  options.UpdateArchiveItself = true;
  
  options.Commands.Clear();
  CUpdateArchiveCommand updateMainCommand;
  updateMainCommand.ActionSet = defaultActionSet;
  options.Commands.Add(updateMainCommand);
  if (parser[NKey::kUpdate].ThereIs)
    ParseUpdateCommandString(options, parser[NKey::kUpdate].PostStrings,
        defaultActionSet);
  if (parser[NKey::kWorkingDir].ThereIs)
  {
    const UString &postString = parser[NKey::kWorkingDir].PostStrings[0];
    if (postString.IsEmpty())
      NDirectory::MyGetTempPath(options.WorkingDir);
    else
      options.WorkingDir = postString;
  }
  options.SfxMode = parser[NKey::kSfx].ThereIs;
  if (options.SfxMode)
    options.SfxModule = parser[NKey::kSfx].PostStrings[0];

  if (parser[NKey::kVolume].ThereIs)
  {
    const UStringVector &sv = parser[NKey::kVolume].PostStrings;
    for (int i = 0; i < sv.Size(); i++)
    {
      UInt64 size;
      if (!ParseComplexSize(sv[i], size))
        ThrowException("Incorrect volume size");
      options.VolumesSizes.Add(size);
    }
  }
}

static void SetMethodOptions(const CParser &parser, CObjectVector<CProperty> &properties)
{
  if (parser[NKey::kProperty].ThereIs)
  {
    // options.MethodMode.Properties.Clear();
    for (int i = 0; i < parser[NKey::kProperty].PostStrings.Size(); i++)
    {
      CProperty property;
      const UString &postString = parser[NKey::kProperty].PostStrings[i];
      int index = postString.Find(L'=');
      if (index < 0)
        property.Name = postString;
      else
      {
        property.Name = postString.Left(index);
        property.Value = postString.Mid(index + 1);
      }
      properties.Add(property);
    }
  }
}

CArchiveCommandLineParser::CArchiveCommandLineParser():
  parser(sizeof(kSwitchForms) / sizeof(kSwitchForms[0])) {}

void CArchiveCommandLineParser::Parse1(const UStringVector &commandStrings,
    CArchiveCommandLineOptions &options)
{
  try
  {
    parser.ParseStrings(kSwitchForms, commandStrings);
  }
  catch(...)
  {
    ThrowUserErrorException();
  }

  options.IsInTerminal = MY_IS_TERMINAL(stdin);
  options.IsStdOutTerminal = MY_IS_TERMINAL(stdout);
  options.IsStdErrTerminal = MY_IS_TERMINAL(stderr);
  options.StdOutMode = parser[NKey::kStdOut].ThereIs;
  options.EnableHeaders = !parser[NKey::kDisableHeaders].ThereIs;
  options.HelpMode = parser[NKey::kHelp1].ThereIs || parser[NKey::kHelp2].ThereIs  || parser[NKey::kHelp3].ThereIs;

  #ifdef _WIN32
  options.LargePages = false;
  if (parser[NKey::kLargePages].ThereIs)
  {
    const UString &postString = parser[NKey::kLargePages].PostStrings.Front();
    if (postString.IsEmpty())
      options.LargePages = true;
  }
  #endif
}

struct CCodePagePair
{
  const wchar_t *Name;
  UINT CodePage;
};

static CCodePagePair g_CodePagePairs[] =
{
  { L"UTF-8", CP_UTF8 },
  { L"WIN",   CP_ACP },
  { L"DOS",   CP_OEMCP }
};

static const int kNumCodePages = sizeof(g_CodePagePairs) / sizeof(g_CodePagePairs[0]);

static bool ConvertStringToUInt32(const wchar_t *s, UInt32 &v)
{
  const wchar_t *end;
  UInt64 number = ConvertStringToUInt64(s, &end);
  if (*end != 0)
    return false;
  if (number > (UInt32)0xFFFFFFFF)
    return false;
  v = (UInt32)number;
  return true;
}

void CArchiveCommandLineParser::Parse2(CArchiveCommandLineOptions &options)
{
  const UStringVector &nonSwitchStrings = parser.NonSwitchStrings;
  int numNonSwitchStrings = nonSwitchStrings.Size();
  if (numNonSwitchStrings < kMinNonSwitchWords)
    ThrowUserErrorException();

  if (!ParseArchiveCommand(nonSwitchStrings[kCommandIndex], options.Command))
    ThrowUserErrorException();

  options.TechMode = parser[NKey::kTechMode].ThereIs;

  if (parser[NKey::kCaseSensitive].ThereIs)
    g_CaseSensitive = (parser[NKey::kCaseSensitive].PostCharIndex < 0);

  NRecursedType::EEnum recursedType;
  if (parser[NKey::kRecursed].ThereIs)
    recursedType = GetRecursedTypeFromIndex(parser[NKey::kRecursed].PostCharIndex);
  else
    recursedType = NRecursedType::kNonRecursed;

  UINT codePage = CP_UTF8;
  if (parser[NKey::kCharSet].ThereIs)
  {
    UString name = parser[NKey::kCharSet].PostStrings.Front();
    name.MakeUpper();
    int i;
    for (i = 0; i < kNumCodePages; i++)
    {
      const CCodePagePair &pair = g_CodePagePairs[i];
      if (name.Compare(pair.Name) == 0)
      {
        codePage = pair.CodePage;
        break;
      }
    }
    if (i >= kNumCodePages)
      ThrowUserErrorException();
  }

  bool thereAreSwitchIncludes = false;
  if (parser[NKey::kInclude].ThereIs)
  {
    thereAreSwitchIncludes = true;
    AddSwitchWildCardsToCensor(options.WildcardCensor,
        parser[NKey::kInclude].PostStrings, true, recursedType, codePage);
  }
  if (parser[NKey::kExclude].ThereIs)
    AddSwitchWildCardsToCensor(options.WildcardCensor,
        parser[NKey::kExclude].PostStrings, false, recursedType, codePage);
 
  int curCommandIndex = kCommandIndex + 1;
  bool thereIsArchiveName = !parser[NKey::kNoArName].ThereIs &&
      options.Command.CommandType != NCommandType::kBenchmark &&
      options.Command.CommandType != NCommandType::kInfo;
  if (thereIsArchiveName)
  {
    if (curCommandIndex >= numNonSwitchStrings)
      ThrowUserErrorException();
    options.ArchiveName = nonSwitchStrings[curCommandIndex++];
  }

  AddToCensorFromNonSwitchesStrings(
      curCommandIndex, options.WildcardCensor,
      nonSwitchStrings, recursedType, thereAreSwitchIncludes, codePage);

  options.YesToAll = parser[NKey::kYes].ThereIs;

  bool isExtractGroupCommand = options.Command.IsFromExtractGroup();

  #ifndef _NO_CRYPTO
  options.PasswordEnabled = parser[NKey::kPassword].ThereIs;
  if (options.PasswordEnabled)
    options.Password = parser[NKey::kPassword].PostStrings[0];
  #endif

  options.StdInMode = parser[NKey::kStdIn].ThereIs;
  options.ShowDialog = parser[NKey::kShowDialog].ThereIs;

  if (parser[NKey::kArchiveType].ThereIs)
    options.ArcType = parser[NKey::kArchiveType].PostStrings[0];

  if (isExtractGroupCommand || options.Command.CommandType == NCommandType::kList)
  {
    if (options.StdInMode)
      ThrowException("Reading archives from stdin is not implemented");
    if (!options.WildcardCensor.AllAreRelative())
      ThrowException("Cannot use absolute pathnames for this command");

    NWildcard::CCensor archiveWildcardCensor;

    if (parser[NKey::kArInclude].ThereIs)
    {
      AddSwitchWildCardsToCensor(archiveWildcardCensor,
        parser[NKey::kArInclude].PostStrings, true, NRecursedType::kNonRecursed, codePage);
    }
    if (parser[NKey::kArExclude].ThereIs)
      AddSwitchWildCardsToCensor(archiveWildcardCensor,
      parser[NKey::kArExclude].PostStrings, false, NRecursedType::kNonRecursed, codePage);

    if (thereIsArchiveName)
      AddCommandLineWildCardToCensr(archiveWildcardCensor, options.ArchiveName, true, NRecursedType::kNonRecursed);

    #ifdef _WIN32
    ConvertToLongNames(archiveWildcardCensor);
    #endif

    archiveWildcardCensor.ExtendExclude();

    UStringVector archivePaths;

    {
      CDirItems dirItems;
      {
        UStringVector errorPaths;
        CRecordVector<DWORD> errorCodes;
        HRESULT res = EnumerateItems(archiveWildcardCensor, dirItems, NULL, errorPaths, errorCodes);
        if (res != S_OK || errorPaths.Size() > 0)
          throw "cannot find archive";
      }
      for (int i = 0; i < dirItems.Items.Size(); i++)
      {
        const CDirItem &dirItem = dirItems.Items[i];
        if (!dirItem.IsDir())
          archivePaths.Add(dirItems.GetPhyPath(i));
      }
    }

    if (archivePaths.Size() == 0)
      throw "there is no such archive";

    UStringVector archivePathsFull;

    int i;
    for (i = 0; i < archivePaths.Size(); i++)
    {
      UString fullPath;
      NFile::NDirectory::MyGetFullPathName(archivePaths[i], fullPath);
      archivePathsFull.Add(fullPath);
    }
    CIntVector indices;
    SortFileNames(archivePathsFull, indices);
    options.ArchivePathsSorted.Reserve(indices.Size());
    options.ArchivePathsFullSorted.Reserve(indices.Size());
    for (i = 0; i < indices.Size(); i++)
    {
      options.ArchivePathsSorted.Add(archivePaths[indices[i]]);
      options.ArchivePathsFullSorted.Add(archivePathsFull[indices[i]]);
    }

    if (isExtractGroupCommand)
    {
      SetMethodOptions(parser, options.ExtractProperties);
      if (options.StdOutMode && options.IsStdOutTerminal && options.IsStdErrTerminal)
        throw kSameTerminalError;
      if (parser[NKey::kOutputDir].ThereIs)
      {
        options.OutputDir = parser[NKey::kOutputDir].PostStrings[0];
        NFile::NName::NormalizeDirPathPrefix(options.OutputDir);
      }

      options.OverwriteMode = NExtract::NOverwriteMode::kAskBefore;
      if (parser[NKey::kOverwrite].ThereIs)
        options.OverwriteMode =
            k_OverwriteModes[parser[NKey::kOverwrite].PostCharIndex];
      else if (options.YesToAll)
        options.OverwriteMode = NExtract::NOverwriteMode::kWithoutPrompt;
    }
  }
  else if (options.Command.IsFromUpdateGroup())
  {
    CUpdateOptions &updateOptions = options.UpdateOptions;

    SetAddCommandOptions(options.Command.CommandType, parser, updateOptions);
    
    SetMethodOptions(parser, updateOptions.MethodMode.Properties);

    if (parser[NKey::kShareForWrite].ThereIs)
      updateOptions.OpenShareForWrite = true;

    options.EnablePercents = !parser[NKey::kDisablePercents].ThereIs;

    if (options.EnablePercents)
    {
      if ((options.StdOutMode && !options.IsStdErrTerminal) ||
         (!options.StdOutMode && !options.IsStdOutTerminal))
        options.EnablePercents = false;
    }

    updateOptions.EMailMode = parser[NKey::kEmail].ThereIs;
    if (updateOptions.EMailMode)
    {
      updateOptions.EMailAddress = parser[NKey::kEmail].PostStrings.Front();
      if (updateOptions.EMailAddress.Length() > 0)
        if (updateOptions.EMailAddress[0] == L'.')
        {
          updateOptions.EMailRemoveAfter = true;
          updateOptions.EMailAddress.Delete(0);
        }
    }

    updateOptions.StdOutMode = options.StdOutMode;
    updateOptions.StdInMode = options.StdInMode;

    if (updateOptions.StdOutMode && updateOptions.EMailMode)
      throw "stdout mode and email mode cannot be combined";
    if (updateOptions.StdOutMode && options.IsStdOutTerminal)
      throw kTerminalOutError;
    if (updateOptions.StdInMode)
      updateOptions.StdInFileName = parser[NKey::kStdIn].PostStrings.Front();

    #ifdef _WIN32
    ConvertToLongNames(options.WildcardCensor);
    #endif
  }
  else if (options.Command.CommandType == NCommandType::kBenchmark)
  {
    options.NumThreads = (UInt32)-1;
    options.DictionarySize = (UInt32)-1;
    options.NumIterations = 1;
    if (curCommandIndex < numNonSwitchStrings)
    {
      if (!ConvertStringToUInt32(nonSwitchStrings[curCommandIndex++], options.NumIterations))
        ThrowUserErrorException();
    }
    for (int i = 0; i < parser[NKey::kProperty].PostStrings.Size(); i++)
    {
      UString postString = parser[NKey::kProperty].PostStrings[i];
      postString.MakeUpper();
      if (postString.Length() < 2)
        ThrowUserErrorException();
      if (postString[0] == 'D')
      {
        int pos = 1;
        if (postString[pos] == '=')
          pos++;
        UInt32 logSize;
        if (!ConvertStringToUInt32((const wchar_t *)postString + pos, logSize))
          ThrowUserErrorException();
        if (logSize > 31)
          ThrowUserErrorException();
        options.DictionarySize = 1 << logSize;
      }
      else if (postString[0] == 'M' && postString[1] == 'T' )
      {
        int pos = 2;
        if (postString[pos] == '=')
          pos++;
        if (postString[pos] != 0)
          if (!ConvertStringToUInt32((const wchar_t *)postString + pos, options.NumThreads))
            ThrowUserErrorException();
      }
      else if (postString[0] == 'M' && postString[1] == '=' )
      {
        int pos = 2;
        if (postString[pos] != 0)
          options.Method = postString.Mid(2);
      }
      else
        ThrowUserErrorException();
    }
  }
  else if (options.Command.CommandType == NCommandType::kInfo)
  {
  }
  else
    ThrowUserErrorException();
  options.WildcardCensor.ExtendExclude();
}
