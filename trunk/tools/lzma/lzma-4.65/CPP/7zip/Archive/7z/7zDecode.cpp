// 7zDecode.cpp

#include "StdAfx.h"

#include "../../Common/LimitedStreams.h"
#include "../../Common/LockedStream.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/StreamObjects.h"

#include "7zDecode.h"

namespace NArchive {
namespace N7z {

static void ConvertFolderItemInfoToBindInfo(const CFolder &folder,
    CBindInfoEx &bindInfo)
{
  bindInfo.Clear();
  int i;
  for (i = 0; i < folder.BindPairs.Size(); i++)
  {
    NCoderMixer::CBindPair bindPair;
    bindPair.InIndex = (UInt32)folder.BindPairs[i].InIndex;
    bindPair.OutIndex = (UInt32)folder.BindPairs[i].OutIndex;
    bindInfo.BindPairs.Add(bindPair);
  }
  UInt32 outStreamIndex = 0;
  for (i = 0; i < folder.Coders.Size(); i++)
  {
    NCoderMixer::CCoderStreamsInfo coderStreamsInfo;
    const CCoderInfo &coderInfo = folder.Coders[i];
    coderStreamsInfo.NumInStreams = (UInt32)coderInfo.NumInStreams;
    coderStreamsInfo.NumOutStreams = (UInt32)coderInfo.NumOutStreams;
    bindInfo.Coders.Add(coderStreamsInfo);
    bindInfo.CoderMethodIDs.Add(coderInfo.MethodID);
    for (UInt32 j = 0; j < coderStreamsInfo.NumOutStreams; j++, outStreamIndex++)
      if (folder.FindBindPairForOutStream(outStreamIndex) < 0)
        bindInfo.OutStreams.Add(outStreamIndex);
  }
  for (i = 0; i < folder.PackStreams.Size(); i++)
    bindInfo.InStreams.Add((UInt32)folder.PackStreams[i]);
}

static bool AreCodersEqual(const NCoderMixer::CCoderStreamsInfo &a1,
    const NCoderMixer::CCoderStreamsInfo &a2)
{
  return (a1.NumInStreams == a2.NumInStreams) &&
    (a1.NumOutStreams == a2.NumOutStreams);
}

static bool AreBindPairsEqual(const NCoderMixer::CBindPair &a1, const NCoderMixer::CBindPair &a2)
{
  return (a1.InIndex == a2.InIndex) &&
    (a1.OutIndex == a2.OutIndex);
}

static bool AreBindInfoExEqual(const CBindInfoEx &a1, const CBindInfoEx &a2)
{
  if (a1.Coders.Size() != a2.Coders.Size())
    return false;
  int i;
  for (i = 0; i < a1.Coders.Size(); i++)
    if (!AreCodersEqual(a1.Coders[i], a2.Coders[i]))
      return false;
  if (a1.BindPairs.Size() != a2.BindPairs.Size())
    return false;
  for (i = 0; i < a1.BindPairs.Size(); i++)
    if (!AreBindPairsEqual(a1.BindPairs[i], a2.BindPairs[i]))
      return false;
  for (i = 0; i < a1.CoderMethodIDs.Size(); i++)
    if (a1.CoderMethodIDs[i] != a2.CoderMethodIDs[i])
      return false;
  if (a1.InStreams.Size() != a2.InStreams.Size())
    return false;
  if (a1.OutStreams.Size() != a2.OutStreams.Size())
    return false;
  return true;
}

CDecoder::CDecoder(bool multiThread)
{
  #ifndef _ST_MODE
  multiThread = true;
  #endif
  _multiThread = multiThread;
  _bindInfoExPrevIsDefined = false;
}

HRESULT CDecoder::Decode(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IInStream *inStream,
    UInt64 startPos,
    const UInt64 *packSizes,
    const CFolder &folderInfo,
    ISequentialOutStream *outStream,
    ICompressProgressInfo *compressProgress
    #ifndef _NO_CRYPTO
    , ICryptoGetTextPassword *getTextPassword, bool &passwordIsDefined
    #endif
    #ifdef COMPRESS_MT
    , bool mtMode, UInt32 numThreads
    #endif
    )
{
  if (!folderInfo.CheckStructure())
    return E_NOTIMPL;
  #ifndef _NO_CRYPTO
  passwordIsDefined = false;
  #endif
  CObjectVector< CMyComPtr<ISequentialInStream> > inStreams;
  
  CLockedInStream lockedInStream;
  lockedInStream.Init(inStream);
  
  for (int j = 0; j < folderInfo.PackStreams.Size(); j++)
  {
    CLockedSequentialInStreamImp *lockedStreamImpSpec = new
        CLockedSequentialInStreamImp;
    CMyComPtr<ISequentialInStream> lockedStreamImp = lockedStreamImpSpec;
    lockedStreamImpSpec->Init(&lockedInStream, startPos);
    startPos += packSizes[j];
    
    CLimitedSequentialInStream *streamSpec = new
        CLimitedSequentialInStream;
    CMyComPtr<ISequentialInStream> inStream = streamSpec;
    streamSpec->SetStream(lockedStreamImp);
    streamSpec->Init(packSizes[j]);
    inStreams.Add(inStream);
  }
  
  int numCoders = folderInfo.Coders.Size();
  
  CBindInfoEx bindInfo;
  ConvertFolderItemInfoToBindInfo(folderInfo, bindInfo);
  bool createNewCoders;
  if (!_bindInfoExPrevIsDefined)
    createNewCoders = true;
  else
    createNewCoders = !AreBindInfoExEqual(bindInfo, _bindInfoExPrev);
  if (createNewCoders)
  {
    int i;
    _decoders.Clear();
    // _decoders2.Clear();
    
    _mixerCoder.Release();

    if (_multiThread)
    {
      _mixerCoderMTSpec = new NCoderMixer::CCoderMixer2MT;
      _mixerCoder = _mixerCoderMTSpec;
      _mixerCoderCommon = _mixerCoderMTSpec;
    }
    else
    {
      #ifdef _ST_MODE
      _mixerCoderSTSpec = new NCoderMixer::CCoderMixer2ST;
      _mixerCoder = _mixerCoderSTSpec;
      _mixerCoderCommon = _mixerCoderSTSpec;
      #endif
    }
    RINOK(_mixerCoderCommon->SetBindInfo(bindInfo));
    
    for (i = 0; i < numCoders; i++)
    {
      const CCoderInfo &coderInfo = folderInfo.Coders[i];

  
      CMyComPtr<ICompressCoder> decoder;
      CMyComPtr<ICompressCoder2> decoder2;
      RINOK(CreateCoder(
          EXTERNAL_CODECS_LOC_VARS
          coderInfo.MethodID, decoder, decoder2, false));
      CMyComPtr<IUnknown> decoderUnknown;
      if (coderInfo.IsSimpleCoder())
      {
        if (decoder == 0)
          return E_NOTIMPL;

        decoderUnknown = (IUnknown *)decoder;
        
        if (_multiThread)
          _mixerCoderMTSpec->AddCoder(decoder);
        #ifdef _ST_MODE
        else
          _mixerCoderSTSpec->AddCoder(decoder, false);
        #endif
      }
      else
      {
        if (decoder2 == 0)
          return E_NOTIMPL;
        decoderUnknown = (IUnknown *)decoder2;
        if (_multiThread)
          _mixerCoderMTSpec->AddCoder2(decoder2);
        #ifdef _ST_MODE
        else
          _mixerCoderSTSpec->AddCoder2(decoder2, false);
        #endif
      }
      _decoders.Add(decoderUnknown);
      #ifdef EXTERNAL_CODECS
      CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo;
      decoderUnknown.QueryInterface(IID_ISetCompressCodecsInfo, (void **)&setCompressCodecsInfo);
      if (setCompressCodecsInfo)
      {
        RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(codecsInfo));
      }
      #endif
    }
    _bindInfoExPrev = bindInfo;
    _bindInfoExPrevIsDefined = true;
  }
  int i;
  _mixerCoderCommon->ReInit();
  
