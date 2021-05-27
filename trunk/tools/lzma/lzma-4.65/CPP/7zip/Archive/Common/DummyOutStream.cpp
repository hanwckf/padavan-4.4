// DummyOutStream.cpp

#include "StdAfx.h"

#include "DummyOutStream.h"

STDMETHODIMP CDummyOutStream::Write(const void *data,  UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  HRESULT result;
  if(!_stream)
  {
    realProcessedSize = size;
    result = S_OK;
  }
  else
    result = _stream->Write(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  return result;
}
