#include <string>
#include <iostream>

// Function 1
void foo() {
    int x = 0;
    x++;
    // Add a while loop
    while (x < 10) {
        x++;
    }
}

// Function 2
int bar(int a, std::string b) {
    if (a > 10) {
        return 0;
    }
    return 1;
}


// A test class
class MyClass {
public:
    // A method definition
    void myMethod() {
        // Call foo
        foo();
    }
};

// Main function
int main() {
    foo(); // Call foo
    bar(1, "hello"); // Call bar
    MyClass m;
    m.myMethod(); // Call myMethod
    
    // Add a for loop
    for (int i = 0; i < 5; i++) {
        // do nothing
    }
    
    return 0;
}