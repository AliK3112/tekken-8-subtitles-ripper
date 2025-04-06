#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cstring>

class BinaryReader
{
public:
  explicit BinaryReader(const std::string &filePath, bool littleEndian = true)
      : stream(filePath, std::ios::binary), isLittleEndian(littleEndian)
  {
    if (!stream)
    {
      throw std::runtime_error("Failed to open file: " + filePath);
    }
  }

  int32_t readInt()
  {
    return static_cast<int32_t>(readPrimitive<uint32_t>());
  }

  uint32_t readUInt()
  {
    return readPrimitive<uint32_t>();
  }

  int16_t readShort()
  {
    return static_cast<int16_t>(readPrimitive<uint16_t>());
  }

  uint16_t readUShort()
  {
    return readPrimitive<uint16_t>();
  }

  int64_t readLong()
  {
    return static_cast<int64_t>(readPrimitive<uint64_t>());
  }

  uint64_t readULong()
  {
    return readPrimitive<uint64_t>();
  }

  uint8_t readByte()
  {
    return readPrimitive<uint8_t>();
  }

  std::string readString(size_t length)
  {
    std::vector<char> buffer(length);
    stream.read(buffer.data(), length);
    if (stream.gcount() != static_cast<std::streamsize>(length))
    {
      throw std::runtime_error("Failed to read string");
    }
    return std::string(buffer.begin(), buffer.end());
  }

  std::string readCString()
  {
    std::string result;
    char ch;
    while (stream.get(ch) && ch != '\0')
    {
      result += ch;
    }
    return result;
  }

  void seek(std::streampos pos)
  {
    stream.seekg(pos);
  }

  std::streampos tell()
  {
    return stream.tellg();
  }

  bool eof() const
  {
    return stream.eof();
  }

private:
  std::ifstream stream;
  bool isLittleEndian;

  template <typename T>
  T readPrimitive()
  {
    T value;
    stream.read(reinterpret_cast<char *>(&value), sizeof(T));
    if (stream.gcount() != sizeof(T))
    {
      throw std::runtime_error("Failed to read primitive type");
    }

    if (isLittleEndian != isSystemLittleEndian())
    {
      value = byteSwap(value);
    }

    return value;
  }

  bool isSystemLittleEndian() const
  {
    uint16_t test = 0x1;
    return *reinterpret_cast<uint8_t *>(&test) == 0x1;
  }

  template <typename T>
  T byteSwap(T val)
  {
    T result = 0;
    uint8_t *src = reinterpret_cast<uint8_t *>(&val);
    uint8_t *dst = reinterpret_cast<uint8_t *>(&result);
    for (size_t i = 0; i < sizeof(T); ++i)
    {
      dst[i] = src[sizeof(T) - i - 1];
    }
    return result;
  }
};
