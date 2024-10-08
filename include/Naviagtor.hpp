#pragma once
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "DirectoryEntry.hpp"

namespace fsext2 {
class Navigator {
public:
  Navigator(const Directory &root) : stack() { stack.push_back(root); }

  std::optional<std::reference_wrapper<Directory>> navigate(std::string path) {
    if (path.empty()) {
      return stack.back();
    }
    if (path.back() == '/' && path.size() != 1) {
      path.pop_back();
    }
    if (path[0] != '/') {
      auto &&dir = stack.back();
      auto slash = path.find('/');
      auto &&pathSub = path.substr(0, slash);
      if (pathSub == "..") {
        if (stack.size() != 1) {
          stack.pop_back();
        }
        if (slash == std::string::npos) {
          return stack.back();
        }
        return navigate(path.substr(slash + 1));
      } else if (pathSub == ".") {
        if (slash == std::string::npos) {
          return stack.back();
        }
        return navigate(path.substr(slash + 1));
      }
      auto &&entry = dir.getDirectoryEntry(path.substr(0, slash));
      if (entry) {
        auto asDir = DirectoryEntry::fromEntry<Directory>(entry.value());
        if (asDir) {
          stack.push_back(asDir.value());
          if (slash == std::string::npos) {
            return stack.back();
          }
          return navigate(path.substr(slash + 1));
        }
      }
    } else {
      while (stack.size() > 1) {
        stack.pop_back();
      }
      return navigate(path.substr(1));
    }
    return {};
  }

  const Directory &getCurrentDirectory() { return stack.back(); }

  std::optional<DirectoryEntry> getEntry(std::string name) {
    if (name.contains('/')) {
      auto temp = *this;
      auto res = temp.navigate(name.substr(0, name.rfind('/')));
      if (!res)
        return {};
      if (name.back() == '/')
        return temp.stack.back();
      else
        return temp.stack.back().getDirectoryEntry(
            name.substr(name.rfind('/') + 1));
    }
    return stack.back().getDirectoryEntry(name);
  }

  std::string getPath() {
    if (stack.size() == 1) {
      return "/";
    }
    std::string path;
    for (auto i = 1; i < stack.size(); i++) {
      path += stack[i].getName() + "/";
    }
    return path;
  }

private:
  std::vector<Directory> stack;
};
} // namespace fsext2
