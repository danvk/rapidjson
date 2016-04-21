#include "include/rapidjson/document.h"
#include "include/rapidjson/filereadstream.h"
#include "include/rapidjson/filewritestream.h"
#include "include/rapidjson/reader.h"
#include "include/rapidjson/writer.h"
#include "include/rapidjson/internal/meta.h"
#include "include/rapidjson/internal/stack.h"
#include "include/rapidjson/allocators.h"
#include "include/rapidjson/error/error.h"

#include <new>      // placement new

#include <string>
#include <iostream>
#include <algorithm>  // std::max

using namespace rapidjson;
using namespace std;

/**
 * For tomorrow:
 * Create a subclass of GenericDocument with a modified Handler.
 * When it gets to a "coordinates" key, it flips a bit.
 * When this bit is flipped, value methods are no-ops.
 *    StartArray() / EndArray() / StartObject() / EndObject() track depth.
 *    Once EndArray() / EndObject() gets back to depth zero, the bit is flipped back.
 *
 * Handlers should look like:
 *
 * bool Null() {
 *   if (!skipping) return GenericDocument::Null();
 *   // ...
 *   return true;
 * }
 */

template <typename Allocator = MemoryPoolAllocator<>, typename StackAllocator = CrtAllocator>
struct FilteredDocument : Value {
  bool skipping;
  int skipDepth;

  static const size_t kDefaultStackCapacity = 1024;
  Allocator* allocator_;
  Allocator* ownAllocator_;
  internal::Stack<StackAllocator> stack_;
  ParseResult parseResult_;

  FilteredDocument(size_t stackCapacity = kDefaultStackCapacity) : 
      allocator_(0), ownAllocator_(0), stack_(0, stackCapacity), parseResult_()
  {
    if (!allocator_) {
      ownAllocator_ = allocator_ = RAPIDJSON_NEW(Allocator());
    }
  }

  // clear stack on any exit from ParseStream, e.g. due to exception
  struct ClearStackOnExit {
      explicit ClearStackOnExit(FilteredDocument& d) : d_(d) {}
      ~ClearStackOnExit() { d_.ClearStack(); }
    private:
      ClearStackOnExit(const ClearStackOnExit&);
      ClearStackOnExit& operator=(const ClearStackOnExit&);
      FilteredDocument& d_;
  };

  void ClearStack() {
    if (Allocator::kNeedFree)
      while (stack_.GetSize() > 0)    // Here assumes all elements in stack array are GenericValue (Member is actually 2 GenericValue objects)
        (stack_.template Pop<ValueType>(1))->~ValueType();
    else
      stack_.Clear();
    stack_.ShrinkToFit();
  }

  // Implementation of Handler
  bool Null() { new (stack_.template Push<Value>()) Value(); return true; }
  bool Bool(bool b) { new (stack_.template Push<Value>()) Value(b); return true; }
  bool Int(int i) { new (stack_.template Push<Value>()) Value(i); return true; }
  bool Uint(unsigned i) { new (stack_.template Push<Value>()) Value(i); return true; }
  bool Int64(int64_t i) { new (stack_.template Push<Value>()) Value(i); return true; }
  bool Uint64(uint64_t i) { new (stack_.template Push<Value>()) Value(i); return true; }
  bool Double(double d) { new (stack_.template Push<Value>()) Value(d); return true; }

  bool RawNumber(const Ch* str, SizeType length, bool copy) { 
      if (copy) 
          new (stack_.template Push<Value>()) Value(str, length, GetAllocator());
      else
          new (stack_.template Push<Value>()) Value(str, length);
      return true;
  }

  bool String(const Ch* str, SizeType length, bool copy) { 
      if (copy) 
          new (stack_.template Push<Value>()) Value(str, length, GetAllocator());
      else
          new (stack_.template Push<Value>()) Value(str, length);
      return true;
  }

  bool StartObject() { new (stack_.template Push<Value>()) Value(kObjectType); return true; }
  
  bool Key(const Ch* str, SizeType length, bool copy) { return String(str, length, copy); }

  bool EndObject(SizeType memberCount) {
      typename Value::Member* members = stack_.template Pop<typename Value::Member>(memberCount);
      // stack_.template Top<Value>()->SetObjectRaw(members, memberCount, GetAllocator());
      Value* v = stack_.template Top<Value>();
      v->SetObject();
      for (int i = 0; i < memberCount; i++) {
        v->AddMember(members[i].name, members[i].value, GetAllocator());
      }
      return true;
  }

