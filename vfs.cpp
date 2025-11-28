#define FUSE_USE_VERSION 35

#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include <sys/types.h>
#include <cerrno>
#include <ctime>
#include <string>
#include <fstream>
#include <sstream>
#include "vfs.h"
#include <sys/wait.h>
#include <fuse3/fuse.h>
#include <pthread.h>
#include <fcntl.h>
#include <vector>
#include <map>
#include <sys/stat.h>  // Добавить
#include <dirent.h>    // Добавить

using namespace std;

const string VFS_PATH = "/opt/users";
static map<string, map<string, string>> vfs_data;
static bool fuse_available = false;  // Флаг доступности FUSE

void sync_vfs_with_passwd() {
    vfs_data.clear();
    
    ifstream passwd_file("/etc/passwd");
    if (!passwd_file.is_open()) {
        cerr << "Cannot open /etc/passwd" << endl;
        return;
    }

    string line;
    while (getline(passwd_file, line)) {
        vector<string> fields;
        string field;
        stringstream ss(line);
        
        while (getline(ss, field, ':')) {
            fields.push_back(field);
        }
        
        if (fields.size() >= 7) {
            string username = fields[0];
            string uid = fields[2];
            string home = fields[5];
            string shell = fields[6];
            
            // Более либеральная фильтрация - включаем всех пользователей с UID >= 1000 и root
            int uid_num = stoi(uid);
            if (uid_num == 0 || uid_num >= 1000) {
                // Включаем пользователей с любым shell кроме явно запрещенных
                if (shell != "/bin/false" && shell != "/usr/sbin/nologin") {
                    vfs_data[username]["id"] = uid;
                    vfs_data[username]["home"] = home;
                    vfs_data[username]["shell"] = shell;
                    
                    // Всегда создаем физическую структуру в прямом режиме
                    if (!fuse_available) {
                        string user_dir = VFS_PATH + "/" + username;
                        mkdir(user_dir.c_str(), 0755);
                        
                        ofstream id_file(user_dir + "/id");
                        id_file << uid;
                        id_file.close();
                        
                        ofstream home_file(user_dir + "/home");
                        home_file << home;
                        home_file.close();
                        
                        ofstream shell_file(user_dir + "/shell");
                        shell_file << shell;
                        shell_file.close();
                    }
                }
            }
        }
    }
    passwd_file.close();
}

void force_vfs_sync() {
    sync_vfs_with_passwd();
}

// Функция для добавления пользователя в систему
bool add_system_user(const string& username) {
    // Пробуем разные команды добавления пользователя
    string command = "useradd -m -s /bin/bash " + username + " 2>/dev/null";
    int result = system(command.c_str());
    
    if (result != 0) {
        command = "adduser --disabled-password --gecos '' " + username + " 2>/dev/null";
        result = system(command.c_str());
    }
    
    if (result == 0) {
        // Даем системе время на создание пользователя
        for (int i = 0; i < 5; i++) {
            usleep(200000); // 200ms
            struct passwd* pw = getpwnam(username.c_str());
            if (pw != nullptr) {
                return true;
            }
        }
    }
    return false;
}

// Функция для удаления пользователя из системы  
bool delete_system_user(const string& username) {
    string command = "userdel -r " + username + " 2>/dev/null";
    int result = system(command.c_str());
    if (result == 0) {
        sync_vfs_with_passwd();
        return true;
    }
    return false;
}

static int vfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    string spath(path + 1);
    
    if (vfs_data.count(spath)) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    size_t pos = spath.find('/');
    if (pos != string::npos) {
        string username = spath.substr(0, pos);
        string filename = spath.substr(pos + 1);
        
        if (vfs_data.count(username) && vfs_data[username].count(filename)) {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = vfs_data[username][filename].size();
            return 0;
        }
    }

    return -ENOENT;
}

static int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, 
                      struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);

    if (strcmp(path, "/") == 0) {
        for (const auto& user : vfs_data) {
            filler(buf, user.first.c_str(), NULL, 0, FUSE_FILL_DIR_PLUS);
        }
    } else {
        string username(path + 1);
        if (vfs_data.count(username)) {
            filler(buf, "id", NULL, 0, FUSE_FILL_DIR_PLUS);
            filler(buf, "home", NULL, 0, FUSE_FILL_DIR_PLUS);
            filler(buf, "shell", NULL, 0, FUSE_FILL_DIR_PLUS);
        }
    }

    return 0;
}

static int vfs_open(const char *path, struct fuse_file_info *fi) {
    (void) fi;
    return 0;
}

static int vfs_read(const char *path, char *buf, size_t size, off_t offset, 
                   struct fuse_file_info *fi) {
    (void) fi;
    
    string spath(path + 1);
    size_t pos = spath.find('/');
    if (pos == string::npos) return -ENOENT;

    string username = spath.substr(0, pos);
    string filename = spath.substr(pos + 1);

    if (!vfs_data.count(username) || !vfs_data[username].count(filename)) {
        return -ENOENT;
    }

    const string& content = vfs_data[username][filename];
    if ((size_t)offset < content.size()) {
        size_t len = min(size, content.size() - offset);
        memcpy(buf, content.c_str() + offset, len);
        return len;
    }
    return 0;
}

