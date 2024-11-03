#include "Disk.hpp"
#include "pch.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <disk image>\n";
    return 1;
  }
  fsext2::Disk reader(argv[1]);
  std::string input{};
  fsext2::Navigator navigator{reader.getNavigator()};

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
        auto tempNav = navigator;
        auto entry = tempNav.navigate(input);
        if (entry) {
          for (const auto &entryName :
               entry.value().get().getDirectoryEntires()) {
            std::cout << entryName << '\t';
          }
          std::cout << std::endl;
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
      auto newNav = navigator;
      auto entry = newNav.navigate(input.substr(0, input.find_last_of('/')));
      if (entry) {
        auto file = entry.value().get().getDirectoryEntry(
            input.substr(input.find_last_of('/')));
        if (file and file->getType() == fsext2::InodeType::File) {
          std::cout << reinterpret_cast<fsext2::File *>(file.get())->getData();
        } else {
          std::cout << std::format("{} is not a file\n", input);
        }
      } else {
        std::cout << std::format("{} is not a valid path\n", input);
      }
    } else if (input == "dumpe2fs") {
      std::cout << reader.dumpe2fs();
    } else if (input == "mkdir") {
      if (std::cin.peek() == '\n') {
        std::cout << "Usage: mkdir <path>\n";
        continue;
      }
      std::cin >> input;
      auto dir = navigator.getCurrentDirectory().addDirectoryEntry(
          input, fsext2::InodeType::Directory);
      if (!dir) {
        std::cout << std::format("{} is not a valid path\n", input);
      }
      navigator.getCurrentDirectory().updateDirectoryEntries();
    } else {
      std::cout << std::format("Unknown command {}\n", input);
    }
  }
}