  bool StartArray() { new (stack_.template Push<Value>()) Value(kArrayType); return true; }
  
  bool EndArray(SizeType elementCount) {
    Value* elements = stack_.template Pop<Value>(elementCount);
    // can't use this since it's private:
    // stack_.template Top<Value>()->SetArrayRaw(elements, elementCount, GetAllocator());
    Value* v = stack_.template Top<Value>();
    v->SetArray();
    v->Reserve(elementCount, GetAllocator());
    for (int i = 0; i < elementCount; i++) {
      v->PushBack(elements[i], GetAllocator());
    }
    return true;
  }

  //! Get the allocator of this document.
  Allocator& GetAllocator() {
    RAPIDJSON_ASSERT(allocator_);
    return *allocator_;
  }

  template <unsigned parseFlags, typename SourceEncoding, typename InputStream>
  FilteredDocument& ParseStream(InputStream& is) {
    GenericReader<SourceEncoding, UTF8<>, StackAllocator> reader(
        stack_.HasAllocator() ? &stack_.GetAllocator() : 0);
    ClearStackOnExit scope(*this);
    parseResult_ = reader.template Parse<parseFlags>(is, *this);
    if (parseResult_) {
      RAPIDJSON_ASSERT(stack_.GetSize() == sizeof(ValueType)); // Got one and only one root object
      ValueType::operator=(*stack_.template Pop<ValueType>(1));// Move value from stack to document
    }
    return *this;
  }

  template <typename InputStream>
  FilteredDocument& ParseStream(InputStream& is) {
    return ParseStream<kParseDefaultFlags, UTF8<>, InputStream>(is);
  }

  /*
  bool Null() {
    if (!skipping) return GenericDocument::Null();
    return true;
  }

  bool Bool(bool b) {
    if (!skipping) return GenericDocument::Bool(b);
    return true;
  }

  bool Int(int i) {
    if (!skipping) return GenericDocument::Int(i);
    return true;
  }

  bool Uint(unsigned u) {
    if (!skipping) return GenericDocument::Uint(u);
    return true;
  }

  bool Int64(int64_t i) {
    if (!skipping) return GenericDocument::Int64(i);
    return true;
  }

  bool Uint64(uint64_t u) {
    if (!skipping) return GenericDocument::Uint64(u);
    return true;
  }

  bool Double(double d) {
    if (!skipping) return GenericDocument::Double(d);
    return true;
  }

  bool RawNumber(const char* str, SizeType length, bool copy) { 
    if (!skipping) return GenericDocument::RawNumber(str, length, copy);
    return true;
  }

  bool String(const char* str, SizeType length, bool copy) { 
    if (!skipping) return GenericDocument::String(str, length, copy);
    return true;
  }

  bool StartObject() {
    if (!skipping) return GenericDocument::StartObject();

    skipDepth += 1;
    return true;
  }

  bool Key(const char* str, SizeType length, bool copy) {
    cerr << "Key " << str << endl;
    if (!skipping && std::string(str, length) == "coordinates") {
      cerr << "Found coordinates!" << endl;
      GenericDocument::Key(str, length, copy);
      GenericDocument::Null();
      skipping = true;
      skipDepth = 0;  // XXX: currently assuming we skip an array/object, not a scalar
      return true;
    }

    if (!skipping) return GenericDocument::Key(str, length, copy);
    return true;
  }

  bool EndObject(SizeType memberCount) {
    if (!skipping) return GenericDocument::EndObject(memberCount);

    skipDepth -= 1;
    if (skipDepth == 0) {
      skipping = false;
    }
    return true;
  }

  bool StartArray() {
    if (!skipping) return GenericDocument::StartArray();

    skipDepth += 1;
    return true;
  }

  bool EndArray(SizeType elementCount) {
    if (!skipping) return GenericDocument::EndArray(elementCount);

    skipDepth -= 1;
    if (skipDepth == 0) {
      skipping = false;
    }

    return true;
  }
  */
};

int main(int argc, char** argv) {

  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " file.json" << std::endl;
    return 1;
  }

  FILE* fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    std::cerr << "Unable to open " << argv[1] << std::endl;
    return 1;
  }

  char readBuffer[65536];
  FileReadStream is(fp, readBuffer, sizeof(readBuffer));

  FilteredDocument<> d;
  d.ParseStream(is);
  fclose(fp);

  char writeBuffer[65536];
  FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
  Writer<FileWriteStream> writer(os);
  d.Accept(writer);

  return 0;
}
