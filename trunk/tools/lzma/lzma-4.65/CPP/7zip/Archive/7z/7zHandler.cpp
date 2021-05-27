// 7zHandler.cpp

#include "StdAfx.h"

extern "C"
{
  #include "../../../../C/CpuArch.h"
}

#include "../../../Common/ComTry.h"
#include "../../../Common/IntToString.h"

#ifdef COMPRESS_MT
#include "../../../Windows/System.h"
#endif

#include "../Common/ItemNameUtils.h"

#include "7zHandler.h"
#include "7zProperties.h"

#ifdef __7Z_SET_PROPERTIES
#ifdef EXTRACT_ONLY
#include "../Common/ParseProperties.h"
#endif
#endif

using namespace NWindows;

extern UString ConvertMethodIdToString(UInt64 id);

namespace NArchive {
namespace N7z {

CHandler::CHandler()
{
  _crcSize = 4;

  #ifndef _NO_CRYPTO
  _passwordIsDefined = false;
  #endif

  #ifdef EXTRACT_ONLY
  #ifdef COMPRESS_MT
  _numThreads = NSystem::GetNumberOfProcessors();
  #endif
  #else
  Init();
  #endif
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _db.Files.Size();
  return S_OK;
}

#ifdef _SFX

IMP_IInArchive_ArcProps_NO

STDMETHODIMP CHandler::GetNumberOfProperties(UInt32 * /* numProperties */)
{
  return E_NOTIMPL;
}

STDMETHODIMP CHandler::GetPropertyInfo(UInt32 /* index */,
      BSTR * /* name */, PROPID * /* propID */, VARTYPE * /* varType */)
{
  return E_NOTIMPL;
}


#else

STATPROPSTG kArcProps[] =
{
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidSolid, VT_BOOL},
  { NULL, kpidNumBlocks, VT_UI4},
  { NULL, kpidPhySize, VT_UI8},
  { NULL, kpidHeadersSize, VT_UI8},
  { NULL, kpidOffset, VT_UI8}
};

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidMethod:
    {
      UString resString;
      CRecordVector<UInt64> ids;
      int i;
      for (i = 0; i < _db.Folders.Size(); i++)
      {
        const CFolder &f = _db.Folders[i];
        for (int j = f.Coders.Size() - 1; j >= 0; j--)
          ids.AddToUniqueSorted(f.Coders[j].MethodID);
      }

      for (i = 0; i < ids.Size(); i++)
      {
        UInt64 id = ids[i];
        UString methodName;
        /* bool methodIsKnown = */ FindMethod(EXTERNAL_CODECS_VARS id, methodName);
        if (methodName.IsEmpty())
          methodName = ConvertMethodIdToString(id);
        if (!resString.IsEmpty())
          resString += L' ';
        resString += methodName;
      }
      prop = resString;
      break;
    }
    case kpidSolid: prop = _db.IsSolid(); break;
    case kpidNumBlocks: prop = (UInt32)_db.Folders.Size(); break;
    case kpidHeadersSize:  prop = _db.HeadersSize; break;
    case kpidPhySize:  prop = _db.PhySize; break;
    case kpidOffset: if (_db.ArchiveInfo.StartPosition != 0) prop = _db.ArchiveInfo.StartPosition; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

IMP_IInArchive_ArcProps

#endif

static void SetPropFromUInt64Def(CUInt64DefVector &v, int index, NCOM::CPropVariant &prop)
{
  UInt64 value;
  if (v.GetItem(index, value))
  {
    FILETIME ft;
    ft.dwLowDateTime = (DWORD)value;
    ft.dwHighDateTime = (DWORD)(value >> 32);
    prop = ft;
  }
}

#ifndef _SFX

static UString ConvertUInt32ToString(UInt32 value)
{
  wchar_t buffer[32];
  ConvertUInt64ToString(value, buffer);
  return buffer;
}

static UString GetStringForSizeValue(UInt32 value)
{
  for (int i = 31; i >= 0; i--)
    if ((UInt32(1) << i) == value)
      return ConvertUInt32ToString(i);
  UString result;
  if (value % (1 << 20) == 0)
  {
    result += ConvertUInt32ToString(value >> 20);
    result += L"m";
  }
  else if (value % (1 << 10) == 0)
  {
    result += ConvertUInt32ToString(value >> 10);
    result += L"k";
  }
  else
  {
    result += ConvertUInt32ToString(value);
    result += L"b";
  }
  return result;
}

static const UInt64 k_Copy = 0x0;
static const UInt64 k_LZMA  = 0x030101;
static const UInt64 k_PPMD  = 0x030401;

static wchar_t GetHex(Byte value)
{
  return (wchar_t)((value < 10) ? (L'0' + value) : (L'A' + (value - 10)));
}
static inline UString GetHex2(Byte value)
{
  UString result;
  result += GetHex((Byte)(value >> 4));
  result += GetHex((Byte)(value & 0xF));
  return result;
}

#endif

static const UInt64 k_AES  = 0x06F10701;

bool CHandler::IsEncrypted(UInt32 index2) const
{
  CNum folderIndex = _db.FileIndexToFolderIndexMap[index2];
  if (folderIndex != kNumNoIndex)
  {
    const CFolder &folderInfo = _db.Folders[folderIndex];
    for (int i = folderInfo.Coders.Size() - 1; i >= 0; i--)
      if (folderInfo.Coders[i].MethodID == k_AES)
        return true;
  }
  return false;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NCOM::CPropVariant prop;
  
  /*
  const CRef2 &ref2 = _refs[index];
  if (ref2.Refs.IsEmpty())
    return E_FAIL;
  const CRef &ref = ref2.Refs.Front();
  */
  
  const CFileItem &item = _db.Files[index];
  UInt32 index2 = index;

  switch(propID)
  {
    case kpidPath:
      if (!item.Name.IsEmpty())
        prop = NItemName::GetOSName(item.Name);
      break;
    case kpidIsDir:  prop = item.IsDir; break;
    case kpidSize:
    {
      prop = item.Size;
      // prop = ref2.Size;
      break;
    }
    case kpidPackSize:
    {
      // prop = ref2.PackSize;
      {
        CNum folderIndex = _db.FileIndexToFolderIndexMap[index2];
        if (folderIndex != kNumNoIndex)
        {
          if (_db.FolderStartFileIndex[folderIndex] == (CNum)index2)
            prop = _db.GetFolderFullPackSize(folderIndex);
          /*
          else
            prop = (UInt64)0;
          */
        }
        else
          prop = (UInt64)0;
      }
      break;
    }
    case kpidPosition:  { UInt64 v; if (_db.StartPos.GetItem(index2, v)) prop = v; break; }
    case kpidCTime:  SetPropFromUInt64Def(_db.CTime, index2, prop); break;
    case kpidATime:  SetPropFromUInt64Def(_db.ATime, index2, prop); break;
    case kpidMTime:  SetPropFromUInt64Def(_db.MTime, index2, prop); break;
    case kpidAttrib:  if (item.AttribDefined) prop = item.Attrib; break;
    case kpidCRC:  if (item.CrcDefined) prop = item.Crc; break;
    case kpidEncrypted:  prop = IsEncrypted(index2); break;
    case kpidIsAnti:  prop = _db.IsItemAnti(index2); break;
    #ifndef _SFX
    case kpidMethod:
      {
        CNum folderIndex = _db.FileIndexToFolderIndexMap[index2];
        if (folderIndex != kNumNoIndex)
        {
          const CFolder &folderInfo = _db.Folders[folderIndex];
          UString methodsString;
          for (int i = folderInfo.Coders.Size() - 1; i >= 0; i--)
          {
            const CCoderInfo &coderInfo = folderInfo.Coders[i];
            if (!methodsString.IsEmpty())
              methodsString += L' ';

            {
              UString methodName;
              bool methodIsKnown = FindMethod(
                  EXTERNAL_CODECS_VARS
                  coderInfo.MethodID, methodName);

              if (methodIsKnown)
              {
                methodsString += methodName;
                if (coderInfo.MethodID == k_LZMA)
                {
                  if (coderInfo.Props.GetCapacity() >= 5)
                  {
                    methodsString += L":";
                    UInt32 dicSize = GetUi32((const Byte *)coderInfo.Props + 1);
                    methodsString += GetStringForSizeValue(dicSize);
                  }
                }
                else if (coderInfo.MethodID == k_PPMD)
                {
                  if (coderInfo.Props.GetCapacity() >= 5)
                  {
                    Byte order = *(const Byte *)coderInfo.Props;
                    methodsString += L":o";
                    methodsString += ConvertUInt32ToString(order);
                    methodsString += L":mem";
                    UInt32 dicSize = GetUi32((const Byte *)coderInfo.Props + 1);
                    methodsString += GetStringForSizeValue(dicSize);
                  }
                }
                else if (coderInfo.MethodID == k_AES)
                {
                  if (coderInfo.Props.GetCapacity() >= 1)
                  {
                    methodsString += L":";
                    const Byte *data = (const Byte *)coderInfo.Props;
                    Byte firstByte = *data++;
                    UInt32 numCyclesPower = firstByte & 0x3F;
                    methodsString += ConvertUInt32ToString(numCyclesPower);
                    /*
                    if ((firstByte & 0xC0) != 0)
                    {
                      methodsString += L":";
                      return S_OK;
                      UInt32 saltSize = (firstByte >> 7) & 1;
                      UInt32 ivSize = (firstByte >> 6) & 1;
                      if (coderInfo.Props.GetCapacity() >= 2)
                      {
                        Byte secondByte = *data++;
                        saltSize += (secondByte >> 4);
                        ivSize += (secondByte & 0x0F);
                      }
                    }
                    */
                  }
                }
                else
                {
                  if (coderInfo.Props.GetCapacity() > 0)
                  {
                    methodsString += L":[";
                    for (size_t bi = 0; bi < coderInfo.Props.GetCapacity(); bi++)
                    {
                      if (bi > 5 && bi + 1 < coderInfo.Props.GetCapacity())
                      {
                        methodsString += L"..";
                        break;
                      }
                      else
                        methodsString += GetHex2(coderInfo.Props[bi]);
                    }
                    methodsString += L"]";
                  }
                }
              }
              else
              {
                methodsString += ConvertMethodIdToString(coderInfo.MethodID);
              }
            }
          }
          prop = methodsString;
        }
      }
      break;
    case kpidBlock:
      {
        CNum folderIndex = _db.FileIndexToFolderIndexMap[index2];
        if (folderIndex != kNumNoIndex)
          prop = (UInt32)folderIndex;
      }
      break;
    case kpidPackedSize0:
    case kpidPackedSize1:
    case kpidPackedSize2:
    case kpidPackedSize3:
    case kpidPackedSize4:
      {
        CNum folderIndex = _db.FileIndexToFolderIndexMap[index2];
        if (folderIndex != kNumNoIndex)
        {
          const CFolder &folderInfo = _db.Folders[folderIndex];
          if (_db.FolderStartFileIndex[folderIndex] == (CNum)index2 &&
              folderInfo.PackStreams.Size() > (int)(propID - kpidPackedSize0))
          {
            prop = _db.GetFolderPackStreamSize(folderIndex, propID - kpidPackedSize0);
          }
          else
            prop = (UInt64)0;
        }
        else
          prop = (UInt64)0;
      }
      break;
    #endif
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  Close();
  #ifndef _SFX
  _fileInfoPopIDs.Clear();
  #endif
  try
  {
    CMyComPtr<IArchiveOpenCallback> openArchiveCallbackTemp = openArchiveCallback;

    #ifndef _NO_CRYPTO
    CMyComPtr<ICryptoGetTextPassword> getTextPassword;
    if (openArchiveCallback)
    {
      openArchiveCallbackTemp.QueryInterface(
          IID_ICryptoGetTextPassword, &getTextPassword);
    }
    #endif
    CInArchive archive;
    RINOK(archive.Open(stream, maxCheckStartPosition));
    #ifndef _NO_CRYPTO
    _passwordIsDefined = false;
    UString password;
    #endif
    HRESULT result = archive.ReadDatabase(
      EXTERNAL_CODECS_VARS
      _db
      #ifndef _NO_CRYPTO
      , getTextPassword, _passwordIsDefined
      #endif
      );
    RINOK(result);
    _db.Fill();
    _inStream = stream;
  }
  catch(...)
  {
    Close();
    return S_FALSE;
  }
  // _inStream = stream;
  #ifndef _SFX
  FillPopIDs();
  #endif
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  COM_TRY_BEGIN
  _inStream.Release();
  _db.Clear();
  return S_OK;
  COM_TRY_END
}

#ifdef __7Z_SET_PROPERTIES
#ifdef EXTRACT_ONLY

STDMETHODIMP CHandler::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties)
{
  COM_TRY_BEGIN
  #ifdef COMPRESS_MT
  const UInt32 numProcessors = NSystem::GetNumberOfProcessors();
  _numThreads = numProcessors;
  #endif

  for (int i = 0; i < numProperties; i++)
  {
    UString name = names[i];
    name.MakeUpper();
    if (name.IsEmpty())
      return E_INVALIDARG;
    const PROPVARIANT &value = values[i];
    UInt32 number;
    int index = ParseStringToUInt32(name, number);
    if (index == 0)
    {
      if(name.Left(2).CompareNoCase(L"MT") == 0)
      {
        #ifdef COMPRESS_MT
        RINOK(ParseMtProp(name.Mid(2), value, numProcessors, _numThreads));
        #endif
        continue;
      }
      else
        return E_INVALIDARG;
    }
  }
  return S_OK;
  COM_TRY_END
}

#endif
#endif

IMPL_ISetCompressCodecsInfo

}}
