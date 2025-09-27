#include <iostream>
#include <string>

using namespace std;

int main() {
  cout << unitbuf;
  cerr << unitbuf;

  cout << "$ ";

  string input;
  while (getline(cin, input)) {
    cout << input << endl;
    cout << "$ ";
    if (input == "\\q")
      break;
  }
  return 0;
}
