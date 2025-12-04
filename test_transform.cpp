int main() {
    int x = 100;
    int a, b, c, d, e;

    a = x * 4;  // Should be: x << 2
    b = x / 2;  // Should be: x >> 1
    c = x * 8;  // Should be: x << 3
    d = x / 16; // Should be: x >> 4

    e = x * 3;  // Should be ignored (not a power of 2)

    return 0;
}