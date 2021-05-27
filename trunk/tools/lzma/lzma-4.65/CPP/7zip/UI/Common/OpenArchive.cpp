// OpenArchive.cpp

#include "StdAfx.h"

#include "OpenArchive.h"

#include "Common/Wildcard.h"

#include "Windows/FileName.h"
#include "Windows/FileDir.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "../../Common/FileStreams.h"
#include "../../Common/StreamUtils.h"

#include "Common/StringConvert.h"

#include "DefaultName.h"

using namespace NWindows;

HRESULT GetArchiveItemPath(IInArchive *archive, UInt32 index, UString &result)
{
  NCOM::CPropVariant prop;
  RINOK(archive->GetProperty(index, kpidPath, &prop));
  if(prop.vt == VT_BSTR)
    result = prop.bstrVal;
  else if (prop.vt == VT_EMPTY)
    result.Empty();
  else
    return E_FAIL;
  return S_OK;
}

HRESULT GetArchiveItemPath(IInArchive *archive, UInt32 index, const UString &defaultName, UString &result)
{
  RINOK(GetArchiveItemPath(archive, index, result));
  if (result.IsEmpty())
  {
    result = defaultName;
    NCOM::CPropVariant prop;
    RINOK(archive->GetProperty(index, kpidExtension, &prop));
    if (prop.vt == VT_BSTR)
    {
      result += L'.';
      result += prop.bstrVal;
    }
    else if (prop.vt != VT_EMPTY)
      return E_FAIL;
  }
  return S_OK;
}

HRESULT GetArchiveItemFileTime(IInArchive *archive, UInt32 index,
    const FILETIME &defaultFileTime, FILETIME &fileTime)
{
  NCOM::CPropVariant prop;
  RINOK(archive->GetProperty(index, kpidMTime, &prop));
  if (prop.vt == VT_FILETIME)
    fileTime = prop.filetime;
  else if (prop.vt == VT_EMPTY)
    fileTime = defaultFileTime;
  else
    return E_FAIL;
  return S_OK;
}

HRESULT IsArchiveItemProp(IInArchive *archive, UInt32 index, PROPID propID, bool &result)
{
  NCOM::CPropVariant prop;
  RINOK(archive->GetProperty(index, propID, &prop));
  if(prop.vt == VT_BOOL)
    result = VARIANT_BOOLToBool(prop.boolVal);
  else if (prop.vt == VT_EMPTY)
    result = false;
  else
    return E_FAIL;
  return S_OK;
}

HRESULT IsArchiveItemFolder(IInArchive *archive, UInt32 index, bool &result)
{
  return IsArchiveItemProp(archive, index, kpidIsDir, result);
}

HRESULT IsArchiveItemAnti(IInArchive *archive, UInt32 index, bool &result)
{
  return IsArchiveItemProp(archive, index, kpidIsAnti, result);
}

// Static-SFX (for Linux) can be big.
const UInt64 kMaxCheckStartPosition = 1 << 22;

HRESULT ReOpenArchive(IInArchive *archive, const UString &fileName, IArchiveOpenCallback *openArchiveCallback)
{
  CInFileStream *inStreamSpec = new CInFileStream;
  CMyComPtr<IInStream> inStream(inStreamSpec);
  inStreamSpec->Open(fileName);
  return archive->Open(inStream, &kMaxCheckStartPosition, openArchiveCallback);
}

#ifndef _SFX
static inline bool TestSignature(const Byte *p1, const Byte *p2, size_t size)
{
  for (size_t i = 0; i < size; i++)
    if (p1[i] != p2[i])
      return false;
  return true;
}
#endif

