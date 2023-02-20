#include <iostream>
#include <vector>

void link_files(std::vector<std::string> list_files, std::string filename_out, std::string target_triplet, std::string linker_additional_flags);
int build_std(std::string path, std::string target_triplet, bool verbose_std_build);
int build_gc(std::string path, std::string target_triplet);