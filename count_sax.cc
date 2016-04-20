#include "include/rapidjson/reader.h"
#include "include/rapidjson/filereadstream.h"

#include <string>
#include <iostream>
#include <algorithm>  // std::max

using namespace rapidjson;
using namespace std;

struct MyHandler {
    int featureCount;
    bool wantFeature;

    bool Null() { wantFeature = false; return true; }
    bool Bool(bool b) { wantFeature = false; return true; }
    bool Int(int i) { wantFeature = false; return true; }
    bool Uint(unsigned u) { wantFeature = false; return true; }
    bool Int64(int64_t i) { wantFeature = false; return true; }
    bool Uint64(uint64_t u) { wantFeature = false; return true; }
    bool Double(double d) { wantFeature = false; return true; }
    bool RawNumber(const char* str, SizeType length, bool copy) { 
        wantFeature = false; 
        return true;
    }
    bool String(const char* str, SizeType length, bool copy) { 
        if (wantFeature && std::string(str, length) == "Feature") {
          featureCount++;
        }
        wantFeature = false;
        return true;
    }
    bool StartObject() {
      wantFeature = false; 
      return true;
    }
    bool Key(const char* str, SizeType length, bool copy) {
        if (std::string(str, length) == "type") {
          wantFeature = true;
        }
        return true;
    }
    bool EndObject(SizeType memberCount) {
      wantFeature = false; 
      return true;
    }
    bool StartArray() { wantFeature = false; return true; }
    bool EndArray(SizeType elementCount) { wantFeature = false; return true; }
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

  MyHandler handler;
  Reader reader;
  reader.Parse(is, handler);

  std::cout << "Parsed " << handler.featureCount << " objects" << std::endl;

  return 0;
}