static int vfs_mkdir(const char *path, mode_t mode) {
    (void) mode;
    
    string username(path + 1);
    
    if (username.empty() || username.find('/') != string::npos) {
        return -EINVAL;
    }

    if (vfs_data.count(username)) {
        return -EEXIST;
    }

    cout << "FUSE: Adding user: " << username << endl;
    
    if (add_system_user(username)) {
        cout << "User " << username << " added successfully via FUSE" << endl;
        return 0;
    }
    
    cerr << "Failed to create user via FUSE: " << username << endl;
    return -EIO;
}

static int vfs_rmdir(const char *path) {
    string username(path + 1);
    
    if (username.empty() || username.find('/') != string::npos) {
        return -EINVAL;
    }

    if (!vfs_data.count(username)) {
        return -ENOENT;
    }

    cout << "FUSE: Deleting user: " << username << endl;
    
    if (delete_system_user(username)) {
        cout << "User " << username << " deleted successfully via FUSE" << endl;
        return 0;
    }
    
    cerr << "Failed to delete user via FUSE: " << username << endl;
    return -EIO;
}

static struct fuse_operations vfs_oper = {
    .getattr = vfs_getattr,
    .mkdir = vfs_mkdir,
    .rmdir = vfs_rmdir,
    .open = vfs_open,
    .read = vfs_read,
    .readdir = vfs_readdir,
};

void fuse_main_wrapper() {
    const char* fuse_argv[] = {
        "kubsh_vfs",
        "-f",
        "-s",
        VFS_PATH.c_str(),
        NULL
    };
    int fuse_argc = 4;

    cout << "Mounting FUSE3 at: " << VFS_PATH << endl;
    int ret = fuse_main(fuse_argc, (char**)fuse_argv, &vfs_oper, NULL);
    if (ret != 0) {
        cerr << "FUSE3 mount failed: " << ret << endl;
    }
}

void initialize_vfs() {
    // Создаем точку монтирования
    if (mkdir(VFS_PATH.c_str(), 0755) == -1 && errno != EEXIST) {
        cerr << "Error creating VFS directory: " << VFS_PATH << " - " << strerror(errno) << endl;
        return;
    }

    // Заполняем начальные данные
    sync_vfs_with_passwd();

    // Проверяем доступность FUSE
    if (access("/dev/fuse", F_OK) != 0) {
        cout << "FUSE not available, using direct mode" << endl;
        fuse_available = false;
        
        // Создаем физическую структуру директорий
        for (const auto& user : vfs_data) {
            string user_dir = VFS_PATH + "/" + user.first;
            mkdir(user_dir.c_str(), 0755);
            
            for (const auto& file : user.second) {
                string file_path = user_dir + "/" + file.first;
                ofstream f(file_path);
                f << file.second;
                f.close();
            }
        }
        return;
    }
    
    fuse_available = true;
    // Запускаем FUSE только если устройство доступно
    pid_t pid = fork();
    if (pid == 0) {
        fuse_main_wrapper();
        exit(0);
    } else if (pid > 0) {
        sleep(2);
        cout << "VFS initialized with FUSE3 at: " << VFS_PATH << endl;
    } else {
        cerr << "Failed to fork for FUSE3" << endl;
    }
}

void add_user(const std::string& username) {
    if (fuse_available) {
        string command = "mkdir -p " + VFS_PATH + "/" + username + " 2>/dev/null";
        system(command.c_str());
    } else {
        // В прямом режиме напрямую добавляем пользователя
        if (add_system_user(username)) {
            cout << "User " << username << " added successfully" << endl;
            // Принудительно создаем структуру директорий
            string user_dir = VFS_PATH + "/" + username;
            mkdir(user_dir.c_str(), 0755);
            
            // Создаем файлы пользователя
            struct passwd* pw = getpwnam(username.c_str());
            if (pw != nullptr) {
                ofstream id_file(user_dir + "/id");
                id_file << pw->pw_uid;
                id_file.close();
                
                ofstream home_file(user_dir + "/home");
                home_file << pw->pw_dir;
                home_file.close();
                
                ofstream shell_file(user_dir + "/shell");
                shell_file << pw->pw_shell;
                shell_file.close();
            }
        } else {
            cerr << "Failed to create user: " << username << endl;
        }
    }
    // Принудительная синхронизация после добавления
    force_vfs_sync();
}


void delete_user(const std::string& username) {
    if (fuse_available) {
        string command = "rmdir " + VFS_PATH + "/" + username + " 2>/dev/null";
        system(command.c_str());
    } else {
        if (delete_system_user(username)) {
            cout << "User " << username << " deleted successfully" << endl;
        } else {
            cerr << "Failed to delete user: " << username << endl;
        }
    }
}

void cleanup_vfs() {
    if (fuse_available) {
        // Отмонтируем FUSE
        string command = "fusermount3 -u " + VFS_PATH + " 2>/dev/null || true";
        system(command.c_str());
    }
}