HRESULT OpenArchive(
    CCodecs *codecs,
    int arcTypeIndex,
    IInStream *inStream,
    const UString &fileName,
    IInArchive **archiveResult,
    int &formatIndex,
    UString &defaultItemName,
    IArchiveOpenCallback *openArchiveCallback)
{
  *archiveResult = NULL;
  UString extension;
  {
    int dotPos = fileName.ReverseFind(L'.');
    if (dotPos >= 0)
      extension = fileName.Mid(dotPos + 1);
  }
  CIntVector orderIndices;
  if (arcTypeIndex >= 0)
    orderIndices.Add(arcTypeIndex);
  else
  {

  int i;
  int numFinded = 0;
  for (i = 0; i < codecs->Formats.Size(); i++)
    if (codecs->Formats[i].FindExtension(extension) >= 0)
      orderIndices.Insert(numFinded++, i);
    else
      orderIndices.Add(i);
  
  #ifndef _SFX
  if (numFinded != 1)
  {
    CIntVector orderIndices2;
    CByteBuffer byteBuffer;
    const size_t kBufferSize = (1 << 21);
    byteBuffer.SetCapacity(kBufferSize);
    RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
    size_t processedSize = kBufferSize;
    RINOK(ReadStream(inStream, byteBuffer, &processedSize));
    if (processedSize == 0)
      return S_FALSE;

    const Byte *buf = byteBuffer;
    Byte hash[1 << 16];
    memset(hash, 0xFF, 1 << 16);
    Byte prevs[256];
    if (orderIndices.Size() > 255)
      return S_FALSE;
    int i;
    for (i = 0; i < orderIndices.Size(); i++)
    {
      const CArcInfoEx &ai = codecs->Formats[orderIndices[i]];
      const CByteBuffer &sig = ai.StartSignature;
      if (sig.GetCapacity() < 2)
        continue;
      UInt32 v = sig[0] | ((UInt32)sig[1] << 8);
      prevs[i] = hash[v];
      hash[v] = (Byte)i;
    }

    processedSize--;
    for (UInt32 pos = 0; pos < processedSize; pos++)
    {
      for (; pos < processedSize && hash[buf[pos] | ((UInt32)buf[pos + 1] << 8)] == 0xFF; pos++);
      if (pos == processedSize)
        break;
      UInt32 v = buf[pos] | ((UInt32)buf[pos + 1] << 8);
      Byte *ptr = &hash[v];
      int i = *ptr;
      do
      {
        int index = orderIndices[i];
        const CArcInfoEx &ai = codecs->Formats[index];
        const CByteBuffer &sig = ai.StartSignature;
        if (sig.GetCapacity() != 0 && pos + sig.GetCapacity() <= processedSize + 1)
          if (TestSignature(buf + pos, sig, sig.GetCapacity()))
          {
            orderIndices2.Add(index);
            orderIndices[i] = 0xFF;
            *ptr = prevs[i];
          }
        ptr = &prevs[i];
        i = *ptr;
      }
      while (i != 0xFF);
    }
    
    for (i = 0; i < orderIndices.Size(); i++)
    {
      int val = orderIndices[i];
      if (val != 0xFF)
        orderIndices2.Add(val);
    }
    orderIndices = orderIndices2;

    if (orderIndices.Size() >= 2)
    {
      int isoIndex = codecs->FindFormatForArchiveType(L"iso");
      int udfIndex = codecs->FindFormatForArchiveType(L"udf");
      int iIso = -1;
      int iUdf = -1;
      for (int i = 0; i < orderIndices.Size(); i++)
      {
        if (orderIndices[i] == isoIndex) iIso = i;
        if (orderIndices[i] == udfIndex) iUdf = i;
      }
      if (iUdf == iIso + 1)
      {
        orderIndices[iUdf] = isoIndex;
        orderIndices[iIso] = udfIndex;
      }
    }
  }
  else if (extension == L"000" || extension == L"001")
  {
    CByteBuffer byteBuffer;
    const size_t kBufferSize = (1 << 10);
    byteBuffer.SetCapacity(kBufferSize);
    Byte *buffer = byteBuffer;
    RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
    size_t processedSize = kBufferSize;
    RINOK(ReadStream(inStream, buffer, &processedSize));
    if (processedSize >= 16)
    {
      Byte kRarHeader[] = {0x52 , 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00};
      if (TestSignature(buffer, kRarHeader, 7) && buffer[9] == 0x73 && (buffer[10] & 1) != 0)
      {
        for (int i = 0; i < orderIndices.Size(); i++)
        {
          int index = orderIndices[i];
          const CArcInfoEx &ai = codecs->Formats[index];
          if (ai.Name.CompareNoCase(L"rar") != 0)
            continue;
          orderIndices.Delete(i--);
          orderIndices.Insert(0, index);
          break;
        }
      }
    }
  }
  #endif
  }

  for(int i = 0; i < orderIndices.Size(); i++)
  {
    inStream->Seek(0, STREAM_SEEK_SET, NULL);

    CMyComPtr<IInArchive> archive;

    formatIndex = orderIndices[i];
    RINOK(codecs->CreateInArchive(formatIndex, archive));
    if (!archive)
      continue;

    #ifdef EXTERNAL_CODECS
    {
      CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo;
      archive.QueryInterface(IID_ISetCompressCodecsInfo, (void **)&setCompressCodecsInfo);
      if (setCompressCodecsInfo)
      {
        RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(codecs));
      }
    }
    #endif

    HRESULT result = archive->Open(inStream, &kMaxCheckStartPosition, openArchiveCallback);
    if (result == S_FALSE)
      continue;
    RINOK(result);
    *archiveResult = archive.Detach();
    const CArcInfoEx &format = codecs->Formats[formatIndex];
    if (format.Exts.Size() == 0)
    {
      defaultItemName = GetDefaultName2(fileName, L"", L"");
    }
    else
    {
      int subExtIndex = format.FindExtension(extension);
      if (subExtIndex < 0)
        subExtIndex = 0;
      defaultItemName = GetDefaultName2(fileName,
          format.Exts[subExtIndex].Ext,
          format.Exts[subExtIndex].AddExt);
    }
    return S_OK;
  }
  return S_FALSE;
}

