// StreamObjects.h

#ifndef __STREAMOBJECTS_H
#define __STREAMOBJECTS_H

#include "../../Common/DynamicBuffer.h"
#include "../../Common/MyCom.h"
#include "../IStream.h"

class CSequentialInStreamImp:
  public ISequentialInStream,
  public CMyUnknownImp
{
  const Byte *_dataPointer;
  size_t _size;
  size_t _pos;

public:
  void Init(const Byte *dataPointer, size_t size)
  {
    _dataPointer = dataPointer;
    _size = size;
    _pos = 0;
  }

  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
};


class CWriteBuffer
{
  CByteDynamicBuffer _buffer;
  size_t _size;
public:
  CWriteBuffer(): _size(0) {}
  void Init() { _size = 0;  }
  void Write(const void *data, size_t size);
  size_t GetSize() const { return _size; }
  const CByteDynamicBuffer& GetBuffer() const { return _buffer; }
};

class CSequentialOutStreamImp:
  public ISequentialOutStream,
  public CMyUnknownImp
{
  CWriteBuffer _writeBuffer;
public:
  void Init() { _writeBuffer.Init(); }
  size_t GetSize() const { return _writeBuffer.GetSize(); }
  const CByteDynamicBuffer& GetBuffer() const { return _writeBuffer.GetBuffer(); }

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};

class CSequentialOutStreamImp2:
  public ISequentialOutStream,
  public CMyUnknownImp
{
  Byte *_buffer;
  size_t _size;
  size_t _pos;
public:

  void Init(Byte *buffer, size_t size)
  {
    _buffer = buffer;
    _pos = 0;
    _size = size;
  }

  size_t GetPos() const { return _pos; }

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};

class CSequentialInStreamSizeCount:
  public ISequentialInStream,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialInStream> _stream;
  UInt64 _size;
public:
  void Init(ISequentialInStream *stream)
  {
    _stream = stream;
    _size = 0;
  }
  UInt64 GetSize() const { return _size; }

  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
};

class CSequentialOutStreamSizeCount:
  public ISequentialOutStream,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialOutStream> _stream;
  UInt64 _size;
public:
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void Init() { _size = 0; }
  UInt64 GetSize() const { return _size; }

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};

#endif
