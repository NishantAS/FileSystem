#include "Disk.hpp"

#include <format>
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <disk image>\n";
    return 1;
  }
  fsext2::Disk disk(argv[1]);
  std::string input{};
  fsext2::Navigator navigator{disk.getNavigator()};

  while (true) {
    if (std::cin.tellg() != 0) {
      std::getline(std::cin, input);
    }
    std::cout << std::format("user@machine:{}$ ", navigator.getPath());
    std::cin >> input;
    if (input.empty()) {
      continue;
    } else if (input == "exit") {
      break;
    } else if (input == "ls") {
      if (std::cin.peek() == '\n') {
        for (auto &&entry :
             navigator.getCurrentDirectory().getDirectoryEntires()) {
          std::cout << entry << '\t';
        }
        std::cout << std::endl;
      } else {
        std::cin >> input;
        auto entry = navigator.getEntry(input);
        if (entry) {
          auto dir = fsext2::DirectoryEntry::fromEntry<fsext2::Directory>(
              entry.value());
          if (dir) {
            for (const auto &entry : dir.value().getDirectoryEntires()) {
              std::cout << entry << '\t';
            }
            std::cout << std::endl;
          } else {
            std::cout << std::format("{} is not a directory\n", input);
          }
        } else {
          std::cout << std::format("{} is not a valid path\n", input);
        }
      }
    } else if (input == "cd") {
      if (std::cin.peek() == '\n') {
        std::cout << "Usage: cd <path>\n";
        continue;
      }
      std::cin >> input;
      auto temp = navigator;
      auto dir = temp.navigate(input);
      if (!dir) {
        std::cout << std::format("{} is not a valid path\n", input);
      } else {
        navigator = std::move(temp);
      }
    } else if (input == "pwd") {
      std::cout << navigator.getPath() << std::endl;
    } else if (input == "cat") {
      if (std::cin.peek() == '\n') {
        std::cout << "Usage: cat <file>\n";
        continue;
      }
      std::cin >> input;
      auto entry = navigator.getEntry(input);
      if (entry) {
        auto file =
            fsext2::DirectoryEntry::fromEntry<fsext2::File>(entry.value());
        if (file) {
          std::cout << file.value().getData();
        } else {
          std::cout << std::format("{} is not a file\n", input);
        }
      } else {
        std::cout << std::format("{} is not a valid path\n", input);
      }
    } else if (input == "dumpe2fs") {
      std::cout << disk.dumpe2fs();
    } else {
      std::cout << std::format("Unknown command {}\n", input);
    }
  }
}