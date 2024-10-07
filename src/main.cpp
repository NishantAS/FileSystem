#include "Disk.hpp";
#include <iostream>
#include <sstream>
int main() {
  fsext2::Disk disk("E:/source/FileSystem/disk2.img");
  std::cout << disk.dumpe2fs();
}