HRESULT OpenArchive(
    CCodecs *codecs,
    int arcTypeIndex,
    const UString &filePath,
    IInArchive **archiveResult,
    int &formatIndex,
    UString &defaultItemName,
    IArchiveOpenCallback *openArchiveCallback)
{
  CInFileStream *inStreamSpec = new CInFileStream;
  CMyComPtr<IInStream> inStream(inStreamSpec);
  if (!inStreamSpec->Open(filePath))
    return GetLastError();
  return OpenArchive(codecs, arcTypeIndex, inStream, ExtractFileNameFromPath(filePath),
    archiveResult, formatIndex,
    defaultItemName, openArchiveCallback);
}

static void MakeDefaultName(UString &name)
{
  int dotPos = name.ReverseFind(L'.');
  if (dotPos < 0)
    return;
  UString ext = name.Mid(dotPos + 1);
  if (ext.IsEmpty())
    return;
  for (int pos = 0; pos < ext.Length(); pos++)
    if (ext[pos] < L'0' || ext[pos] > L'9')
      return;
  name = name.Left(dotPos);
}

HRESULT OpenArchive(
    CCodecs *codecs,
    const CIntVector &formatIndices,
    const UString &fileName,
    IInArchive **archive0,
    IInArchive **archive1,
    int &formatIndex0,
    int &formatIndex1,
    UString &defaultItemName0,
    UString &defaultItemName1,
    IArchiveOpenCallback *openArchiveCallback)
{
  if (formatIndices.Size() >= 3)
    return E_NOTIMPL;
  
  int arcTypeIndex = -1;
  if (formatIndices.Size() >= 1)
    arcTypeIndex = formatIndices[formatIndices.Size() - 1];
  
  HRESULT result = OpenArchive(codecs, arcTypeIndex, fileName,
    archive0, formatIndex0, defaultItemName0, openArchiveCallback);
  RINOK(result);

  if (formatIndices.Size() == 1)
    return S_OK;
  arcTypeIndex = -1;
  if (formatIndices.Size() >= 2)
    arcTypeIndex = formatIndices[formatIndices.Size() - 2];

  HRESULT resSpec = (formatIndices.Size() == 0 ? S_OK : E_NOTIMPL);

  CMyComPtr<IInArchiveGetStream> getStream;
  result = (*archive0)->QueryInterface(IID_IInArchiveGetStream, (void **)&getStream);
  if (result != S_OK || !getStream)
    return resSpec;

  CMyComPtr<ISequentialInStream> subSeqStream;
  result = getStream->GetStream(0, &subSeqStream);
  if (result != S_OK || !subSeqStream)
    return resSpec;

  CMyComPtr<IInStream> subStream;
  result = subSeqStream.QueryInterface(IID_IInStream, &subStream);
  if (result != S_OK || !subStream)
    return resSpec;

  UInt32 numItems;
  RINOK((*archive0)->GetNumberOfItems(&numItems));
  if (numItems < 1)
    return resSpec;

  UString subPath;
  RINOK(GetArchiveItemPath(*archive0, 0, subPath))
  if (subPath.IsEmpty())
  {
    MakeDefaultName(defaultItemName0);
    subPath = defaultItemName0;
    const CArcInfoEx &format = codecs->Formats[formatIndex0];
    if (format.Name.CompareNoCase(L"7z") == 0)
    {
      if (subPath.Right(3).CompareNoCase(L".7z") != 0)
        subPath += L".7z";
    }
  }
  else
    subPath = ExtractFileNameFromPath(subPath);

  CMyComPtr<IArchiveOpenSetSubArchiveName> setSubArchiveName;
  openArchiveCallback->QueryInterface(IID_IArchiveOpenSetSubArchiveName, (void **)&setSubArchiveName);
  if (setSubArchiveName)
    setSubArchiveName->SetSubArchiveName(subPath);

  result = OpenArchive(codecs, arcTypeIndex, subStream, subPath,
      archive1, formatIndex1, defaultItemName1, openArchiveCallback);
  resSpec = (formatIndices.Size() == 0 ? S_OK : S_FALSE);
  if (result != S_OK)
    return resSpec;
  return S_OK;
}

