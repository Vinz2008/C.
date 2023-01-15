#include <iostream>
#include <cstdlib>
#include <vector>

using namespace std;

int build_std(string path, string target_triplet){
    string cmd_clean = "make clean -C ";
    cmd_clean.append(path);
    cout << "cmd clean : " << cmd_clean << endl;
    FILE* pipe_clean = popen(cmd_clean.c_str(), "r");
    pclose(pipe_clean);
    int retcode = -1;

    string cmd = "TARGET=";
    cmd.append(target_triplet);
    cmd.append(" make -C ");
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
    if (target_triplet.find("wasm") != string::npos){
        cmd.append(" --no-standard-libraries -Wl,--export-all -Wl,--no-entry ");
    }
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
