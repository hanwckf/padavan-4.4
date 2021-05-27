// SplitHandler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/Defs.h"
#include "Common/NewHandler.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../../Common/ProgressUtils.h"

#include "../../Compress/CopyCoder.h"

#include "../Common/ItemNameUtils.h"
#include "../Common/MultiStream.h"

#include "SplitHandler.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NSplit {

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

class CSeqName
{
public:
  UString _unchangedPart;
  UString _changedPart;
  bool _splitStyle;
  UString GetNextName()
  {
    UString newName;
    if (_splitStyle)
    {
      int i;
      int numLetters = _changedPart.Length();
      for (i = numLetters - 1; i >= 0; i--)
      {
        wchar_t c = _changedPart[i];
        if (c == 'z')
        {
          c = 'a';
          newName = c + newName;
          continue;
        }
        else if (c == 'Z')
        {
          c = 'A';
          newName = c + newName;
          continue;
        }
        c++;
        if ((c == 'z' || c == 'Z') && i == 0)
        {
          _unchangedPart += c;
          wchar_t newChar = (c == 'z') ? L'a' : L'A';
          newName.Empty();
          numLetters++;
          for (int k = 0; k < numLetters; k++)
            newName += newChar;
          break;
        }
        newName = c + newName;
        i--;
        for (; i >= 0; i--)
          newName = _changedPart[i] + newName;
        break;
      }
    }
    else
    {
      int i;
      int numLetters = _changedPart.Length();
      for (i = numLetters - 1; i >= 0; i--)
      {
        wchar_t c = _changedPart[i];
        if (c == L'9')
        {
          c = L'0';
          newName = c + newName;
          if (i == 0)
            newName = UString(L'1') + newName;
          continue;
        }
        c++;
        newName = c + newName;
        i--;
        for (; i >= 0; i--)
          newName = _changedPart[i] + newName;
        break;
      }
    }
    _changedPart = newName;
    return _unchangedPart + _changedPart;
  }
};

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  Close();
  if (openArchiveCallback == 0)
    return S_FALSE;
  // try
  {
    CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
    CMyComPtr<IArchiveOpenCallback> openArchiveCallbackWrap = openArchiveCallback;
    if (openArchiveCallbackWrap.QueryInterface(IID_IArchiveOpenVolumeCallback,
        &openVolumeCallback) != S_OK)
      return S_FALSE;
    
    {
      NCOM::CPropVariant prop;
      RINOK(openVolumeCallback->GetProperty(kpidName, &prop));
      if (prop.vt != VT_BSTR)
        return S_FALSE;
      _name = prop.bstrVal;
    }
    
    int dotPos = _name.ReverseFind('.');
    UString prefix, ext;
    if (dotPos >= 0)
    {
      prefix = _name.Left(dotPos + 1);
      ext = _name.Mid(dotPos + 1);
    }
    else
      ext = _name;
    UString extBig = ext;
    extBig.MakeUpper();

    CSeqName seqName;

    int numLetters = 2;
    bool splitStyle = false;
    if (extBig.Right(2) == L"AA")
    {
      splitStyle = true;
      while (numLetters < extBig.Length())
      {
        if (extBig[extBig.Length() - numLetters - 1] != 'A')
          break;
        numLetters++;
      }
    }
    else if (ext.Right(2) == L"01")
    {
      while (numLetters < extBig.Length())
      {
        if (extBig[extBig.Length() - numLetters - 1] != '0')
          break;
        numLetters++;
      }
      if (numLetters != ext.Length())
        return S_FALSE;
    }
    else
      return S_FALSE;

    _streams.Add(stream);

    seqName._unchangedPart = prefix + ext.Left(extBig.Length() - numLetters);
    seqName._changedPart = ext.Right(numLetters);
    seqName._splitStyle = splitStyle;

    if (prefix.Length() < 1)
      _subName = L"file";
    else
      _subName = prefix.Left(prefix.Length() - 1);

    _totalSize = 0;
    UInt64 size;
    {
      NCOM::CPropVariant prop;
      RINOK(openVolumeCallback->GetProperty(kpidSize, &prop));
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      size = prop.uhVal.QuadPart;
    }
    _totalSize += size;
    _sizes.Add(size);
    
    if (openArchiveCallback != NULL)
    {
      UInt64 numFiles = _streams.Size();
      RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
    }

    for (;;)
    {
      UString fullName = seqName.GetNextName();
      CMyComPtr<IInStream> nextStream;
      HRESULT result = openVolumeCallback->GetStream(fullName, &nextStream);
      if (result == S_FALSE)
        break;
      if (result != S_OK)
        return result;
      if (!stream)
        break;
      {
        NCOM::CPropVariant prop;
        RINOK(openVolumeCallback->GetProperty(kpidSize, &prop));
        if (prop.vt != VT_UI8)
          return E_INVALIDARG;
        size = prop.uhVal.QuadPart;
      }
      _totalSize += size;
      _sizes.Add(size);
      _streams.Add(nextStream);
      if (openArchiveCallback != NULL)
      {
        UInt64 numFiles = _streams.Size();
        RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
      }
    }
  }
  /*
  catch(...)
  {
    return S_FALSE;
  }
  */
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _sizes.Clear();
  _streams.Clear();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _streams.IsEmpty() ? 0 : 1;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidPath:
      prop = _subName;
      break;
    case kpidSize:
    case kpidPackSize:
      prop = _totalSize;
      break;
  }
  prop.Detach(value);
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 _aTestMode, IArchiveExtractCallback *_anExtractCallback)
{
  COM_TRY_BEGIN

  if (numItems != UInt32(-1))
  {
    if (numItems != 1)
      return E_INVALIDARG;
    if (indices[0] != 0)
      return E_INVALIDARG;
  }
  bool testMode = (_aTestMode != 0);
  CMyComPtr<IArchiveExtractCallback> extractCallback = _anExtractCallback;
  extractCallback->SetTotal(_totalSize);
  
  /*
  CMyComPtr<IArchiveVolumeExtractCallback> volumeExtractCallback;
  if (extractCallback.QueryInterface(&volumeExtractCallback) != S_OK)
    return E_FAIL;
  */

  UInt64 currentTotalSize = 0;
  UInt64 currentItemSize;

  RINOK(extractCallback->SetCompleted(&currentTotalSize));
  CMyComPtr<ISequentialOutStream> realOutStream;
  Int32 askMode;
  askMode = testMode ? NArchive::NExtract::NAskMode::kTest :
  NArchive::NExtract::NAskMode::kExtract;
  Int32 index = 0;
  RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
  
  RINOK(extractCallback->PrepareOperation(askMode));
  if (testMode)
  {
    RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
    return S_OK;
  }
  
  if (!testMode && (!realOutStream))
    return S_OK;

  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder;
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  for (int i = 0; i < _streams.Size(); i++, currentTotalSize += currentItemSize)
  {
    lps->InSize = lps->OutSize = currentTotalSize;
    RINOK(lps->SetCur());
    IInStream *inStream = _streams[i];
    RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
    RINOK(copyCoder->Code(inStream, realOutStream, NULL, NULL, progress));
    currentItemSize = copyCoderSpec->TotalSize;
  }
  realOutStream.Release();
  return extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK);
  COM_TRY_END
}

STDMETHODIMP CHandler::GetStream(UInt32 index, ISequentialInStream **stream)
{
  if (index != 0)
    return E_INVALIDARG;
  *stream = 0;
  CMultiStream *streamSpec = new CMultiStream;
  CMyComPtr<ISequentialInStream> streamTemp = streamSpec;
  for (int i = 0; i < _streams.Size(); i++)
  {
    CMultiStream::CSubStreamInfo subStreamInfo;
    subStreamInfo.Stream = _streams[i];
    subStreamInfo.Pos = 0;
    subStreamInfo.Size = _sizes[i];
    streamSpec->Streams.Add(subStreamInfo);
  }
  streamSpec->Init();
  *stream = streamTemp.Detach();
  return S_OK;
}

}}
