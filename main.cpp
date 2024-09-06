#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <pthread.h>
#include <chrono>
using std::ifstream;
using std::ofstream;
using std::cout;
using std::endl;
using std::cin;
using std::string;
using std::getline;
using std::size_t;
using std::filesystem::current_path;
using std::vector;
using std::sort;
using std::find;
int getting_root;
int getting_config;
int check_custom_updater_files;
int avbroot_working;
int gen_keys;
int execute;
string otapath;
string otakey;
string otacert;
string avbkey;
string root;
string cmd;
string exec;
string folder;
string slot;
string platform_tools_location;
string avbroot_location;
static volatile bool keep_running = true;
void help(){
    cout << R"(
    -?, -h, --help                       Show this message
    1) --generate-config, --gencfg       Generate config.txt
    2) --generate-keys, --genkeys        Generate keys
    3) --patch-ota                       Patch ota package
    4) --extract                         Extract rom

        --all                            Extract all partitions
        --fastboot                       Extract partitions like system, vendor, boot, vendor_boot
        --boot-only                      Extract only boot partitions
    5) --generate-partition-list, 
       --genpartlist                     Generate partition_list
    6) --flash-fastboot                  Flash rom via fastboot (need partition_list)
       --flash-adb                       Flash rom via adb)" << endl << endl;
}

void write(string text, string file) {
    #if _WIN32
      cmd = "echo " + text + " >> " + file;
    #else
      cmd = "echo \"" + text + "\" >> " + file;
    #endif
    exec = system(cmd.c_str());
}

void gencfg() {
    cout << "Drag'n'drop your ota path here >> ";
    cin >> otapath; 
    cout << endl;
    #if _WIN32
      exec = system("del config.txt > NUL");
    #else
      exec = system("rm -rf config.txt");
    #endif
    write("otapath = " + otapath, "config.txt");
    write("otacert = ota.crt", "config.txt");
    write("avbkey = avb.key", "config.txt");
    write("otakey = ota.key", "config.txt");
    cout << "Root method (nonroot/patched boot path) >> ";
    cin >> root;
    cout << endl;
    write("rootmethod = " + root, "config.txt");
    cout << "Working folder >> ";
    cin >> folder;
    cout << endl;
    write("folder = " + folder, "config.txt");
    cout << "Current/another slot >> ";
    cin >> slot;
    cout << endl;
    write("slot = " + slot, "config.txt");
    #if __WIN32
      cout << "Platform-tools location (If you have platform-tools in program folder just write \"path\") >> ";
    #else 
      cout << "Platform-tools location (If you have platform-tools in path just write \"path\") >> ";
    #endif
    cin >> platform_tools_location;
    cout << endl;
    write("platform_tools_location = " + platform_tools_location, "config.txt");
    #if __WIN32
      cout << "Avbroot location (If you have avbroot in program folder just write \"path\") >> ";
    #else
      cout << "Avbroot location (If you have avbroot in path just write \"path\") >> ";
    #endif
    cin >> avbroot_location;
    cout << endl;
    write("avbroot_location = " + avbroot_location, "config.txt");
    exit(0);
}

int generate_keys(){
    cmd = avbroot_location + "avbroot key generate-key -o " + avbkey;
    cout << cmd << endl;
    execute = system(cmd.c_str());
    if(execute != 0){
        return 1;
    }
    cmd = avbroot_location + "avbroot key generate-key -o " + otakey;
    cout << cmd << endl;
    execute = system(cmd.c_str());
    if(execute != 0){
        return 1;
    }
    cmd = avbroot_location + "avbroot key extract-avb -k " + avbkey + " -o avb_pkmd.bin";
    cout << cmd << endl;
    execute = system(cmd.c_str());
    if(execute != 0){
        return 1;
    }
    cmd = avbroot_location + "avbroot key generate-cert -k " + otakey + " -o " + otacert;
    cout << cmd << endl;
    execute = system(cmd.c_str());
    if(execute != 0){
        return 1;
    }
    return 0;
}

void sleep(int time){
    std::this_thread::sleep_for(std::chrono::seconds(time));
}

static void* userInput_thread(void*){
    while(keep_running) {
        if(cin.get() == 'q'){
            keep_running = false;
        }
    }
    return 0;
}

int flash_fastboot(){
    #if _WIN32
      string file = folder + "partition_list.txt";
    #else
      string file = folder + "partition_list";
    #endif
    ifstream ReadFile(file);
    while(ReadFile >> file) {
        if(file == "system" || file == "system_ext" || file == "system_dlkm" || file == "vendor" || file == "vendor_dlkm" || file == "product"){
            if(keep_running){
                cmd = platform_tools_location + "fastboot reboot fastboot";
                exec = system(cmd.c_str());
                keep_running = false;
            }
            
            cmd = platform_tools_location + "fastboot flash " + file + slot + " " + folder + file + ".img";
            exec = system(cmd.c_str());
        } else{
            cmd = platform_tools_location + "fastboot flash " + file + slot + " " + folder + file + ".img";
            exec = system(cmd.c_str());
        }
        
    }
    return 0;
}

int flash_adb(){
    pthread_t tId;
    cmd = "adb devices";
    (void) pthread_create(&tId, 0, userInput_thread, 0);
    
    while(keep_running){
        cout << "Now reboot your phone to recovery mode and enable sideload mode" << endl;
        cout << "When adb finds your phone press q and enter" << endl << endl;
        exec = system(cmd.c_str());
        cout << "^^^^^^^^^^^^^^^^^^^^^^^^" << endl;
        cout << "Your device should be appeared here" << endl;
        sleep(2); // in seconds
        #if _WIN32
          exec = system("cls");
        #else
          exec = system("clear");
        #endif
    }
    (void) pthread_join(tId, NULL);
    cmd = platform_tools_location + "adb sideload " + otapath + ".patched";
    exec = system(cmd.c_str());
    return 0;
}

