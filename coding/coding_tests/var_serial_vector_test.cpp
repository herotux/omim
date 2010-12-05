#include "../../base/SRC_FIRST.hpp"
#include "../var_serial_vector.hpp"

#include "../../testing/testing.hpp"

#include "../../coding/byte_stream.hpp"
#include "../../coding/hex.hpp"
#include "../../coding/reader.hpp"
#include "../../coding/writer.hpp"
#include "../../base/macros.hpp"
#include "../../base/pseudo_random.hpp"
#include "../../std/string.hpp"
#include "../../std/vector.hpp"

char const kHexSerial[] = "03000000" "01000000" "04000000" "06000000" "616263646566";

UNIT_TEST(WriteSerial)
{
  vector<string> elements;
  elements.push_back("a");
  elements.push_back("bcd");
  elements.push_back("ef");

  string output;
  PushBackByteSink<string> sink(output);
  WriteVarSerialVector(elements.begin(), elements.end(), sink);

  TEST_EQUAL(ToHex(output), kHexSerial, ());
}

UNIT_TEST(WriteSerialWithWriter)
{
  string output;
  MemWriter<string> writer(output);
  VarSerialVectorWriter<MemWriter<string> > recordWriter(writer, 3);
  writer.Write("a", 1);
  recordWriter.FinishRecord();
  writer.Write("bcd", 3);
  recordWriter.FinishRecord();
  writer.Write("ef", 2);
  recordWriter.FinishRecord();
  TEST_EQUAL(ToHex(output), kHexSerial, ());
}

UNIT_TEST(ReadSerial)
{
  string serial(FromHex(string(kHexSerial)));
  MemReader memReader(&serial[0], serial.size());
  ReaderSource<MemReader> memSource(memReader);
  VarSerialVectorReader<MemReader> reader(memSource);

  TEST_EQUAL(reader.Read(0), "a", ());
  TEST_EQUAL(reader.Read(1), "bcd", ());
  TEST_EQUAL(reader.Read(2), "ef", ());
}

namespace
{
  string GenString()
  {
    static PseudoRNG32 rng;
    string result;
    size_t size = rng.Generate() % 20;
    for (size_t i = 0; i < size; ++i)
    {
      result.append(1, static_cast<char>(rng.Generate() + 1));
    }
    return result;
  }
}

UNIT_TEST(EncodeDecode)
{
  vector<string> elements;

  for (size_t i = 0; i < 1024; ++i)
    elements.push_back(GenString());

  string serial;
  PushBackByteSink<string> sink(serial);
  WriteVarSerialVector(elements.begin(), elements.end(), sink);

  MemReader memReader(serial.c_str(), serial.size());
  ReaderSource<MemReader> memSource(memReader);
  VarSerialVectorReader<MemReader> reader(memSource);

  for (size_t i = 0; i < elements.size(); ++i)
    TEST_EQUAL(reader.Read(i), elements[i], ());
}
