#include <iostream>
#include <cstdlib>
#include <vector>

using namespace std;

int build_std(string path){
    int retcode = -1;
    string cmd = "make -C ";
    cmd.append(path);
    cout << "cmd : " << cmd << endl;
    FILE* pipe = popen(cmd.c_str(), "r");
    char* out = (char*)malloc(1000 * sizeof(char));
	fread(out, 1, 1000, pipe);
    string out_cpp = out;
    cout << out_cpp << endl;
    retcode = pclose(pipe);
    cout << "retcode : " << retcode << endl;
    return retcode;
}

void link_files(vector<string> list_files, string filename_out, string target_triplet){
    int retcode = -1;
    string cmd = "clang -o ";
    cmd.append(filename_out);
    cmd.append(" -target ");
    cmd.append(target_triplet);
    for (int i = 0; i < list_files.size(); i++){
        cmd.append(" ");
        cmd.append(list_files.at(i));
    }
    cout << "cmd : " << cmd << endl;
    FILE* pipe = popen(cmd.c_str(), "r");
    retcode = pclose(pipe);
    cout << "retcode : " << retcode << endl;
    if (retcode == -1 || retcode == 1){
        cout << "Could not create executable " << filename_out << endl;
        exit(1);
    }
}
