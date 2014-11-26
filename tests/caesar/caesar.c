void print_string(char *s);
int atoi(char *s);
void strcpy(char *dest, char *src);

char caesar_char(char c, int shift) {
  int base;
  if ('A' <= c && 'Z' >= c) {
    base = 'A';
  } else if ('a' <= c && 'z' >= c) {
    base = 'a';
  } else {
    base = 0;
  }

  if (base > 0) {
    c -= base;
    c += shift;
    c %= 26;
    c += base;
  }

  return c;
}

int main(int argc, char *argv[]) {
  char string[100];
  int shift, i;

  strcpy(string, argv[1]);
  shift = atoi(argv[2]);

  i = 0;
  while (string[i]) {
    string[i] = caesar_char(string[i], shift);
    i++;
  }

  print_string("Shifted string: ");
  print_string(string);
  print_string("\\n");
  return 0;
}