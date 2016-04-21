// This is an experiment in parsing all but certain fields of a JSON file.
// For GeoJSON files, it can be a big speedup. For example, for a ~100MB file:
//
//   0.20s wc -l on GeoJSON file
//   0.50s Count features with RapidJSON SAX-style callbacks
//   0.55s Parse Document with RapidJSON skipping coordinate arrays
//   1.10s Parse full Document with RapidJSON
//
// There's significant duplication between FilteredDocument and GenericDocument.
// This may be unavoidable because of private fields and non-virtual methods.
//
// This reads in a GeoJSON file and outputs a version of it to stdout with all
// the coordinate arrays set to null.

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

template <typename Allocator = MemoryPoolAllocator<>, typename StackAllocator = CrtAllocator>
struct FilteredDocument : Value {
  bool skipping;
  int skipDepth;
  int numSkipped;

  static const size_t kDefaultStackCapacity = 1024;
  Allocator* allocator_;
  Allocator* ownAllocator_;
  internal::Stack<StackAllocator> stack_;
  ParseResult parseResult_;

  FilteredDocument(size_t stackCapacity = kDefaultStackCapacity) : 
      skipping(0), skipDepth(0), numSkipped(0), allocator_(0), ownAllocator_(0), stack_(0, stackCapacity), parseResult_()
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
  bool Null() {
    if (!skipping) {
      new (stack_.template Push<Value>()) Value();
    }
    return true;
  }

  bool Bool(bool b) {
    if (!skipping) {
      new (stack_.template Push<Value>()) Value(b);
    }
    return true;
  }

  bool Int(int i) {
    if (!skipping) {
      new (stack_.template Push<Value>()) Value(i);
    }
    return true;
  }

  bool Uint(unsigned i) {
    if (!skipping) {
      new (stack_.template Push<Value>()) Value(i);
    }
    return true;
  }

  bool Int64(int64_t i) {
    if (!skipping) {
      new (stack_.template Push<Value>()) Value(i);
    }
    return true;
  }

  bool Uint64(uint64_t i) {
    if (!skipping) {
      new (stack_.template Push<Value>()) Value(i);
    }
    return true;
  }

  bool Double(double d) {
    if (!skipping) {
      new (stack_.template Push<Value>()) Value(d);
    }
    return true;
  }

  bool RawNumber(const Ch* str, SizeType length, bool copy) { 
    if (!skipping) {
      if (copy) 
        new (stack_.template Push<Value>()) Value(str, length, GetAllocator());
      else
        new (stack_.template Push<Value>()) Value(str, length);
    }
    return true;
  }

  bool String(const Ch* str, SizeType length, bool copy) { 
    if (!skipping) {
      if (copy) 
        new (stack_.template Push<Value>()) Value(str, length, GetAllocator());
      else
        new (stack_.template Push<Value>()) Value(str, length);
    }
    return true;
  }

  bool StartObject() {
    if (!skipping) {
      new (stack_.template Push<Value>()) Value(kObjectType);
    } else {
      skipDepth += 1;
    }
    return true;
  }

  bool Key(const Ch* str, SizeType length, bool copy) {
    if (!skipping && std::string(str, length) == "coordinates") {
      numSkipped++;
      String(str, length, copy);
      Null();
      skipping = true;
      skipDepth = 0;  // XXX: currently assuming we skip an array/object, not a scalar
      return true;
    }

    if (!skipping) {
      return String(str, length, copy);
    }
    return true;
  }

  bool EndObject(SizeType memberCount) {
    if (!skipping) {
      typename Value::Member* members = stack_.template Pop<typename Value::Member>(memberCount);
      // can't use this since it's private:
      // stack_.template Top<Value>()->SetObjectRaw(members, memberCount, GetAllocator());
      Value* v = stack_.template Top<Value>();
      v->SetObject();
      for (int i = 0; i < memberCount; i++) {
        v->AddMember(members[i].name, members[i].value, GetAllocator());
      }
    }

    skipDepth -= 1;
    if (skipDepth == 0) {
      skipping = false;
    }

    return true;
  }

  bool StartArray() {
    if (!skipping) {
      new (stack_.template Push<Value>()) Value(kArrayType);
    } else {
      skipDepth += 1;
    }
    return true;
  }

  bool EndArray(SizeType elementCount) {
    if (!skipping) {
      Value* elements = stack_.template Pop<Value>(elementCount);
      // can't use this since it's private:
      // stack_.template Top<Value>()->SetArrayRaw(elements, elementCount, GetAllocator());
      Value* v = stack_.template Top<Value>();
      v->SetArray();
      v->Reserve(elementCount, GetAllocator());
      for (int i = 0; i < elementCount; i++) {
        v->PushBack(elements[i], GetAllocator());
      }
    } else {
      skipDepth -= 1;
      if (skipDepth == 0) {
        skipping = false;
      }
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

  // cerr << "Skipped " << d.numSkipped << " coordinate arrays." << endl;

  char writeBuffer[65536];
  FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
  Writer<FileWriteStream> writer(os);
  d.Accept(writer);

  return 0;
}
