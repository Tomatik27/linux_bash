#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

int main() {
    cout << unitbuf;
    cerr << unitbuf;

    string hist = "kubsh_history.txt";
    ofstream F(hist, ios::app);
    
    cout << "$ ";

    string input;
    while (getline(cin, input)) {
        // Удаляем начальные пробелы безопасно
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
        
        // Команда echo (переименована из debug)
        if (input.find("echo") == 0) {
            string content = input.substr(4);
            // Удаляем пробелы после echo
            while (!content.empty() && content[0] == ' ') {
                content.erase(0, 1);
            }
            // Удаляем кавычки если есть
            if (!content.empty() && 
                ((content[0] == '"' && content[content.length()-1] == '"') || 
                 (content[0] == '\'' && content[content.length()-1] == '\''))) {
                content = content.substr(1, content.length()-2);
            }
            cout << content << '\n';
        }
        else if (input.find("\\e") == 0) {
            size_t pos = input.find("\\e") + 3;

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
            // Убрана обработка бинарников - просто сообщаем что команда не найдена
            cout << input << ": command not found" << '\n';
        }
        
        cout << "$ ";
    }
    return 0;
}