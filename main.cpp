#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>

using namespace std;

int main() {
  cout << unitbuf;
  cerr << unitbuf;

  string hist = "kubsh_history.txt";
  ofstream F(hist, ios::app);
  
  cout << "$ ";

  string input;
  while (getline(cin, input)) {

    F << '$' << input << '\n';
    F.flush();
    
    if (input == "\\q")
    break;
    
    if (input.empty())
    continue;
    
    // Проверяем команду echo (должна быть в начале строки)
    if (input.find("echo") == 0)
    {
      cout << input.substr(5) << '\n';
    }
    // Проверяем команду \e (должна быть в начале строки)
    else if (input.find("\\e") == 0)
    {
      size_t pos = 3; // пропускаем "\e "

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
      // Попытка выполнить бинарный файл из PATH
      pid_t pid = fork();
      
      if (pid == 0) {
        // Дочерний процесс
        vector<char*> args;
        stringstream ss(input);
        string token;
        
        // Разбиваем команду на аргументы
        while (ss >> token) {
          char* arg = new char[token.size() + 1];
          copy(token.begin(), token.end(), arg);
          arg[token.size()] = '\0';
          args.push_back(arg);
        }
        
        // Завершаем список аргументов NULL
        args.push_back(nullptr);
        
        // Выполняем команду
        execvp(args[0], args.data());
        
        // Если execvp вернул управление, значит произошла ошибка
        cerr << input << ": command not found" << '\n';
        
        // Освобождаем память
        for (char* arg : args) {
          delete[] arg;
        }
        exit(1);
      }
      else if (pid > 0) {
        // Родительский процесс ждет завершения дочернего
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