  UInt32 packStreamIndex = 0, unpackStreamIndex = 0;
  UInt32 coderIndex = 0;
  // UInt32 coder2Index = 0;
  
  for (i = 0; i < numCoders; i++)
  {
    const CCoderInfo &coderInfo = folderInfo.Coders[i];
    CMyComPtr<IUnknown> &decoder = _decoders[coderIndex];
    
    {
      CMyComPtr<ICompressSetDecoderProperties2> setDecoderProperties;
      decoder.QueryInterface(IID_ICompressSetDecoderProperties2, &setDecoderProperties);
      if (setDecoderProperties)
      {
        const CByteBuffer &props = coderInfo.Props;
        size_t size = props.GetCapacity();
        if (size > 0xFFFFFFFF)
          return E_NOTIMPL;
        if (size > 0)
        {
          RINOK(setDecoderProperties->SetDecoderProperties2((const Byte *)props, (UInt32)size));
        }
      }
    }

    #ifdef COMPRESS_MT
    if (mtMode)
    {
      CMyComPtr<ICompressSetCoderMt> setCoderMt;
      decoder.QueryInterface(IID_ICompressSetCoderMt, &setCoderMt);
      if (setCoderMt)
      {
        RINOK(setCoderMt->SetNumberOfThreads(numThreads));
      }
    }
    #endif

    #ifndef _NO_CRYPTO
    {
      CMyComPtr<ICryptoSetPassword> cryptoSetPassword;
      decoder.QueryInterface(IID_ICryptoSetPassword, &cryptoSetPassword);
      if (cryptoSetPassword)
      {
        if (getTextPassword == 0)
          return E_FAIL;
        CMyComBSTR passwordBSTR;
        RINOK(getTextPassword->CryptoGetTextPassword(&passwordBSTR));
        CByteBuffer buffer;
        passwordIsDefined = true;
        const UString password(passwordBSTR);
        const UInt32 sizeInBytes = password.Length() * 2;
        buffer.SetCapacity(sizeInBytes);
        for (int i = 0; i < password.Length(); i++)
        {
          wchar_t c = password[i];
          ((Byte *)buffer)[i * 2] = (Byte)c;
          ((Byte *)buffer)[i * 2 + 1] = (Byte)(c >> 8);
        }
        RINOK(cryptoSetPassword->CryptoSetPassword((const Byte *)buffer, sizeInBytes));
      }
    }
    #endif

    coderIndex++;
    
    UInt32 numInStreams = (UInt32)coderInfo.NumInStreams;
    UInt32 numOutStreams = (UInt32)coderInfo.NumOutStreams;
    CRecordVector<const UInt64 *> packSizesPointers;
    CRecordVector<const UInt64 *> unpackSizesPointers;
    packSizesPointers.Reserve(numInStreams);
    unpackSizesPointers.Reserve(numOutStreams);
    UInt32 j;
    for (j = 0; j < numOutStreams; j++, unpackStreamIndex++)
      unpackSizesPointers.Add(&folderInfo.UnpackSizes[unpackStreamIndex]);
    
    for (j = 0; j < numInStreams; j++, packStreamIndex++)
    {
      int bindPairIndex = folderInfo.FindBindPairForInStream(packStreamIndex);
      if (bindPairIndex >= 0)
        packSizesPointers.Add(
        &folderInfo.UnpackSizes[(UInt32)folderInfo.BindPairs[bindPairIndex].OutIndex]);
      else
      {
        int index = folderInfo.FindPackStreamArrayIndex(packStreamIndex);
        if (index < 0)
          return E_FAIL;
        packSizesPointers.Add(&packSizes[index]);
      }
    }
    
    _mixerCoderCommon->SetCoderInfo(i,
        &packSizesPointers.Front(),
        &unpackSizesPointers.Front());
  }
  UInt32 mainCoder, temp;
  bindInfo.FindOutStream(bindInfo.OutStreams[0], mainCoder, temp);

  if (_multiThread)
    _mixerCoderMTSpec->SetProgressCoderIndex(mainCoder);
  /*
  else
    _mixerCoderSTSpec->SetProgressCoderIndex(mainCoder);;
  */
  
  if (numCoders == 0)
    return 0;
  CRecordVector<ISequentialInStream *> inStreamPointers;
  inStreamPointers.Reserve(inStreams.Size());
  for (i = 0; i < inStreams.Size(); i++)
    inStreamPointers.Add(inStreams[i]);
  ISequentialOutStream *outStreamPointer = outStream;
  return _mixerCoder->Code(&inStreamPointers.Front(), NULL,
    inStreams.Size(), &outStreamPointer, NULL, 1, compressProgress);
}

}}