static void SetCallback(const UString &archiveName,
    IOpenCallbackUI *openCallbackUI,
    IArchiveOpenCallback *reOpenCallback,
    CMyComPtr<IArchiveOpenCallback> &openCallback)
{
  COpenCallbackImp *openCallbackSpec = new COpenCallbackImp;
  openCallback = openCallbackSpec;
  openCallbackSpec->Callback = openCallbackUI;
  openCallbackSpec->ReOpenCallback = reOpenCallback;

  UString fullName;
  int fileNamePartStartIndex;
  NFile::NDirectory::MyGetFullPathName(archiveName, fullName, fileNamePartStartIndex);
  openCallbackSpec->Init(
      fullName.Left(fileNamePartStartIndex),
      fullName.Mid(fileNamePartStartIndex));
}

HRESULT MyOpenArchive(
    CCodecs *codecs,
    int arcTypeIndex,
    const UString &archiveName,
    IInArchive **archive, UString &defaultItemName, IOpenCallbackUI *openCallbackUI)
{
  CMyComPtr<IArchiveOpenCallback> openCallback;
  SetCallback(archiveName, openCallbackUI, NULL, openCallback);
  int formatInfo;
  return OpenArchive(codecs, arcTypeIndex, archiveName, archive, formatInfo, defaultItemName, openCallback);
}

HRESULT MyOpenArchive(
    CCodecs *codecs,
    const CIntVector &formatIndices,
    const UString &archiveName,
    IInArchive **archive0,
    IInArchive **archive1,
    UString &defaultItemName0,
    UString &defaultItemName1,
    UStringVector &volumePaths,
    UInt64 &volumesSize,
    IOpenCallbackUI *openCallbackUI)
{
  volumesSize = 0;
  COpenCallbackImp *openCallbackSpec = new COpenCallbackImp;
  CMyComPtr<IArchiveOpenCallback> openCallback = openCallbackSpec;
  openCallbackSpec->Callback = openCallbackUI;

  UString fullName;
  int fileNamePartStartIndex;
  NFile::NDirectory::MyGetFullPathName(archiveName, fullName, fileNamePartStartIndex);
  UString prefix = fullName.Left(fileNamePartStartIndex);
  UString name = fullName.Mid(fileNamePartStartIndex);
  openCallbackSpec->Init(prefix, name);

  int formatIndex0, formatIndex1;
  RINOK(OpenArchive(codecs, formatIndices, archiveName,
      archive0,
      archive1,
      formatIndex0,
      formatIndex1,
      defaultItemName0,
      defaultItemName1,
      openCallback));
  volumePaths.Add(prefix + name);
  for (int i = 0; i < openCallbackSpec->FileNames.Size(); i++)
    volumePaths.Add(prefix + openCallbackSpec->FileNames[i]);
  volumesSize = openCallbackSpec->TotalSize;
  return S_OK;
}

HRESULT CArchiveLink::Close()
{
  if (Archive1 != 0)
    RINOK(Archive1->Close());
  if (Archive0 != 0)
    RINOK(Archive0->Close());
  IsOpen = false;
  return S_OK;
}

void CArchiveLink::Release()
{
  IsOpen = false;
  Archive1.Release();
  Archive0.Release();
}

HRESULT OpenArchive(
    CCodecs *codecs,
    const CIntVector &formatIndices,
    const UString &archiveName,
    CArchiveLink &archiveLink,
    IArchiveOpenCallback *openCallback)
{
  HRESULT res = OpenArchive(codecs, formatIndices, archiveName,
    &archiveLink.Archive0, &archiveLink.Archive1,
    archiveLink.FormatIndex0, archiveLink.FormatIndex1,
    archiveLink.DefaultItemName0, archiveLink.DefaultItemName1,
    openCallback);
  archiveLink.IsOpen = (res == S_OK);
  return res;
}

HRESULT MyOpenArchive(CCodecs *codecs,
    const CIntVector &formatIndices,
    const UString &archiveName,
    CArchiveLink &archiveLink,
    IOpenCallbackUI *openCallbackUI)
{
  HRESULT res = MyOpenArchive(codecs, formatIndices, archiveName,
    &archiveLink.Archive0, &archiveLink.Archive1,
    archiveLink.DefaultItemName0, archiveLink.DefaultItemName1,
    archiveLink.VolumePaths,
    archiveLink.VolumesSize,
    openCallbackUI);
  archiveLink.IsOpen = (res == S_OK);
  return res;
}

HRESULT ReOpenArchive(CCodecs *codecs, CArchiveLink &archiveLink, const UString &fileName,
    IArchiveOpenCallback *openCallback)
{
  if (archiveLink.GetNumLevels() > 1)
    return E_NOTIMPL;

  if (archiveLink.GetNumLevels() == 0)
    return MyOpenArchive(codecs, CIntVector(), fileName, archiveLink, 0);

  CMyComPtr<IArchiveOpenCallback> openCallbackNew;
  SetCallback(fileName, NULL, openCallback, openCallbackNew);

  HRESULT res = ReOpenArchive(archiveLink.GetArchive(), fileName, openCallbackNew);
  archiveLink.IsOpen = (res == S_OK);
  return res;
}
