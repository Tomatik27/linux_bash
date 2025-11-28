#ifndef VFS_H
#define VFS_H

#include <string>

void initialize_vfs();
void cleanup_vfs();
void add_user(const std::string& username);
void delete_user(const std::string& username);
void force_vfs_sync();  // Добавьте эту строку

#endif