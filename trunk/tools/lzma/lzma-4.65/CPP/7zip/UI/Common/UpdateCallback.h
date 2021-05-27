// UpdateCallback.h

#ifndef __UPDATECALLBACK_H
#define __UPDATECALLBACK_H

#include "Common/MyCom.h"
#include "Common/MyString.h"

#include "../../IPassword.h"
#include "../../ICoder.h"

#include "../Common/UpdatePair.h"
#include "../Common/UpdateProduce.h"

#define INTERFACE_IUpdateCallbackUI(x) \
  virtual HRESULT SetTotal(UInt64 size) x; \
  virtual HRESULT SetCompleted(const UInt64 *completeValue) x; \
  virtual HRESULT SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize) x; \
  virtual HRESULT CheckBreak() x; \
  virtual HRESULT Finilize() x; \
  virtual HRESULT SetNumFiles(UInt64 numFiles) x; \
  virtual HRESULT GetStream(const wchar_t *name, bool isAnti) x; \
  virtual HRESULT OpenFileError(const wchar_t *name, DWORD systemError) x; \
  virtual HRESULT SetOperationResult(Int32 operationResult) x; \
  virtual HRESULT CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password) x; \
  // virtual HRESULT ShowDeleteFile(const wchar_t *name) x; \
  // virtual HRESULT CloseProgress() { return S_OK; };

struct IUpdateCallbackUI
{
  INTERFACE_IUpdateCallbackUI(=0)
};

class CArchiveUpdateCallback:
  public IArchiveUpdateCallback2,
  public ICryptoGetTextPassword2,
  public ICompressProgressInfo,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP3(
      IArchiveUpdateCallback2,
      ICryptoGetTextPassword2,
      ICompressProgressInfo)

  STDMETHOD(SetRatioInfo)(const UInt64 *inSize, const UInt64 *outSize);

  INTERFACE_IArchiveUpdateCallback2(;)

  STDMETHOD(CryptoGetTextPassword2)(Int32 *passwordIsDefined, BSTR *password);

public:
  CRecordVector<UInt64> VolumesSizes;
  UString VolName;
  UString VolExt;

  IUpdateCallbackUI *Callback;

  bool ShareForWrite;
  bool StdInMode;
  const CDirItems *DirItems;
  const CObjectVector<CArcItem> *ArcItems;
  const CRecordVector<CUpdatePair2> *UpdatePairs;
  const UStringVector *NewNames;
  CMyComPtr<IInArchive> Archive;

  CArchiveUpdateCallback();
};

#endif
