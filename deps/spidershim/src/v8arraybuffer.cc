// Copyright Mozilla Foundation. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include "v8.h"
#include "v8conversions.h"
#include "conversions.h"
#include "v8local.h"
#include "jsapi.h"
#include "jsfriendapi.h"
#include "js/Conversions.h"

namespace v8 {

ArrayBuffer*
ArrayBuffer::Cast(Value* val)
{
  assert(val->IsArrayBuffer());
  return static_cast<ArrayBuffer*>(val);
}

Local<ArrayBuffer>
ArrayBuffer::New(Isolate* isolate, size_t size)
{
  JSContext* cx = JSContextFromIsolate(isolate);
  JSObject* buf = JS_NewArrayBuffer(cx, size);
  JS::Value val;
  val.setObject(*buf);
  return internal::Local<ArrayBuffer>::New(isolate, val);
}

Local<ArrayBuffer>
ArrayBuffer::New(Isolate* isolate, void* data, size_t size,
	       	ArrayBufferCreationMode mode)
{
  JSContext* cx = JSContextFromIsolate(isolate);
  JSObject* buf;
  if (mode == ArrayBufferCreationMode::kExternalized) {
	  buf = JS_NewArrayBufferWithExternalContents(cx, size, data);
  } else {
	  buf = JS_NewArrayBufferWithContents(cx, size, data);
  }
  JS::Value val;
  val.setObject(*buf);
  return internal::Local<ArrayBuffer>::New(isolate, val);
}

size_t
ArrayBuffer::ByteLength() const
{
  const JS::Value* val = GetValue(this);
  JSObject& obj = val->toObject();
  return JS_GetArrayBufferByteLength(&obj);
}

ArrayBuffer::Contents
ArrayBuffer::GetContents()
{
  const JS::Value thisVal = *GetValue(this);
  JSObject* obj = &thisVal.toObject();
  uint8_t *data;
  bool shared;
  uint32_t length;
  js::GetArrayBufferLengthAndData(obj, &length, &shared, &data);
  Contents contents;
  contents.data_ = static_cast<void*>(data);
  contents.byte_length_ = length;
  return contents;
}

Local<ArrayBuffer>
ArrayBufferView::Buffer()
{
  Isolate* isolate = GetIsolate();
  JSContext* cx = JSContextFromIsolate(isolate);
  JS::RootedObject view(cx, &reinterpret_cast<JS::Value*>(this)->toObject());
  bool shared;
  JSObject* buf = JS_GetArrayBufferViewBuffer(cx, view, &shared);
  if (!buf) {
    return Local<ArrayBuffer>();
  }

  JS::Value bufVal;
  bufVal.setObject(*buf);
  return internal::Local<ArrayBuffer>::New(isolate, bufVal);
}

size_t
ArrayBufferView::ByteOffset()
{
  JSObject* view = &reinterpret_cast<JS::Value*>(this)->toObject();
  if (JS_IsTypedArrayObject(view)) {
    return JS_GetTypedArrayByteOffset(view);
  }

  return JS_GetDataViewByteOffset(view);
}

size_t
ArrayBufferView::ByteLength()
{
  JSObject* view = &reinterpret_cast<JS::Value*>(this)->toObject();
  if (JS_IsTypedArrayObject(view)) {
    return JS_GetTypedArrayByteLength(view);
  }

  return JS_GetDataViewByteLength(view);
}

void
ArrayBuffer::Neuter()
{
  JSContext* cx = JSContextFromIsolate(Isolate::GetCurrent());
  JS::RootedObject obj(cx, &reinterpret_cast<JS::Value*>(this)->toObject());
  JS_DetachArrayBuffer(cx, obj, KeepData);
}

#define ES_BUILTIN(X, Y)
#define COMMON_VALUE(X)
#define TYPED_ARRAY(TYPE) \
  Local<TYPE ## Array> \
  TYPE ## Array::New(Handle<ArrayBuffer> buffer, size_t offset, size_t length) \
  { \
    Isolate* isolate = buffer->GetIsolate(); \
    JSContext* cx = JSContextFromIsolate(isolate); \
    JS::RootedObject buf(cx, &reinterpret_cast<JS::Value*>(*buffer)->toObject()); \
    JSObject* array = JS_New ## TYPE ## ArrayWithBuffer(cx, buf, offset, length); \
    if (!array) { \
      return Local<TYPE ## Array>(); \
    } \
    JS::Value arrayVal; \
    arrayVal.setObject(*array); \
    return internal::Local<TYPE ## Array>::New(isolate, arrayVal); \
  } \
\
  TYPE ## Array* \
  TYPE ## Array::Cast(Value* v) \
  { \
    assert(v->Is ## TYPE ## Array()); \
    return static_cast<TYPE ## Array*>(v); \
  }

#include "valuemap.inc"
#undef TYPED_ARRAY
#undef ES_BUILTIN
#undef COMMON_VALUE
}
