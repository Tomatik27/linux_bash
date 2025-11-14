#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

int main() {
  cout << unitbuf;
  cerr << unitbuf;

  string hist = "kubsh_history.txt";
  ofstream F(hist, ios::app);

  string input;
  while (getline(cin, input)) {
    // Безопасное удаление начальных пробелов
    while (!input.empty() && input[0] == ' ') {
      input.erase(0, 1);
    }

    F << '$' << input << '\n';  
    F.flush(); 
    
    if (input == "\\q") 
      break;
    
    if (input.empty()) {
      cout << "$ ";
      continue;
    }
    
    // Обработка debug
    if (input.find("debug") == 0) {
      input = input.substr(5); // -"debug"
      
      // -пробелы после debug
      while (!input.empty() && input[0] == ' ') {
        input.erase(0, 1);
      }
      
      //  -кавычки
      if (!input.empty()) {
        if ((input[0] == '"' && input[input.length()-1] == '"') || 
            (input[0] == '\'' && input[input.length()-1] == '\'')) {
          input = input.substr(1, input.length()-2); 
        }
      }
      cout << input << '\n';
    }

    // \e для переменных окружения
    else if (input.find("\\e") == 0) {
      size_t pos = input.find("\\e") + 3; // -"\e "

      if (pos < input.length()) {  // Чек есть ли аргументы после команды
        string var_name = input.substr(pos);  // Имя переменной
        
        // -'$' если в начале имени
        if (!var_name.empty() && var_name[0] == '$') 
          var_name = var_name.substr(1);  // - $
        
        // Значение переменной окружения
        const char* env_value = getenv(var_name.c_str());
        if (env_value != nullptr) {  // Если переменная существует
          string value = env_value;  // string для удобства
          size_t start = 0;  // Нач поз для substring
          size_t end = value.find(':');  // Первый разделитель
          
          // Разделяем по ':' и выводим
          while (end != string::npos) {  // Пока есть разделители
            cout << value.substr(start, end - start) << '\n';  // Часть до разделителя
            start = end + 1;  // Сдвиг начала за разделитель
            end = value.find(':', start);  // Следующий разделитель
          }
          // Оставшаяся часть после последнего разделителя
          cout << value.substr(start) << '\n';
        } else {
          // Переменная не найдена
          cout << "Environment variable '" << var_name << "' not found" << '\n';
        }
      } else {
        // Неправильный формат команды
        cout << "Usage: \\e $VARIABLE" << '\n';
      }
    } 
    // Обработка бинарников
    else {
      // Дочерний процесс
      pid_t pid = fork();
      
      if (pid == 0) {
        // Команда
        
        // Разбиение на аргументы
        vector<string> args;
        stringstream ss(input);
        string token;
        
        while (ss >> token) {
          args.push_back(token);
        }
        
        // Аргументы для execvp
        vector<char*> argv;
        for (auto& arg : args) {  // Прогон по аргументам
          argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr); // Конец нуллом
        
        // Попытка выполнения
        execvp(argv[0], argv.data());
        
        // Если дошли сюда - команда не найдена
        cerr << input << ": command not found" << '\n';
        exit(1);
        
      } else if (pid > 0) {
        // Ждем дочь
        int status;
        waitpid(pid, &status, 0);
      } else {
        cerr << "Failed to create process" << '\n';
      }
    }
    
  }
}