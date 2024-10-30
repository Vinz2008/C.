//#include <string>
#include <functional>

bool FileExists(std::string filename);
bool FolderExists(std::string foldername);
void listFiles(const std::string &path, std::function<void(const std::string &)> cb);
std::string getFileExtension(std::string path);
bool doesExeExist(std::string filename);

int stringDistance(std::string s1, std::string s2);
bool is_char_one_of_these(int c, std::string chars);