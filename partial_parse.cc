#include "include/rapidjson/document.h"
#include "include/rapidjson/filereadstream.h"
#include "include/rapidjson/filewritestream.h"
#include "include/rapidjson/writer.h"

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

struct FilteredDocument : Document {
  bool skipping;
  int skipDepth;

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
    // if (wantFeature && std::string(str, length) == "Feature") {
    //   featureCount++;
    // }
    return true;
  }

  bool StartObject() {
    if (!skipping) return GenericDocument::StartObject();
    return true;
  }

  bool Key(const char* str, SizeType length, bool copy) {
    if (!skipping) return GenericDocument::Key(str, length, copy);
    // if (std::string(str, length) == "type") {
    //   wantFeature = true;
    // }
    return true;
  }

  bool EndObject(SizeType memberCount) {
    if (!skipping) return GenericDocument::EndObject(memberCount);
    return true;
  }

  bool StartArray() {
    if (!skipping) return GenericDocument::StartArray();
    return true;
  }

  bool EndArray(SizeType elementCount) {
    if (!skipping) return GenericDocument::EndArray(elementCount);
    return true;
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

  FilteredDocument d;
  d.ParseStream(is);
  fclose(fp);

  char writeBuffer[65536];
  FileWriteStream os(stdout, writeBuffer, sizeof(writeBuffer));
  Writer<FileWriteStream> writer(os);
  d.Accept(writer);

  return 0;
}
