// ArchiveCommandLine.h

#ifndef __ARCHIVECOMMANDLINE_H
#define __ARCHIVECOMMANDLINE_H

#include "Common/Wildcard.h"
#include "Common/CommandLineParser.h"

#include "Extract.h"
#include "Update.h"

struct CArchiveCommandLineException: public AString
{
  CArchiveCommandLineException(const char *errorMessage): AString(errorMessage) {}
};

namespace NCommandType { enum EEnum
{
  kAdd = 0,
  kUpdate,
  kDelete,
  kTest,
  kExtract,
  kFullExtract,
  kList,
  kBenchmark,
  kInfo
};}

namespace NRecursedType { enum EEnum
{
  kRecursed,
  kWildCardOnlyRecursed,
  kNonRecursed
};}

struct CArchiveCommand
{
  NCommandType::EEnum CommandType;
  bool IsFromExtractGroup() const;
  bool IsFromUpdateGroup() const;
  bool IsTestMode() const { return CommandType == NCommandType::kTest; }
  NExtract::NPathMode::EEnum GetPathMode() const;
};

struct CArchiveCommandLineOptions
{
  bool HelpMode;

  #ifdef _WIN32
  bool LargePages;
  #endif

  bool IsInTerminal;
  bool IsStdOutTerminal;
  bool IsStdErrTerminal;
  bool StdInMode;
  bool StdOutMode;
  bool EnableHeaders;

  bool YesToAll;
  bool ShowDialog;
  // NWildcard::CCensor ArchiveWildcardCensor;
  NWildcard::CCensor WildcardCensor;

  CArchiveCommand Command;
  UString ArchiveName;

  #ifndef _NO_CRYPTO
  bool PasswordEnabled;
  UString Password;
  #endif

  bool TechMode;
  // Extract
  bool AppendName;
  UString OutputDir;
  NExtract::NOverwriteMode::EEnum OverwriteMode;
  UStringVector ArchivePathsSorted;
  UStringVector ArchivePathsFullSorted;
  CObjectVector<CProperty> ExtractProperties;

  CUpdateOptions UpdateOptions;
  UString ArcType;
  bool EnablePercents;

  // Benchmark
  UInt32 NumIterations;
  UInt32 NumThreads;
  UInt32 DictionarySize;
  UString Method;


  CArchiveCommandLineOptions(): StdInMode(false), StdOutMode(false) {};
};

class CArchiveCommandLineParser
{
  NCommandLineParser::CParser parser;
public:
  CArchiveCommandLineParser();
  void Parse1(const UStringVector &commandStrings, CArchiveCommandLineOptions &options);
  void Parse2(CArchiveCommandLineOptions &options);
};

#endif
