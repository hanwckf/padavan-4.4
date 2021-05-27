// PercentPrinter.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/MyString.h"

#include "PercentPrinter.h"

const int kPaddingSize = 2;
const int kPercentsSize = 4;
const int kMaxExtraSize = kPaddingSize + 32 + kPercentsSize;

static void ClearPrev(char *p, int num)
{
  int i;
  for (i = 0; i < num; i++) *p++ = '\b';
  for (i = 0; i < num; i++) *p++ = ' ';
  for (i = 0; i < num; i++) *p++ = '\b';
  *p = '\0';
}

void CPercentPrinter::ClosePrint()
{
  if (m_NumExtraChars == 0)
    return;
  char s[kMaxExtraSize * 3 + 1];
  ClearPrev(s, m_NumExtraChars);
  (*OutStream) << s;
  m_NumExtraChars = 0;
}

void CPercentPrinter::PrintString(const char *s)
{
  ClosePrint();
  (*OutStream) << s;
}

void CPercentPrinter::PrintString(const wchar_t *s)
{
  ClosePrint();
  (*OutStream) << s;
}

void CPercentPrinter::PrintNewLine()
{
  ClosePrint();
  (*OutStream) << "\n";
}

void CPercentPrinter::RePrintRatio()
{
  char s[32];
  ConvertUInt64ToString(((m_Total == 0) ? 0 : (m_CurValue * 100 / m_Total)), s);
  int size = (int)strlen(s);
  s[size++] = '%';
  s[size] = '\0';

  int extraSize = kPaddingSize + MyMax(size, kPercentsSize);
  if (extraSize < m_NumExtraChars)
    extraSize = m_NumExtraChars;

  char fullString[kMaxExtraSize * 3];
  char *p = fullString;
  int i;
  if (m_NumExtraChars == 0)
  {
    for (i = 0; i < extraSize; i++)
      *p++ = ' ';
    m_NumExtraChars = extraSize;
  }

  for (i = 0; i < m_NumExtraChars; i++)
    *p++ = '\b';
  m_NumExtraChars = extraSize;
  for (; size < m_NumExtraChars; size++)
    *p++ = ' ';
  MyStringCopy(p, s);
  (*OutStream) << fullString;
  OutStream->Flush();
  m_PrevValue = m_CurValue;
}

void CPercentPrinter::PrintRatio()
{
  if (m_CurValue < m_PrevValue + m_MinStepSize &&
      m_CurValue + m_MinStepSize > m_PrevValue && m_NumExtraChars != 0)
    return;
  RePrintRatio();
}