int get_imgs(){
    current_path(folder);
    #if _WIN32
      string file = "partition_list.txt";
      cmd = "dir /B *.img > " + file;
    #else
      string file = "partition_list"; 
      cmd = "ls -1Sr *.img > " + file;
    #endif
    exec = system(cmd.c_str());
    ifstream inputFile(file);
    vector<string> fileNames;
    string line;
    while(getline(inputFile, line)){
        size_t pos = line.rfind(".img");
        if(pos != string::npos) {
            line.erase(pos);
        }
        fileNames.push_back(line);
    }
    inputFile.close();
    sort(fileNames.begin(), fileNames.end(), 
          [](const string& a, const string& b) {
              vector<string> specialSections = {"system", "system_ext", "system_dlkm", "vendor", "vendor_dlkm", "product"};
              bool isSpecialA = find(specialSections.begin(), specialSections.end(), a) != specialSections.end();
              bool isSpecialB = find(specialSections.begin(), specialSections.end(), b) != specialSections.end();
              if (isSpecialA && isSpecialB) {
                  return a < b;
              } else if (isSpecialA) {
                  return false;
              } else if (isSpecialB) {
                  return true;
              } else {
                  return a < b;
              }
          });
    ofstream outputFile(file);
    for(const auto& partitions : fileNames){
        outputFile << partitions << endl;
    }
    outputFile.close();
    return 0;
}
int extract_rom(string mode){
    if(mode == "--boot-only" || mode == "--all" || mode == "--fastboot"){
        cmd = avbroot_location + "avbroot ota extract --input " + otapath + ".patched --directory " + folder + " " + mode;
        exec = system(cmd.c_str());
    } else{
        cout << "unknown command " + mode << endl;
    }
    return 0;
}
int get_config(string mode){
    ifstream ReadFile("config.txt");
    if(!ReadFile.is_open()) {
        cout << "config.txt doesn't exist" << endl;
        exit(1);
    }
    string config;
    while(ReadFile >> config){
        if(config == "otapath"){
            ReadFile.ignore(3);
            ReadFile >> otapath;
        } else if(config == "otacert"){
            ReadFile.ignore(3);
            ReadFile >> otacert;
        } else if(config == "avbkey"){
            ReadFile.ignore(3);
            ReadFile >> avbkey;
        } else if(config == "otakey"){
            ReadFile.ignore(3);
            ReadFile >> otakey;           
        } else if(config == "rootmethod"){
            ReadFile.ignore(3);
            ReadFile >> root;
        } else if(config == "folder"){
            ReadFile.ignore(3);
            ReadFile >> folder;
        } else if(config == "slot"){
            ReadFile.ignore(3);
            ReadFile >> slot;
        } else if(config == "platform_tools_location"){
            ReadFile.ignore(3);
            ReadFile >> platform_tools_location;
        } else if(config == "avbroot_location"){
            ReadFile.ignore(3);
            ReadFile >> avbroot_location;
        }
    }
    if(mode == "test"){
        cout << otapath << endl;
        cout << otacert << endl;
        cout << avbkey << endl;
        cout << otakey << endl;
        cout << root << endl;
        cout << folder << endl;
        cout << slot << endl;
        cout << platform_tools_location << endl;
        cout << avbroot_location << endl;
    }
    if(platform_tools_location == "path"){
        platform_tools_location = "";
    }
    if(avbroot_location == "path"){
        avbroot_location = "";
    }
    ReadFile.close();
    return 0;
}
int patch_ota(){
    if(root == "nonroot"){
        exec = avbroot_location + "avbroot ota patch --input " + otapath + " --key-avb " + avbkey + " --key-ota " + otakey + " --cert-ota " + otacert + " --rootless";
        cout << exec << endl;
    } else{
        exec = avbroot_location + "avbroot ota patch --input " + otapath + " --key-avb " + avbkey + " --key-ota " + otakey + " --cert-ota " + otacert + " --prepatched " + root;
        cout << exec << endl;
    }
    avbroot_working = system(exec.c_str());
    return 0;
}
int main(int argc, char *argv[])
{
    std::vector<std::string> args(argv, argv+argc);
    for (size_t i = 1; i < args.size(); ++i) {
        if(args[i] == "--help" || args[i] == "-h" || args[i] == "-?"){
            help();
            return 0;
        }
        else if(args[i] == "--generate-config" || args[i] == "--gencfg") {
            gencfg();
            return 0;
        } else if (args[i] == "--generate-keys" || args[i] == "--genkeys") {
            getting_config = get_config("default");
            gen_keys = generate_keys();
            return 0;
        } else if(args[i] == "--patch-ota"){
            getting_config = get_config("default");
            avbroot_working = patch_ota();
            return 0;
        } else if(args[i] == "--extract") {  
            getting_config = get_config("default");
            for (size_t i = 2; i < args.size(); ++i){
                string work_type = args[i];
                exec = extract_rom(work_type);
            }
            return 0;          
        } else if(args[i] == "--flash-fastboot"){
            getting_config = get_config("default");
            exec = flash_fastboot();
            return 0;
        } else if(args[i] == "--flash-adb"){
            getting_config = get_config("default");
            exec = flash_adb();
            return 0;
        } else if(args[i] == "--generate-partition-list" || args[i] == "--genpartlist") {
            getting_config = get_config("default");
            exec = get_imgs();
        } else if(args[i] == "--test"){
            getting_config = get_config("test");
        } else{
            return 0;
        }
    }
    if(argc == 1){
        help();
    }
    return 0;
}
