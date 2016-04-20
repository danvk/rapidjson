// rapidjson/example/simpledom/simpledom.cpp

// TODO: get rid of the "include/" bit
#include "include/rapidjson/filereadstream.h"
#include "include/rapidjson/document.h"
#include "include/rapidjson/writer.h"
#include "include/rapidjson/stringbuffer.h"
#include <cstdio>
#include <iostream>

using namespace rapidjson;

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

  Document d;
  d.ParseStream(is);
  fclose(fp);

  assert(d.HasMember("type"));
  assert(d["type"].IsString());
  if (d["type"] != "FeatureCollection") {
    std::cerr << "Expected type=FeatureCollection, got type=" << d["type"].GetString() << std::endl;
    return 2;
  }

  assert(d.HasMember("features"));
  assert(d["features"].IsArray());

  std::cout << d["features"].GetArray().Size() << " features" << std::endl;

  return 0;
}
