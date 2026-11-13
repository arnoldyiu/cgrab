#include <iostream>

// ! using namespace std to write cout instead of std::cout
using namespace std;

int main() {
  int num{10};
  cout << "Value of num is: " << num << "\n";
  cout << "Size of num is: " << sizeof num << "\n";

  // ! &num "memory address of num"
  cout << "Address of num is: " << &num << "\n";

  cout << "\n";

  // ! "int pointer called p holds the address of num"
  // !! subsequent uses of the pointer itself should not use *
  int *p{&num};
  cout << "Value of p is: " << p << "\n";

  // ! compare the size of p to the thing it is pointing to (num)
  cout << "Size of p is: " << sizeof p << "\n";

  cout << "Address of p is: " << &p << "\n";

  // ! dereference *p with *p again
  cout << "Dereferenced value of p is: " << *p << "\n\n";

  // ! what happens when you change the pointer target?
  *p = 200;
  cout << "Value of p is: " << p << "\n";
  cout << "Size of p is: " << sizeof p << "\n";
  cout << "Address of p is: " << &p << "\n";
  cout << "Dereferenced value of p is: " << *p << "\n\n";

  cout << "New value of num is: " << num << "\n";
  return 0;
}