#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include "binreader.h"

namespace fs = std::filesystem;

bool includes(const std::string &str, const std::string &toFind)
{
  return str.find(toFind) != std::string::npos;
}

std::vector<std::string> processUAssetFile(const std::string &filePath)
{
  std::vector<std::string> output;
  try
  {
    BinaryReader reader(filePath);

    const std::string pattern = "gbtd@";
    const size_t searchStart = 0x100;
    const size_t searchEnd = 0x200;

    reader.seek(searchStart);
    std::streampos matchPos = -1;

    for (size_t i = searchStart; i <= searchEnd - pattern.size(); ++i)
    {
      reader.seek(i);
      std::string str = reader.readString(pattern.size());
      if (str == pattern)
      {
        matchPos = i;
        break;
      }
    }

    if (matchPos == -1)
    {
      std::cout << "Pattern 'gbtd@' not found in " + filePath << std::endl;
      return output;
    }

    std::streampos countPos = matchPos + static_cast<std::streamoff>(20);
    reader.seek(countPos);
    int32_t count = reader.readInt();
    std::cout << "Item count: " + std::to_string(count) << std::endl;

    std::streampos itemPos = matchPos + static_cast<std::streamoff>(64);
    reader.seek(itemPos);

    for (int i = 0; i < count; i++)
    {
      std::string head = reader.readString(4);
      if (head.compare("text") != 0)
      {
        output.push_back("Invalid Text");
        break;
      }

      int headerLen = reader.readInt();
      int nameMaxLen = reader.readInt();
      int textMaxLen = reader.readInt();
      int _unk1 = reader.readInt();
      int _unk2 = reader.readInt();
      std::string name = reader.readString(nameMaxLen);
      std::string text = reader.readString(textMaxLen);

      if (includes(name, "TEXT_VO_") || includes(name, "TEXT_200_"))
      {
        char buffer[1024];
        std::snprintf(buffer, sizeof(buffer), "%-50s | %s", name.c_str(), text.c_str());
        output.push_back(std::string(buffer));
      }
    }
  }
  catch (const std::exception &e)
  {
    std::cout << "Error processing " + filePath + ": " + e.what();
  }

  return output;
}

int main()
{
  std::string folderPath = "";
  std::cout << "Enter the absolute path to the folder: ";
  std::getline(std::cin, folderPath);

  if (!fs::exists(folderPath) || !fs::is_directory(folderPath))
  {
    std::cerr << "Invalid folder path.\n";
    return 1;
  }

  std::vector<std::string> results;

  for (const auto &entry : fs::directory_iterator(folderPath))
  {
    if (entry.is_regular_file() && entry.path().extension() == ".uasset")
    {
      std::string filePath = entry.path().string();
      std::cout << "Processing: " << entry.path().filename() << std::endl;

      std::vector<std::string> result = processUAssetFile(filePath);
      results.insert(results.end(), result.begin(), result.end());
    }
  }

  std::ofstream output("results.txt");
  if (!output)
  {
    std::cerr << "Failed to write results file.\n";
    return 1;
  }

  for (const auto &line : results)
  {
    output << line << '\n';
  }

  std::cout << "Processing complete. Results written to results.txt\n";
  return 0;
}
