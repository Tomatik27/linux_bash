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
    
    cout << input << endl;
    cout << "$ ";
    
    if (input == "\\q")
      break;
    
      if (input.empty())
      continue;


  }
  return 0;
}
