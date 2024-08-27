//#include <string>
#include <functional>

bool FileExists(std::string filename);
bool FolderExists(std::string foldername);
void listFiles(const std::string &path, std::function<void(const std::string &)> cb);
std::string getFileExtension(std::string path);
bool doesExeExist(std::string filename);