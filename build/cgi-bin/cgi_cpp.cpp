// g++ cgi_cpp.cpp -o cgi_cpp
#include <iostream>
using namespace std;
int main() {
  cout << "Content-Type: text/html\r\n\r\n";
  cout << "<html>\n";
  cout << "<head>\n";
  cout << "<title>C++ cgi</title>\n";
  cout << "</head>\n";
  cout << "<body>\n";
  cout << "<h1>Hello World</h1>\n";
  cout << "</body>\n";
  cout << "</html>\n";
  return 0;
}
