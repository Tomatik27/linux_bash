#include <iostream>
#include <fstream>
#include <string>

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
    
    // cout << input << endl;
    
    if (input == "\\q")
    break;
    
    if (input.empty())
    continue;
    
    if (input.find("echo") != -1)
    {
      cout << input.substr(input.find("echo")+6, input.find_last_of('"')-1) << '\n';
    }
    
    cout << "$ ";
    
  }
  return 0;
}
