// rapidjson/example/simpledom/simpledom.cpp

// TODO: get rid of the "include/" bit
#include "include/rapidjson/filereadstream.h"
#include "include/rapidjson/document.h"
#include "include/rapidjson/writer.h"
#include "include/rapidjson/stringbuffer.h"

#include <cstdio>

#include <map>
#include <string>
#include <iostream>

using namespace rapidjson;
using namespace std;


#define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::exit(EXIT_FAILURE); \
        } \
    } while (false)


int main(int argc, char** argv) {
  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " file.json" << endl;
    return 1;
  }

  FILE* fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    cerr << "Unable to open " << argv[1] << endl;
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
    cerr << "Expected type=FeatureCollection, got type=" << d["type"].GetString() << endl;
    return 2;
  }

  ASSERT(d.HasMember("features"), "Feature collection has no features");
  ASSERT(d["features"].IsArray(), "features is not an array");

  map<string, int> geometry_counts;
  for (const auto& feature : d["features"].GetArray()) {
    ASSERT(feature.HasMember("geometry"), "feature has no geometry");
    const auto& geometry = feature["geometry"];
    ASSERT(geometry.IsObject(), "geometry is not an object");
    ASSERT(geometry.HasMember("type"), "geometry has no type");
    const auto& type = geometry["type"];
    ASSERT(type.IsString(), "geometry type is not a string");
    geometry_counts[type.GetString()]++;
  }

  for (const auto type_count : geometry_counts) {
    cout << type_count.first << ": " << type_count.second << endl;
  }

  return 0;
}
