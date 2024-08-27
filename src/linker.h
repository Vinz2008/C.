//#include <iostream>
#include <vector>
#include <string>

void link_files(std::vector<std::string> list_files, std::string filename_out, std::string target_triplet, std::string linker_additional_flags, bool should_use_internal_lld, std::string cpoint_path);
int build_std(std::string path, std::string target_triplet, bool verbose_std_build, bool use_native_target = false, bool is_gc = true);
int build_gc(std::string path, std::string target_triplet);