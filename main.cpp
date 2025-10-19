#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <csignal>
#include <cstring>

using namespace std;

// Флаг для отслеживания получения сигнала
volatile sig_atomic_t got_sighup = 0;

// Обработчик сигнала SIGHUP
void sighup_handler(int sig) {
    got_sighup = 1;
}

int main() {
    cout << unitbuf;
    cerr << unitbuf;

    // Установка обработчика сигнала SIGHUP
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sighup_handler;
    sa.sa_flags = SA_RESTART; // Важно: перезапускать системные вызовы после сигнала
    sigaction(SIGHUP, &sa, NULL);

    string hist = "kubsh_history.txt";
    ofstream F(hist, ios::app);
    
    cout << "$ ";

    string input;
    while (getline(cin, input)) {

        // Проверяем, был ли получен сигнал SIGHUP
        if (got_sighup) {
            cout << "Configuration reloaded" << endl;
            got_sighup = 0;
            cout << "$ ";
            continue; // Переходим к следующей итерации
        }

        F << '$' << input << '\n';
        F.flush();
        
        if (input == "\\q")
        break;
        
        if (input.empty())
        continue;
        
        if (input.find("echo") == 0)
        {
            cout << input.substr(5) << '\n';
        }
        else if (input.find("\\e") == 0)
        {
            size_t pos = 3;

            if (pos < input.length()) 
            {
                string var_name = input.substr(pos);
                
                if (!var_name.empty() && var_name[0] == '$') 
                var_name = var_name.substr(1);
                
                const char* env_value = getenv(var_name.c_str());
                if (env_value != nullptr) 
                {
                    string value = env_value;
                    size_t start = 0;
                    size_t end = value.find(':');
                
                    while (end != string::npos) 
                    {
                        cout << value.substr(start, end - start) << '\n';
                        start = end + 1;
                        end = value.find(':', start);
                    }

                    cout << value.substr(start) << '\n';
                }
                else
                    cout << "Environment variable '" << var_name << "' not found" << '\n';

            } 
            else cout << "Usage: \\e $VARIABLE" << '\n';
        }
        else {
            pid_t pid = fork();
            
            if (pid == 0) {
                vector<char*> args;
                stringstream ss(input);
                string token;
                
                while (ss >> token) {
                    char* arg = new char[token.size() + 1];
                    copy(token.begin(), token.end(), arg);
                    arg[token.size()] = '\0';
                    args.push_back(arg);
                }
                
                args.push_back(nullptr);
                
                execvp(args[0], args.data());
                
                cerr << input << ": command not found" << '\n';
                
                for (char* arg : args) {
                    delete[] arg;
                }
                exit(1);
            }
            else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
            }
            else {
                cerr << "Failed to fork process" << '\n';
            }
        }
        
        cout << "$ ";
    }
    
    cout << "\n";
    return 0;
}
